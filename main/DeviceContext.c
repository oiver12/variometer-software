#include "DeviceContext.h"
#include "SoundManager.h"
#include "Screen/Widget_Vario.h"
#include "Util.h"
#include "Screen/ScreenManager.h"
#include "FlightState.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define RADIUS (6371000) // 6371e3

#define TO_RADIAN(x) ((x) * (PI / 180))
#define GET_DISTANCELATLON(angle) (2.0 * RADIUS * sin(TO_RADIAN((angle) / 2)))

SoundManager_Tone_t gpsFix_ToneArray[3] =
    {
        {262, 500},
        {339, 500},
        {392, 500},
};

const static char *TAG = "DEVICE CONTEXT";

vario_state_t vario_state;
vario_settings_t vario_settings;
paraglider_info_t paraglider_info;
device_state_t device_state;
flight_stats_t flight_stats;

uint16_t currSizeFIFOVertVelBiep;

SemaphoreHandle_t semaphore_vario_state;
SemaphoreHandle_t semaphore_vario_settings;
SemaphoreHandle_t semaphore_paraglider_info;
SemaphoreHandle_t semaphore_device_state;
SemaphoreHandle_t semaphore_flight_stats;

static void updateFlightStatsTrackHistory(gps_t *gps, float vertVel)
{
    // ESP_LOGI("DEVICE", "Update Fligt Stats");
    int nextPoint = (flight_stats.frontPoint + 1) % MAX_TRACKPOINT_SIZE;
    if (nextPoint == flight_stats.rearPoint)
        flight_stats.rearPoint = (flight_stats.rearPoint + 1) % MAX_TRACKPOINT_SIZE;

    flight_stats.trackPoints[flight_stats.frontPoint].lat = gps->latitude;
    flight_stats.trackPoints[flight_stats.frontPoint].lon = gps->longitude;
    flight_stats.trackPoints[flight_stats.frontPoint].vario = vertVel;

    flight_stats.frontPoint = nextPoint;

    int latest = (flight_stats.frontPoint + MAX_TRACKPOINT_SIZE - 1) % MAX_TRACKPOINT_SIZE;

    for (int i = flight_stats.rearPoint; i != latest;)
    {
        flight_stats.trackDistance[i].dx = -GET_DISTANCELATLON(flight_stats.trackPoints[latest].lon - flight_stats.trackPoints[i].lon);
        flight_stats.trackDistance[i].dy = GET_DISTANCELATLON(flight_stats.trackPoints[latest].lat - flight_stats.trackPoints[i].lat);
        i = (i + 1) % MAX_TRACKPOINT_SIZE;
    }
    flight_stats.trackDistance[latest].dx = 0;
    flight_stats.trackDistance[latest].dy = 0;
    flight_stats.stateChange |= flight_stats_trackPoints;
}

static void updateFlightStats(gps_t *gps)
{
    // Update Flight State
    float vario;
    int heading;
    vario_state_get_member(&vario, vario_state_vertVel, false);
    vario_state_get_member(&heading, vario_state_heading, false);
    FlightState_update(gps, vario, (int16_t)heading);

    // Update flight Stats
    xSemaphoreTake(semaphore_flight_stats, portMAX_DELAY);
    flight_stats.altitudeMax = fmaxf(flight_stats.altitudeMax, gps->altitude);
    flight_stats.altitudeMin = fminf(flight_stats.altitudeMin, gps->altitude);
    flight_stats.maxVelX = fmaxf(flight_stats_maxVelX, vario);
    flight_stats.minVelX = fminf(flight_stats_minVelX, vario);
    // we are flying
    if (flight_stats.takeOffTime > 0)
    {
        flight_stats.flightTime = (esp_timer_get_time() / 1000) - flight_stats.takeOffTime;
        flight_stats.distTakeOff = GET_DISTANCE(gps->latitude, gps->longitude, flight_stats.takeOffPosition.lat, flight_stats.takeOffPosition.lon);
        flight_stats.distFlight += GET_DISTANCE(gps->latitude, gps->longitude, flight_stats.lastPosition.lat, flight_stats.lastPosition.lon);
    }
    flight_stats.lastPosition.lat = gps->latitude;
    flight_stats.lastPosition.lon = gps->longitude;
    flight_stats.lastPosition.alt = gps->altitude;
    updateFlightStatsTrackHistory(gps, vario);
    xSemaphoreGive(semaphore_flight_stats);
}

