#include "Watchdog.h"
#include "ErrorHandling.h"

#include <esp_task_wdt.h>
#include <esp_err.h>

void watchdog_init()
{
    // Nothing to do here
}

void watchdog_add(TaskHandle_t task)
{
    ERROR_CHECKRETURN(esp_task_wdt_add(task));
    ERROR_CHECKRETURN(esp_task_wdt_status(task));
}

void watchdog_reset()
{
    esp_task_wdt_reset();
}

void watchdog_delete(TaskHandle_t task)
{
    ERROR_CHECKRETURN(esp_task_wdt_delete(task));
}

void watchdog_deinit()
{
}