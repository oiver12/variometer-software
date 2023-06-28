#ifndef RenderSystem_h
#define RenderSystem_h

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef enum
{
    RenderMode_Update,
    RenderMode_Push,
} RenderModes_t;

esp_err_t startRendering(uint8_t *screenBuffer);
void stopRendering_disableScreen();
void rendering_Push(bool varfullRedraw);
void rendering_Update(bool varfullRedraw);
bool screenIsInit();

#endif