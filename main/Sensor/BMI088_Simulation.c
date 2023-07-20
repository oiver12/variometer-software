#ifdef ISSIMULATION

#include "BMI088.h"
#include "SimulationData.h"

esp_err_t bmi088_init(bmi088_t *dev, int I2C_Master_Num, uint8_t acc_i2c_addr, uint8_t gyr_i2c_addr)
{
    dev->I2C_Master_Num = I2C_Master_Num;
    dev->acc_i2c_addr = acc_i2c_addr;
    dev->gyr_i2c_addr = gyr_i2c_addr;
    return ESP_OK;
}
bool bmi088_canRead(bmi088_t *dev)
{
    return true;
}
esp_err_t bmi088_ReadAccelerometer(bmi088_t *dev)
{
    return simulation_nextAcc(dev);
}
esp_err_t bmi088_ReadGyroskop(bmi088_t *dev)
{
    // gets read in Acc
    return ESP_OK;
}

#endif
