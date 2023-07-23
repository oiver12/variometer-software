#include "FlightState.h"
#include "Util.h"
#include "DeviceContext.h"
#include "Screen/Widget_Vario.h"
#include "SoundManager.h"
#include "Screen/ScreenManager.h"

#include <stdlib.h>
#include <esp_timer.h>

#define CIRCLING_HEADING_THRESHOLD 10
#define CIRCLING_EXIT_COUNT 3
#define GLIDING_COUNT_MAX 10
#define GLIDING_COUNT_MIN 0
#define GLIDING_START_COUNT 6
#define GLIDING_EXIT_COUNT 6

SoundManager_Tone_t takeOffMelody[8] =
    {
        {262, 1000 / 4},
        {196, 1000 / 8},
        {196, 1000 / 8},
        {220, 1000 / 4},
        {196, 1000 / 4},
        {0, 1000 / 4},
        {247, 1000 / 4},
        {262, 1000 / 4},
};

SoundManager_Tone_t landingMelody[8] =
    {
        {262, 1000 / 4},
        {196, 1000 / 8},
        {196, 1000 / 8},
        {220, 1000 / 4},
        {196, 1000 / 4},
        {0, 1000 / 4},
        {247, 1000 / 4},
        {262, 1000 / 4},
};

FlightMode_t currFlightM;
FlightState_t currFlightSt;
int16_t deltaHeading_SUM;
int32_t glidingCount;
uint64_t lastUnderLandingSpeed;
int16_t lastHeading;

uint64_t circlingStartTime;
Position_t circlingStartPos;

Position_t glidingStartPos;

void FlightState_init()
{
    currFlightM = FM_STANDING;
    currFlightSt = FST_READY;
    deltaHeading_SUM = 0;
    glidingCount = 0;
    lastUnderLandingSpeed = 0;
    lastHeading = 0;
    circlingStartTime = 0;
}

static void startFlight(gps_t *gps)
{
    currFlightSt = FST_FLYING;
    currFlightM = FM_FLYING;
    uint64_t takeOffTime = esp_timer_get_time() / 1000;
    Position_t takeOffPosition = {
        .lat = gps->latitude,
        .lon = gps->longitude,
        .alt = gps->altitude};
    flight_stats_set_member(&takeOffTime, flight_stats_takeOffTime);
    flight_stats_set_member(&takeOffPosition, flight_stats_takeOffPosition);
    PopUpConfig_t config = {
        .x = 150,
        .y = 100,
        .w = 100,
        .h = 60,
        .type = PopUpType_TakeOff,
        .timeToDisplay = 3000,
    };
    ScreenManager_showPopUp(config);
    SoundManager_playToneArray(takeOffMelody, 8);
    // startLogging();
    // start_IGCLogger();
}

static void stopFlight()
{
    currFlightSt = FST_LANDED;
    currFlightM = FM_STANDING;
    // stopLogging();
    // stop_IGCLogging();
    SoundManager_playToneArray(landingMelody, 8);
    // show Flight Stats Screen
}

static void startCircling(gps_t *gps)
{
    currFlightM = FM_CIRCLING;
    circlingStartTime = esp_timer_get_time();
    circlingStartPos.alt = gps->altitude;
    circlingStartPos.lat = gps->latitude;
    circlingStartPos.lon = gps->longitude;
    glidingCount = 0;
    // show Circling display
    return;
}

static void stopCircling()
{
    currFlightM = FM_FLYING;
    deltaHeading_SUM = 0;
    // show normal flying screen
}

static void startGliding(gps_t *gps)
{
    currFlightM = FM_GLIDING;
    glidingStartPos.lat = gps->latitude;
    glidingStartPos.lon = gps->longitude;
    glidingStartPos.alt = gps->altitude;
    float glideRatio = -1;
    flight_stats_set_member(&glideRatio, flight_stats_glideRatio);
}

static void stopGliding()
{
    currFlightM = FM_FLYING;
    float glideRatio = -1;
    flight_stats_set_member(&glideRatio, flight_stats_glideRatio);
}

static void update_FlightMode(gps_t *gps, int16_t heading)
{
    if (currFlightSt != FST_FLYING)
        return;

    int16_t deltaHeading = heading - lastHeading;
    lastHeading = heading;
    if (deltaHeading > 180)
        deltaHeading = deltaHeading - 360;
    if (deltaHeading < -180)
        deltaHeading = deltaHeading + 360;

    deltaHeading_SUM += deltaHeading;
    deltaHeading_SUM = _CLAMP(deltaHeading_SUM, -450, 450); // 1.5 turns
    if (abs(deltaHeading) < CIRCLING_HEADING_THRESHOLD)
        glidingCount = _MIN(GLIDING_COUNT_MAX, glidingCount + 1);
    else
        glidingCount = _MAX(GLIDING_COUNT_MIN, glidingCount - 1);
    switch (currFlightM)
    {
    case FM_FLYING:
        if (abs(deltaHeading_SUM) > 360)
            startCircling(gps); // flying (normal) -> circling
        else if (glidingCount > GLIDING_START_COUNT)
            startGliding(gps); // flying (normal) -> gliding
        break;
    case FM_CIRCLING:
        uint64_t circlingTime = esp_timer_get_time() - circlingStartTime;
        float circlingGain = gps->altitude - circlingStartPos.alt;
        flight_stats_set_member(&circlingTime, flight_stats_circlingTime);
        flight_stats_set_member(&circlingGain, flight_stats_circlingGain);
        // Update Circling Stats
        if (glidingCount > CIRCLING_EXIT_COUNT)
        {
            stopCircling(); // circling -> flying (normal)
        }
        break;
    case FM_GLIDING:
        float dist = GET_DISTANCEFAST(glidingStartPos.lat, glidingStartPos.lon, gps->latitude, gps->longitude);
        if (dist > 100)
        {
            float glideRatio = 0;
            if (glidingStartPos.alt > gps->altitude)
            {
                glideRatio = dist / (glidingStartPos.alt - gps->altitude);
            }
            flight_stats_set_member(&glideRatio, flight_stats_glideRatio);
        }
        if (glidingCount < GLIDING_EXIT_COUNT)
            stopGliding(); // gliding -> flying (normal)
        break;
    default:
        printf("Flight Mode not found\n");
        break;
    }
}

void FlightState_update(gps_t *gps, float vario, int16_t heading)
{
    if (!gps->valid)
        return;
    if (currFlightSt == FST_READY && gps->speed * 3.6 > TAKEOFF_SPEED)
    {
        currFlightSt = FST_FLYING;
        startFlight(gps);
    }
    if (currFlightSt == FST_FLYING)
    {
        if (gps->speed * 3.6 < LANDING_SPEED)
        {
            if ((esp_timer_get_time() - lastUnderLandingSpeed) > LANDING_THERSHOLD)
            {
                currFlightSt = FST_LANDED;
                stopFlight();
            }
        }
        else
        {
            lastUnderLandingSpeed = esp_timer_get_time();
            update_FlightMode(gps, heading);
        }
    }
}
// void FlightState_get(FlightState_t *flightState, FlightMode_t *mode)
// {
//     flightState = &currFlightSt;
//     mode = &currFlightM;
// }