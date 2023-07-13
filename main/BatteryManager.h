#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <stdbool.h>

typedef struct
{
    int oversampling;
    float voltage;
    float percentage;
    adc_cali_handle_t adc_chars;
    adc_oneshot_unit_handle_t unit_handle;
    bool cali_enable;
} Battery_Manager_t;

esp_err_t init_Battery_Manager(int oversampling);
esp_err_t deinit_Battery_Manager();

#endif