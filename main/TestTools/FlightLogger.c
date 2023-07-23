#include "FlightLogger.h"
#include "nmea_parser.h"
#include "DeviceContext.h"
#include "vario_config.h"
#include "esp_spiffs.h"

#include <string.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

bool sdCard_init = false;
static const char *TAG = "FlightLogger";
char fileNameVario[40];
char fileNameNMEA[40];
char fileNameGPSUpdate[40];
FILE *fileVario;
FILE *fileNMEA;
FILE *fileGPSUpdate;
int countVarioFile;
nmea_parser_handle_t nmea_parser_hndl;
const char *directoryLogs = "/Trackloggs";

void FlightLogger_NMEASentenceLogger(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (!fileGPSUpdate)
        return;
    int64_t time;
    static int count = 0;
    gps_t *gps;
    switch (event_id)
    {
#ifdef NotifyNMEASentence
    case GPS_NMEA_SENTECE:
        break;
#endif
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
#if LOGVARIODATA == 1 || LOGVARIODATA == 2
        // Write GPS Time
        fwrite(&gps->tim.hour, sizeof(uint8_t), 1, fileGPSUpdate);
        fwrite(&gps->tim.minute, sizeof(uint8_t), 1, fileGPSUpdate);
        fwrite(&gps->tim.second, sizeof(uint8_t), 1, fileGPSUpdate);
        fwrite(&gps->tim.thousand, sizeof(uint16_t), 1, fileGPSUpdate);
        fwrite(&gps->fix, sizeof(gps_fix_t), 1, fileGPSUpdate);
        fwrite(&gps->fix_mode, sizeof(gps_fix_mode_t), 1, fileGPSUpdate);
        fwrite(&gps->valid, sizeof(bool), 1, fileGPSUpdate);
        fwrite(&gps->dop_h, sizeof(float), 1, fileGPSUpdate);
        fwrite(&gps->dop_p, sizeof(float), 1, fileGPSUpdate);
        fwrite(&gps->dop_v, sizeof(float), 1, fileGPSUpdate);
        fwrite(&gps->sats_in_use, sizeof(uint8_t), 1, fileGPSUpdate);
        fwrite(&gps->sats_in_view, sizeof(uint8_t), 1, fileGPSUpdate);
        fwrite(&gps->latitude, sizeof(float), 1, fileGPSUpdate);
        fwrite(&gps->longitude, sizeof(float), 1, fileGPSUpdate);
        fwrite(&gps->altitude, sizeof(float), 1, fileGPSUpdate);
        fwrite(&gps->speed, sizeof(float), 1, fileGPSUpdate);
        fwrite(&gps->cog, sizeof(float), 1, fileGPSUpdate);
        fwrite(&gps->variation, sizeof(float), 1, fileGPSUpdate);
        fprintf(fileGPSUpdate, "\n"); // new line
        if (count % 10 == 0)
        {
            // fflush(fileGPSUpdate);
            fclose(fileGPSUpdate);
#if LOGVARIODATA == 1
            fileGPSUpdate = openFile_sdcard(directoryLogs, fileNameGPSUpdate, "a");
#elif LOGVARIODATA == 2
            fileGPSUpdate = fopen("/spiffs/gps.txt", "a");
#endif
        }
#endif
    default:
        break;
    }
    count++;
}

static esp_err_t startLogging_SDcard()
{
    esp_err_t err = init_sdcard("/sdcard");
    if (err != ESP_OK)
        return err;
    int count = 0;
    do
    {
        snprintf(fileNameVario, sizeof(fileNameVario), "Vario%02d.txt", count++);
    } while (fileExists_sdcard(directoryLogs, fileNameVario));
    ESP_LOGI(TAG, "Found file: %s", fileNameVario);
    fileVario = openFile_sdcard(directoryLogs, fileNameVario, "w");
    if (fileVario == NULL)
    {
        ESP_LOGE(TAG, "Could not create Vario File\n");
        return ESP_ERR_NOT_FOUND;
    }

    count = 0;
    do
    {
        snprintf(fileNameGPSUpdate, sizeof(fileNameGPSUpdate), "GPS%02d.txt", count++);
    } while (fileExists_sdcard(directoryLogs, fileNameGPSUpdate));
    ESP_LOGI(TAG, "Found file: %s", fileNameGPSUpdate);
    fileGPSUpdate = openFile_sdcard(directoryLogs, fileNameGPSUpdate, "w");

    if (fileGPSUpdate == NULL)
    {
        ESP_LOGE(TAG, "Could not create GPS Update File\n");
        return ESP_ERR_NOT_FOUND;
    }
    sd_card_state_t sdstate;
    device_state_get_member(&sdstate, device_state_statusSDCard, false);
    sdstate.status |= SD_CARD_STATE_FLIGHTLOGGING;
    device_state_set_member(&sdstate, device_state_statusSDCard);
    sdCard_init = true;
    return ESP_OK;
}

