/*****************************************************************************
* | File      	:   EPD_3IN7.C
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
#include "EPD_3in7.h"
#include <driver/spi_master.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "ErrorHandling.h"

#define SPIBUS_READ (0x80)  /*!< addr | SPIBUS_READ  */
#define SPIBUS_WRITE (0x7F) /*!< addr & SPIBUS_WRITE */

spi_device_handle_t screen_spi_handle;

static const UBYTE lut_4Gray_GC[] =
    {
        0x2A, 0x06, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1
        0x28, 0x06, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 2
        0x20, 0x06, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3
        0x14, 0x06, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
        0x00, 0x02, 0x02, 0x0A, 0x00, 0x00, 0x00, 0x08, 0x08, 0x02, // 6
        0x00, 0x02, 0x02, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 10
        0x22, 0x22, 0x22, 0x22, 0x22};

static const UBYTE lut_1Gray_GC[] =
    {
        0x2A, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1
        0x05, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 2
        0x2A, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3
        0x05, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
        0x00, 0x02, 0x03, 0x0A, 0x00, 0x02, 0x06, 0x0A, 0x05, 0x00, // 6
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 10
        0x22, 0x22, 0x22, 0x22, 0x22};

static const UBYTE lut_1Gray_DU[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1
        0x01, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0A, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
        0x00, 0x00, 0x05, 0x05, 0x00, 0x05, 0x03, 0x05, 0x05, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x22, 0x22, 0x22, 0x22, 0x22};

static const UBYTE lut_1Gray_A2[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1
        0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 2
        0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
        0x00, 0x00, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 6
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 10
        0x22, 0x22, 0x22, 0x22, 0x22};

esp_err_t EPD_3IN7_SPI_Init()
{
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // Clock out at 10 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = EPD_CS_PIN,         // CS pin
        .queue_size = 7,                    // We want to be able to queue 7 transactions at a time
    };
    int i = 0;
    esp_err_t err;
    while ((err = spi_bus_add_device(SPI2_HOST, &devcfg, &screen_spi_handle)) != ESP_OK && i <= 5)
    {
        i++;
    }
    if (err != ESP_OK)
    {
        printf("Could not add Screen to spi bus: %s\n", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}

esp_err_t DEV_SPI_WriteByte(UBYTE value)
{
    spi_transaction_t transaction;
    transaction.flags = 0;
    transaction.cmd = 0;
    transaction.length = 8;
    transaction.rxlength = 0;
    transaction.user = NULL;
    transaction.tx_buffer = &value;
    transaction.rx_buffer = NULL;
    return spi_device_transmit(screen_spi_handle, &transaction);
}

esp_err_t DEV_SPI_WriteBytes(const UBYTE *value, size_t length)
{
    spi_transaction_t transaction;
    transaction.flags = 0;
    transaction.cmd = 0;
    transaction.length = length * 8;
    transaction.rxlength = 0;
    transaction.user = NULL;
    transaction.tx_buffer = value;
    transaction.rx_buffer = NULL;
    return spi_device_transmit(screen_spi_handle, &transaction);
}

esp_err_t DEV_Module_Init(void)
{
    gpio_config_t io_conf_output = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << EPD_DC_PIN) | (1ULL << EPD_RST_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0};
    gpio_config(&io_conf_output);
    gpio_config_t io_conf_input = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << EPD_BUSY_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0};
    gpio_config(&io_conf_input);
    DEV_Digital_Write(EPD_DC_PIN, 0);
    // DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    return EPD_3IN7_SPI_Init();
}

void DEV_Module_Exit(void)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    // DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_RST_PIN, 0);
}

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_3IN7_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(300);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(300);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static esp_err_t EPD_3IN7_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    return DEV_SPI_WriteByte(Reg);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static esp_err_t EPD_3IN7_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    return DEV_SPI_WriteByte(Data);
}

