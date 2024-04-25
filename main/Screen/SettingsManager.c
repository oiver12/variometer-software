#include "SettingsManager.h"
#include "E_Ink/GUI_Paint.h"
#include "ScreenManager.h"
#include "RenderSystem.h"
#include "../SDCard.h"
#include "ErrorHandling.h"

WidgetData widgetData_Audio_Settings[2] = {
    {.extraStyle = 0, .h = 40, .w = 459, .style = Widget_Settings_Slider, .type = WidgetContent_Settings_VolumeSlider},
    {.extraStyle = 0, .h = 100, .w = 459, .style = Widget_Settings_SoundThreshold, .type = WidgetContent_Settings_SoundLiftThreshold},
};

WidgetData widgetData_Config_Settings[2] = {
    {.extraStyle = 0, .h = 40, .w = 459, .style = Widget_Settings_Slider, .type = WidgetContent_Settings_VertVelLazySlider},
    {.extraStyle = 0, .h = 60, .w = 459, .style = Widget_Settings_CalibrationPageButton, .type = WidgetContent_Settings_MagnCalibrationPageButton},
};

static VarioScreen_Settings_t screenSettingsInstance;
static SemaphoreHandle_t buffer_Mutex = NULL;

esp_err_t Init_SettingsWidgets(SemaphoreHandle_t semaphore)
{
    if (semaphore == NULL)
        return ESP_ERR_INVALID_ARG;
    buffer_Mutex = semaphore;
    WidgetData pageButtonSettingsWidget[3] = {
        {.x = 10, .y = 27, .w = 459, .h = 40, .style = Widget_Settings_PageButton, .type = WidgetContent_Settings_PageButton_Audio, .extraStyle = 0},
        {.x = 10, .y = 67, .w = 459, .h = 40, .style = Widget_Settings_PageButton, .type = WidgetContent_Settings_PageButton_Config, .extraStyle = 0},
        {.x = 10, .y = 67, .w = 459, .h = 40, .style = Widget_Settings_PageButton, .type = WidgetContent_Settings_PageButton_Flights, .extraStyle = 0}};
    WidgetData backButtonWidget = {.x = 10, .y = 27, .w = 459, .h = 40, .style = Widget_settings_BackButton, .extraStyle = 0};

    /* Init Page Buttons Setting Widgets */
    widget_Settings_linkedList *backButtonItem = calloc(1, sizeof(widget_Settings_linkedList));
    backButtonWidget.type = WidgetContent_Settings_PageButton;
    backButtonItem->data = backButtonWidget;
    screenSettingsInstance.headListPageButtons = backButtonItem;
    screenSettingsInstance.topFielPageButtons = backButtonItem;
    widget_Settings_linkedList *prev = backButtonItem;
    widget_Settings_linkedList *listElement;
    for (size_t i = 0; i < sizeof(pageButtonSettingsWidget) / sizeof(WidgetData); i++)
    {
        listElement = calloc(1, sizeof(widget_Settings_linkedList));
        if (!listElement)
        {
            printf("Cant initialize Settings Button\n");
            return ESP_ERR_NO_MEM;
        }
        listElement->prev = prev;
        prev->next = listElement;
        listElement->data = pageButtonSettingsWidget[i];
        listElement->data.x = 10;
        prev = listElement;
    }
    // make it circular (skip to to top when all the way down)
    listElement->next = backButtonItem;
    backButtonItem->prev = listElement;

    /* Init Audio Setting Widgets */
    backButtonItem = calloc(1, sizeof(widget_Settings_linkedList));
    if (!backButtonItem)
        return ESP_ERR_NO_MEM;
    backButtonWidget.type = WidgetContent_Settings_PageButton_Audio;
    backButtonItem->data = backButtonWidget;
    screenSettingsInstance.headListAudio = backButtonItem;
    screenSettingsInstance.topFieldAudio = backButtonItem;
    prev = backButtonItem;
    for (size_t i = 0; i < sizeof(widgetData_Audio_Settings) / sizeof(WidgetData); i++)
    {
        listElement = calloc(1, sizeof(widget_Settings_linkedList));
        if (!listElement)
            return ESP_ERR_NO_MEM;
        listElement->prev = prev;
        prev->next = listElement;
        listElement->data = widgetData_Audio_Settings[i];
        listElement->data.x = 10;
        prev = listElement;
    }
    listElement->next = backButtonItem;
    backButtonItem->prev = listElement;

    /* Init Config Setting Widget */
    backButtonItem = calloc(1, sizeof(widget_Settings_linkedList));
    if (!backButtonItem)
        return ESP_ERR_NO_MEM;
    backButtonWidget.type = WidgetContent_Settings_PageButton_Config;
    backButtonItem->data = backButtonWidget;
    screenSettingsInstance.headListConfig = backButtonItem;
    screenSettingsInstance.topFieldConfig = backButtonItem;
    prev = backButtonItem;
    for (size_t i = 0; i < sizeof(widgetData_Config_Settings) / sizeof(WidgetData); i++)
    {
        listElement = calloc(1, sizeof(widget_Settings_linkedList));
        if (!listElement)
            return ESP_ERR_NO_MEM;
        listElement->prev = prev;
        prev->next = listElement;
        listElement->data = widgetData_Config_Settings[i];
        listElement->data.x = 10;
        prev = listElement;
    }
    listElement->next = backButtonItem;
    backButtonItem->prev = listElement;

    /* Init Flight Setting Widget */
    backButtonItem = calloc(1, sizeof(widget_Settings_linkedList));
    if (!backButtonItem)
        return ESP_ERR_NO_MEM;
    backButtonWidget.type = WidgetContent_Settings_PageButton_Flights;
    backButtonItem->data = backButtonWidget;
    screenSettingsInstance.headListFlightList = backButtonItem;
    screenSettingsInstance.topFieldFlightList = backButtonItem;
    prev = backButtonItem;

    WidgetData widgetFlight = {.x = 10, .y = 67, .w = 459, .h = 35, .style = Widget_Settings_FlightListElement, .type = WidgetContent_Settings_FlightListElement};
    for (size_t i = 0; i < SIZE_FLIGHTS; i++)
    {
        listElement = calloc(1, sizeof(widget_Settings_linkedList));
        if (!listElement)
            return ESP_ERR_NO_MEM;
        listElement->prev = prev;
        prev->next = listElement;
        listElement->data = widgetFlight;
        listElement->data.extr_data = calloc(1, sizeof(FlightList_Widget_Data));
        listElement->data.x = 10;
        prev = listElement;
        listElement->data.y += 35;
    }
    listElement->next = backButtonItem;
    backButtonItem->prev = listElement;
    return ESP_OK;
}

