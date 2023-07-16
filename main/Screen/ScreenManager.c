#include "ScreenManager.h"
#include "E_Ink/GUI_Paint.h"
#include "DeviceContext.h"
#include "Display_Vario.h"
#include "RenderSystem.h"
#include "assets/assets.h"
#include "SettingsManager.h"
#include "ErrorHandling.h"
#include "pinDefines.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

Vario_Screen_Manager_t screenManagerInstance;
static SemaphoreHandle_t buffer_Mutex = NULL;

// Zeichne die Status Bar am oberen Rand.
void ScreenManager_draw_status_bar()
{
    // Lösche die alte Anzeige
    Paint_ClearWindows(0, 0, Paint.Width, 24, WHITE);

    gps_fix_mode_t gpsState;
    device_state_get_member(&gpsState, device_state_statusGPS, true);
    // Draw GPS Icon
    if (gpsState == GPS_MODE_INVALID)
        Paint_DrawBitMap_Paste(gImage_gps_aus, 0, 0, 24, 24, FALSE);
    else
        Paint_DrawBitMap_Paste(gImage_gps_an, 0, 0, 24, 24, FALSE);

    uint16_t volume = 0;
    vario_settings_get_member(&volume, vario_settings_soundVolume, true);
    // Draw Lautsprecher Icon
    if (volume == 0)
        Paint_DrawBitMap_Paste(gImage_lautsprecher_aus, 30, 0, 24, 24, FALSE);
    else
        Paint_DrawBitMap_Paste(gImage_lautsprecher_an, 30, 0, 24, 24, FALSE);

    // Draw Sd-Card Icon
    sd_card_state_t sdcard_state;
    device_state_get_member(&sdcard_state, device_state_statusSDCard, true);
    if (sdcard_state.status | SD_CARD_STATE_INIT)
        Paint_DrawBitMap_Paste(gImage_sd_card_good, 60, 0, 24, 24, FALSE);
    else
        Paint_DrawBitMap_Paste(gImage_sd_card_error, 60, 0, 24, 24, FALSE);

    float percentage = 0;
    device_state_get_member(&percentage, device_state_batteryPercentage, true);
    // Draw Battery Icon and Texts
    if (percentage < 10)
        Paint_DrawBitMap_Paste(gImage_battery_empty, Paint.Width - 24 - 2, 0, 24, 24, FALSE);
    else if (percentage < 25)
        Paint_DrawBitMap_Paste(gImage_battery_halfEmpty, Paint.Width - 24 - 2, 0, 24, 24, FALSE);
    else if (percentage < 50)
        Paint_DrawBitMap_Paste(gImage_battery_half, Paint.Width - 24 - 2, 0, 24, 24, FALSE);
    else if (percentage < 75)
        Paint_DrawBitMap_Paste(gImage_battery_halfFull, Paint.Width - 24 - 2, 0, 24, 24, FALSE);
    else
    {
        Paint_DrawBitMap_Paste(gImage_battery_full, Paint.Width - 24 - 2, 0, 24, 24, FALSE);
        if (percentage >= 100)
            percentage = 99;
    }
    // Clear Surface from old Text
    Paint_ClearWindows(Paint.Width - 26 - 50, 5, Paint.Width - 26, 25, WHITE);
    char buffer[5];
    snprintf(buffer, 5, "%i%%", (int)(percentage));
    uint16_t stringWidth = strlen(buffer) * Font16.Width;
    Paint_DrawString_EN(Paint.Width - 26 - stringWidth, 8, buffer, &Font16, WHITE, BLACK, FALSE);
}

/* Initialisiere den Bildschirm Manager(initialisiere eine Singleton)
 * Zeichne auch die Status Bar
 */
esp_err_t ScreenManager_init(SemaphoreHandle_t semaphore)
{
    if (semaphore == NULL)
        return ESP_ERR_INVALID_ARG;
    buffer_Mutex = semaphore;
    memset(screenManagerInstance.screens, 0, sizeof(screenManagerInstance.screens));
    screenManagerInstance.curr_Screen = 0;
    screenManagerInstance.screensAdded = 0;
    screenManagerInstance.screenMode = ScreenMode_Widget;

    ScreenManager_draw_status_bar();

    return Init_SettingsWidgets(semaphore);
}

