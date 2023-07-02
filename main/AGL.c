#include "AGL.h"
#include "DeviceContext.h"

#include <esp_log.h>
#include <string.h>
#include <math.h>

static const char* directory = "/AGL";
const char* TAG = "AGL";

AGLContext_t agl_context;

static const int secondsPerPx = 1;  //arc seconds per pixel (1 equals cca 30m)
static const int totalPx = 3601;

static void readPixel_HGTFile(int y, int x, int* height)
{
    int row = (totalPx - 1) - y;
    int col = x;
    int pos = (row * totalPx + col) * 2;
    
    uint8_t temp[2];
    fseek(agl_context.currFile, pos, SEEK_SET);
    fread(temp, sizeof(uint8_t), 2, agl_context.currFile);
	*height = 0 | (temp[0] << 8) | temp[1];
}

static void AGL_gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gps_t *gps = NULL;
    switch (event_id) 
	{
		case GPS_UPDATE:
            gps = (gps_t*)event_data;
            if(gps->fix_mode != GPS_MODE_INVALID && sdCard_Init())
            {
                gps = (gps_t *)event_data;
                int latDec = (int)floor(gps->latitude);
                int lonDec = (int)floor(gps->longitude);
                if(!agl_context.currFile || (int)gps->latitude != agl_context.mLat || (int)gps->longitude != agl_context.mLong)
                {
                    agl_context.mLat = (int)gps->latitude;
                    agl_context.mLong = (int)gps->longitude;
                    char fileName[50];
                    sprintf(fileName, "%c%02d%c%03d.hgt",
                    gps->longitude > 0 ? 'N' : 'S', abs(latDec),
                    gps->longitude > 0 ? 'E' : 'W', abs(lonDec));

                    ESP_LOGI(TAG, "AGL File: %s", fileName);

                    if(agl_context.currFile)
                        fclose(agl_context.currFile);
                    if(!fileExists_sdcard(directory, fileName))
                    {
                        ESP_LOGW(TAG, "AGL File doens't exist: %s", fileName);
                        return;
                    }
                    agl_context.currFile = openFile_sdcard(directory, fileName, "r");
                }
                if(!agl_context.currFile)
                {
                    ESP_LOGW(TAG, "AGL File couldn't be opened");
                    return;
                }
                //http://viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org1.htm
                //https://gis.stackexchange.com/questions/43743/extracting-elevation-from-hgt-file
                float secondsLat = (gps->latitude - latDec) * 60 * 60;
                float secondsLon = (gps->longitude - lonDec) * 60 * 60;
                int y = secondsLat / secondsPerPx;
                int x = secondsLon / secondsPerPx;

                int height[4];
                readPixel_HGTFile(y, x, &height[2]);
                readPixel_HGTFile(y + 1, x, &height[0]);
                readPixel_HGTFile(y, x + 1, &height[3]);
                readPixel_HGTFile(y + 1, x + 1, &height[1]);

                //ratio where X lays
                float dy = fmod(secondsLat, secondsPerPx) / secondsPerPx;
                float dx = fmod(secondsLon, secondsPerPx) / secondsPerPx;

                // Bilinear interpolation
                // h0------------h1
                // |
                // |--dx-- .
                // |       |
                // |      dy
                // |       |
                // h2------------h3   
                    
                float groundLevel = height[0] * dy * (1 - dx) +
                    height[1] * dy * (dx)+
                    height[2] * (1 - dy) * (1 - dx) +
                    height[3] * (1 - dy) * dx;
                float agl = gps->altitude - groundLevel;
                if(agl < 0)
                    agl = 0;
                vario_state_set_member(&groundLevel, vario_state_groundLevel);
                vario_state_set_member(&agl, vario_state_agl);
            }
			break;
		default:
			break;
    }
}


void AGL_startAGL(nmea_parser_handle_t nmea_parser_handle)
{
    agl_context.nmea_parser_handle = nmea_parser_handle;
    agl_context.mLat = 0;
    agl_context.mLong = 0;
    agl_context.currFile = NULL;
    nmea_parser_add_handler(nmea_parser_handle, AGL_gps_event_handler, NULL, GPS_UPDATE);
}