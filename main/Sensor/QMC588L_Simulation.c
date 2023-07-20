#ifdef ISSIMULATION

#include "QMC588L.h"
#include "SimulationData.h"

qmc588l_t qmc588l_data;

qmc588l_t *qmc588l_getInstance()
{
    return &qmc588l_data;
}
esp_err_t qmc588l_init(i2c_port_t I2C_Master_Num, uint8_t i2c_addr, qmc588l_mr_t mr, qmc588l_osr_t osr, qmc588l_scale_t scale)
{
    qmc588l_data.I2C_Master_Num = I2C_Master_Num;
    qmc588l_data.qmc588l_i2c_addr = i2c_addr;
    qmc588l_data.useCalibration = false;
    qmc588l_data.isCalibrating = false;
    return ESP_OK;
}
esp_err_t qmc588l_setMode(qmc588l_mr_t mr, qmc588l_osr_t osr, qmc588l_scale_t scale)
{
    qmc588l_data.measurement_rate = mr;
    qmc588l_data.oversampling = osr;
    qmc588l_data.scale = scale;
    return ESP_OK;
}
esp_err_t qmc588l_setCalibration(int x_min, int x_max, int y_min, int y_max, int z_min, int z_max)
{
    qmc588l_data.useCalibration = true;
    qmc588l_data._vCalibration[0][0] = x_min;
    qmc588l_data._vCalibration[0][1] = x_max;
    qmc588l_data._vCalibration[1][0] = y_min;
    qmc588l_data._vCalibration[1][1] = y_max;
    qmc588l_data._vCalibration[2][0] = z_min;
    qmc588l_data._vCalibration[2][1] = z_max;
    return ESP_OK;
}
void qmc588l_calculateCalibration()
{
    return;
}
esp_err_t qmc588l_readMag()
{
    return simulation_nextMagn(&qmc588l_data);
}

#endif