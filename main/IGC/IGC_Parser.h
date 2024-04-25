#ifndef __IGC_PARSER_H
#define __IGC_PARSER_H

#include "Util.h"
#include "nmea_parser.h"
#include <stdio.h>
#include "DeviceContext.h"

#define MAX_STRING_SIZE 16

#include <stdbool.h>

typedef enum
{
    IGC_FXA,
    IGC_VXA,
    IGC_GSP,
    IGC_CCO,
    IGC_HDT,
    IGC_UNKNOWN
} IFieldTypeIGC;

// translated from go in c from: https://github.com/ezgliding/goigc
typedef struct HeaderIGC_t
{
    // TODO: add all Headers
    char manufacture[MAX_STRING_SIZE];
    char model[MAX_STRING_SIZE];
} HeaderIGC_t;

typedef struct TimeIGC_t
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} TimeIGC_t;

typedef int64_t IFieldPointIGC_t;

typedef struct TrackPointIGC_t
{
    IFieldPointIGC_t *IFields;
    TimeIGC_t time;
    bool FixValidity;
    int pressureAlt;
    int gnssAlt;
    float lat;
    float lon;
} TrackPointIGC_t;

typedef struct IFieldIGC_t
{
    uint8_t posStart;
    uint8_t posEnd;
    IFieldTypeIGC type;
} IFieldIGC_t;

typedef struct Track_t
{
    HeaderIGC_t header;
    flight_stats_t flightStats;
    TrackPointIGC_t *trackPoints;
    size_t sizeTrackPoints;
    size_t totalPoints;
    IFieldIGC_t *IFields;
    size_t sizeIFields;
    bool pointStackInit;
} Track_t;

/*
 * Wenn pointStackInit = true müssen die Punkte auf dem Stack von der Callerfunction allociert werden sein. Sonst werden sie hier
 * mit malloc allociert.
 * Wenn parseIFields = true, werden iFields vom Track mit malloc allociert und es wird für jeden Punkt im track die IFieldPointIGC_t allociert.
 * Mit parseIFields = false und pointStackInit = true wird also in dieser Funktion nicht allociert (in diesem Fall kann FreeTrackFile aufgerufen werden
 * es wird jedoch nicht passieren). Im anderen Fal muss FreeTrackFile aufgerufen werden
 */
int ParseIGCFile(Track_t *track, size_t maxSizeTrackPoints, FILE *igcFile, bool pointStackInit, bool parseIFields, char *fileName);
int ParseIGCWithCached(Track_t *track, size_t maxSizeTrackPoints, FILE *igcFile, FILE *cachedFile, bool pointStackInit, bool parseIFields, char *fileName);
// TODO: mit parseIFields = true funktioniert es nicht!!!!!!
int FreeTrackFile(Track_t *track);
int SetPointCountFromFile(Track_t *track, FILE *igcFile, size_t maxSizeTrackPoints);
void Get_duration_diff(TimeIGC_t start_time, TimeIGC_t end_time, uint8_t *hours, uint8_t *minutes);

#endif