#include "BatteryManager.h"
#include "DeviceContext.h"
#include "ErrorHandling.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BAT_MS 3

#define maxVoltage 4 // nicht 4.2, weil der ESP nur bis 3.1V und nicht bis 3.3v lesen kann
#define minVoltage 3.3
// static const char *TAG = "Battery Manager";

Battery_Manager_t battery_Manager;

static esp_err_t adc_calibration_init(adc_unit_t unit, adc_atten_t atten)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);

    battery_Manager.adc_chars = handle;

    return ret;
}

esp_err_t deinit_Battery_Manager()
{
    free(battery_Manager.adc_chars);
    battery_Manager.adc_chars = NULL;
    return ESP_OK;
}

esp_err_t readPercentage_Battery()
{
    gpio_set_level(BAT_MS, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    uint64_t sumValue = 0;
    for (size_t i = 0; i < battery_Manager.oversampling; i++)
    {
        int tempValue = 0;
        ERROR_CHECKRETURN(adc_oneshot_read(battery_Manager.unit_handle, ADC_CHANNEL_3, &tempValue));
        sumValue += tempValue;
    }
    sumValue /= battery_Manager.oversampling;
    if (battery_Manager.cali_enable)
    {
        int voltage = 0;
        ERROR_CHECKRETURN(adc_cali_raw_to_voltage(battery_Manager.adc_chars, sumValue, &voltage));
        float voltageBattery = ((float)voltage / 1000.0f) * (1.0f + (10000.0f / 33000.0f));
        // Nicht über 100% oder unter 0%
        if (voltageBattery > 3.0f && voltageBattery < 4.2f)
        {
            battery_Manager.voltage = voltageBattery;
        }
    }
    else
    {
        return ESP_ERR_INVALID_RESPONSE;
    }
    gpio_set_level(BAT_MS, 0);
    return ESP_OK;
}

static void BatteryManagerTask(void *parameters)
{
    esp_err_t err;
    while (1)
    {
        err = readPercentage_Battery();
        if (err != ESP_OK)
        {
            // TODO: track how often this happens
            error_handle(err, ERR_BATTERY, "Can't measure Batterylevel");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        float percentage = (battery_Manager.voltage - minVoltage) * 100.0f / (maxVoltage - minVoltage);
        if (percentage > 100.0f)
            percentage = 100.0f;
        else if (percentage < 0.0f)
            percentage = 0.0f;
        device_state_set_member(&percentage, device_state_batteryPercentage);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

esp_err_t init_Battery_Manager(int oversampling)
{
    //-------------ADC2 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ERROR_CHECKRETURN(adc_oneshot_new_unit(&init_config2, &battery_Manager.unit_handle));

    //-------------ADC2 Calibration Init---------------//
    esp_err_t err = adc_calibration_init(ADC_UNIT_2, ADC_ATTEN_DB_11);
    battery_Manager.cali_enable = (err == ESP_OK);
    if (err != ESP_OK)
    {
        error_handle(err, ERR_BATTERY, "Can't use calibration");
    }
    //-------------ADC2 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ERROR_CHECKRETURN(adc_oneshot_config_channel(battery_Manager.unit_handle, ADC_CHANNEL_3, &config));

    battery_Manager.oversampling = oversampling;

    gpio_config_t gpio_config_t = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BAT_MS,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&gpio_config_t);
    gpio_set_level(BAT_MS, 0);

    xTaskCreate(BatteryManagerTask, "Battery", 4 * 1024, NULL, 5, NULL);
    return ESP_OK;
}