static esp_err_t EPD_3IN7_SendDataStream(const UBYTE *Data, size_t size)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    int blockSize = 4090;
    for (size_t i = 0; i < size; i += blockSize)
    {
        if (i + blockSize > size)
        {
            ERROR_CHECKRETURN(DEV_SPI_WriteBytes(&Data[i], size - i));
        }
        else
        {
            ERROR_CHECKRETURN(DEV_SPI_WriteBytes(&Data[i], blockSize));
        }
    }
    return ESP_OK;
}

static void IRAM_ATTR gpio_busy_isr_handler(void *arg)
{
    TaskHandle_t taskHandle = (TaskHandle_t)arg;
    BaseType_t HPTaskWoken = pdFALSE;
    // unblock TaskMeasureAcc
    vTaskNotifyGiveFromISR(taskHandle, &HPTaskWoken);
#ifndef ISSIMULATION
    if (HPTaskWoken == pdTRUE)
        portYIELD_FROM_ISR();
#endif
}

static esp_err_t EPD_3IN7_ReadBusy_HIGH(void)
{
    gpio_isr_handler_add((gpio_num_t)EPD_BUSY_PIN, gpio_busy_isr_handler, xTaskGetCurrentTaskHandle());
    uint64_t startTime = esp_timer_get_time();
    uint32_t notificationValue = ulTaskNotifyTake(pdTRUE, 5000 / portTICK_PERIOD_MS);
    esp_err_t err = ESP_OK;
    if (notificationValue == 0)
    {
        ESP_LOGW("EPD", "Interrupt didn't fire");
        err = ESP_ERR_TIMEOUT;
    }
    else
    {
        // ESP_LOGI("EPD", "Interrupt did fire: %i ms", (int)(esp_timer_get_time() - startTime) / 1000);
        err = ESP_OK;
    }
    gpio_isr_handler_remove(EPD_BUSY_PIN);
    return err;
}

/******************************************************************************
function :	set the look-up tables
parameter: //https://www.waveshare.net/w/upload/c/c4/E-paper-mode-declaration.pdf
LUT0 (4Gray_GC): Full Screen 4 Gray Scale Refresh (= 3.04s)
LUT1 (1Gray_GC): Full Screen 1 Gray Scale Refresh (=0.997s)
LUT2 (1Gray_DU): Part Dispaly Update from GS to B/W (=0.797s)
LUT3 (1Gray_A2): Part Display Update from B/W to B/W (=0.397s)
******************************************************************************/
esp_err_t EPD_3IN7_Load_LUT(UBYTE lut)
{
    ERROR_CHECKRETURN(EPD_3IN7_SendCommand(0x32));
    if (lut == 0)
        ERROR_CHECKRETURN(EPD_3IN7_SendDataStream(lut_4Gray_GC, 105));
    else if (lut == 1)
        ERROR_CHECKRETURN(EPD_3IN7_SendDataStream(lut_1Gray_GC, 105));
    else if (lut == 2)
        ERROR_CHECKRETURN(EPD_3IN7_SendDataStream(lut_1Gray_DU, 105));
    else if (lut == 3)
        ERROR_CHECKRETURN(EPD_3IN7_SendDataStream(lut_1Gray_A2, 105));
    else
        return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
esp_err_t EPD_3IN7_4Gray_Init(void)
{
    EPD_3IN7_Reset();
    esp_err_t err = ESP_OK;
    err |= EPD_3IN7_SendCommand(0x12);
    DEV_Delay_ms(300);

    err |= EPD_3IN7_SendCommand(0x46);
    err |= EPD_3IN7_SendData(0xF7);
    ERROR_CHECKRETURN(err);
    EPD_3IN7_ReadBusy_HIGH();
    err |= EPD_3IN7_SendCommand(0x47);
    err |= EPD_3IN7_SendData(0xF7);
    ERROR_CHECKRETURN(err);
    EPD_3IN7_ReadBusy_HIGH();

    err |= EPD_3IN7_SendCommand(0x01); // setting gaet number
    err |= EPD_3IN7_SendData(0xDF);
    err |= EPD_3IN7_SendData(0x01);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x03); // set gate voltage
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x04); // set source voltage
    err |= EPD_3IN7_SendData(0x41);
    err |= EPD_3IN7_SendData(0xA8);
    err |= EPD_3IN7_SendData(0x32);

    err |= EPD_3IN7_SendCommand(0x11); // set data entry sequence
    err |= EPD_3IN7_SendData(0x03);

    err |= EPD_3IN7_SendCommand(0x3C); // set border
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x0C); // set booster strength
    err |= EPD_3IN7_SendData(0xAE);
    err |= EPD_3IN7_SendData(0xC7);
    err |= EPD_3IN7_SendData(0xC3);
    err |= EPD_3IN7_SendData(0xC0);
    err |= EPD_3IN7_SendData(0xC0);

    err |= EPD_3IN7_SendCommand(0x18); // set internal sensor on
    err |= EPD_3IN7_SendData(0x80);

    err |= EPD_3IN7_SendCommand(0x2C); // set vcom value
    err |= EPD_3IN7_SendData(0x44);

    err |= EPD_3IN7_SendCommand(0x37); // set display option, these setting turn on previous function
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x44); // setting X direction start/end position of RAM
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x17);
    err |= EPD_3IN7_SendData(0x01);

    err |= EPD_3IN7_SendCommand(0x45); // setting Y direction start/end position of RAM
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0xDF);
    err |= EPD_3IN7_SendData(0x01);

    err |= EPD_3IN7_SendCommand(0x22); // Display Update Control 2
    err |= EPD_3IN7_SendData(0xCF);
    return err;
}

