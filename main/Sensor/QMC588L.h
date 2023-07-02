#ifndef QMC588L_I2C_DRIVER
#define QMC588L_I2C_DRIVER

#include "Screen/Widget_Vario.h"

#include <stdbool.h>
#include <esp_err.h>
#include <driver/i2c.h>

typedef enum
{
    QMC588L_MR_10HZ = 0b00 << 2,
    QMC588L_MR_50HZ = 0b01 << 2,
    QMC588L_MR_100HZ = 0b10 << 2,
    QMC588L_MR_200HZ = 0b11 << 2
} qmc588l_mr_t;

typedef enum
{
    QMC588L_OSR_512 = 0b00 << 6,
    QMC588L_OSR_256 = 0b01 << 6,
    QMC588L_OSR_128 = 0b10 << 6,
    QMC588L_OSR_64 = 0b11 << 6
} qmc588l_osr_t;

typedef enum
{
    QMC588L_SCALE_2G = 0b00 << 4,
    QMC588L_SCALE_8G = 0b01 << 4
} qmc588l_scale_t;

typedef struct
{
    i2c_port_t I2C_Master_Num;
    uint8_t qmc588l_i2c_addr;
    int16_t rawMagnValue[3];
    float CorrectedMagnValuemG[3];
    int _vCalibration[3][2];
    bool useCalibration;
    qmc588l_mr_t measurement_rate;
    qmc588l_osr_t oversampling;
    qmc588l_scale_t scale;
    bool isCalibrating;
} qmc588l_t;

qmc588l_t *qmc588l_getInstance();
esp_err_t qmc588l_init(i2c_port_t I2C_Master_Num, uint8_t i2c_addr, qmc588l_mr_t mr, qmc588l_osr_t osr, qmc588l_scale_t scale);
esp_err_t qmc588l_setMode(qmc588l_mr_t mr, qmc588l_osr_t osr, qmc588l_scale_t scale);
esp_err_t qmc588l_setCalibration(int x_min, int x_max, int y_min, int y_max, int z_min, int z_max);
void qmc588l_calculateCalibration();
esp_err_t qmc588l_readMag();

#endif