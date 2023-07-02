#ifndef SOUNDMANAGER_VARIO_H
#define SOUNDMANAGER_VARIO_H

#include "main.h"

#include <esp_err.h>

typedef enum
{
    constant_Deep = 0,
    no_Sound = 1,
    Biepen = 2,
} biepState_t;

typedef struct
{
    biepState_t state;
    float frequency;
    int duration;
    float lowerVelocity;
    float upVelocity;
} Tonestate_t;

typedef struct
{
    Tonestate_t *ToneArray;
    size_t sizeToneArray;
    uint16_t volume;
    int indexCurrentToneArray;
    // benötigt um zu biepen --> nur einmal ausrechnen wenn Geschwindigkiet gesetzt wird
    int frequencyNow;
    int durationNow;
    float lastBeepVelocity;
} SoundManager_t;

typedef struct
{
    int frequency;
    int duration;
} SoundManager_Tone_t;

esp_err_t init_SoundManager();
esp_err_t deinit_SoundManager();
void SoundManager_calculateNewBiep();
esp_err_t SoundManager_playSinWave(int frequency, int duration);
esp_err_t SoundManager_playToneArray(SoundManager_Tone_t *toneArray, size_t sizeToneArray);

void SoundManager_setVolume(uint16_t volume);

#endif