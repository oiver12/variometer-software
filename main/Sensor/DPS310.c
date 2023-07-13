#ifndef ISSIMULATION

#include "DPS310.h"
#include "I2C_Manager.h"
#include "ErrorHandling.h"
#include "vario_config.h"
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_timer.h>

#define I2C_MASTER_TIMEOUT_MS 100

#define DPS310__BUSYTIME_FAILSAFE 10U
#define DPS__BUSYTIME_SCALING 10U
#define DPS310__MAX_BUSYTIME ((1000U - DPS310__BUSYTIME_FAILSAFE) * DPS__BUSYTIME_SCALING)

const int32_t scaling_facts[8] = {524288, 1572864, 3670016, 7864320, 253952, 516096, 1040384, 2088960};

static const char *TAG = "DPS310";

static inline esp_err_t write_Bytes(dps310_t *dev, uint8_t addr, size_t length, uint8_t *data)
{
    return i2c_manager_writeBytes(dev->I2C_Master_Num, dev->dps310_t_i2c_addr, addr, data, length);
}

static inline esp_err_t write_Byte(dps310_t *dev, uint8_t addr, const uint8_t data)
{
    return i2c_manager_writeByte(dev->I2C_Master_Num, dev->dps310_t_i2c_addr, addr, data);
}

static inline esp_err_t read_Bytes(dps310_t *dev, uint8_t addr, size_t size, uint8_t *data)
{
    return i2c_manager_readBytes(dev->I2C_Master_Num, dev->dps310_t_i2c_addr, addr, data, size);
}

static esp_err_t writeBitByteField(dps310_t *dev, uint8_t addr, uint8_t data, uint8_t mask, uint8_t shift)
{
    return i2c_manager_writeBitByteField(dev->I2C_Master_Num, dev->dps310_t_i2c_addr, addr, data, mask, shift);
}

static void getTwosComplement(int32_t *raw, uint8_t length)
{
    if (*raw & ((uint32_t)1 << (length - 1)))
    {
        *raw -= (uint32_t)1 << length;
    }
}

static uint16_t calcBusyTime(dps310_mr_t mr, dps310_osr_t osr)
{
    // formula from datasheet (optimized)
    return ((uint32_t)20U << (uint16_t)mr) + ((uint32_t)16U << ((uint16_t)osr + (uint16_t)mr));
}

static void calcCoefficents(dps310_t *dev, uint8_t buffer[])
{
    // compose coefficients from buffer content
    dev->config_data.m_c0Half = ((uint32_t)buffer[0] << 4) | (((uint32_t)buffer[1] >> 4) & 0x0F);
    getTwosComplement(&dev->config_data.m_c0Half, 12);
    // c0 is only used as c0*0.5, so c0_half is calculated immediately
    dev->config_data.m_c0Half = dev->config_data.m_c0Half / 2U;

    // now do the same thing for all other coefficients
    dev->config_data.m_c1 = (((uint32_t)buffer[1] & 0x0F) << 8) | (uint32_t)buffer[2];
    getTwosComplement(&dev->config_data.m_c1, 12);
    dev->config_data.m_c00 = ((uint32_t)buffer[3] << 12) | ((uint32_t)buffer[4] << 4) | (((uint32_t)buffer[5] >> 4) & 0x0F);
    getTwosComplement(&dev->config_data.m_c00, 20);
    dev->config_data.m_c10 = (((uint32_t)buffer[5] & 0x0F) << 16) | ((uint32_t)buffer[6] << 8) | (uint32_t)buffer[7];
    getTwosComplement(&dev->config_data.m_c10, 20);

    dev->config_data.m_c01 = ((uint32_t)buffer[8] << 8) | (uint32_t)buffer[9];
    getTwosComplement(&dev->config_data.m_c01, 16);

    dev->config_data.m_c11 = ((uint32_t)buffer[10] << 8) | (uint32_t)buffer[11];
    getTwosComplement(&dev->config_data.m_c11, 16);
    dev->config_data.m_c20 = ((uint32_t)buffer[12] << 8) | (uint32_t)buffer[13];
    getTwosComplement(&dev->config_data.m_c20, 16);
    dev->config_data.m_c21 = ((uint32_t)buffer[14] << 8) | (uint32_t)buffer[15];
    getTwosComplement(&dev->config_data.m_c21, 16);
    dev->config_data.m_c30 = ((uint32_t)buffer[16] << 8) | (uint32_t)buffer[17];
    getTwosComplement(&dev->config_data.m_c30, 16);
}

