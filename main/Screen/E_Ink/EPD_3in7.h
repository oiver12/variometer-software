/*****************************************************************************
* | File      	:   EPD_3IN7.h
* | Author      :   Waveshare team
* | Function    :   3.7inch e-paper
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2020-07-17
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#ifndef __EPD_3IN7_H_
#define __EPD_3IN7_H_

#include <stdio.h>
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>

/**
 * data
 **/
#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

typedef enum
{
    FULL_G_T_G,  // LUT0 (4Gray_GC): Full Screen 4 Gray Scale Refresh (= 3.04s)
    FULL_G_T_BW, // LUT1 (1Gray_GC): Full Screen 1 Gray Scale Refresh (=0.997s)
    PART_G_T_BW, // LUT2 (1Gray_DU): Part Dispaly Update from GS to B/W (=0.797s)
    PART_BW_T_BW // LUT3 (1Gray_A2): Part Display Update from B/W to B/W (=0.397s)
} Display_RefreshMode;

/**
 * e-Paper GPIO
 **/
#define EPD_RST_PIN 45
#define EPD_DC_PIN 42
#define EPD_CS_PIN 39
#define EPD_BUSY_PIN 40
#define EPD_MOSI_PIN 41
#define EPD_CLK_PIN 46

/**
 * GPIO read and write
 **/
#define DEV_Digital_Write(_pin, _value) gpio_set_level(_pin, _value)
#define DEV_Digital_Read(_pin) gpio_get_level(_pin)

/**
 * delay x ms
 **/
#define DEV_Delay_ms(__xms) vTaskDelay(__xms / portTICK_PERIOD_MS)

// Display resolution
#define EPD_3IN7_WIDTH 280
#define EPD_3IN7_HEIGHT 480

esp_err_t DEV_SPI_WriteByte(UBYTE value);
esp_err_t DEV_Module_Init(void);
void DEV_Module_Exit(void);

esp_err_t EPD_3IN7_4Gray_Clear(void);
esp_err_t EPD_3IN7_4Gray_Init(void);
esp_err_t EPD_3IN7_4Gray_Display(const UBYTE *Image);

esp_err_t EPD_3IN7_1Gray_Clear(void);
esp_err_t EPD_3IN7_1Gray_Init(void);
esp_err_t EPD_3IN7_1Gray_Display(const UBYTE *Image, Display_RefreshMode refreshMode);
esp_err_t EPD_3IN7_1Gray_Display_Part(const UBYTE *PartBuffer, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, Display_RefreshMode refreshMode);
esp_err_t EPD_3IN7_1Gray_Display_Part_FullScreenBuffer(const UBYTE *Image, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, Display_RefreshMode refreshMode);
esp_err_t EDP_3IN7_1Gray_Start(const UBYTE *Image, Display_RefreshMode refreshMode);
void EPD_3IN7_1Gray_WaitBusy();

void EPD_3IN7_Sleep(void);

#endif