static void draw_Settings_Scrollbar()
{
    uint32_t currY = 28;
    widget_Settings_linkedList *startElement = screenSettingsInstance.topFielPageButtons;
    if (screenSettingsInstance.currPage == Settings_Page_Audio)
    {
        startElement = screenSettingsInstance.topFieldAudio;
    }
    else if (screenSettingsInstance.currPage == Settings_Page_Config)
    {
        startElement = screenSettingsInstance.topFieldConfig;
    }
    else if (screenSettingsInstance.currPage == Settings_Page_Flights)
    {
        startElement = screenSettingsInstance.topFieldFlightList;
    }
    Paint_ClearWindows(0, currY, Paint.Width, Paint.Height, WHITE);
    widget_Settings_linkedList *element = startElement;
    do
    {
        element->data.y = currY;
        if (screenSettingsInstance.hoveredField != element)
            vario_display_add_settings_widget(&element->data, screenSettingsInstance.currPage, false, screenSettingsInstance.selectedField == element, false);
        currY += element->data.h;
        element = element->next;
    } while (element != startElement && currY + element->data.h <= Paint.Height);
    vario_display_add_settings_widget(&screenSettingsInstance.hoveredField->data, screenSettingsInstance.currPage, true, screenSettingsInstance.selectedField == element, false);
}

void SettingsManager_openSettings()
{
    xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
    Paint_Clear(WHITE);
    ScreenManager_draw_status_bar();
    ScreenManager_setMode(ScreenMode_Settings);
    screenSettingsInstance.currPage = Settings_Page_PageButtons;
    screenSettingsInstance.hoveredField = screenSettingsInstance.topFielPageButtons;
    screenSettingsInstance.selectedField = NULL;
    draw_Settings_Scrollbar();
    xSemaphoreGive(buffer_Mutex);
    rendering_Push(true);
}

