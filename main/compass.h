#ifndef COMPASS_VARIO_H
#define COMPASS_VARIO_H

#include <stdint.h>

void compass_get(int16_t *magnValues, float *attitudeQuaternion, float *heading);

#endif