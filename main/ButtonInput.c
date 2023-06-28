#include "ButtonInput.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "Screen/ScreenManager.h"
#include <esp_log.h>
#include <esp_timer.h>
#include "ErrorHandling.h"

#define Button_Enter 1 // Button 1
#define Button_Menu 2  // Button 2
#define Button_Up 9    // Button 3
#define Button_Down 48 // Button 4

#define ButtonEnter_Short (1 << 0)
#define ButtonEnter_Long (1 << 1)
#define ButtonMenu_Short (1 << 2)
#define ButtonMenu_Long (1 << 3)
#define ButtonUp_Short (1 << 4)
#define ButtonUp_Long (1 << 5)
#define ButtonDown_Short (1 << 6)
#define ButtonDown_Long (1 << 7)

#define TAG "ButtonInput"
#define LONG_PRESS_TIME_MS 500

TimerHandle_t button_press_Up_semaphore;
TimerHandle_t button_press_Down_semaphore;
TimerHandle_t button_press_Enter_semaphore;
TimerHandle_t button_press_Menu_semaphore;

// EventGroup für kommunikation zwischen Interrupt und Task
EventGroupHandle_t button_press_EventGroup;

static void button_read_polling(void *arg)
{
    static uint16_t btn_up = 0;
    static uint16_t btn_down = 0;
    static uint16_t btn_enter = 0;
    static uint16_t btn_menu = 0;

    btn_up = (btn_up << 1) | gpio_get_level(Button_Up) | 0xe000;
    btn_down = (btn_down << 1) | gpio_get_level(Button_Down) | 0xe000;
    btn_enter = (btn_enter << 1) | gpio_get_level(Button_Enter) | 0xe000;
    btn_menu = (btn_menu << 1) | gpio_get_level(Button_Menu) | 0xe000;

    /* Button Up */
    if (btn_up == 0xf000)
    { // High for 1 bits low for 12 (each bit is 1 loop thru this code).
        // started Pressing
        xTimerStart(button_press_Up_semaphore, 0);
    }
    else if (btn_up == 0xefff)
    {
        // Released
        if (xTimerIsTimerActive(button_press_Up_semaphore) == pdTRUE)
        {
            xTimerStop(button_press_Up_semaphore, 0);
            xEventGroupSetBits(button_press_EventGroup, ButtonUp_Short);
        }
    }

    /* Button Down */
    if (btn_down == 0xf000)
    { // High for 1 bits low for 12 (each bit is 1 loop thru this code).
        // started Pressing
        xTimerStart(button_press_Down_semaphore, 0);
    }
    else if (btn_down == 0xefff)
    {
        // Released
        if (xTimerIsTimerActive(button_press_Down_semaphore) == pdTRUE)
        {
            xTimerStop(button_press_Down_semaphore, 0);
            xEventGroupSetBits(button_press_EventGroup, ButtonDown_Short);
        }
    }

    /* Button Enter */
    if (btn_enter == 0xf000)
    { // High for 1 bits low for 12 (each bit is 1 loop thru this code).
        // started Pressing
        xTimerStart(button_press_Enter_semaphore, 0);
    }
    else if (btn_enter == 0xefff)
    {
        // Released
        if (xTimerIsTimerActive(button_press_Enter_semaphore) == pdTRUE)
        {
            xTimerStop(button_press_Enter_semaphore, 0);
            xEventGroupSetBits(button_press_EventGroup, ButtonEnter_Short);
        }
    }

    /* Button Menu */
    if (btn_menu == 0xf000)
    { // High for 1 bits low for 12 (each bit is 1 loop thru this code).
        // started Pressing
        xTimerStart(button_press_Menu_semaphore, 0);
    }
    else if (btn_menu == 0xefff)
    {
        // Released
        if (xTimerIsTimerActive(button_press_Menu_semaphore) == pdTRUE)
        {
            xTimerStop(button_press_Menu_semaphore, 0);
            xEventGroupSetBits(button_press_EventGroup, ButtonMenu_Short);
        }
    }
}

