#include "BMI088.h"
#include <esp_log.h>
#include <freertos/task.h>
#include "ErrorHandling.h"
#include "vario_config.h"
#include "I2C_Manager.h"

#define I2C_MASTER_TIMEOUT_MS 1000

#define ACC_CHIP_ID 0x1E
#define GYR_CHIP_ID 0x0F
/* Register defines */
#define BMI_ACC_CHIP_ID 0x00
#define BMI_ACC_DATA 0x12
#define BMI_TEMP_DATA 0x22
#define BMI_ACC_CONF 0x40
#define BMI_ACC_RANGE 0x41
#define BMI_INT1_IO_CONF 0x53
#define BMI_INT1_INT2_MAP_DATA 0x58
#define BMI_ACC_PWR_CONF 0x7C
#define BMI_ACC_PWR_CTRL 0x7D
#define BMI_ACC_SOFTRESET 0x7E

#define BMI_GYR_CHIP_ID 0x00
#define BMI_GYR_DATA 0x02
#define BMI_GYR_RANGE 0x0F
#define BMI_GYR_BANDWIDTH 0x10
#define BMI_GYR_SOFTRESET 0x14
#define BMI_GYR_INT_CTRL 0x15
#define BMI_INT3_INT4_IO_CONF 0x16
#define BMI_INT3_INT4_IO_MAP 0x18

#define accConversion 0.00089813232421875        /* (9.81f / 32768.0f * 2.0f * 1.5f) */
#define gyroConversion 0.00053263221771240234375 /* (0.01745329251f * 1000.0f / 32768.0f) */

static const char *TAG = "BMI088";

static inline esp_err_t writeAccByte(bmi088_t *dev, uint8_t regAddr, const uint8_t data)
{
    return i2c_manager_writeByte(dev->I2C_Master_Num, dev->acc_i2c_addr, regAddr, data);
}

static inline esp_err_t writeGyrByte(bmi088_t *dev, uint8_t regAddr, const uint8_t data)
{
    return i2c_manager_writeByte(dev->I2C_Master_Num, dev->gyr_i2c_addr, regAddr, data);
}

static inline esp_err_t readAccByte(bmi088_t *dev, uint8_t regAddr, uint8_t *data)
{
    return i2c_manager_readByte(dev->I2C_Master_Num, dev->acc_i2c_addr, regAddr, data);
}

static inline esp_err_t readGyroByte(bmi088_t *dev, uint8_t regAddr, uint8_t *data)
{
    return i2c_manager_readByte(dev->I2C_Master_Num, dev->gyr_i2c_addr, regAddr, data);
}

