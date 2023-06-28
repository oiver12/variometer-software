#ifndef __ERRORHANDLING_VARIO_H
#define __ERRORHANDLING_VARIO_H

#include <esp_err.h>

#define ERROR_CHECKRETURN(val) \
    do                         \
    {                          \
        esp_err_t err = val;   \
        if (err != ESP_OK)     \
            return err;        \
    } while (0)

#define ERROR_RETRYTIMES(func, retries, err)          \
    do                                                \
    {                                                 \
        int i = 0;                                    \
        while ((err = func) != ESP_OK && i < retries) \
        {                                             \
            i++;                                      \
        }                                             \
    } while (0)

#define ERROR_HANDLEBYERROR(err, location, message) \
    do                                              \
    {                                               \
        if (err != ESP_OK)                          \
            error_handle(err, location, message);   \
    } while (0)

typedef enum error_location
{
    ERR_AIRSPACEPARCER,
    ERR_BLE,
    ERR_IGC,
    ERR_SCREEN,
    ERR_BMI088,
    ERR_DPS310,
    ERR_QMC588L,
    ERR_AGL,
    ERR_BATTERY,
    ERR_BUTTONS,
    ERR_FLASH,
    ERR_MAIN,
    ERR_MAINTASKS,
    ERR_NMEA,
    ERR_SDCARD,
    ERR_SOUNDMANAGER,
    ERR_LOGGING
} error_location;

void error_handle(esp_err_t err, error_location location, char *message);

#endif