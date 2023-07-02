#ifndef LOGGER_VARIO_H
#define LOGGER_VARIO_H

#include "SDCard.h"
#include "../main.h"
#include "nmea_parser.h"

#include <esp_err.h>

void initLogging(nmea_parser_handle_t nmea_parser_handle);
esp_err_t startLogging();
void endLogging();

esp_err_t logVarioData(FlightLogger_t state);

#endif