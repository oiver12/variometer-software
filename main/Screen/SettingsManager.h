#ifndef __SETTINGSMANAGER_C
#define __SETTINGSMANAGER_C

#include "Widget_Vario.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_err.h>

#define SIZE_FLIGHTS 6

typedef struct
{
    void *prev;
    void *next;
    WidgetData data;
} widget_Settings_linkedList;

typedef struct
{
    Settings_Page_t currPage;
    widget_Settings_linkedList *headListPageButtons;
    widget_Settings_linkedList *topFielPageButtons;
    widget_Settings_linkedList *headListAudio;
    widget_Settings_linkedList *topFieldAudio; // in Scrollbar from Audio field on top
    widget_Settings_linkedList *headListConfig;
    widget_Settings_linkedList *topFieldConfig; // in Scrollbar from Config field on top
    widget_Settings_linkedList *headListFlightList;
    widget_Settings_linkedList *topFieldFlightList;
    widget_Settings_linkedList *hoveredField;
    widget_Settings_linkedList *selectedField;
    uint16_t widgetsAddedConfig;
} VarioScreen_Settings_t;

esp_err_t Init_SettingsWidgets(SemaphoreHandle_t semaphore);
void SettingsManager_down_press();
void SettingsManager_up_press();
void SettingsManager_openSettings();
void SettingsManager__enter_press();

#endif