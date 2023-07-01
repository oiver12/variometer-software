#include "flashMemory.h"
#include <nvs_flash.h>
#include <nvs.h>

nvs_handle_t nvsHandle;

esp_err_t flashMemoryInit(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        err = nvs_flash_erase();
        if (err != ESP_OK)
            return err;
        err = nvs_flash_init();
        if (err != ESP_OK)
            return err;
    }
    return nvs_open("storage", NVS_READWRITE, &nvsHandle);
}

esp_err_t flashMemoryWriteInt32(const char *name, int32_t value)
{
    if (nvsHandle == 0)
        return ESP_ERR_INVALID_STATE;
    esp_err_t err = nvs_set_i32(nvsHandle, name, value);
    if (err != ESP_OK)
        return err;
    return nvs_commit(nvsHandle);
}

esp_err_t flashMemoryReadInt32(const char *name, int32_t *value)
{
    if (nvsHandle == 0)
        return ESP_ERR_INVALID_STATE;
    return nvs_get_i32(nvsHandle, name, value);
}