esp_err_t bmi088_init(bmi088_t *dev, int I2C_Master_Num, uint8_t acc_i2c_addr, uint8_t gyr_i2c_addr)
{
    dev->I2C_Master_Num = I2C_Master_Num;
    dev->acc_i2c_addr = acc_i2c_addr;
    dev->gyr_i2c_addr = gyr_i2c_addr;
    /* Soft Reset Acc */
    esp_err_t err;
    ERROR_RETRYTIMES(writeAccByte(dev, BMI_ACC_SOFTRESET, 0xB6), VARIO_I2C_RETRYS_INIT, err);
    if (err != ESP_OK)
    {
        return err;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
    /* Check Device ID */
    uint8_t chipID;
    ERROR_CHECKRETURN(readAccByte(dev, BMI_ACC_CHIP_ID, &chipID));
    if (chipID != ACC_CHIP_ID)
    {
        ESP_LOGI(TAG, "Chip ID should be: %u, but is %u", (unsigned int)ACC_CHIP_ID, (unsigned int)chipID);
        return ESP_ERR_INVALID_RESPONSE;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);

    /* Cofigure Acc */
    ERROR_CHECKRETURN(writeAccByte(dev, BMI_ACC_CONF, 0xA8)); /* (no oversampling, ODR = 100 Hz, BW = 40 Hz) */
    vTaskDelay(10 / portTICK_PERIOD_MS);
    dev->acc_odr = ACC_ODR_100HZ_BW_40HZ;

    ERROR_CHECKRETURN(writeAccByte(dev, BMI_ACC_RANGE, 0x00)); /* +- 3g range */
    vTaskDelay(10 / portTICK_PERIOD_MS);
    dev->accelRange = ACCEL_RANGE_3G;

    /* Enable Acc Data Ready Interrupt */
    ERROR_CHECKRETURN(writeAccByte(dev, BMI_INT1_IO_CONF, 0x0A)); /* INT1 = push-pull output, active high */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    ERROR_CHECKRETURN(writeAccByte(dev, BMI_INT1_INT2_MAP_DATA, 0x04)); /* Map DataReady Interrupt to Pin 1 */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    /* Put Acc in active Mode */
    ERROR_CHECKRETURN(writeAccByte(dev, BMI_ACC_PWR_CONF, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);

    /* Turn on Acc */
    ERROR_CHECKRETURN(writeAccByte(dev, BMI_ACC_PWR_CTRL, 0x04));

    // TODO: why Soft Reset Not working?
    //  /* Gyro soft reset */
    //  volatile esp_err_t errer = writeGyrByte(dev, BMI_GYR_SOFTRESET, 0xB6);
    //  ESP_LOGI(TAG, "%s", esp_err_to_name(errer));
    vTaskDelay(250 / portTICK_PERIOD_MS);

    /* Check chip ID */
    ERROR_CHECKRETURN(readGyroByte(dev, BMI_GYR_CHIP_ID, &chipID));
    if (chipID != GYR_CHIP_ID)
    {
        ESP_LOGI(TAG, "Chip ID should be: %u, but is %u", (unsigned int)GYR_CHIP_ID, (unsigned int)chipID);
        return ESP_ERR_INVALID_RESPONSE;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);

    /* Configue Gyro */
    ERROR_CHECKRETURN(writeGyrByte(dev, BMI_GYR_RANGE, 0x01)); /* +- 1000 deg/s */
    vTaskDelay(10 / portTICK_PERIOD_MS);
    dev->gyroRange = GYRO_RANGE_1000DPS;

    ERROR_CHECKRETURN(writeGyrByte(dev, BMI_GYR_BANDWIDTH, 0x07)); /* ODR = 100 Hz, Filter bandwidth = 32 Hz */
    vTaskDelay(10 / portTICK_PERIOD_MS);
    dev->gyr_odr = GYR_ODR_100HZ_BW_32HZ;

    ERROR_CHECKRETURN(writeGyrByte(dev, BMI_GYR_INT_CTRL, 0x80)); /* New data interrupt enabled */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    ERROR_CHECKRETURN(writeGyrByte(dev, BMI_INT3_INT4_IO_CONF, 0x01)); /* INT3 = push-pull, active high */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    ERROR_CHECKRETURN(writeGyrByte(dev, BMI_INT3_INT4_IO_MAP, 0x01)); /* Data ready interrupt mapped to INT3 pin */
    vTaskDelay(10 / portTICK_PERIOD_MS);
    return ESP_OK;
}

bool bmi088_canRead(bmi088_t *dev)
{
    uint8_t accReady;
    readAccByte(dev, 0x03, &accReady);
    return accReady && (1UL << 7);
}

esp_err_t bmi088_ReadAccelerometer(bmi088_t *dev)
{
    uint8_t rxBuf[6];
    esp_err_t err = i2c_manager_readBytes(dev->I2C_Master_Num, dev->acc_i2c_addr, BMI_ACC_DATA, rxBuf, sizeof(rxBuf));
    if (err != ESP_OK)
    {
        return err;
    }
    /* Form signed 16-bit integers */
    int16_t accX = (int16_t)((rxBuf[1] << 8) | rxBuf[0]);
    int16_t accY = (int16_t)((rxBuf[3] << 8) | rxBuf[2]);
    int16_t accZ = (int16_t)((rxBuf[5] << 8) | rxBuf[4]);
    /* Convert to m/s^2 */
    dev->acc_mps2[0] = accConversion * accX;
    dev->acc_mps2[1] = accConversion * accY;
    dev->acc_mps2[2] = accConversion * accZ;
    return ESP_OK;
}

esp_err_t bmi088_ReadGyroskop(bmi088_t *dev)
{
    uint8_t rxBuf[6]; /* 6 Bytes Data */
    esp_err_t err = i2c_manager_readBytes(dev->I2C_Master_Num, dev->gyr_i2c_addr, BMI_GYR_DATA, rxBuf, sizeof(rxBuf));
    if (err != ESP_OK)
    {
        return err;
    }
    /* Form signed 16-bit integers */
    int16_t gyrX = (int16_t)((rxBuf[1] << 8) | rxBuf[0]);
    int16_t gyrY = (int16_t)((rxBuf[3] << 8) | rxBuf[2]);
    int16_t gyrZ = (int16_t)((rxBuf[5] << 8) | rxBuf[4]);
    /* Convert to rad/s */
    dev->gyr_rps[0] = gyroConversion * gyrX;
    dev->gyr_rps[1] = gyroConversion * gyrY;
    dev->gyr_rps[2] = gyroConversion * gyrZ;
    return ESP_OK;
}
