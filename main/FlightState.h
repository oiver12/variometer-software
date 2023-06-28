#ifndef __FLIGHTSTATE_H
#define __FLIGHTSTATE_H

#include "nmea_parser.h"

#define TAKEOFF_SPEED 8         // km/h
#define LANDING_SPEED 2         // km/h
#define LANDING_THERSHOLD 10000 // ms

typedef enum FlightState_t
{
    FST_READY,
    FST_FLYING,
    FST_LANDED
} FlightState_t;

typedef enum FlightMode_t
{
    FM_STANDING,
    FM_FLYING,
    FM_GLIDING,
    FM_CIRCLING,
} FlightMode_t;

void FlightState_init();
void FlightState_update(gps_t *gps, float vario, int16_t heading);
// void FlightState_get(FlightState_t *flightState, FlightMode_t *mode);

#endif