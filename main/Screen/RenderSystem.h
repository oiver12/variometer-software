#ifndef RenderSystem_h
#define RenderSystem_h

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

typedef enum
{
    RenderMode_Update,
    RenderMode_Push,
} RenderModes_t;

typedef struct
{
    QueueHandle_t renderQueue;
    uint8_t *ScreenBuffer;
    SemaphoreHandle_t buffer_Mutex;
} ScreenRender_arg;

esp_err_t startRendering(uint8_t *screenBuffer);
void stopRendering_disableScreen();
void rendering_Push(bool varfullRedraw);
void rendering_Update(bool varfullRedraw);
bool screenIsInit();

#endif