// TODO: Implement this function. Free different buffers
esp_err_t ScreenManager_deinit()
{
    return ESP_OK;
}

/*
 * Zeige einen anderen Bildschirm an mit anderen Widgets
 * Es wird zuerst mit Screeen_redraw_Screen() die alte Anzeige gelöscht
 * und die Widgets initialisieren. Wenn Widgets refreshed werden, wird dann der
 * neue Bildschirm refreshed
 */
esp_err_t ScreenManager_show_Screen(size_t screenIndex)
{
    if (screenManagerInstance.screensAdded <= screenIndex)
    {
        printf("Screen not added yet!\n");
        return ESP_ERR_INVALID_ARG;
    }
    if (screenManagerInstance.curr_Screen == screenIndex)
    {
        printf("Already displaying this Screen, force redraw with Screen_redraw_Screen()");
        return ESP_OK;
    }
    screenManagerInstance.curr_Screen = screenIndex;
    Screen_redraw_Screen();
    rendering_Update(true);
    return ESP_OK;
}

/*
 * Füge einen neuen Bildschirm mit den mitgegeben widgets hinzu.
 * Ein neuer Bildschirm wird automatisch angelegt
 */
esp_err_t ScreenManager_add_Screen_add_widgets(WidgetData *widgets, size_t size)
{
    if (screenManagerInstance.screensAdded >= MAX_SCREENS - 1)
    {
        printf("To many Screens added!\n");
        return ESP_ERR_NO_MEM;
    }
    VarioScreen_t *screen = &screenManagerInstance.screens[screenManagerInstance.screensAdded];
    screenManagerInstance.screensAdded++;
    for (size_t i = 0; i < size; i++)
    {
        // screen->widgets[screen->widgetsAdded++] = *(widgets + i);
        memcpy(&screen->widgets[screen->widgetsAdded], (widgets + i), sizeof(WidgetData));
        // Render this Screen, if it's the current Screen
        if (screenManagerInstance.curr_Screen == screenManagerInstance.screensAdded - 1)
            vario_display_add_widget(&screen->widgets[screen->widgetsAdded]);
        screen->widgetsAdded++;
    }
    return ESP_OK;
}

/*
 * Update den Screen je nach screenMode momentan
 */
bool ScreenManager_update()
{
    if (screenManagerInstance.screenMode == ScreenMode_Widget)
    {
        for (size_t i = 0; i < screenManagerInstance.screens[screenManagerInstance.curr_Screen].widgetsAdded; i++)
        {
            if (screenManagerInstance.screenMode != ScreenMode_Widget)
                return false;
            vario_update_widget(&screenManagerInstance.screens[screenManagerInstance.curr_Screen].widgets[i]);
        }
        return true;
    }
    return false;
}

/*
 * Wenn der PowerButton gedrüct wird, kann entweder das Menu oder das Abschalten angezeigt werden
 */