static void device_state_gps_handle(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gps_t *gps = NULL;
    gps_fix_mode_t oldFixMode;
    switch (event_id)
    {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        // ESP_LOGI("GPS", "Sats IV: %i, Sats IU: %i, fix: %i", (int)gps->sats_in_view, (int)gps->sats_in_use, (int)gps->fix);
        //  update vario state
        vario_state_set_member(&gps->altitude, vario_state_altitudeGPS);
        float speedGroundKMH = gps->speed * 3.6f;
        vario_state_set_member(&speedGroundKMH, vario_state_groundSpeed);
        float lastLong;
        float lastLat;
        vario_state_get_member(&lastLong, vario_state_longitude, false);
        vario_state_get_member(&lastLat, vario_state_latitude, false);
        int gpsHeading = GET_BEARING(gps->latitude, gps->longitude, lastLat, lastLong);
        vario_state_set_member(&gpsHeading, vario_state_gpsHeading);
        vario_state_set_member(&gps->latitude, vario_state_latitude);
        vario_state_set_member(&gps->longitude, vario_state_longitude);
        vario_state_set_member(gps, vario_state_gps_data);
        // update device state
        xSemaphoreTake(semaphore_device_state, portMAX_DELAY);
        oldFixMode = device_state.statusGPS;
        device_state.statusGPS = gps->fix_mode;
        xSemaphoreGive(semaphore_device_state);
        // check for GPS Fix
        if (gps->fix_mode != GPS_MODE_INVALID && oldFixMode == GPS_MODE_INVALID)
        {
            PopUpConfig_t config = {
                .x = 150,
                .y = 100,
                .w = 100,
                .h = 100,
                .type = PopUpType_GPSFix,
                .timeToDisplay = 1000,
            };
            ScreenManager_showPopUp(config);
            SoundManager_playToneArray(gpsFix_ToneArray, 3);
        }
        if (gps->fix_mode != GPS_MODE_INVALID)
        {
            updateFlightStats(gps);
        }
        break;
    default:
        printf("GPS Event konnte nicht gefunden werden!\n");
        break;
    }
}

void init_DeviceContext()
{
    semaphore_vario_state = xSemaphoreCreateMutex();
    semaphore_vario_settings = xSemaphoreCreateMutex();
    semaphore_paraglider_info = xSemaphoreCreateMutex();
    semaphore_device_state = xSemaphoreCreateMutex();
    semaphore_flight_stats = xSemaphoreCreateMutex();
    vario_state.vertVel_Fifo = fifo_create(FIFO_VERTLAZY_SIZE, sizeof(float));
    currSizeFIFOVertVelBiep = 0;
    flight_stats.rearPoint = flight_stats.frontPoint = 0;

    vario_settings.soundVolume = 10000;
    device_state.statusGPS = GPS_MODE_INVALID;
    FlightState_init();
}

void startGps_DeviceContext(nmea_parser_handle_t nmea_parser_handle)
{
    nmea_parser_add_handler(nmea_parser_handle, device_state_gps_handle, NULL, GPS_UPDATE);
}

static void vertVelBiepSetFIFOCount(uint16_t size)
{
    xSemaphoreTake(semaphore_vario_state, portMAX_DELAY);
    currSizeFIFOVertVelBiep = size;
    fifo_create_static(&vario_state.vertVelBiep_Fifo, vario_state.VertVelbuffer, size, sizeof(float));
    vario_state.vertVelBiep_Sum = 0;
    xSemaphoreGive(semaphore_vario_state);
}

