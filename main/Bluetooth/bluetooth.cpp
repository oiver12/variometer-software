#include "bluetooth.h"
#include "nkolban/BLEDevice.h"
#include "nkolban/BLEServer.h"
#include "nkolban/BLEUtils.h"
#include "nkolban/BLE2902.h"
#include <esp_log.h>
#include <string>
extern "C"
{
#include "DeviceContext.h"
}

static char LOG_TAG[] = "Bluetooth";

class CallbackHandlerBatteryService : public BLECharacteristicCallbacks
{
    void onRead(BLECharacteristic *pCharacteristic)
    {
        float batteryLevel = 100;
        device_state_get_member(&batteryLevel, device_state_batteryPercentage, false);
        uint8_t batteryLevelPercentage = (int)batteryLevel;
        pCharacteristic->setValue(&batteryLevelPercentage, 1);
        ESP_LOGD(LOG_TAG, "Battery Characteristic was read");
    }
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        ESP_LOGE(LOG_TAG, "Battery Characteristic written no allowed");
    }
};

class CallbackHandlerSettingsService : public BLECharacteristicCallbacks
{
    void onRead(BLECharacteristic *pCharacteristic)
    {
        ESP_LOGD(LOG_TAG, "Service Characteristic was read");
    }
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        ESP_LOGD(LOG_TAG, "Service Characteristic was written");
    }
};

#ifdef __cplusplus
extern "C"
{
#endif

    // https://github.com/nkolban/esp32-snippets/blob/master/Documentation/BLE%20C%2B%2B%20Guide.pdf

    int startBluetooth()
    {
        ESP_LOGD(LOG_TAG, "Starting BLE Server");

        // contains (among other things) two distinct bit fields, in which the client can write, wether he wants to recieve notifications and/or indications
        BLE2902 *p2902Descriptor = new BLE2902();
        p2902Descriptor->setNotifications(false);

        BLEDevice::init("Variometer");
        BLEServer *pServer = BLEDevice::createServer();

        // Create Battery Service
        BLEService *pServiceBattery = pServer->createService("180F");
        // Battery Characteristic
        BLECharacteristic *pCharacteristicBattery = pServiceBattery->createCharacteristic(
            BLEUUID("2A19"),
            BLECharacteristic::PROPERTY_BROADCAST | BLECharacteristic::PROPERTY_READ);
        pCharacteristicBattery->setCallbacks(new CallbackHandlerBatteryService());
        uint8_t batteryLevel = 100;
        pCharacteristicBattery->setValue(&batteryLevel, 1);
        pCharacteristicBattery->addDescriptor(p2902Descriptor);
        pServiceBattery->start();

        // Create Settings Service
        BLEService *pServiceSettings = pServer->createService("4245c617-3968-4fee-8ea4-2eec17ec2591");
        // Sound Settings Characteristic
        BLECharacteristic *pCharacteristicSoundSettings = pServiceSettings->createCharacteristic(
            BLEUUID("1000"),
            BLECharacteristic::PROPERTY_BROADCAST | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
        pCharacteristicSoundSettings->setCallbacks(new CallbackHandlerSettingsService());
        pCharacteristicSoundSettings->addDescriptor(p2902Descriptor);
        pServiceSettings->start();

        BLEAdvertising *pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(BLEUUID(pServiceBattery->getUUID()));
        pAdvertising->addServiceUUID(BLEUUID(pServiceSettings->getUUID()));
        pAdvertising->start();

        ESP_LOGD(LOG_TAG, "Advertising started!");
        return 0;
    }

#ifdef __cplusplus
}
#endif
