#include "IGCLogger.h"
#include <string.h>
#include <esp_log.h>
#include <math.h>
#include "DeviceContext.h"
#include <esp_timer.h>
#include "ErrorHandling.h"

#define IGC_HEADER_DATE (2)
#define IGC_HEADER_PILOT (4)
#define IGC_HEADER_CLASS (6)
#define IGC_HEADER_GLIDER (8)
#define IGC_HEADER_GPSDATUM (10)

static const char *TAG = "IGCLogger";

static const char *directory = "/IGCFlights";
static const char *sdcard_Mount_Point = "/sdcard";

static const char *nameManufacture = "MIN";
static const char *serialNumber = "XXX";
static const char *tailOfFileName = "01.igc";

igc_logger_t logger;

// HBaumgartner Variometer
// HFDTExxxx
// HFPLTPILOT:xxxxxx
// HFCCLCOMPETITION CLASS:FAI-3 (PG)
// HFGTYGLIDERTYPE:xxxxx
// HFDTM100GPSDATUM:WGS-84
const char *igcHeader[] =
	{
		"HBaumgartner Variometer",
		"\r\nHFDTE", // ex) DDMMYY
		NULL,
		"\r\nHFPLTPILOT:", // ex) akkdong
		NULL,
		"\r\nHFCCLCOMPETITION CLASS:", // ex) FAI-3 (PG)
		NULL,
		"\r\nHFGTYGLIDERTYPE:", // ex) NIVIUK IcePeak6  {MANUFACTURE} {MODEL}
		NULL,
		"\r\nHFDTM100GPSDATUM:WGS-84",
		NULL,
		"\r\n"};

static const char *makeFileName(char *buf, size_t buflength)
{
	// name format : YYYY-MM-DD-NRC-XXX-nn.igc
	// ...
	int pos = 0;
	int i;
	const char *ptr;

	// year
	snprintf(buf, buflength, "%04d-%02d-%02d-", logger.gps_data.date.year, logger.gps_data.date.month, logger.gps_data.date.day);
	pos += 11;

	// manufacture code
	for (i = 0, ptr = nameManufacture; i < 3; i++)
		buf[pos++] = (*ptr) ? *ptr++ : 'X';
	buf[pos++] = '-';

	// FR serial number
	ptr = serialNumber;
	for (i = 0; i < 3; i++)
		buf[pos++] = ptr[i];
	buf[pos++] = '-';

	// flight number of the day & file extension
	for (ptr = tailOfFileName; *ptr; ptr++)
		buf[pos++] = *ptr;
	buf[pos] = '\0'; // null-terminate

	// char pathBuf[64];

	for (i = 2; i < 101; i++) // valid number : 01 ~ 99
	{
		// strcpy(pathBuf, directory);
		// strcat(pathBuf, "/");
		// strcat(pathBuf, buf);

		if (!fileExists_sdcard(directory, buf))
			return buf;

		buf[19] = (i / 10) + '0';
		buf[20] = (i % 10) + '0';
	}
	return NULL;
}

static void IGCLogger_gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	// TODO: GNZDA kann ignoriert werden
	gps_t *gps = NULL;
	switch (event_id)
	{
	case GPS_UPDATE:
		gps = (gps_t *)event_data;
		logger.gps_data.altitude = gps->altitude;
		logger.gps_data.date = gps->date;
		logger.gps_data.latitude = gps->latitude;
		logger.gps_data.longitude = gps->longitude;
		logger.gps_data.time = gps->tim;
		logger.gps_data.fix3D = gps->fix_mode == GPS_MODE_3D;
		// ESP_LOGI(TAG, "GPS Update: %lli ms", (int64_t)((esp_timer_get_time() - logger.gpsLastTimeUpdate)/1000));
		if (logger.gps_data.fix3D)
			logger.gpsLastTimeUpdate = esp_timer_get_time();
		break;
	default:
		break;
	}
}

void updatePressureAltitude_IGCLogger(float height)
{
	logger.pressureAltitude = height;
}