/******************************************************************************
function :  Initialize the e-Paper register
parameter:
******************************************************************************/
esp_err_t EPD_3IN7_1Gray_Init(void)
{
    esp_err_t err = ESP_OK;
    EPD_3IN7_Reset();

    err |= EPD_3IN7_SendCommand(0x12);
    DEV_Delay_ms(300);

    err |= EPD_3IN7_SendCommand(0x46);
    err |= EPD_3IN7_SendData(0xF7);
    ERROR_CHECKRETURN(err);
    ERROR_CHECKRETURN(EPD_3IN7_ReadBusy_HIGH());

    err |= EPD_3IN7_SendCommand(0x47);
    err |= EPD_3IN7_SendData(0xF7);
    ERROR_CHECKRETURN(err);
    ERROR_CHECKRETURN(EPD_3IN7_ReadBusy_HIGH());

    err |= EPD_3IN7_SendCommand(0x01); // setting gaet number
    err |= EPD_3IN7_SendData(0xDF);
    err |= EPD_3IN7_SendData(0x01);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x03); // set gate voltage
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x04); // set source voltage
    err |= EPD_3IN7_SendData(0x41);
    err |= EPD_3IN7_SendData(0xA8);
    err |= EPD_3IN7_SendData(0x32);

    err |= EPD_3IN7_SendCommand(0x11); // set data entry sequence
    err |= EPD_3IN7_SendData(0x03);

    err |= EPD_3IN7_SendCommand(0x3C); // set border
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x0C); // set booster strength
    err |= EPD_3IN7_SendData(0xAE);
    err |= EPD_3IN7_SendData(0xC7);
    err |= EPD_3IN7_SendData(0xC3);
    err |= EPD_3IN7_SendData(0xC0);
    err |= EPD_3IN7_SendData(0xC0);

    err |= EPD_3IN7_SendCommand(0x18); // set internal sensor on
    err |= EPD_3IN7_SendData(0x80);

    err |= EPD_3IN7_SendCommand(0x2C); // set vcom value
    err |= EPD_3IN7_SendData(0x44);

    err |= EPD_3IN7_SendCommand(0x37); // set display option, these setting turn on previous function
    err |= EPD_3IN7_SendData(0x00);    // can switch 1 gray or 4 gray
    err |= EPD_3IN7_SendData(0xFF);
    err |= EPD_3IN7_SendData(0xFF);
    err |= EPD_3IN7_SendData(0xFF);
    err |= EPD_3IN7_SendData(0xFF);
    err |= EPD_3IN7_SendData(0x4F);
    err |= EPD_3IN7_SendData(0xFF);
    err |= EPD_3IN7_SendData(0xFF);
    err |= EPD_3IN7_SendData(0xFF);
    err |= EPD_3IN7_SendData(0xFF);

    err |= EPD_3IN7_SendCommand(0x44); // setting X direction start/end position of RAM
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x17);
    err |= EPD_3IN7_SendData(0x01);

    err |= EPD_3IN7_SendCommand(0x45); // setting Y direction start/end position of RAM
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0xDF);
    err |= EPD_3IN7_SendData(0x01);

    err |= EPD_3IN7_SendCommand(0x22); // Display Update Control 2
    err |= EPD_3IN7_SendData(0xCF);
    return err;
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
esp_err_t EPD_3IN7_4Gray_Clear(void)
{
    uint64_t startTime = esp_timer_get_time();
    UWORD Width, Height;
    Width = (EPD_3IN7_WIDTH % 8 == 0) ? (EPD_3IN7_WIDTH / 8) : (EPD_3IN7_WIDTH / 8 + 1);
    Height = EPD_3IN7_HEIGHT;

    esp_err_t err = ESP_OK;

    err |= EPD_3IN7_SendCommand(0x49);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendCommand(0x4E);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendCommand(0x4F);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x24);
    ERROR_CHECKRETURN(err);

    for (UWORD j = 0; j < Height; j++)
    {
        for (UWORD i = 0; i < Width; i++)
        {
            err |= EPD_3IN7_SendData(0xff);
        }
    }
    ERROR_CHECKRETURN(err);

    err |= EPD_3IN7_SendCommand(0x4E);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x4F);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x26);
    ERROR_CHECKRETURN(err);

    for (UWORD j = 0; j < Height; j++)
    {
        for (UWORD i = 0; i < Width; i++)
        {
            err |= EPD_3IN7_SendData(0xff);
        }
    }
    ERROR_CHECKRETURN(err);

    err |= EPD_3IN7_Load_LUT(0);
    err |= EPD_3IN7_SendCommand(0x22);
    err |= EPD_3IN7_SendData(0xC7);

    err |= EPD_3IN7_SendCommand(0x20);
    ERROR_CHECKRETURN(err);
    ERROR_CHECKRETURN(EPD_3IN7_ReadBusy_HIGH());

    printf("Update Time: %llu \n\r", (esp_timer_get_time() - startTime) / 1000);
    return err;
}

