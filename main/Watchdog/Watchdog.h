#ifndef _WATCHDOG__H
#define _WATCHDOG__H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// TODO: add some logic, so when the Watchdog triggers do some better thing than reset whole device
void watchdog_init();
void watchdog_add(TaskHandle_t task); // null means current Task
void watchdog_reset();
void watchdog_delete(TaskHandle_t task); // null means current Task
void watchdog_deinit();

#endif