void vario_state_set_member(void *value, vario_state_member_t member)
{
    xSemaphoreTake(semaphore_vario_state, portMAX_DELAY);
    switch (member)
    {
    case vario_state_vertVel:
        vario_state.vertVelocity = *((float *)value);
        float temp;
        if (fifo_is_full(vario_state.vertVel_Fifo))
        {
            fifo_get(vario_state.vertVel_Fifo, &temp);
            vario_state.vertVel_Sum -= temp;
        }
        vario_state.vertVel_Sum += vario_state.vertVelocity;
        fifo_add(vario_state.vertVel_Fifo, &vario_state.vertVelocity);
        vario_state.stateChange |= vario_state_vertVelLazy;

        if (currSizeFIFOVertVelBiep == 0)
            break;
        if (fifo_is_full(&vario_state.vertVelBiep_Fifo))
        {
            fifo_get(&vario_state.vertVelBiep_Fifo, &temp);
            vario_state.vertVelBiep_Sum -= temp;
        }
        vario_state.vertVelBiep_Sum += vario_state.vertVelocity;
        fifo_add(&vario_state.vertVelBiep_Fifo, &vario_state.vertVelocity);
        vario_state.stateChange |= vario_state_vertVelLazyBiep;
        break;
    case vario_state_groundSpeed:
        vario_state.groundSpeed = *((float *)value);
        break;
    case vario_state_vertVelLazy:
        ESP_LOGW(TAG, "Cant set vertVelLazy set vertVel, and then get vertVelLazy");
        break;
    case vario_state_vertAcc:
        vario_state.vertAcc = *((float *)value);
        break;
    case vario_state_pressure:
        vario_state.pressure = *((float *)value);
        break;
    case vario_state_temperature:
        vario_state.temperature = *((float *)value);
        break;
    case vario_state_heading:
        if (*((int *)value) > 360)
            *((int *)value) = (int)*((int *)value) % 360;
        else if (*((int *)value) < 0)
            *((int *)value) = (int)-*((int *)value) % 360;
        vario_state.heading = *((int *)value);
        break;
    case vario_state_altitudeBaro:
        vario_state.altitudeBaro = *((float *)value);
        break;
    case vario_state_altitudeGPS:
        vario_state.altitudeGps = *((float *)value);
        break;
    case vario_state_gps_data:
        vario_state.gps_data = *((gps_t *)value);
        break;
    case vario_state_groundLevel:
        vario_state.groundLevel = *((float *)value);
        break;
    case vario_state_agl:
        vario_state.agl = *((float *)value);
        break;
    case vario_state_longitude:
        vario_state.longitude = *((float *)value);
        break;
    case vario_state_latitude:
        vario_state.latitude = *((float *)value);
        break;
    case vario_state_gpsHeading:
        vario_state.gpsHeading = *((int *)value);
        break;
    case vario_state_airspaceInfo:
        vario_state.airspaceInfo = *((Airspace_Info_t *)value);
        break;
    default:
        ESP_LOGW(TAG, "Member von Vario State konnte nicht gefunden werden!");
        break;
    }
    vario_state.stateChange |= member;
    xSemaphoreGive(semaphore_vario_state);
}

