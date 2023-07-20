#ifdef ISSIMULATION

#include "DPS310.h"
#include "SimulationData.h"

esp_err_t dps310_init(dps310_t *dev, int I2C_Master_Num, uint8_t i2c_addr)
{
    dev->I2C_Master_Num = I2C_Master_Num;
    dev->dps310_t_i2c_addr = i2c_addr;
    dps310_config_Temp_Pressure(dev, DPS310_MR_1, DPS310_OSR_64, DPS310_MR_32, DPS310_OSR_16);
    return ESP_OK;
}
esp_err_t dps310_config_Temp_Pressure(dps310_t *dev, dps310_mr_t temp_mr, dps310_osr_t temp_osr, dps310_mr_t prs_mr, dps310_osr_t prs_osr)
{
    dev->temp_measurement_rate = temp_mr;
    dev->temp_oversampling = temp_osr;
    dev->prs_measurement_rate = prs_mr;
    dev->prs_oversampling = prs_osr;
    return ESP_OK;
}
esp_err_t dsp310_get_Pressure(dps310_t *dev)
{
    return simulation_nextPressure(dev);
}
esp_err_t dps310_get_Temperature(dps310_t *dev)
{
    // nothing to do in simulation, get read in in pressure
    return ESP_OK;
}

#endif