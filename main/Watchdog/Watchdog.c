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
    ERROR_HANDLEBYERROR(esp_task_wdt_add(task), ERR_WATCHDOG, "Can't add Task to watchdog");
    ERROR_HANDLEBYERROR(esp_task_wdt_status(task), ERR_WATCHDOG, "Task not added to Watchdog");
}

void watchdog_reset()
{
    esp_task_wdt_reset();
}

void watchdog_delete(TaskHandle_t task)
{
    ERROR_HANDLEBYERROR(esp_task_wdt_delete(task), ERR_WATCHDOG, "Task can't be deleted from Watchdog");
}

void watchdog_deinit()
{
}