esp_err_t dps310_init(dps310_t *dev, int I2C_Master_Num, uint8_t i2c_addr)
{
    dev->I2C_Master_Num = I2C_Master_Num;
    dev->dps310_t_i2c_addr = i2c_addr;
    vTaskDelay(50 / portTICK_PERIOD_MS);
    /* Read Coeffs */
    uint8_t buffer[18];
    esp_err_t err;
    ERROR_RETRYTIMES(read_Bytes(dev, 0x10, 18, buffer), VARIO_I2C_RETRYS_INIT, err);
    if (err != ESP_OK)
    {
        return err;
    }
    calcCoefficents(dev, buffer);
    /* Configure DPS310 Background Measure Rate / OSR */
    ERROR_CHECKRETURN(dps310_config_Temp_Pressure(dev, DPS310_MR_1, DPS310_OSR_64, DPS310_MR_32, DPS310_OSR_16));
    /* Enable Background Measure */
    ERROR_CHECKRETURN(writeBitByteField(dev, 0x08, 0x07, 0x07, 0));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    /*  Enable Interrupt on Pressure Reading (active high) */
    ERROR_CHECKRETURN(writeBitByteField(dev, 0x09, 0x90, 0x90, 0));

    // tried to fix Temp reading
    //  ERROR_CHECKRETURN(write_Byte(dev, 0x0E, 0xA5));
    //  ERROR_CHECKRETURN(write_Byte(dev, 0x0F, 0x96));
    //  ERROR_CHECKRETURN(write_Bytes(dev, 0x62, 0x02));
    //  ERROR_CHECKRETURN(write_Bytes(dev, 0x0E, 0x00));
    //  ERROR_CHECKRETURN(write_Bytes(dev, 0x0F, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    return ESP_OK;
}

esp_err_t dps310_config_Temp_Pressure(dps310_t *dev, dps310_mr_t temp_mr, dps310_osr_t temp_osr, dps310_mr_t prs_mr, dps310_osr_t prs_osr)
{
    if (calcBusyTime(temp_mr, temp_osr) + calcBusyTime(prs_mr, prs_osr) >= DPS310__MAX_BUSYTIME)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t temp_cfg = (temp_mr << 4) | temp_osr;
    ERROR_CHECKRETURN(write_Bytes(dev, 0x07, 1, &temp_cfg));
    uint8_t prs_cfg = (prs_mr << 4) | prs_osr;
    ERROR_CHECKRETURN(write_Bytes(dev, 0x06, 1, &prs_cfg));
    if (prs_osr > DPS310_OSR_8)
    {
        ERROR_CHECKRETURN(writeBitByteField(dev, 0x09, 1UL, 0x04, 2));
    }
    else
    {
        ERROR_CHECKRETURN(writeBitByteField(dev, 0x09, 0UL, 0x04, 2));
    }
    if (temp_osr > DPS310_OSR_8)
    {
        ERROR_CHECKRETURN(writeBitByteField(dev, 0x09, 1UL, 0x08, 3));
    }
    else
    {
        ERROR_CHECKRETURN(writeBitByteField(dev, 0x09, 0UL, 0x08, 3));
    }
    dev->temp_measurement_rate = temp_mr;
    dev->temp_oversampling = temp_osr;
    dev->prs_measurement_rate = prs_mr;
    dev->prs_oversampling = prs_osr;
    return ESP_OK;
}

esp_err_t dsp310_get_Pressure(dps310_t *dev)
{
    // read raw pressure
    uint8_t buffer[3] = {0};
    ERROR_CHECKRETURN(read_Bytes(dev, 0x00, 3, buffer));
    // clear interrupt register
    uint8_t bufferInterrupt = 0;
    ERROR_CHECKRETURN(read_Bytes(dev, 0x0A, 1, &bufferInterrupt));
    int32_t raw = (uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2];
    getTwosComplement(&raw, 24);
    float prs = raw;
    prs /= scaling_facts[dev->prs_oversampling];
    prs = dev->config_data.m_c00 + prs * (dev->config_data.m_c10 + prs * (dev->config_data.m_c20 + prs * dev->config_data.m_c30)) + dev->m_lastTempScal * (dev->config_data.m_c01 + prs * (dev->config_data.m_c11 + prs * dev->config_data.m_c21));
    dev->Pressure = prs;
    return ESP_OK;
}

esp_err_t dps310_get_Temperature(dps310_t *dev)
{
    uint8_t buffer[3] = {0};
    esp_err_t err = read_Bytes(dev, 0x03, 3, buffer);
    if (err != ESP_OK)
    {
        return err;
    }
    int32_t raw = (uint32_t)buffer[0] << 16 | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2];
    getTwosComplement(&raw, 24);
    float temp = raw;
    temp /= scaling_facts[dev->temp_oversampling];
    dev->m_lastTempScal = temp;
    temp = dev->config_data.m_c0Half + dev->config_data.m_c1 * temp;
    dev->Temperature = temp;
    return ESP_OK;
}

#endif