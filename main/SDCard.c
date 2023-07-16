#ifndef ISSIMULATION

#include "SDCard.h"
#include "DeviceContext.h"
#include "pinDefines.h"
#include "ErrorHandling.h"

#include <unistd.h>
#include <sys/stat.h>
#include <esp_vfs_fat.h>
#include <esp_log.h>
#include <string.h>
#include <driver/sdmmc_host.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>

static const char *TAG = "SDCard";

#define SDCARD_DETECT 14
sdcard_t sdCardInstance;

void sdcard_storageCheck(void *params)
{
    while (1)
    {
        sdcard_remainingSpace_update();
        vTaskDelay(20000 / portTICK_PERIOD_MS);
    }
}

esp_err_t init_sdcard(const char *Mount_Point)
{
    if (sdCardInstance.cardInit)
        return ESP_OK;
    // /* Configure SDCard */
    gpio_config_t sdcard_detect_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << SDCARD_DETECT,
        .pull_down_en = 0,
        .pull_up_en = 0};
    gpio_config(&sdcard_detect_conf);
    int sdCardLevel = gpio_get_level(SDCARD_DETECT);
    ESP_LOGI(TAG, "SDCard level: %i", sdCardLevel);

    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 512}; // 512 bytes is standard can't be changed
    sdmmc_card_t *card;
    // TODO: try 40MHz instead of 20 MHz
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = 400;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    slot_config.clk = SD_CLK;
    slot_config.cmd = SD_CMD;
    slot_config.d0 = SD_D0;
    slot_config.d1 = SD_D1;
    slot_config.d2 = SD_D2;
    slot_config.d3 = SD_D3;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    sd_card_state_t sdcard_state_vario; // for Vario State
    memset(&sdcard_state_vario, 0, sizeof(sd_card_state_t));

    ESP_LOGI(TAG, "Mounting filesystem");
    ERROR_RETRYTIMES(esp_vfs_fat_sdmmc_mount(Mount_Point, &host, &slot_config, &mount_config, &card), 10, ret);

    if (ret != ESP_OK)
    {
        sdcard_state_vario.status |= SD_CARD_STATE_ERROR_INIT;
        device_state_set_member(&sdcard_state_vario, device_state_statusSDCard);
        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    sdCardInstance.cardInit = true;
    sdCardInstance.card = card;
    sdCardInstance.Mount_Point = malloc(strlen(Mount_Point) + 1);
    strcpy(sdCardInstance.Mount_Point, Mount_Point);
    /* Get volume information and free clusters of sdcard */
    FATFS *fs;
    uint32_t fre_clust, fre_sect, tot_sect;
    FRESULT res = f_getfree(sdCardInstance.Mount_Point, &fre_clust, &fs);
    if (res)
    {
        printf("Couldn't find out SdCard size\n");
        sdcard_state_vario.freeSpace = 0;
        sdcard_state_vario.maxSpace = 0;
        sdcard_state_vario.status |= SD_CARD_STATE_INIT;
        device_state_set_member(&sdcard_state_vario, device_state_statusSDCard);
        return ret;
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    uint64_t tmp_total_bytes = (uint64_t)tot_sect * FF_SS_SDCARD;
    uint64_t tmp_free_bytes = (uint64_t)fre_sect * FF_SS_SDCARD;

    sdcard_state_vario.status |= SD_CARD_STATE_INIT;
    sdcard_state_vario.maxSpace = tmp_total_bytes;
    sdcard_state_vario.freeSpace = tmp_free_bytes;
    device_state_set_member(&sdcard_state_vario, device_state_statusSDCard);

    xTaskCreate(sdcard_storageCheck, "Storage Check", 1024 * 3, NULL, 2, NULL);
    return ret;
}

bool sdCard_Init()
{
    return sdCardInstance.cardInit;
}

esp_err_t deinit_sdcard()
{
    esp_vfs_fat_sdcard_unmount(sdCardInstance.Mount_Point, sdCardInstance.card);
    free(sdCardInstance.Mount_Point);
    return ESP_OK;
}

FILE *openFile_sdcard(const char *directory, const char *fileName, const char *openMethod)
{
    char buffer[100];
    strcpy(buffer, sdCardInstance.Mount_Point);
    struct stat st = {0};
    if (directory != NULL)
    {
        if (directory[0] != '/')
            strcat(buffer, "/");
        strcat(buffer, directory);
        if (stat(buffer, &st) == -1)
        {
            if (mkdir(buffer, 0700) != 0)
            {
                ESP_LOGE(TAG, "Could not create Directory: %s", strerror(errno));
            }
            else
                ESP_LOGI(TAG, "Createt Directory");
        }
    }
    strcat(buffer, "/");
    strcat(buffer, fileName);
    return fopen(buffer, openMethod);
}

bool fileExists_sdcard(const char *directory, const char *fileName)
{
    char buffer[strlen(sdCardInstance.Mount_Point) + strlen(directory) + strlen(fileName) + 2];
    strcpy(buffer, sdCardInstance.Mount_Point);
    if (directory != NULL)
    {
        strcat(buffer, directory);
        strcat(buffer, "/");
    }
    strcat(buffer, fileName);
    struct stat st = {0};
    return stat(buffer, &st) != -1;
}

uint64_t sdcard_remainingSpace_update()
{
    if (!sdCardInstance.cardInit)
        return 0;
    sd_card_state_t state;
    device_state_get_member(&state, device_state_statusSDCard, false);

    FATFS *fs;
    uint32_t fre_clust, fre_sect, tot_sect;
    FRESULT res = f_getfree(sdCardInstance.Mount_Point, &fre_clust, &fs);
    if (res)
    {
        printf("Couldn't find out SdCard size\n");
        state.freeSpace = 0;
        state.maxSpace = 0;
        state.status |= SD_CARD_STATE_INIT;
        device_state_set_member(&state, device_state_statusSDCard);
        return 0;
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    uint64_t tmp_total_bytes = (uint64_t)tot_sect * FF_SS_SDCARD;
    uint64_t tmp_free_bytes = (uint64_t)fre_sect * FF_SS_SDCARD;

    state.freeSpace = tmp_free_bytes;
    state.maxSpace = tmp_total_bytes;
    printf("%llu total bytes. %llu free bytes. sector_size=%u\n", tmp_total_bytes, tmp_free_bytes, FF_SS_SDCARD);
    device_state_set_member(&state, device_state_statusSDCard);
    return tmp_free_bytes;
}

esp_err_t GetFlights(int startIndex, widget_Settings_linkedList *flightListHead)
{
    DIR *dir;
    struct dirent *ent;
    char *extension = ".igc";

    if ((dir = opendir("/sdcard/IGCFlights")) != NULL)
    {
        int index = 0;
        int tookenFiles = 0;
        while ((ent = readdir(dir)) != NULL)
        {
            char *fileName = ent->d_name;
            int len = strlen(fileName);
            if (len > 4 && strcmp(fileName + len - 4, extension) == 0)
            {
                index++;
                if (index < startIndex)
                    continue;
                if (flightListHead == NULL)
                {
                    closedir(dir);
                    printf("Couldn't find next Flight\n");
                    return ESP_ERR_INVALID_ARG;
                }
                printf("FlighName: %s\n", fileName);
                strcpy(((FlightList_Widget_Data *)flightListHead->data.extr_data)->FlightName, fileName);
                flightListHead = flightListHead->next;
                tookenFiles++;
            }
            if (tookenFiles >= 6)
                break;
        }
        closedir(dir);
        if (tookenFiles < 6 && flightListHead)
        {
            ((FlightList_Widget_Data *)flightListHead->data.extr_data)->FlightName[0] = '\0';
        }
    }
    else
    {
        printf("Cant open directory\n");
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_OK;
}

#endif