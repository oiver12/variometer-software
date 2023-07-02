#include "RenderSystem.h"
#include "E_Ink/EPD_3in7.h"
#include "E_Ink/GUI_Paint.h"
#include "ScreenManager.h"
#include "SoundManager.h"
#include "Watchdog/Watchdog.h"
#include "ErrorHandling.h"
#include "vario_config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>

RenderModes_t currentRenderMode = RenderMode_Update;
bool isSending = false;
bool fullRedraw = true;
bool displayInit = false;
static SemaphoreHandle_t buffer_Mutex = NULL;
QueueHandle_t renderQueue = NULL;

static const char *TAG = "RENDER";

void rendering_Task(void *pvParameters)
{
    watchdog_add(NULL);
    uint32_t counter = 0;
    Display_RefreshMode refreshMode = PART_G_T_BW;
    while (1)
    {
        watchdog_reset();
        if (currentRenderMode == RenderMode_Update)
        {
            if (counter % 10 == 0)
                ScreenManager_draw_status_bar();

            xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
            bool screenUpdate = ScreenManager_update();
            xSemaphoreGive(buffer_Mutex);
            if (screenUpdate)
            {
                refreshMode = fullRedraw || counter % 100 == 0 ? FULL_G_T_BW : PART_BW_T_BW;
                xQueueSend(renderQueue, &refreshMode, portMAX_DELAY);
            }
            counter++;
            fullRedraw = false;
        }
        else
            vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void displayRefreshTask(void *pvParameters)
{
    uint8_t *ScreenBuffer = (uint8_t *)pvParameters;
    Display_RefreshMode refreshMode;
    esp_err_t err = ESP_OK;
    while (1)
    {
        xQueueReceive(renderQueue, &refreshMode, portMAX_DELAY);
        xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
        ERROR_RETRYTIMES(EDP_3IN7_1Gray_Start(ScreenBuffer, refreshMode), VARIO_SPI_RETRYS, err);
        xSemaphoreGive(buffer_Mutex);
        if (err != ESP_OK)
        {
            // TODO: do something intelligent here e.g. reset Screen and check again
            error_handle(err, ERR_SCREEN, "Can't draw to screen");
        }
        EPD_3IN7_1Gray_WaitBusy();
    }
}

esp_err_t startRendering(uint8_t *ScreenBuffer)
{
    buffer_Mutex = xSemaphoreCreateMutex();
    ScreenManager_init(buffer_Mutex);

    static WidgetData widgetDataScreen0[8] = {
        {.extraStyle = WS_FONT_24_CONTENT, .h = 60, .w = 160, .style = Widget_TextBox, .type = WidgetContent_Vario_Active, .x = 0, .y = 27},
        {.extraStyle = WS_FONT_20_CONTENT, .h = 130, .w = 100, .style = Widget_Compass, .type = WidgetContent_CompassSensor, .x = 0, .y = 87},
        {.extraStyle = WS_FONT_20_CONTENT, .h = 193, .w = 220, .style = Widget_Chart, .type = WidgetContent_Altitude_Baro, .x = 100, .y = 87},
        {.extraStyle = WS_FONT_24_CONTENT, .h = 60, .w = 160, .style = Widget_TextBox, .type = WidgetContent_VertAcc, .x = 160, .y = 27},
        {.extraStyle = WS_FONT_16_CONTENT, .h = 60, .w = 160, .style = Widget_TextBox, .type = WidgetContent_Pressure, .x = 320, .y = 27},
        {.extraStyle = WS_FONT_24_CONTENT, .h = 60, .w = 160, .style = Widget_TextBox, .type = WidgetContent_Altitude_GPS, .x = 320, .y = 87},
        {.extraStyle = WS_FONT_24_CONTENT, .h = 60, .w = 160, .style = Widget_TextBox, .type = WidgetContent_Speed_Ground, .x = 320, .y = 147},
        {.extraStyle = WS_FONT_24_CONTENT, .h = 73, .w = 160, .style = Widget_TextBox, .type = WidgetContent_Vario_Lazy, .x = 320, .y = 207},
    };
    ScreenManager_add_Screen_add_widgets(widgetDataScreen0, 8);
    static WidgetData widgetDataScreen1[5] = {
        {.extraStyle = WS_FONT_32_CONTENT, .h = 60, .w = 160, .style = Widget_TextBox, .type = WidgetContent_Time_GPS, .x = 0, .y = 27},
        {.extraStyle = WS_FONT_24_CONTENT, .h = 60, .w = 160, .style = Widget_TextBox, .type = WidgetContent_Altitude_AGL, .x = 160, .y = 27},
        {.extraStyle = WS_FONT_24_CONTENT, .h = 60, .w = 160, .style = Widget_TextBox, .type = WidgetContent_Speed_Ground, .x = 320, .y = 27},
        {.extraStyle = WS_FONT_32_CONTENT, .h = 100, .w = 200, .style = Widget_SdCardInfo, .type = WidgetContent_SDCardInfo, .x = 280, .y = 87},
        {.extraStyle = WS_FONT_12_CONTENT, .h = 100, .w = 200, .style = Widget_Airspace, .type = WidgetContent_Airspace, .x = 0, .y = 87}};
    ScreenManager_add_Screen_add_widgets(widgetDataScreen1, 5);
    ScreenManager_show_Screen(0);

    renderQueue = xQueueCreate(1, sizeof(Display_RefreshMode));

    xTaskCreate(rendering_Task, "rendering_Task", 4096, NULL, 10, NULL);
    xTaskCreate(displayRefreshTask, "displayRefreshTask", 4096, ScreenBuffer, 10, NULL);
    displayInit = true;
    return ESP_OK;
}
void stopRendering_disableScreen()
{
}
void rendering_Push(bool varfullRedraw)
{
    currentRenderMode = RenderMode_Push;
    Display_RefreshMode refreshMode = varfullRedraw ? FULL_G_T_BW : PART_BW_T_BW;
    xQueueOverwrite(renderQueue, &refreshMode);
}
void rendering_Update(bool varfullRedraw)
{
    currentRenderMode = RenderMode_Update;
    fullRedraw = varfullRedraw;
}

bool screenIsInit()
{
    return displayInit;
}