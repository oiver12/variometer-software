#include "ErrorHandling.h"
#include <esp_log.h>

static const char *TAG = "ERROR HANDLER";

void error_handle(esp_err_t err, error_location location, char *message)
{
    ESP_LOGE(TAG, "%s, error: %s, place: %i", message, esp_err_to_name(err), (int)location);
}