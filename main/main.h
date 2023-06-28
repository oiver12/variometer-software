#ifndef __Main_H_
#define __Main_H_

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(VARIO_STATE_EVENT);

// typedef struct {
//     float height;
//     float velocity;
//     float accX;
// } vario_state_t;

typedef struct {
    float height;
    float velocity;
    float accX;
    float rotation[4];
} FlightLogger_t;

typedef enum {
    Vert_Velocity_Update,
    Sound_Update,
    GPS_Update
} vario_state_event_id;

#endif