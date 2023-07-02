#ifndef SDCARD_VARIO_H
#define SDCARD_VARIO_H

#include "Screen/SettingsManager.h"

#include <esp_err.h>
#include <sdmmc_cmd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdint.h>

#define MAX_FILENAME 100

typedef struct FlightFile_t
{
    char fileName[MAX_FILENAME];
    long timeStamp;
} FlightFile_t;
typedef struct
{
    bool cardInit;
    char *Mount_Point;
    sdmmc_card_t *card;
} sdcard_t;

esp_err_t init_sdcard(const char *Mount_Point);
esp_err_t deinit_sdcard();
FILE *openFile_sdcard(const char *directory, const char *fileName, const char *openMethod);
bool fileExists_sdcard(const char *directory, const char *fileName);
bool sdCard_Init();
esp_err_t writeToFile_sdcard();
uint64_t sdcard_remainingSpace_update();
esp_err_t GetFlights(int startIndex, widget_Settings_linkedList *flightListHead);

#endif