esp_err_t logging_outputToUart()
{
    esp_vfs_spiffs_conf_t config = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_vfs_spiffs_register(&config);

    FILE *file = fopen("/spiffs/vario.txt", "r");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "File does not exist!");
    }
    else
    {
        printf("VARIO:::::::::::::::::::\n");
        const uint32_t bufferSize = 20;
        char buffer[bufferSize];
        while (1)
        {
            size_t read = fread(buffer, sizeof(char), bufferSize, file);
            if (read < bufferSize)
                break;
            fwrite(buffer, sizeof(char), read, stdout);
            vTaskDelay((TickType_t)5);
        }
        fclose(file);
    }
    return ESP_OK;
    file = fopen("/spiffs/gps.txt", "r");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "File does not exist!");
    }
    else
    {
        printf("GPS:::::::::::::::::::\n");
        const uint32_t bufferSize = 20;
        char buffer[bufferSize];
        while (1)
        {
            size_t read = fread(buffer, sizeof(char), bufferSize, file);
            if (read < bufferSize)
                break;
            fwrite(buffer, sizeof(char), read, stdout);
            vTaskDelay((TickType_t)1);
        }
        fclose(file);
    }
    esp_vfs_spiffs_unregister(NULL);
    return ESP_OK;
}

esp_err_t startLogging_Flash()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    // #ifdef CONFIG_EXAMPLE_SPIFFS_CHECK_ON_START
    //     ESP_LOGI(TAG, "Performing SPIFFS_check().");
    //     ret = esp_spiffs_check(conf.partition_label);
    //     if (ret != ESP_OK)
    //     {
    //         ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
    //         return;
    //     }
    //     else
    //     {
    //         ESP_LOGI(TAG, "SPIFFS_check() successful");
    //     }
    // #endif

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    ret = esp_spiffs_format(conf.partition_label);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
    //     esp_spiffs_format(conf.partition_label);
    //     return ret;
    // }
    // else
    // {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    // }

    // Check consistency of reported partiton size info.
    if (used > total)
    {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return ret;
        }
        else
        {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }
    fileVario = fopen("/spiffs/vario.txt", "w");
    if (!fileVario)
    {
        ESP_LOGE(TAG, "cant open Vario file");
        return ESP_ERR_FLASH_BASE;
    }
    ESP_LOGI(TAG, "Opend fileVario");
    fileGPSUpdate = fopen("/spiffs/gps.txt", "w");
    if (!fileGPSUpdate)
    {
        ESP_LOGE(TAG, "cant open Vario file");
        return ESP_ERR_FLASH_BASE;
    }
    return ESP_OK;
}

esp_err_t startLogging()
{
#if LOGVARIODATA == 0
    return ESP_OK;
#endif
#ifdef NotifyNMEASentence
    nmea_parser_add_handler(nmea_parser_hndl, FlightLogger_NMEASentenceLogger, NULL, GPS_NMEA_SENTECE);
#endif
    nmea_parser_add_handler(nmea_parser_hndl, FlightLogger_NMEASentenceLogger, NULL, GPS_UPDATE);
#if LOGVARIODATA == 1
    startLogging_SDcard();
#elif LOGVARIODATA == 2
    startLogging_Flash();
#endif
    return ESP_OK;
}

void initLogging(nmea_parser_handle_t nmea_parser_handle)
{
    nmea_parser_hndl = nmea_parser_handle;
}

void endLogging()
{
}

esp_err_t logVarioData(FlightLogger_t *state)
{
#if LOGVARIODATA == 0
    return ESP_OK;
#endif
    if (!fileVario)
    {
        printf("Not valid file\n");
        return ESP_ERR_INVALID_STATE;
    }
    // fwrite(state, sizeof(FlightLogger_t), 1, fileVario); // write vario_state to file
    fwrite(&state->acc, sizeof(float) * 3, 1, fileVario);
    fwrite(&state->gyr, sizeof(float), 3, fileVario);
    fwrite(&state->magn, sizeof(float), 3, fileVario);
    fwrite(&state->pressure, sizeof(float), 1, fileVario);
    fprintf(fileVario, "\n");     // new line
    if (countVarioFile % 40 == 0) // save File all 40th time ==> all 1200 ms
    {
        // fflush(fileVario);
        fclose(fileVario);
#if LOGVARIODATA == 1
        fileVario = openFile_sdcard(directoryLogs, fileNameVario, "a");
#elif LOGVARIODATA == 2
        fileVario = fopen("/spiffs/vario.txt", "a");
#endif
        // fileVario = openFile_sdcard(sdcard, directoryLogs, fileNameVario, true);
    }
    countVarioFile++;
    return ESP_OK;
}