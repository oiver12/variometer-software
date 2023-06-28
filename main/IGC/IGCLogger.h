#ifndef IGC_LOGGER_H
#define IGC_LOGGER_H

#include <esp_err.h>
#include "SDCard.h"
#include <time.h>
#include "nmea_parser.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

typedef struct
{
    gps_time_t time;
    gps_date_t date;
    float latitude;  /*Latitude (degrees) */
    float longitude; /*Longitude (degrees) */
    float altitude;  /*Altitude (meters) */
    bool fix3D;
} igc_gps_data_t;

typedef struct
{
    char nameBuffer[60];
    float pressureAltitude;
    volatile int64_t gpsLastTimeUpdate;
    FILE *file;
    nmea_parser_handle_t nmea_parser_handle;
    igc_gps_data_t gps_data;
} igc_logger_t;

void init_IGCLogger(nmea_parser_handle_t nmea_parser_handle);
void start_IGCLogger();
void updatePressureAltitude_IGCLogger(float height);
esp_err_t stop_IGCLogger();

#endif