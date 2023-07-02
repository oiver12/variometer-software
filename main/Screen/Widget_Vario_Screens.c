#include "Widget_Vario.h"
#include "E_Ink/GUI_Paint.h"
#include "assets/assets.h"
#include "Widget_Vario_Common.h"
#include "IGC/IGC_Parser.h"
#include "SDCard.h"
#include "Widget_Vario.h"
#include "RenderSystem.h"
#include "ErrorHandling.h"

#include <esp_timer.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

void vario_display_PowerOffScreen()
{
    Paint_Clear(WHITE);
    Paint_DrawBitMap_Paste(gImage_PowerOff, Paint.Width / 2 - 50, 50, 100, 100, FALSE);
    const char *stringToShow = "Confirm Power Off";
    uint32_t stringLength = strlen(stringToShow) * Font24.Width;
    Paint_DrawString_EN(Paint.Width / 2 - stringLength / 2, 175, stringToShow, &Font24, WHITE, BLACK, FALSE);
    const char *nextString = "Press the power button again to turn off the device";
    const char *nextString1 = "Press any other button to cancel";
    stringLength = strlen(nextString) * Font12.Width;
    Paint_DrawString_EN(Paint.Width / 2 - stringLength / 2, 220, nextString, &Font12, WHITE, BLACK, FALSE);
    stringLength = strlen(nextString1) * Font12.Width;
    Paint_DrawString_EN(Paint.Width / 2 - stringLength / 2, 240, nextString1, &Font12, WHITE, BLACK, FALSE);
}