void ScreenManager_powerButtonPressed(bool longPress)
{
    if (screenManagerInstance.screenMode != ScreenMode_PowerAccept)
    {
        if (longPress)
        {
            screenManagerInstance.screenMode = ScreenMode_PowerAccept;
            xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
            vario_display_PowerOffScreen();
            xSemaphoreGive(buffer_Mutex);
            rendering_Push(false);
            return;
        }
        else
        {
            if (screenManagerInstance.screenMode == ScreenMode_Widget)
                SettingsManager_openSettings();
            else if (screenManagerInstance.screenMode == ScreenMode_Settings)
            {
                screenManagerInstance.screenMode = ScreenMode_Widget;
                rendering_Update(true);
                Screen_redraw_Screen();
            }
        }
    }
    else
    {
        // PowerOff
        esp_err_t err = esp_sleep_enable_ext0_wakeup(2, 0);
        if (err != ESP_OK)
        {
            // grosses Problem!! können nicht mehr aufwachen, was machen?
            error_handle(err, ERR_SCREEN, "Can't initialize Wakeup Pin!");
        }
        Display_powerOff_Screen();
        gpio_set_level(POWSW_ENABLE, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        esp_deep_sleep_start();
    }
}

/*
 * Bei Widgets anzeige, wird der nächste Bildschirm angezeigt
 * Bei Power Accept, soll gecancelt werden
 */
void ScreenManager_DownArrowPressed()
{
    if (screenManagerInstance.screenMode == ScreenMode_Widget)
    {
        // When there is only one Screen, we dont have to rerender something
        if (screenManagerInstance.screensAdded > 1)
        {
            if (screenManagerInstance.curr_Screen == 0)
                ScreenManager_show_Screen(screenManagerInstance.screensAdded - 1);
            else
                ScreenManager_show_Screen(screenManagerInstance.curr_Screen - 1);
        }
    }
    else if (screenManagerInstance.screenMode == ScreenMode_PowerAccept)
    {
        // TODO: evtl. in letzte Seite zurückkehren und nicht einfach Widgets laden
        screenManagerInstance.screenMode = ScreenMode_Widget;
        Screen_redraw_Screen();
        rendering_Update(false);
    }
    else if (screenManagerInstance.screenMode == ScreenMode_Settings)
    {
        SettingsManager_down_press();
    }
}

void ScreenManager_UpArrowPressed()
{
    if (screenManagerInstance.screenMode == ScreenMode_Widget)
    {
        // When there is only one Screen, we dont have to Rerender something
        if (screenManagerInstance.screensAdded > 1)
        {
            if (screenManagerInstance.curr_Screen == screenManagerInstance.screensAdded - 1)
                ScreenManager_show_Screen(0);
            else
                ScreenManager_show_Screen(screenManagerInstance.curr_Screen + 1);
        }
    }
    else if (screenManagerInstance.screenMode == ScreenMode_PowerAccept)
    {
        // TODO: evtl. in letzte Seite zurückkehren und nicht einfach Widgets laden
        screenManagerInstance.screenMode = ScreenMode_Widget;
        Screen_redraw_Screen();
        rendering_Update(false);
    }
    else if (screenManagerInstance.screenMode == ScreenMode_Settings)
    {
        SettingsManager_up_press();
    }
}

void ScreenManager_EnterKeyPressed()
{
    if (screenManagerInstance.screenMode == ScreenMode_PowerAccept)
    {
        // TODO: evtl. in letzte Seite zurückkehren und nicht einfach Widgets laden
        screenManagerInstance.screenMode = ScreenMode_Widget;
        Screen_redraw_Screen();
        rendering_Update(false);
    }
    else if (screenManagerInstance.screenMode == ScreenMode_Settings)
    {
        SettingsManager__enter_press();
    }
}

/*
 * Zeichen den ganzen Bildschirm neu. Z.B. wenn ein neuer Bildschirm geladen wird (es werden z.B. alle Frames von den Widgets gezeichnet)
 */
void Screen_redraw_Screen()
{
    // Clear Screen and redraw all widgets
    if (screenManagerInstance.screenMode == ScreenMode_Widget)
    {
        xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
        Paint_Clear(WHITE);
        ScreenManager_draw_status_bar();
        for (size_t i = 0; i < screenManagerInstance.screens[screenManagerInstance.curr_Screen].widgetsAdded; i++)
        {
            vario_display_add_widget(&screenManagerInstance.screens[screenManagerInstance.curr_Screen].widgets[i]);
        }
        xSemaphoreGive(buffer_Mutex);
    }
}

bool ScreenManager_showPopUp(PopUpConfig_t config)
{
    // TODO: add Posibility to do Pop Up over Pop Up or save PopUp in Queue or something like that.
    if (!screenIsInit() || screenManagerInstance.screenMode == ScreenMode_PopUp)
        return false;
    screenManagerInstance.screenMode = ScreenMode_PopUp;
    xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
    esp_err_t err = vario_display_showPopUp(config);
    xSemaphoreGive(buffer_Mutex);
    if (err != ESP_OK)
    {
        // we dont really care
        error_handle(err, ERR_SCREEN, "Can't show PopUp");
    }
    rendering_Push(false);
    return true;
}

void ScreenManager_closePopUp()
{
    screenManagerInstance.screenMode = ScreenMode_Widget;
    Screen_redraw_Screen();
    rendering_Update(false);
}

void ScreenManager_setMode(ScreenMode mode)
{
    screenManagerInstance.screenMode = mode;
}