bool vario_state_get_member(void *value, vario_state_member_t member, bool stateChange)
{
    xSemaphoreTake(semaphore_vario_state, portMAX_DELAY);
    switch (member)
    {
    case vario_state_vertVel:
        *(float *)value = vario_state.vertVelocity;
        break;
    case vario_state_groundSpeed:
        *(float *)value = vario_state.groundSpeed;
        break;
    case vario_state_vertVelLazy:
        *(float *)value = vario_state.vertVel_Sum / FIFO_VERTLAZY_SIZE;
        break;
    case vario_state_vertAcc:
        *(float *)value = vario_state.vertAcc;
        break;
    case vario_state_pressure:
        *(float *)value = vario_state.pressure;
        break;
    case vario_state_temperature:
        *(float *)value = vario_state.temperature;
        break;
    case vario_state_heading:
        *(int *)value = vario_state.heading;
        break;
    case vario_state_altitudeBaro:
        *(float *)value = vario_state.altitudeBaro;
        break;
    case vario_state_altitudeGPS:
        *(float *)value = vario_state.altitudeGps;
        break;
    case vario_state_gps_data:
        *(gps_t *)value = vario_state.gps_data;
        break;
    case vario_state_groundLevel:
        *(float *)value = vario_state.groundLevel;
        break;
    case vario_state_agl:
        *(float *)value = vario_state.agl;
        break;
    case vario_state_longitude:
        *(float *)value = vario_state.longitude;
        break;
    case vario_state_latitude:
        *(float *)value = vario_state.latitude;
        break;
    case vario_state_gpsHeading:
        *(int *)value = vario_state.gpsHeading;
        break;
    case vario_state_airspaceInfo:
        *(Airspace_Info_t *)value = vario_state.airspaceInfo;
        break;
    case vario_state_vertVelLazyBiep:
        if (currSizeFIFOVertVelBiep == 0)
        {
            *(float *)value = vario_state.vertVelocity;
            break;
        }
        *(float *)value = vario_state.vertVelBiep_Sum / currSizeFIFOVertVelBiep;
        break;
    default:
        printf("Member von Vario State konnte nicht gefunden werden!\n");
        break;
    }
    bool returnValue = vario_state.stateChange & member;
    if (stateChange)
        vario_state.stateChange &= 0xFFFF - member;
    xSemaphoreGive(semaphore_vario_state);
    return returnValue;
}

void device_state_set_member(void *value, device_state_member_t member)
{
    xSemaphoreTake(semaphore_device_state, portMAX_DELAY);
    switch (member)
    {
    case device_state_batteryPercentage:
        device_state.batteryPercentage = *((float *)value);
        break;
    case device_state_statusGPS:
        device_state.statusGPS = *((gps_fix_mode_t *)value);
        break;
    case device_state_statusSDCard:
        device_state.status_sd_card = *((sd_card_state_t *)value);
        break;
    default:
        printf("Member von Device State konnte nicht gefunden werden!\n");
        break;
    }
    device_state.stateChange |= member;
    xSemaphoreGive(semaphore_device_state);
}

bool device_state_get_member(void *value, device_state_member_t member, bool stateChange)
{
    xSemaphoreTake(semaphore_device_state, portMAX_DELAY);
    switch (member)
    {
    case device_state_batteryPercentage:
        *(float *)value = device_state.batteryPercentage;
        break;
    case device_state_statusGPS:
        *(gps_fix_mode_t *)value = device_state.statusGPS;
        break;
    case device_state_statusSDCard:
        *(sd_card_state_t *)value = device_state.status_sd_card;
        break;
    default:
        printf("Member von Device State konnte nicht gefunden werden!\n");
        break;
    }
    bool returnValue = device_state.stateChange & member;
    if (stateChange)
        device_state.stateChange &= 0xFFFF - member;
    xSemaphoreGive(semaphore_device_state);
    return returnValue;
}

void vario_settings_set_member(void *value, vario_settings_member_t member)
{
    xSemaphoreTake(semaphore_vario_settings, portMAX_DELAY);
    switch (member)
    {
    case vario_settings_soundVolume:
        if (*((uint16_t *)value) <= Volume_max_Size)
        {
            vario_settings.soundVolume = *((uint16_t *)value);
            SoundManager_setVolume(vario_settings.soundVolume);
        }
        else
            printf("Volume to high!\n");
        break;
    case vario_settings_takeOffSpeed:
        vario_settings.takeOffSpeed = *((float *)value);
        break;
    case vario_settings_landingSpeed:
        vario_settings.landingSpeed = *((float *)value);
        break;
    case vario_settings_landingTimout:
        vario_settings.landingTimout = *((float *)value);
        break;
    case vario_settings_biepLazyTime:
        if (*((uint16_t *)value) <= FIFO_MAX_BIEPVERTLAZY_SIZE)
        {
            vario_settings.biepLazyTime = *((uint16_t *)value);
            vertVelBiepSetFIFOCount(*((uint16_t *)value));
            currSizeFIFOVertVelBiep = *((uint16_t *)value);
        }
        else
            printf("Mean Time Lazy to high\n");
        break;
    default:
        printf("Member von Vario Settings konnte nicht gefunden werden!\n");
        break;
    }
    vario_settings.stateChange |= member;
    xSemaphoreGive(semaphore_vario_settings);
}

