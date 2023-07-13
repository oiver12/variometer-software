#ifdef ISSIMULATION
#include "SDCard.h"
#include <unistd.h>
#include <sys/stat.h>
#include <esp_log.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>

static const char *TAG = "SDCard";

sdcard_t sdCardInstance;

esp_err_t init_sdcard(const char *Mount_Point)
{
    sdCardInstance.cardInit = true;
    sdCardInstance.Mount_Point = malloc(strlen(Mount_Point) + 1);
    strcpy(sdCardInstance.Mount_Point, Mount_Point);
    return ESP_OK;
}
esp_err_t deinit_sdcard()
{
    free(sdCardInstance.Mount_Point);
    sdCardInstance.Mount_Point = NULL;
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
bool sdCard_Init()
{
    return sdCardInstance.cardInit;
}
esp_err_t writeToFile_sdcard()
{
}
uint64_t sdcard_remainingSpace_update()
{
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