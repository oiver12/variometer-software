#ifndef BMI088_SPI_DRIVER_H
#define BMI088_SPI_DRIVER_H

#include "vario_config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_err.h>
#include <driver/i2c.h>

typedef enum
{
    ACCEL_RANGE_3G,
    ACCEL_RANGE_6G,
    ACCEL_RANGE_12G,
    ACCEL_RANGE_24G
} bmi088_accelRange_t;

typedef enum
{
    GYRO_RANGE_2000DPS,
    GYRO_RANGE_1000DPS,
    GYRO_RANGE_500DPS,
    GYRO_RANGE_250DPS,
    GYRO_RANGE_125DPS
} bmi088_gyroRange_t;

typedef enum
{
    ACC_ODR_1600HZ_BW_280HZ,
    ACC_ODR_1600HZ_BW_234HZ,
    ACC_ODR_1600HZ_BW_145HZ,
    ACC_ODR_800HZ_BW_230HZ,
    ACC_ODR_800HZ_BW_140HZ,
    ACC_ODR_800HZ_BW_80HZ,
    ACC_ODR_400HZ_BW_145HZ,
    ACC_ODR_400HZ_BW_75HZ,
    ACC_ODR_400HZ_BW_40HZ,
    ACC_ODR_200HZ_BW_80HZ,
    ACC_ODR_200HZ_BW_38HZ,
    ACC_ODR_200HZ_BW_20HZ,
    ACC_ODR_100HZ_BW_40HZ,
    ACC_ODR_100HZ_BW_19HZ,
    ACC_ODR_100HZ_BW_10HZ,
    ACC_ODR_50HZ_BW_20HZ,
    ACC_ODR_50HZ_BW_9HZ,
    ACC_ODR_50HZ_BW_5HZ,
    ACC_ODR_25HZ_BW_10HZ,
    ACC_ODR_25HZ_BW_5HZ,
    ACC_ODR_25HZ_BW_3HZ,
    ACC_ODR_12_5HZ_BW_5HZ,
    ACC_ODR_12_5HZ_BW_2HZ,
    ACC_ODR_12_5HZ_BW_1HZ
} bmi088_acc_odr_t;

typedef enum
{
    GYR_ODR_2000HZ_BW_532HZ = 0x80,
    GYR_ODR_2000HZ_BW_230HZ = 0x81,
    GYR_ODR_1000HZ_BW_116HZ = 0x82,
    GYR_ODR_400HZ_BW_47HZ = 0x83,
    GYR_ODR_200HZ_BW_23HZ = 0x84,
    GYR_ODR_100HZ_BW_12HZ = 0x85,
    GYR_ODR_200HZ_BW_64HZ = 0x86,
    GYR_ODR_100HZ_BW_32HZ = 0x87
} bmi088_gyr_odr_t;

typedef struct
{
    i2c_port_t I2C_Master_Num;
    uint8_t acc_i2c_addr;
    uint8_t gyr_i2c_addr;
    bmi088_accelRange_t accelRange;
    bmi088_gyroRange_t gyroRange;
    bmi088_acc_odr_t acc_odr;
    bmi088_gyr_odr_t gyr_odr;
    float acc_mps2[3];
    float gyr_rps[3];
} bmi088_t;

esp_err_t bmi088_init(bmi088_t *dev, int I2C_Master_Num, uint8_t acc_i2c_addr, uint8_t gyr_i2c_addr);
// esp_err_t bmi088_setOdr(bmi088_t *dev, bmi088_acc_odr_t odr, bmi088_gyr_odr_t gyr_odr);
// esp_err_t bmi088_setRange(bmi088_t *dev, bmi088_accelRange_t accelRange, bmi088_gyroRange_t gyroRange);
bool bmi088_canRead(bmi088_t *dev);
esp_err_t bmi088_ReadAccelerometer(bmi088_t *dev);
esp_err_t bmi088_ReadGyroskop(bmi088_t *dev);

#endif