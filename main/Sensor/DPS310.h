#ifndef DPS310_SPI_DRIVER_H
#define DPS310_SPI_DRIVER_H

#include <esp_err.h>

typedef struct
{
    int32_t m_c0Half;
    int32_t m_c1; 
    int32_t m_c00;
	int32_t m_c10;
	int32_t m_c01;
	int32_t m_c11;
	int32_t m_c20;
	int32_t m_c21;
	int32_t m_c30;
} dsp310_config_data_t;

typedef enum
{
    DPS310_MR_1 = 0,
    DPS310_MR_2 = 1,
    DPS310_MR_4 = 2,
    DPS310_MR_8 = 3,
    DPS310_MR_16 = 4,
    DPS310_MR_32 = 5,
    DPS310_MR_64 = 6,
    DPS310_MR_128 = 7,
} dps310_mr_t;

typedef enum
{
    DPS310_OSR_1 = 0,
    DPS310_OSR_2 = 1,
    DPS310_OSR_4 = 2,
    DPS310_OSR_8 = 3,
    DPS310_OSR_16 = 4,
    DPS310_OSR_32 = 5,
    DPS310_OSR_64 = 6,
    DPS310_OSR_128 = 7,
} dps310_osr_t;

typedef struct 
{
    int I2C_Master_Num;
    uint8_t dps310_t_i2c_addr;
    double Pressure;
    double Temperature;
    uint32_t lastRawTemp;
    dsp310_config_data_t config_data;
    dps310_mr_t temp_measurement_rate;
    dps310_osr_t temp_oversampling;
    dps310_mr_t prs_measurement_rate;
    dps310_osr_t prs_oversampling;
    float m_lastTempScal;
} dps310_t;

esp_err_t dps310_init(dps310_t *dev, int I2C_Master_Num, uint8_t i2c_addr);
esp_err_t dps310_config_Temp_Pressure(dps310_t *dev, dps310_mr_t temp_mr, dps310_osr_t temp_osr, dps310_mr_t prs_mr, dps310_osr_t prs_osr);
esp_err_t dsp310_get_Pressure(dps310_t *dev);
esp_err_t dps310_get_Temperature(dps310_t *dev);

#endif