/******************************************************************************
function :  Clear screen
parameter:
******************************************************************************/
esp_err_t EPD_3IN7_1Gray_Clear(void)
{
    UWORD i;
    UWORD IMAGE_COUNTER = EPD_3IN7_WIDTH * EPD_3IN7_HEIGHT / 8;
    esp_err_t err = ESP_OK;
    err |= EPD_3IN7_SendCommand(0x4E);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendCommand(0x4F);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x24);
    ERROR_CHECKRETURN(err);

    for (i = 0; i < IMAGE_COUNTER; i++)
    {
        err |= EPD_3IN7_SendData(0xff);
    }
    ERROR_CHECKRETURN(err);

    err |= EPD_3IN7_Load_LUT(2);

    err |= EPD_3IN7_SendCommand(0x20);
    ERROR_CHECKRETURN(err);
    ERROR_CHECKRETURN(EPD_3IN7_ReadBusy_HIGH());
    return err;
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
esp_err_t EPD_3IN7_4Gray_Display(const UBYTE *Image)
{
    UDOUBLE i, j, k;
    UBYTE temp1, temp2, temp3;
    esp_err_t err = ESP_OK;
    err |= EPD_3IN7_SendCommand(0x49);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x4E);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x4F);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x24);
    ERROR_CHECKRETURN(err);

    for (i = 0; i < 16800; i++)
    {
        temp3 = 0;
        for (j = 0; j < 2; j++)
        {
            temp1 = Image[i * 2 + j];
            for (k = 0; k < 2; k++)
            {
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01; // white
                else if (temp2 == 0x00)
                    temp3 |= 0x00; // black
                else if (temp2 == 0x80)
                    temp3 |= 0x00; // gray1
                else               // 0x40
                    temp3 |= 0x01; // gray2
                temp3 <<= 1;

                temp1 <<= 2;
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0) // white
                    temp3 |= 0x01;
                else if (temp2 == 0x00) // black
                    temp3 |= 0x00;
                else if (temp2 == 0x80)
                    temp3 |= 0x00; // gray1
                else               // 0x40
                    temp3 |= 0x01; // gray2
                if (j != 1 || k != 1)
                    temp3 <<= 1;

                temp1 <<= 2;
            }
        }
        err |= EPD_3IN7_SendData(temp3);
    }
    ERROR_CHECKRETURN(err);
    // new  data
    err |= EPD_3IN7_SendCommand(0x4E);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x4F);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x26);
    ERROR_CHECKRETURN(err);

    for (i = 0; i < 16800; i++)
    {
        temp3 = 0;
        for (j = 0; j < 2; j++)
        {
            temp1 = Image[i * 2 + j];
            for (k = 0; k < 2; k++)
            {
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01; // white
                else if (temp2 == 0x00)
                    temp3 |= 0x00; // black
                else if (temp2 == 0x80)
                    temp3 |= 0x01; // gray1
                else               // 0x40
                    temp3 |= 0x00; // gray2
                temp3 <<= 1;

                temp1 <<= 2;
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0) // white
                    temp3 |= 0x01;
                else if (temp2 == 0x00) // black
                    temp3 |= 0x00;
                else if (temp2 == 0x80)
                    temp3 |= 0x01; // gray1
                else               // 0x40
                    temp3 |= 0x00; // gray2
                if (j != 1 || k != 1)
                    temp3 <<= 1;

                temp1 <<= 2;
            }
        }
        err |= EPD_3IN7_SendData(temp3);
    }
    ERROR_CHECKRETURN(err);

    err |= EPD_3IN7_Load_LUT(0);

    err |= EPD_3IN7_SendCommand(0x22);
    err |= EPD_3IN7_SendData(0xC7);

    err |= EPD_3IN7_SendCommand(0x20);

    ERROR_CHECKRETURN(err);
    ERROR_CHECKRETURN(EPD_3IN7_ReadBusy_HIGH());
    return err;
}

