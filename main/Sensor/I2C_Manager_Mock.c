#ifdef ISSIMULATION

#include "BMI088.h"

esp_err_t i2c_manager_init(i2c_port_t port, gpio_num_t SDA, gpio_num_t SCL, uint32_t clockFrequency)
{
    return ESP_OK;
}
esp_err_t i2c_manager_writeByte(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, const uint8_t data)
{
    return ESP_OK;
}
esp_err_t i2c_manager_writeBytes(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, uint8_t *data, size_t writeSize)
{
    return ESP_OK;
}
esp_err_t i2c_manager_readByte(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, uint8_t *data)
{
    return ESP_OK;
}
esp_err_t i2c_manager_readBytes(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, uint8_t *data, size_t readSize)
{
    return ESP_OK;
}
esp_err_t i2c_manager_writeBitByteField(i2c_port_t port, uint8_t deviceAddr, uint8_t regAddr, uint8_t data, uint8_t mask, uint8_t shift)
{
    return ESP_OK;
}

#endif