void vario_display_FlightStatistics(void *params)
{
    char *fileName = (char *)params;
    char buffer[100];
    FILE *igcFile = openFile_sdcard("/IGCFlights", fileName, "r");
    if (igcFile == NULL)
    {
        error_handle(ESP_ERR_NOT_FOUND, ERR_SCREEN, "Can't find IGC File");
        vTaskDelete(NULL);
        return;
    }
    const int x = 5;
    const int y = 50;
    const int width = 470;
    const int height = 100;
    Track_t track;
    memset(&track, 0, sizeof(Track_t));
    FILE *cachedFile = openFile_sdcard("/cache", fileName, "r");
    if (cachedFile == NULL)
    {
        // doesn't exist
        // maybe deleted so parse full and write cache
        long time = esp_timer_get_time();
        esp_err_t err = ParseIGCFile(&track, width, igcFile, false, false, fileName);
        printf("Clock: %i\n", (int)(esp_timer_get_time() - time));
        if (err != ESP_OK)
        {
            error_handle(err, ERR_IGC, "Can't parse IGC File without cache");
            fclose(igcFile);
            // TODO: display error message
            vTaskDelete(NULL);
            return;
        }
    }
    else
    {
        long time = esp_timer_get_time();
        esp_err_t err = ParseIGCWithCached(&track, width, igcFile, cachedFile, false, false, fileName);
        printf("Clock cached: %i\n", (int)(esp_timer_get_time() - time));
        fclose(cachedFile);
        if (err != ESP_OK)
        {
            error_handle(err, ERR_IGC, "Can't parse cached IGC File");
            fclose(igcFile);
            // TODO: display error message
            vTaskDelete(NULL);
            return;
        }
    }
    Paint_Clear(WHITE);
    DrawTextWithAlign(fileName, 480 / 2, 10, &Font16, true, ALIGN_CENTER, ALIGN_TOP);
    Paint_DrawRectangle(x, y, x + width, y + height, BLACK, DOT_PIXEL_1X1, FALSE);
    DrawTextWithAlign("Height Diagramm", 240, y, &Font16, true, ALIGN_CENTER, ALIGN_MID);
    int gapDia = (track.flightStats.altitudeMax - track.flightStats.altitudeMin) * 0.1f;
    int maxHeightDiagramm = track.flightStats.altitudeMax + gapDia;
    int minHeightDiagramm = track.flightStats.altitudeMin - gapDia;
    int i = 0;
    while (i < track.sizeTrackPoints && i <= width)
    {
        float percentageFromHeight = ((float)(track.trackPoints[i].gnssAlt - minHeightDiagramm)) / (float)(maxHeightDiagramm - minHeightDiagramm);
        float heightY = (uint16_t)((1 - percentageFromHeight) * (float)height) + y;
        Paint_DrawPoint(x + i, heightY, BLACK, DOT_PIXEL_1X1, DOT_FILL_AROUND);
        i++;
    }
    sprintf(buffer, "%im", (int)track.flightStats.altitudeMax);
    Paint_DrawString_EN(5, 50, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "%im", (int)track.flightStats.altitudeMin);
    Paint_DrawString_EN(5, 140, buffer, &Font12, WHITE, BLACK, true);
    Paint_DrawRectangle(5, 150, 240, 275, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawString_EN(5, 160, "Flug", &Font16, WHITE, BLACK, true);
    uint8_t dauerH, dauerM;
    Get_duration_diff(track.trackPoints[0].time, track.trackPoints[track.sizeTrackPoints - 1].time, &dauerH, &dauerM);
    snprintf(buffer, 50, "Duration: %i Hour %i min", (int)dauerH, (int)dauerM);
    Paint_DrawString_EN(15, 180, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Length Tracklog: %.1f km", (float)track.flightStats.distFlight / 1000.0f);
    Paint_DrawString_EN(15, 195, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Max. Height: %i m", (int)track.flightStats.altitudeMax);
    Paint_DrawString_EN(15, 210, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Min. Height: %i m", (int)track.flightStats.altitudeMin);
    Paint_DrawString_EN(15, 225, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Max. Climb: %.1f m/s", track.flightStats.maxVelX);
    Paint_DrawString_EN(15, 240, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Max. Sink: %.1f m/s", track.flightStats.minVelX);
    Paint_DrawString_EN(15, 255, buffer, &Font12, WHITE, BLACK, true);

    Paint_DrawRectangle(240, 150, 475, 212, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawString_EN(250, 160, "Start", &Font16, WHITE, BLACK, true);
    snprintf(buffer, 50, "Time: %i:%i:%i", track.trackPoints[0].time.hour, track.trackPoints[0].time.minute, track.trackPoints[0].time.second);
    Paint_DrawString_EN(255, 180, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Height: %im", track.trackPoints[0].gnssAlt);
    Paint_DrawString_EN(255, 195, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Lat: %.2f ", fabsf(track.flightStats.takeOffPosition.lat));
    strcat(buffer, track.flightStats.takeOffPosition.lat < 0 ? "S" : "N");
    DrawTextWithAlign(buffer, 465, 180, &Font12, true, ALIGN_RIGHT, ALIGN_TOP);
    snprintf(buffer, 50, "Lon: %.2f ", fabsf(track.flightStats.takeOffPosition.lon));
    strcat(buffer, track.flightStats.takeOffPosition.lat < 0 ? "W" : "E");
    DrawTextWithAlign(buffer, 465, 195, &Font12, true, ALIGN_RIGHT, ALIGN_TOP);

    Paint_DrawRectangle(240, 212, 475, 275, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawString_EN(250, 222, "Landung", &Font16, WHITE, BLACK, true);
    snprintf(buffer, 50, "Time: %i:%i:%i", track.trackPoints[track.sizeTrackPoints - 1].time.hour, track.trackPoints[0].time.minute, track.trackPoints[0].time.second);
    Paint_DrawString_EN(255, 242, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Height: %im", track.trackPoints[track.sizeTrackPoints - 1].gnssAlt);
    Paint_DrawString_EN(255, 257, buffer, &Font12, WHITE, BLACK, true);
    snprintf(buffer, 50, "Lat: %.2f ", fabsf(track.flightStats.takeOffPosition.lat));
    strcat(buffer, track.flightStats.takeOffPosition.lat < 0 ? "S" : "N");
    DrawTextWithAlign(buffer, 465, 242, &Font12, true, ALIGN_RIGHT, ALIGN_TOP);
    snprintf(buffer, 50, "Lon: %.2f ", fabsf(track.flightStats.takeOffPosition.lon));
    strcat(buffer, track.flightStats.takeOffPosition.lat < 0 ? "W" : "E");
    DrawTextWithAlign(buffer, 465, 257, &Font12, true, ALIGN_RIGHT, ALIGN_TOP);
    FreeTrackFile(&track);
    rendering_Push(true);
    vTaskDelete(NULL);
    return;
}
