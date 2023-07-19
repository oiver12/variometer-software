#include "FlightLogger.h"
#include "nmea_parser.h"
#include "DeviceContext.h"
#include "vario_config.h"

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
#if LOGVARIODATA == 1
        time = esp_timer_get_time();
        fwrite(&time, sizeof(time), 1, fileGPSUpdate); // write time to file
        // Write GPS Time
        fwrite(&gps->tim.hour, sizeof(uint8_t), 1, fileGPSUpdate);
        fwrite(&gps->tim.minute, sizeof(uint8_t), 1, fileGPSUpdate);
        fwrite(&gps->tim.second, sizeof(uint8_t), 1, fileGPSUpdate);
        fwrite(&gps->tim.thousand, sizeof(uint8_t), 1, fileGPSUpdate);
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
            fclose(fileGPSUpdate);
            fileGPSUpdate = openFile_sdcard(directoryLogs, fileNameGPSUpdate, "a");
        }
#elif LOGVARIODATA == 2
        printf("g");
        fwrite(&gps->tim.hour, sizeof(uint8_t), 1, stdout);
        fwrite(&gps->tim.minute, sizeof(uint8_t), 1, stdout);
        fwrite(&gps->tim.second, sizeof(uint8_t), 1, stdout);
        fwrite(&gps->tim.thousand, sizeof(uint8_t), 1, stdout);
        fwrite(&gps->fix, sizeof(gps_fix_t), 1, stdout);
        fwrite(&gps->fix_mode, sizeof(gps_fix_mode_t), 1, stdout);
        fwrite(&gps->valid, sizeof(bool), 1, stdout);
        fwrite(&gps->dop_h, sizeof(float), 1, stdout);
        fwrite(&gps->dop_p, sizeof(float), 1, stdout);
        fwrite(&gps->dop_v, sizeof(float), 1, stdout);
        fwrite(&gps->sats_in_use, sizeof(uint8_t), 1, stdout);
        fwrite(&gps->sats_in_view, sizeof(uint8_t), 1, stdout);
        fwrite(&gps->latitude, sizeof(float), 1, stdout);
        fwrite(&gps->longitude, sizeof(float), 1, stdout);
        fwrite(&gps->altitude, sizeof(float), 1, stdout);
        fwrite(&gps->speed, sizeof(float), 1, stdout);
        fwrite(&gps->cog, sizeof(float), 1, stdout);
        fwrite(&gps->variation, sizeof(float), 1, stdout);
        printf("\n");
#endif
    default:
        break;
    }
    count++;
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
    return
#elif LOGVARIODATA == 1
    if (!sdCard_init)
        return ESP_ERR_INVALID_STATE;
    fwrite(state, sizeof(FlightLogger_t), 1, fileVario); // write vario_state to file
    int64_t time = esp_timer_get_time();
    fwrite(&time, sizeof(int64_t), 1, fileVario); // write Time since startup to file
    fprintf(fileVario, "\n");                     // new line
    if (countVarioFile % 40 == 0)                 // save File all 40th time ==> all 1200 ms
    {
        // fflush(fileVario);
        fclose(fileVario);
        fileVario = openFile_sdcard(directoryLogs, fileNameVario, "a");
        // fileVario = openFile_sdcard(sdcard, directoryLogs, fileNameVario, true);
    }
    countVarioFile++;
#elif LOGVARIODATA == 2
    printf("d");
    fwrite(state, sizeof(FlightLogger_t), 1, stdout);
    printf("\n");
#endif
        return ESP_OK;
}