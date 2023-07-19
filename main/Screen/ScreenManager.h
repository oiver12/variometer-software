#ifndef __VarioScreen_H_
#define __VarioScreen_H_

/*
 * Diese File managed den Bildschirm. Vario_Screen_Manager_t ist zuständig, um verschiedene Bildschirme (verschiedene Ansichten) zu speichern. Einzelne
 * Bildschirme sind in VarioScreen_t gespeichert. In diesen Bildschirmen, sind dann Widgets gespeichert, welche angeben was und wie angezeigt wird im Feld.
 * Je nach ScreenMode wird anders geupdatet
 */

#include "Widget_Vario.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define MAX_SCREENS 5
#define MAX_WIDGETS_SCREEN 10

// In welchem Ansichtsmodus man sich befinden
typedef enum
{
    ScreenMode_Widget = 0,
    ScreenMode_Settings = 1,
    ScreenMode_PowerAccept = 2,
    ScreenMode_PopUp = 3,
} ScreenMode;

typedef struct
{
    WidgetData widgets[MAX_WIDGETS_SCREEN];
    uint16_t widgetsAdded;
} VarioScreen_t;
// TODO: Change to list_head

typedef struct
{
    VarioScreen_t screens[MAX_SCREENS];
    size_t screensAdded;
    uint8_t curr_Screen;
    ScreenMode screenMode;
} Vario_Screen_Manager_t;

esp_err_t ScreenManager_init(SemaphoreHandle_t semaphore);
esp_err_t ScreenManager_deinit();
esp_err_t ScreenManager_show_Screen(size_t screenIndex);
void ScreenManager_draw_status_bar();

// gives back, if the screen should be full redrawn
bool ScreenManager_update();

void ScreenManager_powerButtonPressed(bool longPress);
void ScreenManager_DownArrowPressed();
void ScreenManager_UpArrowPressed();
void ScreenManager_EnterKeyPressed();

void Screen_redraw_Screen(uint8_t screenToDraw);
esp_err_t ScreenManager_add_Screen_add_widgets(WidgetData *widgets, size_t size);
void ScreenManager_deselctSettingsWidget();
// returns if it could show the PopUp
bool ScreenManager_showPopUp(PopUpConfig_t config);
void ScreenManager_closePopUp();

void ScreenManager_setMode(ScreenMode mode);

#endif