void SettingsManager__enter_press()
{
    // Erste Page Button Seite
    if (screenSettingsInstance.currPage == Settings_Page_PageButtons)
    {
        switch (screenSettingsInstance.hoveredField->data.type)
        {
        case WidgetContent_Settings_PageButton:
            ScreenManager_setMode(ScreenMode_Widget);
            Screen_redraw_Screen(0);
            rendering_Update(true);
            return;
        case WidgetContent_Settings_PageButton_Audio:
            screenSettingsInstance.currPage = Settings_Page_Audio;
            screenSettingsInstance.hoveredField = screenSettingsInstance.topFieldAudio;
            break;
        case WidgetContent_Settings_PageButton_Config:
            screenSettingsInstance.currPage = Settings_Page_Config;
            screenSettingsInstance.hoveredField = screenSettingsInstance.topFieldConfig;
            break;
        case WidgetContent_Settings_PageButton_Flights:
            screenSettingsInstance.currPage = Settings_Page_Flights;
            screenSettingsInstance.hoveredField = screenSettingsInstance.topFieldFlightList;
            esp_err_t err = GetFlights(0, screenSettingsInstance.topFieldFlightList->next);
            if (err != ESP_OK)
            {
                error_handle(err, ERR_SCREEN, "Can't find Flights in Folder");
                widget_Settings_linkedList *firstFlight = (widget_Settings_linkedList *)screenSettingsInstance.topFieldFlightList->next;
                ((FlightList_Widget_Data *)firstFlight->data.extr_data)->FlightName[0] = '\0';
            }
            break;
        default:
            printf("Page Button not found\n");
            break;
        }
        xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
        draw_Settings_Scrollbar();
        xSemaphoreGive(buffer_Mutex);
        rendering_Push(false);
    }
    else if (screenSettingsInstance.currPage == Settings_Page_FlightStatistics)
    {
        screenSettingsInstance.currPage = Settings_Page_Flights;
        draw_Settings_Scrollbar();
        rendering_Push(false);
    }
    // Button in Setting drin
    else
    {
        // deselect field
        if (screenSettingsInstance.selectedField)
        {
            screenSettingsInstance.selectedField = NULL;
            xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
            vario_display_add_settings_widget(&screenSettingsInstance.hoveredField->data, screenSettingsInstance.currPage, true, false, true);
            xSemaphoreGive(buffer_Mutex);
            rendering_Push(false);
        }
        else
        {
            if (screenSettingsInstance.hoveredField->data.style == Widget_settings_BackButton)
            {
                screenSettingsInstance.currPage = Settings_Page_PageButtons;
                screenSettingsInstance.hoveredField = screenSettingsInstance.topFielPageButtons;
                xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
                draw_Settings_Scrollbar();
                xSemaphoreGive(buffer_Mutex);
                rendering_Push(false);
                return;
            }
            else if (screenSettingsInstance.currPage == Settings_Page_Flights)
            {
                screenSettingsInstance.currPage = Settings_Page_FlightStatistics;
                xTaskCreate(vario_display_FlightStatistics, "IGC Show", 1024 * 3, (void *)((FlightList_Widget_Data *)screenSettingsInstance.hoveredField->data.extr_data)->FlightName, 10, NULL);
                return;
            }
            // select field
            screenSettingsInstance.selectedField = screenSettingsInstance.hoveredField;
            xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
            vario_display_add_settings_widget(&screenSettingsInstance.selectedField->data, screenSettingsInstance.currPage, true, true, false);
            xSemaphoreGive(buffer_Mutex);
            rendering_Push(false);
        }
    }
}

void SettingsManager_up_press()
{
    if (screenSettingsInstance.currPage == Settings_Page_FlightStatistics)
    {
        // TODO: Change Flight Stat Site etc.
        return;
    }
    if (!screenSettingsInstance.selectedField)
    {
        if (screenSettingsInstance.hoveredField->prev)
        {
            xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
            vario_display_select_settings_widget(&screenSettingsInstance.hoveredField->data, screenSettingsInstance.currPage, false, false);
            screenSettingsInstance.hoveredField = screenSettingsInstance.hoveredField->prev;
            // we are hovering a field
            vario_display_select_settings_widget(&screenSettingsInstance.hoveredField->data, screenSettingsInstance.currPage, true, false);
            xSemaphoreGive(buffer_Mutex);
            rendering_Push(false);
        }
    }
    else
    {
        xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
        // change Value in Field
        vario_display_settings_widget_handle_input(&screenSettingsInstance.selectedField->data, Button_Direction_up);
        xSemaphoreGive(buffer_Mutex);
        rendering_Push(false);
    }
}

void SettingsManager_down_press()
{
    if (screenSettingsInstance.currPage == Settings_Page_FlightStatistics)
    {
        // TODO: implement
        return;
    }
    if (!screenSettingsInstance.selectedField)
    {
        // we are hovering a field
        if (screenSettingsInstance.hoveredField->next)
        {
            // TODO: Go to top of list, when last element
            xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
            vario_display_select_settings_widget(&screenSettingsInstance.hoveredField->data, screenSettingsInstance.currPage, false, false);
            screenSettingsInstance.hoveredField = screenSettingsInstance.hoveredField->next;
            vario_display_select_settings_widget(&screenSettingsInstance.hoveredField->data, screenSettingsInstance.currPage, true, false);
            xSemaphoreGive(buffer_Mutex);
            rendering_Push(false);
        }
    }
    else
    {
        xSemaphoreTake(buffer_Mutex, portMAX_DELAY);
        // change Value in Field
        vario_display_settings_widget_handle_input(&screenSettingsInstance.selectedField->data, Button_Direction_down);
        xSemaphoreGive(buffer_Mutex);
        rendering_Push(false);
    }
}
