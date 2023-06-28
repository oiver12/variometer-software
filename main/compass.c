#include "compass.h"
#include <math.h>

#define M_PI 3.14159265358979323846

//https://github.com/fhorinek/SkyDrop/blob/master/skydrop/src/fc/compass.cpp

void quaternion_multiply(float *q1, float *q2, float *result)
{
    result[0] = q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2] - q1[3]*q2[3];
    result[1] = q1[0]*q2[1] + q1[1]*q2[0] + q1[2]*q2[3] - q1[3]*q2[2];
    result[2] = q1[0]*q2[2] - q1[1]*q2[3] + q1[2]*q2[0] + q1[3]*q2[1];
    result[3] = q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1] + q1[3]*q2[0];
}

void compass_get(int16_t *magnValues, float *attitudeQuaternion, float *heading)
{
    float q_conj[4] = { attitudeQuaternion[0], -attitudeQuaternion[1], -attitudeQuaternion[2], -attitudeQuaternion[3] };
    float v[4] = { 0, magnValues[0], magnValues[1], magnValues[2] };
    float vTemp[4];
    quaternion_multiply(attitudeQuaternion, v, vTemp);
    float v_body[4];
    quaternion_multiply(vTemp, q_conj, v_body);
    *heading = atan2(v_body[2], v_body[1]) * 180.0 / M_PI;
    // float a12 =   2.0f * (attitudeQuaternion[1] * attitudeQuaternion[2] + attitudeQuaternion[0] * attitudeQuaternion[3]);
	// float a22 =   attitudeQuaternion[0] * attitudeQuaternion[0] + attitudeQuaternion[1] *attitudeQuaternion[1] - attitudeQuaternion[2] * attitudeQuaternion[2] - attitudeQuaternion[3] * attitudeQuaternion[3];

	// *heading = atan2f(a12, a22)* 180 / M_PI;
    while(*heading > 360.0)
    	*heading -= 360.0;
    while(*heading < 0.0)
    	*heading += 360.0;
}