static void logICGSentence(void *arg)
{
	static int count = 0;
	fprintf(logger.file, "B");
	fprintf(logger.file, "%02d%02d%02d", logger.gps_data.time.hour, logger.gps_data.time.minute, logger.gps_data.time.second); // log Time HHMMSS
	int degree = floor(logger.gps_data.latitude);																			   // DD
	int minutes = (logger.gps_data.latitude - degree) * 60 * 1000;															   // MMmmm
	fprintf(logger.file, "%02d%05d", abs(degree), abs(minutes));
	fprintf(logger.file, logger.gps_data.latitude > 0 ? "N" : "S");
	degree = floor(logger.gps_data.longitude);					// DDD
	minutes = (logger.gps_data.longitude - degree) * 60 * 1000; // MMmmm
	fprintf(logger.file, "%03d%05d", abs(degree), abs(minutes));
	fprintf(logger.file, logger.gps_data.longitude > 0 ? "E" : "W");
	fprintf(logger.file, logger.gps_data.fix3D ? "A" : "V");
	fprintf(logger.file, "%05d", (int)logger.pressureAltitude);	 // PPPPP
	fprintf(logger.file, "%05d", (int)logger.gps_data.altitude); // GGGGG
	fprintf(logger.file, "\r\n");
	if (count % 10 == 0)
	{
		fclose(logger.file);
		logger.file = openFile_sdcard(directory, logger.nameBuffer, "a");
	}
	count++;
}

static void igcLogget_Task(void *params)
{
	while (logger.gpsLastTimeUpdate == 0)
	{
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	ESP_LOGI(TAG, "IGC Logging started");
	char buffer[26 + strlen(directory)];
	if (init_sdcard(sdcard_Mount_Point) != ESP_OK)
	{
		vTaskDelete(NULL);
		return;
	}
	const char *FileName = makeFileName(buffer, 26 + strlen(directory));
	ESP_LOGI(TAG, "File Name: %s", FileName);
	FILE *f = openFile_sdcard("/IGCFlights", FileName, "w");
	strcpy(logger.nameBuffer, FileName);
	if (!f)
	{
		printf("Cannot open IGC File!\n");
		vTaskDelete(NULL);
		return;
	}

	sd_card_state_t sdstate;
	device_state_get_member(&sdstate, device_state_statusSDCard, false);
	sdstate.status |= SD_CARD_STATE_IGC_LOGGING;
	device_state_set_member(&sdstate, device_state_statusSDCard);

	logger.file = f;
	if (f)
	{
		for (int i = 0; i < sizeof(igcHeader) / sizeof(igcHeader[0]); i++)
		{
			switch (i)
			{
			case IGC_HEADER_DATE:
				fprintf(f, "%02d%02d%02d", logger.gps_data.date.day, logger.gps_data.date.month, logger.gps_data.date.year);
				break;
			case IGC_HEADER_PILOT:
				break;
			case IGC_HEADER_CLASS:
				break;
			case IGC_HEADER_GLIDER:
				break;
			case IGC_HEADER_GPSDATUM:
				// leave it empty!!
				break;
			default:
				fprintf(f, igcHeader[i]);
				break;
			}
		}
		/* Start Timer, to log IGC every Second */
		const esp_timer_create_args_t periodic_timer_args = {
			.callback = &logICGSentence,
			.name = "periodicIGC"};
		esp_timer_handle_t periodic_timer;
		esp_err_t err = esp_timer_create(&periodic_timer_args, &periodic_timer);
		if (err != ESP_OK)
		{
			error_handle(err, ERR_SCREEN, "Can't initizialize timer for logging");
		}
		else
		{
			err = esp_timer_start_periodic(periodic_timer, 1000000);
			if (err != ESP_OK)
				error_handle(err, ERR_SCREEN, "Can't start periodic timer");
		}
	}
	vTaskDelete(NULL);
}

void init_IGCLogger(nmea_parser_handle_t nmea_parser_handle)
{
	nmea_parser_add_handler(nmea_parser_handle, IGCLogger_gps_event_handler, NULL, GPS_UPDATE);
	logger.nmea_parser_handle = nmea_parser_handle;
}

void start_IGCLogger()
{
	xTaskCreate(igcLogget_Task, "igcLogget_Task", 1024 * 3, NULL, 5, NULL);
}

esp_err_t stop_IGCLogger()
{
	fclose(logger.file);
	logger.file = NULL;
	nmea_parser_remove_handler(logger.nmea_parser_handle, IGCLogger_gps_event_handler, GPS_UPDATE);
	return ESP_OK;
}
