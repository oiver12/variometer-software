#include "Display_Vario.h"
#include "DeviceContext.h"
#include "ScreenManager.h"
#include "E_Ink/EPD_3in7.h"
#include "E_Ink/GUI_Paint.h"
#include "RenderSystem.h"
#include "Screen/assets/assets.h"
#include "ErrorHandling.h"

#include <esp_log.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <inttypes.h>

#define DISP_BUF_SIZE 16800
#define TAG "DISPLAY_VARIO"

uint8_t *ScreenBuffer;
bool screenRefreshing = true;
bool Screen_FullRefresh = false;

esp_err_t Display_powerOff_Screen()
{
    screenRefreshing = false;
    Paint_DrawBitMap_Paste(logo_img_buffer, 0, 0, 470, 274, 0);
    esp_err_t err = EPD_3IN7_1Gray_Display(ScreenBuffer, FULL_G_T_BW);
    EPD_3IN7_Sleep();
    DEV_Module_Exit();
    return err;
}

esp_err_t init_Display()
{
    spi_bus_config_t spi2cfg = {
        .miso_io_num = -1,
        .mosi_io_num = EPD_MOSI_PIN,
        .sclk_io_num = EPD_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092};
    esp_err_t err = ESP_OK;
    ERROR_RETRYTIMES(spi_bus_initialize(SPI2_HOST, &spi2cfg, SPI_DMA_CH_AUTO), VARIO_SPI_RETRYS_INIT, err);
    if (err != ESP_OK)
    {
        error_handle(err, ERR_SCREEN, "Can't initialize SPI BUS");
        return err;
    }
    ESP_LOGI(TAG, "SPI initialized");

    ERROR_RETRYTIMES(DEV_Module_Init(), VARIO_SPI_RETRYS_INIT, err);
    if (err != ESP_OK)
    {
        error_handle(err, ERR_SCREEN, "Can't add Screen to SPI BUS");
    }

    ERROR_RETRYTIMES(EPD_3IN7_1Gray_Init(), VARIO_SPI_RETRYS_INIT, err); // init 1 Gray mode
    if (err != ESP_OK)
    {
        error_handle(err, ERR_SCREEN, "Can't initialize Screen");
        return err;
    }

    ERROR_RETRYTIMES(EPD_3IN7_1Gray_Clear(), VARIO_SPI_RETRYS, err);
    if (err != ESP_OK)
    {
        error_handle(err, ERR_SCREEN, "Can't clear Screen");
        return err;
    }

    ESP_LOGI(TAG, "Screen initialized");
    if ((ScreenBuffer = (uint8_t *)heap_caps_malloc(DISP_BUF_SIZE, MALLOC_CAP_DMA)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        return ESP_ERR_NO_MEM;
    }

    Paint_NewImage(ScreenBuffer, EPD_3IN7_WIDTH, EPD_3IN7_HEIGHT, 270, WHITE);

    Paint_SelectImage(ScreenBuffer);
    Paint_SetScale(2);
    Paint_Clear(WHITE);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return startRendering(ScreenBuffer);
}