#include "SoundManager.h"
#include "DeviceContext.h"
#include "Watchdog/Watchdog.h"
#include "ErrorHandling.h"
#include "pinDefines.h"

#include <driver/i2s.h>
#include <math.h>
#include <string.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define BUFF_SIZE 2048
#define PDM_TX_FREQ_HZ 44100 // I2S PDM TX frequency
#define I2S_NUM (0)
#define VELOCITY_THRESHOLD 0.2

#define Sound_Size_Queue 10

// #define WAVE_AMPTITUDE          (6000.0)        // 1~32767
#define SINE_WAVE_LEN(tone) (uint32_t)((PDM_TX_FREQ_HZ / (float)tone) + 0.5) // The sample point number per sine wave to generate the tone
#define BYTE_NUM_EVERY_TONE(duration) (duration * PDM_TX_FREQ_HZ / 1000)

static const char *TAG = "SoundManager";

int16_t *buffer;
int16_t bufferZero[200];

QueueHandle_t soundQueue = NULL;
SoundManager_t soundManager_instance;
// SemaphoreHandle_t soundManager_Semaphore = NULL;

// 349, 440, 523
SoundManager_Tone_t startSeqeunce[3] = {
    {349, 500},
    {440, 500},
    {523, 500}};

static void SoundManager_NoSound(float duration)
{
    size_t w_bytes = 0;
    esp_err_t err;
    for (int tot_bytes = 0; tot_bytes < BYTE_NUM_EVERY_TONE(duration); tot_bytes += w_bytes)
    {
        err = i2s_write(I2S_NUM, bufferZero, sizeof(bufferZero), &w_bytes, 100 / portTICK_PERIOD_MS) != ESP_OK;
        if (err != ESP_OK)
            error_handle(err, ERR_SOUNDMANAGER, "Can't write I2S data");
    }
}

void SoundManager_Task(void *parameters)
{
    watchdog_add(NULL);
    // No race Conditiom, because all used variables are atomic read and write
    float lastFreqeuncy = soundManager_instance.frequencyNow - 1;
    biepState_t lastBiepState = no_Sound;
    size_t w_bytes = 0;
    int tone_point = 0;
    // SoundManager_playToneArray(startSeqeunce, 3);
    while (true)
    {
        watchdog_reset();
        // Spielen von anderen Tönen. Gegeben über SoundManager_playToneArray
        if (uxQueueMessagesWaiting(soundQueue) != 0)
        {
            SoundManager_Tone_t tempTone;
            xQueueReceive(soundQueue, &tempTone, 0);
            if (tempTone.frequency == 0)
            {
                SoundManager_NoSound(100);
                continue;
            }
            SoundManager_playSinWave(tempTone.frequency, tempTone.duration);
            if (uxQueueMessagesWaiting(soundQueue) == 0)
            {
                SoundManager_NoSound(100);
            }
            continue;
        }

        biepState_t currentBiepState = soundManager_instance.ToneArray[soundManager_instance.indexCurrentToneArray].state;
        float frequencyNow = soundManager_instance.frequencyNow;
        float durationNow = soundManager_instance.durationNow;
        float volume = soundManager_instance.volume;

        if (currentBiepState == no_Sound)
        {
            if (lastBiepState == constant_Deep)
                SoundManager_NoSound(100);
            vTaskDelay(20 / portTICK_PERIOD_MS);
            lastBiepState = currentBiepState;
            continue;
        }
        lastBiepState = currentBiepState;
        // Buffer neu berechnen, wenn Frequenz geändert wurde
        if (lastFreqeuncy != frequencyNow)
        {
            tone_point = SINE_WAVE_LEN(frequencyNow);
            /* Generate the tone buffer (needs 1.2ms for it)*/
            for (int i = 0; i < tone_point; i++)
            {
                buffer[i] = (int16_t)((sin(2 * (float)i * M_PI / tone_point)) * volume);
            }
            lastFreqeuncy = frequencyNow;
        }
        w_bytes = 0;
#ifdef SOUND_ON
        for (int tot_bytes = 0; tot_bytes < BYTE_NUM_EVERY_TONE(durationNow); tot_bytes += w_bytes)
        {
            err = i2s_write(I2S_NUM, buffer, tone_point * sizeof(int16_t), &w_bytes, 100 / portTICK_PERIOD_MS);
            if (err != ESP_OK)
            {
                error_handle(err, ERR_SOUNDMANAGER, "Can't write I2S data");
            }
        }
#else
        vTaskDelay(500 / portTICK_PERIOD_MS);
#endif
        // spiele für die gleich lange Zeit kein Ton aus (sende nur Nullen)
        if (currentBiepState != constant_Deep)
        {
            SoundManager_NoSound(durationNow);
        }
    }
    vTaskDelete(NULL);
}