bool vario_settings_get_member(void *value, vario_settings_member_t member, bool stateChange)
{
    xSemaphoreTake(semaphore_vario_settings, portMAX_DELAY);
    switch (member)
    {
    case vario_settings_soundVolume:
        *(uint16_t *)value = vario_settings.soundVolume;
        break;
    case vario_settings_takeOffSpeed:
        *(float *)value = vario_settings.takeOffSpeed;
        break;
    case vario_settings_landingSpeed:
        *(float *)value = vario_settings.landingSpeed;
        break;
    case vario_settings_landingTimout:
        *(float *)value = vario_settings.landingTimout;
        break;
    case vario_settings_biepLazyTime:
        *(uint16_t *)value = vario_settings.biepLazyTime;
        break;
    default:
        printf("Member von Vario Settings konnte nicht gefunden werden!\n");
        break;
    }
    bool returnValue = vario_settings.stateChange & member;
    if (stateChange)
        vario_settings.stateChange &= 0xFFFF - member;
    xSemaphoreGive(semaphore_vario_settings);
    return returnValue;
}

void flight_stats_set_member(void *value, flight_stats_member_t member)
{

    xSemaphoreTake(semaphore_flight_stats, portMAX_DELAY);
    switch (member)
    {
    case flight_stats_circlingTime:
        flight_stats.circlingTime = *((uint64_t *)value);
        break;
    case flight_stats_circlingGain:
        flight_stats.circlingGain = *((float *)value);
        break;
    case flight_stats_glideRatio:
        flight_stats.glideRatio = *((float *)value);
        break;
    case flight_stats_takeOffTime:
        flight_stats.takeOffTime = *((uint64_t *)value);
        break;
    case flight_stats_takeOffPosition:
        flight_stats.takeOffPosition = *((Position_t *)value);
        break;
    default:
        printf("Member von Flight Stats konnte nicht gefunden werden!\n");
        break;
    }
    flight_stats.stateChange |= member;
    xSemaphoreGive(semaphore_flight_stats);
}

bool flight_stats_get_member(void *value, flight_stats_member_t member, bool stateChange)
{
    xSemaphoreTake(semaphore_flight_stats, portMAX_DELAY);
    switch (member)
    {
    case flight_stats_altitudeMax:
        *(float *)value = flight_stats.altitudeMax;
        break;
    case flight_stats_altitudeMin:
        *(float *)value = flight_stats.altitudeMin;
        break;
    case flight_stats_maxVelX:
        *(float *)value = flight_stats.maxVelX;
        break;
    case flight_stats_minVelX:
        *(float *)value = flight_stats.minVelX;
        break;
    case flight_stats_takeOffTime:
        *(uint32_t *)value = flight_stats.takeOffTime;
        break;
    case flight_stats_flightTime:
        *(uint32_t *)value = flight_stats.flightTime;
        break;
    case flight_stats_takeOffPosition:
        *(Position_t *)value = flight_stats.takeOffPosition;
        break;
    case flight_stats_distTakeOff:
        *(float *)value = flight_stats.distTakeOff;
        break;
    case flight_stats_distFlight:
        *(float *)value = flight_stats.distFlight;
        break;
    case flight_stats_lastPosition:
        *(Position_t *)value = flight_stats.lastPosition;
        break;
    default:
        printf("Member von Flight Stats konnte nicht gefunden werden!\n");
        break;
    }
    bool returnValue = flight_stats.stateChange & member;
    if (stateChange)
        flight_stats.stateChange &= 0xFFFF - member;
    xSemaphoreGive(semaphore_flight_stats);
    return returnValue;
}

flight_stats_t *flight_stats_take_trackPoints()
{
    if (!(flight_stats.stateChange & flight_stats_trackPoints))
        return NULL;
    xSemaphoreTake(semaphore_flight_stats, portMAX_DELAY);
    return &flight_stats;
}

void flight_stats_give_trackPoints()
{
    xSemaphoreGive(semaphore_flight_stats);
}