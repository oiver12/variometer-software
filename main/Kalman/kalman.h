#ifndef _KALMAN_H_
#define _KALMAN_H_

#include <stdint.h>
#include "matrix.h"

#define KALMAN_NUM_STATES 2       // height, velocity
#define KALMAN_NUM_INPUTS 1       // vertical acceleration
#define KALMAN_NUM_MEASUREMENTS 1 // height from Barometer

#define KALMAN_AUX_ROWS_SIZE ((KALMAN_NUM_STATES > KALMAN_NUM_MEASUREMENTS) ? KALMAN_NUM_STATES : KALMAN_NUM_MEASUREMENTS)

typedef struct
{
    // State Vector
    matrix_t x;
    float x_Buffer[KALMAN_NUM_STATES * 1];
    // Output Vector
    matrix_t z;
    float z_Buffer[KALMAN_NUM_MEASUREMENTS * 1];
    // State Transition Matrix
    matrix_t F;
    float F_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_STATES];
    // Input Variable
    matrix_t u;
    float u_Buffer[KALMAN_NUM_INPUTS * 1];
    // Control Matrix
    matrix_t G;
    float G_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_INPUTS];
    // Estimate Uncertainty
    matrix_t P;
    float P_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_STATES];
    // Process Noise Uncertainty
    matrix_t Q;
    float Q_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_STATES];
    // Measurement Uncertainty
    matrix_t R;
    float R_Buffer[KALMAN_NUM_MEASUREMENTS * KALMAN_NUM_MEASUREMENTS];
    // Oberservation Matrix
    matrix_t H;
    float H_Buffer[KALMAN_NUM_MEASUREMENTS * KALMAN_NUM_STATES];
    // Kalman Gain
    matrix_t K;
    float K_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_MEASUREMENTS];
    // Residual covariance matrix
    matrix_t S;
    float S_Buffer[KALMAN_NUM_MEASUREMENTS * KALMAN_NUM_MEASUREMENTS];
    // Innovativ vector
    matrix_t y;
    float y_Buffer[KALMAN_NUM_MEASUREMENTS * 1];
    struct
    {
        matrix_data_t aux[KALMAN_AUX_ROWS_SIZE * 1];
        matrix_t S_inv;
        float S_inv_Buffer[KALMAN_NUM_MEASUREMENTS * KALMAN_NUM_MEASUREMENTS];
        matrix_t HP;
        float HP_Buffer[KALMAN_NUM_MEASUREMENTS * KALMAN_NUM_STATES];
        matrix_t KH;
        float KH_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_STATES];
        matrix_t KHP;
        float KHP_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_STATES];
        matrix_t KHtP;
        float KHtP_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_STATES];
        matrix_t PHt;
        float PHt_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_MEASUREMENTS];
        matrix_t identity;
        float identity_Buffer[KALMAN_NUM_STATES * KALMAN_NUM_STATES];
        matrix_t xtemp;
        float xtemp_Buffer[KALMAN_NUM_STATES * 1];
        matrix_t Gu;
        float Gu_Buffer[KALMAN_NUM_STATES * 1];
    } temporary;

} kalman_t;

void kalman_filter_init(kalman_t *kf);
void kalman_filter_reset(kalman_t *kf);

void kalman_update(kalman_t *kf);

void kalman_predict(kalman_t *kf);

#endif