void SoundManager_calculateNewBiep()
{
    float newVelocity;
    vario_state_get_member(&newVelocity, vario_state_vertVelLazyBiep, true);
    // printf("VertVel: %.2f\n", vertVel);
    //  in einer For schleife herausfinden, zwischen welchen Punkten die Geschwindigkeit ist
    for (size_t i = 0; i < soundManager_instance.sizeToneArray; i++)
    {
        if (newVelocity >= soundManager_instance.ToneArray[i].lowerVelocity && newVelocity <= soundManager_instance.ToneArray[i].upVelocity)
        {
            float difference = soundManager_instance.lastBeepVelocity - newVelocity;
            // Wenn Differenz Geschwindigkeit kliener als Thershold ist, dann nicht neuer Ton ausgeben, ausser man geht von nicht piepsen in piepsen oder umgekehrt
            if (fabs(difference) < VELOCITY_THRESHOLD && !(soundManager_instance.indexCurrentToneArray > 1 && i == 1) && !(soundManager_instance.indexCurrentToneArray <= 1 && i > 1))
            {
                return;
            }
            soundManager_instance.lastBeepVelocity = newVelocity;
            soundManager_instance.indexCurrentToneArray = i;
            // wenn letzter Ton, dann einfach Geschwindigkeit nehmen
            if ((soundManager_instance.sizeToneArray - 1) == i || i == 0)
            {
                soundManager_instance.frequencyNow = soundManager_instance.ToneArray[i].frequency;
                soundManager_instance.durationNow = soundManager_instance.ToneArray[i].duration;
            }
            // sonst linear Interplorieren
            else
            {
                // linear zwischen zwei States machen
                float factor = (newVelocity - soundManager_instance.ToneArray[i].lowerVelocity) / (soundManager_instance.ToneArray[i].upVelocity - soundManager_instance.ToneArray[i].lowerVelocity);
                soundManager_instance.frequencyNow = factor * (soundManager_instance.ToneArray[i + 1].frequency - soundManager_instance.ToneArray[i].frequency) + soundManager_instance.ToneArray[i].frequency;
                soundManager_instance.durationNow = factor * (soundManager_instance.ToneArray[i + 1].duration - soundManager_instance.ToneArray[i].duration) + soundManager_instance.ToneArray[i].duration;
            }
        }
    }
}

esp_err_t init_SoundManager()
{
    esp_err_t err;
#ifdef VERSION2n2
    gpio_config_t enableGPIOOutput = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << AUDIO_EN,
        .pull_down_en = 0,
        .pull_up_en = 0};

    gpio_config(&enableGPIOOutput);
#ifdef SOUND_ON
    gpio_set_level(AUDIO_EN, 1);
#endif
#endif
#ifndef SOUND_ON
    ESP_LOGI(TAG, "Sound not on returning...");
    return ESP_OK;
#endif
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = PDM_TX_FREQ_HZ,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .dma_buf_count = 6,
        .dma_buf_len = 240,
        .use_apll = false,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1 // Interrupt level 1
    };
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCLK,
        .data_out_num = I2S_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE};
    ERROR_CHECKRETURN(i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL));
    ERROR_CHECKRETURN(i2s_set_pin(I2S_NUM, &pin_config));
    buffer = (int16_t *)calloc(1, BUFF_SIZE);
    if (!buffer)
        return ESP_ERR_NO_MEM;

    soundManager_instance.ToneArray = calloc(6, sizeof(Tonestate_t));
    if (!soundManager_instance.ToneArray)
        return ESP_ERR_NO_MEM;

    soundManager_instance.sizeToneArray = 6;
    Tonestate_t stateBeep[6] = {
        {.state = constant_Deep, .frequency = 250, .duration = 200, .lowerVelocity = -999, .upVelocity = -2.5f},
        {.state = no_Sound, .frequency = 0, .duration = 0, .lowerVelocity = -2.5f, .upVelocity = 0.2f},
        {.state = Biepen, .frequency = 700, .duration = 750, .lowerVelocity = 0.2f, .upVelocity = 1.5f},
        {.state = Biepen, .frequency = 850, .duration = 540, .lowerVelocity = 1.5f, .upVelocity = 3.5f},
        {.state = Biepen, .frequency = 1000, .duration = 300, .lowerVelocity = 3.5f, .upVelocity = 10},
        {.state = Biepen, .frequency = 1600, .duration = 90, .lowerVelocity = 10, .upVelocity = 999}};
    memcpy(soundManager_instance.ToneArray, stateBeep, sizeof(stateBeep));
    soundManager_instance.indexCurrentToneArray = 1;
    uint16_t volume = 6000;
    vario_settings_set_member(&volume, vario_settings_soundVolume);
    soundQueue = xQueueCreate(Sound_Size_Queue, sizeof(SoundManager_Tone_t));
    xTaskCreate(SoundManager_Task, "SoundManager_Task", 2 * 1024, NULL, 7, NULL);
    return ESP_OK;
}

esp_err_t deinit_SoundManager()
{
#ifdef VERSIOn2n2
    gpio_set_level(AUDIO_EN, 0);
#endif
    free(soundManager_instance.ToneArray);
    free(buffer);
    return ESP_OK;
}

esp_err_t SoundManager_playToneArray(SoundManager_Tone_t *toneArray, size_t sizeToneArray)
{
#ifndef SOUND_ON
    return ESP_OK;
#endif
    for (size_t i = 0; i < sizeToneArray; i++)
    {
        BaseType_t returnValue = xQueueSend(soundQueue, &toneArray[i], 3);
        if (returnValue != pdTRUE)
        {
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

esp_err_t SoundManager_playSinWave(int frequency, int duration)
{
    esp_err_t err;
    size_t w_bytes = 0;
    int tone_point = SINE_WAVE_LEN(frequency);
    /* Generate the tone buffer */
    for (int i = 0; i < tone_point; i++)
    {
        buffer[i] = (int16_t)((sin(2 * (float)i * M_PI / tone_point)) * soundManager_instance.volume);
    }

    for (int tot_bytes = 0; tot_bytes < BYTE_NUM_EVERY_TONE(duration * 2); tot_bytes += w_bytes)
    {
        err = i2s_write(I2S_NUM, buffer, tone_point * sizeof(int16_t), &w_bytes, 1000) != ESP_OK;
        if (err != ESP_OK)
        {
            error_handle(err, ERR_SOUNDMANAGER, "Can't write I2S data");
        }
    }
    return ESP_OK;
}

void SoundManager_setVolume(uint16_t volume)
{
    soundManager_instance.volume = volume;
}