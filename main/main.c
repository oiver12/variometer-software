#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "nmea_parser.h"
#include "SDCard.h"
#include "main.h"
#include "BatteryManager.h"
#include "SoundManager.h"
#include "IGC/IGCLogger.h"
#include "TestTools/FlightLogger.h"
#include "DeviceContext.h"
#include "Screen/Display_Vario.h"
#include "ButtonInput.h"
#include "AGL.h"
#include "Flash/flashMemory.h"
#include "AirspaceParcer/airspaceParcer.h"
#include "MainTasks.h"
#include "pinDefines.h"
// #include "Bluetooth/bluetooth.h"
#include "ErrorHandling.h"
#include "Sensor/I2C_Manager.h"

#include <driver/gpio.h>

/* GPS */
#define TIME_ZONE (+1)   // Schweiz
#define YEAR_BASE (2000) // date in GPS starts from 2000

static const char *TAG = "main";

// void sendAttitude(void *params)
// {
//     float rotation[4];
//     // float roll = 0.0f, pitch = 0.0f, yaw = 0.0f;
//     while (1)
//     {
//         // Quaternion_get_RPY(rotation[0], rotation[1], rotation[2], rotation[3], &roll, &pitch, &tmep);
//         // fwrite(&roll, sizeof(float), 1, stdout);
//         // fwrite(&pitch, sizeof(float), 1, stdout);
//         // fwrite(&yaw, sizeof(float), 1, stdout);
//         fwrite(&rotation, sizeof(float), 4, stdout);
//         char ending = 0xff;
//         fwrite(&ending, sizeof(char), 1, stdout);
//         printf("\n");
//         vTaskDelay(200 / portTICK_PERIOD_MS);
//     }
//     vTaskDelete(NULL);
// }

void app_main(void)
{
    esp_err_t err;
    gpio_config_t enableGPIOOutput = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << POWSW_ENABLE,
        .pull_down_en = 0,
        .pull_up_en = 0};

    err = gpio_config(&enableGPIOOutput);
    err |= gpio_set_level(POWSW_ENABLE, 1);
    if (err != ESP_OK)
    {
        error_handle(err, ERR_MAIN, "Can't initialize Power EN GPIO");
        return;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    err = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1); // gpio interrupts are generally not that important
    if (err != ESP_OK)
    {
        error_handle(err, ERR_MAIN, "Can't initialize ISR Service");
    }

    // // /* Configure GPS Reader Library */
    nmea_parser_config_t gps_config = NMEA_PARSER_CONFIG_DEFAULT();
    gps_config.uart.rx_pin = USART2_RX;
    nmea_parser_handle_t gps_nmea_hdl = nmea_parser_init(&gps_config, 5 * 1024);
    ESP_LOGI(TAG, "GPS started");

    // sdcard_t *sdcard = calloc(sizeof(sdcard_t), 1);
    init_DeviceContext();

    err = init_sdcard("/sdcard");
    if (err == ESP_FAIL)
    {
        error_handle(err, ERR_SDCARD, "Can't mount File System");
    }
    else if (err != ESP_OK)
    {
        error_handle(err, ERR_SDCARD, "Can't initialite SD Card");
    }
    init_IGCLogger(gps_nmea_hdl);
    initLogging(gps_nmea_hdl);

    // gps handle wird beim DeviceContext erst nach SD Karte gestartet, da wenn direkt abfulg ist, soll
    // das loggen auf SDKarte direkt starten. Wenn die Sd Karte gestartet wird, muss jedoch device kontext
    // schon gestartet sein
    startGps_DeviceContext(gps_nmea_hdl);

    err = init_SoundManager();
    ERROR_HANDLEBYERROR(err, ERR_SOUNDMANAGER, "Can't start Sound Manager");

    err = init_Display();
    ERROR_HANDLEBYERROR(err, ERR_SCREEN, "Can't start display");

    err = flashMemoryInit();
    ERROR_HANDLEBYERROR(err, ERR_FLASH, "Can't start Flash");

    err = init_Battery_Manager(4);
    ERROR_HANDLEBYERROR(err, ERR_BATTERY, "Can't start Battery Manager");

    err = ButtonInput_Init();
    ERROR_HANDLEBYERROR(err, ERR_BUTTONS, "Can't start Button Manager");

    err = startLogging();
    ERROR_HANDLEBYERROR(err, ERR_LOGGING, "Can't start Testlogging");
    // AGL_startAGL(sdcard, gps_nmea_hdl);

    StartAirspaceParse();

    // startBluetooth();

    vTaskDelay(100 / portTICK_PERIOD_MS);
#ifdef VERSION2n2
    i2c_manager_init(I2C_NUM_0, I2C_SDA, I2C_SCL, I2C_MASTER_FREQ_HZ);
#elif defined VERSION2n2n1
    i2c_manager_init(I2C_NUM_0, I2C1_SDA, I2C1_SCL, I2C_MASTER_FREQ_HZ);
    i2c_manager_init(I2C_NUM_1, I2C2_SDA, I2C2_SCL, I2C_MASTER_FREQ_HZ);
#endif
    startMainTasks();
    // xTaskCreate(sendAttitude, "Send Attitude", 4 * 1024, NULL, 6, NULL);
    // perfmon_start();
    while (1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    return;
}
