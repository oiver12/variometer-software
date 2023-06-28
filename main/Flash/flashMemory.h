#ifndef flashMemory_h
#define flashMemory_h

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

esp_err_t flashMemoryInit(void);

esp_err_t flashMemoryWriteInt32(const char *name, int32_t value);
esp_err_t flashMemoryReadInt32(const char *name, int32_t *value);

#endif