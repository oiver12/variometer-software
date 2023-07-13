#ifdef ISSIMULATION

#include "DPS310.h"

esp_err_t dps310_init(dps310_t *dev, int I2C_Master_Num, uint8_t i2c_addr)
{
    return ESP_OK;
}
esp_err_t dps310_config_Temp_Pressure(dps310_t *dev, dps310_mr_t temp_mr, dps310_osr_t temp_osr, dps310_mr_t prs_mr, dps310_osr_t prs_osr)
{
    return ESP_OK;
}
esp_err_t dsp310_get_Pressure(dps310_t *dev)
{
    return ESP_OK;
}
esp_err_t dps310_get_Temperature(dps310_t *dev)
{
    return ESP_OK;
}

#endif