static void buttonUP_TimerCallback(TimerHandle_t xTimer)
{
    xEventGroupSetBits(button_press_EventGroup, ButtonUp_Long);
}
static void buttonDown_TimerCallback(TimerHandle_t xTimer)
{
    xEventGroupSetBits(button_press_EventGroup, ButtonDown_Long);
}
static void buttonEnter_TimerCallback(TimerHandle_t xTimer)
{
    xEventGroupSetBits(button_press_EventGroup, ButtonEnter_Long);
}
static void buttonMenu_TimerCallback(TimerHandle_t xTimer)
{
    xEventGroupSetBits(button_press_EventGroup, ButtonMenu_Long);
}

static void Button_Task(void *arg)
{
    while (true)
    {
        EventBits_t bits = xEventGroupWaitBits(button_press_EventGroup, ButtonEnter_Short | ButtonEnter_Long | ButtonMenu_Short | ButtonMenu_Long | ButtonUp_Short | ButtonUp_Long | ButtonDown_Short | ButtonDown_Long, pdTRUE, pdFALSE, portMAX_DELAY);
        if (bits & ButtonEnter_Short)
        {
            ScreenManager_EnterKeyPressed();
            ESP_LOGI(TAG, "Button Enter Short Press");
        }
        if (bits & ButtonEnter_Long)
        {
            ESP_LOGI(TAG, "Button Enter Long Press");
        }
        if (bits & ButtonMenu_Short)
        {
            ScreenManager_powerButtonPressed(false);
            ESP_LOGI(TAG, "Button Menu Short Press");
        }
        if (bits & ButtonMenu_Long)
        {
            ScreenManager_powerButtonPressed(true);
            ESP_LOGI(TAG, "Button Menu Long Press");
        }
        if (bits & ButtonUp_Short)
        {
            ScreenManager_UpArrowPressed();
            ESP_LOGI(TAG, "Button Up Short Press");
        }
        if (bits & ButtonUp_Long)
        {
            ESP_LOGI(TAG, "Button Up Long Press");
        }
        if (bits & ButtonDown_Short)
        {
            ScreenManager_DownArrowPressed();
            ESP_LOGI(TAG, "Button Down Short Press");
        }
        if (bits & ButtonDown_Long)
        {
            ESP_LOGI(TAG, "Button Down Long Press");
        }
    }
}

esp_err_t ButtonInput_Init()
{
    // Set Button pins as input and enable interrupt
    gpio_config_t gpio_config_INT_Buttons = {
        .pin_bit_mask = (1ULL << Button_Enter) | (1ULL << Button_Menu) | (1ULL << Button_Up) | (1ULL << Button_Down),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&gpio_config_INT_Buttons);

    button_press_EventGroup = xEventGroupCreate();
    xEventGroupClearBits(button_press_EventGroup, ButtonEnter_Short | ButtonEnter_Long | ButtonMenu_Short | ButtonMenu_Long | ButtonUp_Short | ButtonUp_Long | ButtonDown_Short | ButtonDown_Long);

    button_press_Up_semaphore = xTimerCreate("Button_Up", LONG_PRESS_TIME_MS / portTICK_PERIOD_MS, pdFALSE, (void *)0, buttonUP_TimerCallback);
    button_press_Down_semaphore = xTimerCreate("Button_Down", LONG_PRESS_TIME_MS / portTICK_PERIOD_MS, pdFALSE, (void *)0, buttonDown_TimerCallback);
    button_press_Enter_semaphore = xTimerCreate("Button_Enter", LONG_PRESS_TIME_MS / portTICK_PERIOD_MS, pdFALSE, (void *)0, buttonEnter_TimerCallback);
    button_press_Menu_semaphore = xTimerCreate("Button_Menu", LONG_PRESS_TIME_MS / portTICK_PERIOD_MS, pdFALSE, (void *)0, buttonMenu_TimerCallback);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &button_read_polling,
        .name = "periodic_button",
    };
    esp_timer_handle_t periodic_timer;
    ERROR_CHECKRETURN(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ERROR_CHECKRETURN(esp_timer_start_periodic(periodic_timer, 1 * 1000)); // one time all 1 ms

    xTaskCreate(Button_Task, "Input Button Task", 1024 * 3, NULL, 3, NULL);
    return ESP_OK;
}