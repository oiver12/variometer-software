#include "Watchdog.h"
#include <esp_task_wdt.h>

void watchdog_init()
{
    // Nothing to do here
}

void watchdog_add(TaskHandle_t task)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(task));
    ESP_ERROR_CHECK(esp_task_wdt_status(task));
}

void watchdog_reset()
{
    esp_task_wdt_reset();
}

void watchdog_delete(TaskHandle_t task)
{
    ESP_ERROR_CHECK(esp_task_wdt_delete(task));
}

void watchdog_deinit()
{
}