/******************************************************************************
function :  Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
esp_err_t EPD_3IN7_1Gray_Display(const UBYTE *Image, Display_RefreshMode refreshMode)
{
    // UWORD i;
    UWORD IMAGE_COUNTER = EPD_3IN7_WIDTH * EPD_3IN7_HEIGHT / 8;
    esp_err_t err = ESP_OK;

    err |= EPD_3IN7_SendCommand(0x4E);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendCommand(0x4F);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x24);
    ERROR_CHECKRETURN(err);

    err |= EPD_3IN7_SendDataStream(Image, IMAGE_COUNTER);
    ERROR_CHECKRETURN(err);

    err |= EPD_3IN7_Load_LUT(refreshMode);
    err |= EPD_3IN7_SendCommand(0x20);
    ERROR_CHECKRETURN(err);
    ERROR_CHECKRETURN(EPD_3IN7_ReadBusy_HIGH());
    return err;
}

/******************************************************************************
function :  Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
esp_err_t EPD_3IN7_1Gray_Display_Part(const UBYTE *PartBuffer, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, Display_RefreshMode refreshMode)
{
    UWORD Width;
    // 1 bit per Pixel. Pro Byte 8 Pixel. Muss ganzes Byte senden darum 8+1
    Width = (Xend - Xstart) % 8 == 0 ? (Xend - Xstart) / 8 : (Xend - Xstart) / 8 + 1;
    UWORD IMAGE_COUNTER = Width * (Yend - Ystart);

    Xend -= 1;
    Yend -= 1;

    esp_err_t err = ESP_OK;
    // Sendet start Adresse von X und Endadresse von X
    err |= EPD_3IN7_SendCommand(0x44);
    err |= EPD_3IN7_SendData(Xstart & 0xff);
    err |= EPD_3IN7_SendData((Xstart >> 8) & 0x03);
    err |= EPD_3IN7_SendData(Xend & 0xff);
    err |= EPD_3IN7_SendData((Xend >> 8) & 0x03);
    // Sendet start Adresse von Y und Endadresse von Y
    err |= EPD_3IN7_SendCommand(0x45);
    err |= EPD_3IN7_SendData(Ystart & 0xff);
    err |= EPD_3IN7_SendData((Ystart >> 8) & 0x03);
    err |= EPD_3IN7_SendData(Yend & 0xff);
    err |= EPD_3IN7_SendData((Yend >> 8) & 0x03);

    err |= EPD_3IN7_SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    err |= EPD_3IN7_SendData(Xstart & 0xFF);

    err |= EPD_3IN7_SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    err |= EPD_3IN7_SendData(Ystart & 0xFF);
    err |= EPD_3IN7_SendData((Ystart >> 8) & 0xFF);

    // Starte senden von Daten in den RAM
    err |= EPD_3IN7_SendCommand(0x24);
    err |= EPD_3IN7_SendDataStream(PartBuffer, IMAGE_COUNTER);

    err |= EPD_3IN7_Load_LUT(refreshMode);
    err |= EPD_3IN7_SendCommand(0x20);
    ERROR_CHECKRETURN(err);
    ERROR_CHECKRETURN(EPD_3IN7_ReadBusy_HIGH());
    return err;
}

esp_err_t EPD_3IN7_1Gray_Display_Part_FullScreenBuffer(const UBYTE *Image, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, Display_RefreshMode refreshMode)
{
    // 35 = Paint.WidthByte
    UDOUBLE Addr = Xstart / 8 + Ystart * 35;
    return EPD_3IN7_1Gray_Display_Part(&Image[Addr], Xstart, Ystart, Xend, Yend, refreshMode);
}

esp_err_t EDP_3IN7_1Gray_Start(const UBYTE *Image, Display_RefreshMode refreshMode)
{
    // UWORD i;
    UWORD IMAGE_COUNTER = EPD_3IN7_WIDTH * EPD_3IN7_HEIGHT / 8;

    esp_err_t err = ESP_OK;
    err |= EPD_3IN7_SendCommand(0x4E);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendCommand(0x4F);
    err |= EPD_3IN7_SendData(0x00);
    err |= EPD_3IN7_SendData(0x00);

    err |= EPD_3IN7_SendCommand(0x24);
    ERROR_CHECKRETURN(err);

    err |= EPD_3IN7_SendDataStream(Image, IMAGE_COUNTER);
    ERROR_CHECKRETURN(err);

    err |= EPD_3IN7_Load_LUT(refreshMode);
    err |= EPD_3IN7_SendCommand(0x20);
    return err;
}
esp_err_t EPD_3IN7_1Gray_WaitBusy()
{
    return EPD_3IN7_ReadBusy_HIGH();
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_3IN7_Sleep(void)
{
    EPD_3IN7_SendCommand(0X50);
    EPD_3IN7_SendData(0xf7);
    EPD_3IN7_SendCommand(0X02); // power off
    EPD_3IN7_SendCommand(0X07); // deep sleep
    EPD_3IN7_SendData(0xA5);
}