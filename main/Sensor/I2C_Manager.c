#include "I2C_Manager.h"
#include "ErrorHandling.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <string.h>

#define I2C_MASTER_TIMOUT_MS 10000

static SemaphoreHandle_t local_mutex[2] = {NULL, NULL};

esp_err_t i2c_manager_init(i2c_port_t port, gpio_num_t SDA, gpio_num_t SCL, uint32_t clockFrequency)
{
    if (port != I2C_NUM_0 && port != I2C_NUM_1)
        return ESP_ERR_INVALID_ARG;
    i2c_config_t conf_I2C = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA,
        .scl_io_num = SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clockFrequency,
    };
    i2c_param_config(port, &conf_I2C);
    ERROR_CHECKRETURN(i2c_driver_install(port, conf_I2C.mode, 0, 0, 0));
    local_mutex[(int)port] = xSemaphoreCreateMutex();
    return ESP_OK;
}

esp_err_t i2c_manager_writeByte(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, const uint8_t data)
{
    if (!local_mutex[port])
        return ESP_ERR_INVALID_STATE;
    uint8_t write_buf[2] = {regAddr, data};
    xSemaphoreTake(local_mutex[port], portMAX_DELAY);
    esp_err_t err = i2c_master_write_to_device(port, deviceAddr, write_buf, sizeof(write_buf), I2C_MASTER_TIMOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(local_mutex[port]);
    return err;
}

esp_err_t i2c_manager_writeBytes(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, uint8_t *data, size_t writeSize)
{
    if (!local_mutex[port])
        return ESP_ERR_INVALID_STATE;
    uint8_t write_buf[writeSize + 1];
    write_buf[0] = regAddr;
    memcpy(&write_buf[1], data, writeSize);
    xSemaphoreTake(local_mutex[port], portMAX_DELAY);
    esp_err_t err = i2c_master_write_to_device(port, deviceAddr, write_buf, sizeof(write_buf), I2C_MASTER_TIMOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(local_mutex[port]);
    return err;
}

esp_err_t i2c_manager_readByte(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, uint8_t *data)
{
    if (!local_mutex[port])
        return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(local_mutex[port], portMAX_DELAY);
    esp_err_t err = i2c_master_write_read_device(port, deviceAddr, &regAddr, 1, data, 1, I2C_MASTER_TIMOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(local_mutex[port]);
    return err;
}

esp_err_t i2c_manager_readBytes(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, uint8_t *data, size_t readSize)
{
    if (!local_mutex[port])
        return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(local_mutex[port], portMAX_DELAY);
    esp_err_t err = i2c_master_write_read_device(port, deviceAddr, &regAddr, 1, data, readSize, I2C_MASTER_TIMOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(local_mutex[port]);
    return err;
}

esp_err_t i2c_manager_writeBitByteField(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, uint8_t data, uint8_t mask, uint8_t shift)
{
    if (!local_mutex[port])
        return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(local_mutex[port], portMAX_DELAY);
    uint8_t oldValue = 0;
    esp_err_t err = i2c_master_write_read_device(port, deviceAddr, &regAddr, 1, &oldValue, 1, I2C_MASTER_TIMOUT_MS / portTICK_PERIOD_MS);
    if (err != ESP_OK)
        goto end;
    uint8_t newData = ((uint8_t)oldValue & ~mask) | ((data << shift) & mask);
    uint8_t write_buf[2] = {regAddr, newData};
    err = i2c_master_write_to_device(port, deviceAddr, write_buf, sizeof(write_buf), I2C_MASTER_TIMOUT_MS / portTICK_PERIOD_MS);
end:
    xSemaphoreGive(local_mutex[port]);
    return err;
}