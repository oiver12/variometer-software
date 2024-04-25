#include "IGC_Parser.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include "Util.h"
#include <math.h>
#include "FIFO.h"
#include "../SDCard.h"

#define MAX_LINELENGTH 256
#define POINTS_MEAN_VARIO 5
#define VARIO_MAXSINKCLIMB 10

fifo_t vario_fifo;

#define ERROR_CHECK(val) ({if(val != 0) return 1; })

static int str2int(int *out, const char *s)
{
    char *end;
    if (s[0] == '\0' || isspace((unsigned char)s[0]))
        return 1;
    errno = 0;
    long l = strtol(s, &end, 10);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    // Owerflow
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return 1;
    // Underflow
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return 1;
    if (*end != '\0')
        return 1;
    *out = l;
    return 0;
}

static void sliceString(const char *oldString, char *newString, int start, int size)
{
    strncpy(newString, &oldString[start], size);
    newString[size] = '\0';
}

static int sliceStringToInt(int *out, const char *stringIn, int start, int size)
{
    char buffer[size + 1];
    sliceString(stringIn, buffer, start, size);
    return str2int(out, buffer);
}

static int floatFROMDMD(const char *dmd, float *valueRad)
{
    char tempBuffer[9];
    size_t sizeString = strlen(dmd);
    if (sizeString != 8 && sizeString != 9)
    {
        printf("Can't parse dmd\n");
        return 1;
    }
    int degress, minutes, dminutes = 0;
    if (dmd[0] == 'S' || dmd[0] == 'N')
    {
        sliceString(dmd, tempBuffer, 1, 2);
        ERROR_CHECK(str2int(&degress, tempBuffer));
        sliceString(dmd, tempBuffer, 3, 2);
        ERROR_CHECK(str2int(&minutes, tempBuffer));
        sliceString(dmd, tempBuffer, 5, 2);
        ERROR_CHECK(str2int(&dminutes, tempBuffer));
    }
    else if (dmd[sizeString - 1] == 'S' || dmd[sizeString - 1] == 'N')
    {
        sliceString(dmd, tempBuffer, 0, 2);
        ERROR_CHECK(str2int(&degress, tempBuffer));
        sliceString(dmd, tempBuffer, 2, 2);
        ERROR_CHECK(str2int(&minutes, tempBuffer));
        sliceString(dmd, tempBuffer, 4, 3);
        ERROR_CHECK(str2int(&dminutes, tempBuffer));
    }
    else if (dmd[0] == 'W' || dmd[0] == 'E')
    {
        sliceString(dmd, tempBuffer, 1, 3);
        ERROR_CHECK(str2int(&degress, tempBuffer));
        sliceString(dmd, tempBuffer, 4, 2);
        ERROR_CHECK(str2int(&minutes, tempBuffer));
        sliceString(dmd, tempBuffer, 6, 3);
        ERROR_CHECK(str2int(&dminutes, tempBuffer));
    }
    else if (dmd[sizeString - 1] == 'W' || dmd[sizeString - 1] == 'E')
    {
        sliceString(dmd, tempBuffer, 0, 3);
        ERROR_CHECK(str2int(&degress, tempBuffer));
        sliceString(dmd, tempBuffer, 3, 2);
        ERROR_CHECK(str2int(&minutes, tempBuffer));
        sliceString(dmd, tempBuffer, 5, 3);
        ERROR_CHECK(str2int(&dminutes, tempBuffer));
    }
    float r = (float)degress + (((float)minutes + ((float)dminutes / 1000.0f)) / 60.0f);
    if (dmd[0] == 'S' || dmd[0] == 'W' || dmd[sizeString - 1] == 'S' || dmd[sizeString - 1] == 'W')
        r *= -1;
    *valueRad = r;
    return 0;
}

static int parseTime(const char *timeString, TimeIGC_t *date)
{
    if (strlen(timeString) != 6)
    {
        printf("Can't parse Time\n");
    }
    int hour, minute, second;
    char tempBuffer[4];
    sliceString(timeString, tempBuffer, 0, 2);
    ERROR_CHECK(str2int(&hour, tempBuffer));
    sliceString(timeString, tempBuffer, 2, 2);
    ERROR_CHECK(str2int(&minute, tempBuffer));
    sliceString(timeString, tempBuffer, 4, 2);
    ERROR_CHECK(str2int(&second, tempBuffer));
    date->hour = hour;
    date->minute = minute;
    date->second = second;
    return 0;
}

static int parseB(const char *line, Track_t *track, TrackPointIGC_t *point, IFieldPointIGC_t *IFields)
{
    if (point == NULL)
    {
        printf("Not enough Heap Space\n");
        return 1;
    }
    if (strlen(line) < 35)
    {
        printf("Line not enough long\n");
        return 1;
    }

    char tempBuffer[MAX_LINELENGTH];
    // Parse Latitude
    sliceString(line, tempBuffer, 7, 8);
    ERROR_CHECK(floatFROMDMD(tempBuffer, &point->lat));
    // Parse Longitude
    sliceString(line, tempBuffer, 15, 9);
    ERROR_CHECK(floatFROMDMD(tempBuffer, &point->lon));
    // Parse Time
    sliceString(line, tempBuffer, 1, 6);
    ERROR_CHECK(parseTime(tempBuffer, &point->time));
    if (line[24] == 'A')
        point->FixValidity = true;
    else if (line[24] == 'V')
        point->FixValidity = false;
    else
    {
        printf("Invalid fix validity: %s", line);
    }
    // Parse PressureAlt
    ERROR_CHECK(sliceStringToInt(&point->pressureAlt, line, 25, 5));
    // Parse GNSSAlt
    ERROR_CHECK(sliceStringToInt(&point->gnssAlt, line, 30, 5));
    if (track->IFields == NULL)
        return 0;
    for (size_t i = 0; i < track->sizeIFields; i++)
    {
        size_t sizeNumber = track->IFields[i].posEnd - track->IFields[i].posStart + 1;
        int field;
        int posStart = track->IFields[i].posStart - 1;
        ERROR_CHECK(sliceStringToInt(&field, line, posStart, sizeNumber));
        IFields[i] = field;
    }
    return 0;
}

static int getIFieldType(const char *Field, IFieldTypeIGC *type)
{
    if (strlen(Field) != 3)
    {
        printf("Couldn't parse IField Type\n");
        return 1;
    }
    if (strcmp(Field, "FXA") == 0)
        *type = IGC_FXA;
    else if (strcmp(Field, "VXA") == 0)
        *type = IGC_VXA;
    else if (strcmp(Field, "GSP") == 0)
        *type = IGC_GSP;
    else if (strcmp(Field, "CCO") == 0)
        *type = IGC_CCO;
    else if (strcmp(Field, "HDT") == 0)
        *type = IGC_HDT;
    else
    {
        *type = IGC_UNKNOWN;
    }
    return 0;
}

static int parseI(const char *line, Track_t *track, bool mallocIFields)
{
    if (strlen(line) < 3)
    {
        return 1;
    }
    if (!mallocIFields)
    {
        track->IFields = NULL;
        track->sizeIFields = 0;
        return 0;
    }
    int n;
    ERROR_CHECK(sliceStringToInt(&n, line, 1, 2));
    if (strlen(line) != n * 7 + 1 && strlen(line) != n * 7 + 5)
    {
        printf("Wrong I Field Length\n");
        return 1;
    }
    if (track->IFields == NULL)
    {
        track->IFields = malloc(sizeof(IFieldIGC_t) * n);
        if (track->IFields == NULL)
        {
            printf("Couldn't allocate memory for IFields\n");
            return 1;
        }
    }
    char buffer[4];
    track->sizeIFields = n;
    if (track->IFields != NULL)
    {
        for (size_t i = 0; i < n; i++)
        {
            int s = i * 7 + 3;
            int start, end;
            ERROR_CHECK(sliceStringToInt(&start, line, s, 2));
            ERROR_CHECK(sliceStringToInt(&end, line, s + 2, 2));
            sliceString(line, buffer, s + 4, 3);
            ERROR_CHECK(getIFieldType(buffer, &track->IFields[i].type));
            track->IFields[i].posStart = start;
            track->IFields[i].posEnd = end;
        }
    }
    return 0;
}

void updateFlightStats(TrackPointIGC_t trackPoint, TrackPointIGC_t previousTrackPoint, Track_t *track)
{
    static float sumVertVel = 0.0f;
    if (trackPoint.FixValidity)
    {
        track->flightStats.altitudeMax = fmaxf(trackPoint.gnssAlt, track->flightStats.altitudeMax);
        track->flightStats.altitudeMin = fminf(trackPoint.gnssAlt, track->flightStats.altitudeMin);
        if (previousTrackPoint.FixValidity)
        {
            track->flightStats.distFlight += GET_DISTANCEFAST(trackPoint.lat, trackPoint.lon, previousTrackPoint.lat, previousTrackPoint.lon);
        }
    }
    float vario = (float)trackPoint.pressureAlt - (float)previousTrackPoint.pressureAlt;
    if (vario > (float)VARIO_MAXSINKCLIMB || vario < -(float)VARIO_MAXSINKCLIMB)
        return;
    float temp;
    if (fifo_is_full(vario_fifo))
    {
        fifo_get(vario_fifo, &temp);
        sumVertVel -= temp;
    }
    sumVertVel += vario;
    fifo_add(vario_fifo, &vario);
    if (sumVertVel / (float)POINTS_MEAN_VARIO < -30.0f)
    {
        printf("Heree\n");
    }
    track->flightStats.maxVelX = fmaxf(sumVertVel / (float)POINTS_MEAN_VARIO, track->flightStats.maxVelX);
    track->flightStats.minVelX = fminf(sumVertVel / (float)POINTS_MEAN_VARIO, track->flightStats.minVelX);
}

int ParseIGCFile(Track_t *track, size_t maxSizeTrackPoints, FILE *igcFile, bool pointStackInit, bool parseIFields, char *fileName)
{
    printf("Start parse IGC \n");
    if (igcFile == NULL)
        return 1;
    FILE *cachedF = openFile_sdcard("/cache", fileName, "w");
    if (cachedF == NULL)
    {
        char *erroName = strerror(errno);
        printf("Cached can't be created%s!\n", erroName);
        return 1;
    }
    char line[MAX_LINELENGTH];
    track->pointStackInit = pointStackInit;
    if (track->totalPoints == 0)
    {
        // Determine total points in track, so that we can sample the points in the second run
        memset(track, 0, sizeof(Track_t));
        while (fgets(line, sizeof(line), igcFile))
        {
            if (line[0] == 'B')
                track->totalPoints++;
            if (line[0] == 'I')
                ERROR_CHECK(parseI(line, track, parseIFields));
        }
    }
    fseek(igcFile, 0, SEEK_SET);
    memset(&track->flightStats, 0, sizeof(flight_stats_t));
    if (vario_fifo == NULL)
        vario_fifo = fifo_create(POINTS_MEAN_VARIO, sizeof(float));
    track->flightStats.altitudeMin = 100000;
    track->flightStats.minVelX = 1000;
    if (maxSizeTrackPoints > track->totalPoints)
        track->sizeTrackPoints = track->totalPoints;
    else
        track->sizeTrackPoints = maxSizeTrackPoints;

    if (!pointStackInit)
        track->trackPoints = malloc(sizeof(TrackPointIGC_t) * track->sizeTrackPoints);

    if (track->trackPoints == NULL)
    {
        printf("Couldn' allocate TrackPoints or not on Stack\n");
        return 1;
    }

    int k = track->totalPoints / maxSizeTrackPoints;
    int countPoints = 0;
    int countAddedPoints = 0;
    TrackPointIGC_t tempPoint;
    TrackPointIGC_t tempPointPrev;
    tempPointPrev.FixValidity = false;
    IFieldPointIGC_t IFields[track->sizeIFields];
    while (fgets(line, sizeof(line), igcFile))
    {
        switch (line[0])
        {
        case 'B':
            if (maxSizeTrackPoints > track->sizeTrackPoints || countPoints % k == 0)
            {
                if (countAddedPoints == track->sizeTrackPoints)
                {
                    countPoints++;
                    // printf("This sould not happen. countPoints: %i, totalPoints: %i, countAddedPoints%i\n", countPoints, (int)totalPoints, (int)countAddedPoints);
                    break;
                }
                if (parseIFields)
                {
                    track->trackPoints[countAddedPoints].IFields = malloc(sizeof(IFieldPointIGC_t) * track->sizeIFields);
                    ERROR_CHECK(parseB(line, track, &track->trackPoints[countAddedPoints], track->trackPoints[countAddedPoints].IFields));
                }
                else
                {
                    track->trackPoints[countAddedPoints].IFields = NULL;
                    ERROR_CHECK(parseB(line, track, &track->trackPoints[countAddedPoints], IFields));
                }
                updateFlightStats(track->trackPoints[countAddedPoints], tempPointPrev, track);
                tempPointPrev = track->trackPoints[countAddedPoints];
                countPoints++;
                countAddedPoints++;
                break;
            }
            ERROR_CHECK(parseB(line, track, &tempPoint, IFields));
            updateFlightStats(tempPoint, tempPointPrev, track);
            tempPointPrev = tempPoint;
            countPoints++;
            break;
        default:
            break;
        }
    }
    track->flightStats.takeOffPosition.lat = track->trackPoints[0].lat;
    fwrite(&track->trackPoints[0].lat, sizeof(float), 1, cachedF);
    track->flightStats.takeOffPosition.lon = track->trackPoints[0].lon;
    fwrite(&track->trackPoints[0].lon, sizeof(float), 1, cachedF);
    // TODO: Take the last Valid GNSS Alt
    track->flightStats.takeOffPosition.alt = track->trackPoints[0].gnssAlt;
    fwrite(&track->trackPoints[0].gnssAlt, sizeof(int), 1, cachedF);

    fwrite(&track->totalPoints, sizeof(size_t), 1, cachedF);
    fwrite(&track->flightStats.altitudeMax, sizeof(float), 1, cachedF);
    fwrite(&track->flightStats.altitudeMin, sizeof(float), 1, cachedF);
    fwrite(&track->flightStats.distFlight, sizeof(float), 1, cachedF);
    fwrite(&track->flightStats.maxVelX, sizeof(float), 1, cachedF);
    fwrite(&track->flightStats.minVelX, sizeof(float), 1, cachedF);
    fwrite(&track->sizeIFields, sizeof(size_t), 1, cachedF);
    fclose(cachedF);
    printf("End parse IGC \n");
    return 0;
}

int ParseIGCWithCached(Track_t *track, size_t maxSizeTrackPoints, FILE *igcFile, FILE *cachedFile, bool pointStackInit, bool parseIFields, char *fileName)
{
    printf("Start parse IGC \n");
    char line[MAX_LINELENGTH];
    if (!igcFile || !cachedFile)
        return 1;
    fseek(cachedFile, 0, SEEK_END);
    long fileSize = ftell(cachedFile);
    if (fileSize != (7 * sizeof(float) + 2 * sizeof(size_t) + 1 * sizeof(int)))
    {
        // The cache File is not valid
        printf("Cache File not valid!\n");
        fclose(cachedFile);
        return ParseIGCFile(track, maxSizeTrackPoints, igcFile, pointStackInit, parseIFields, fileName);
    }
    fseek(cachedFile, 0, SEEK_SET);
    Position_t FirstPosition;
    fread(&FirstPosition.lat, sizeof(float), 1, cachedFile);
    fread(&FirstPosition.lon, sizeof(float), 1, cachedFile);
    int height;
    fread(&height, sizeof(int), 1, cachedFile);
    track->flightStats.takeOffPosition.lat = FirstPosition.lat;
    track->flightStats.takeOffPosition.lon = FirstPosition.lon;
    track->flightStats.takeOffPosition.alt = height;
    fread(&track->totalPoints, sizeof(size_t), 1, cachedFile);
    fread(&track->flightStats.altitudeMax, sizeof(float), 1, cachedFile);
    fread(&track->flightStats.altitudeMin, sizeof(float), 1, cachedFile);
    fread(&track->flightStats.distFlight, sizeof(float), 1, cachedFile);
    fread(&track->flightStats.maxVelX, sizeof(float), 1, cachedFile);
    fread(&track->flightStats.minVelX, sizeof(float), 1, cachedFile);
    fread(&track->sizeIFields, sizeof(size_t), 1, cachedFile);
    track->pointStackInit = pointStackInit;
    fseek(igcFile, 0, SEEK_SET);
    if (maxSizeTrackPoints > track->totalPoints)
        track->sizeTrackPoints = track->totalPoints;
    else
        track->sizeTrackPoints = maxSizeTrackPoints;

    if (!pointStackInit)
        track->trackPoints = malloc(sizeof(TrackPointIGC_t) * track->sizeTrackPoints);

    if (track->trackPoints == NULL)
    {
        printf("Couldn' allocate TrackPoints or not on Stack\n");
        return 1;
    }

    int k = track->totalPoints / maxSizeTrackPoints;
    int countPoints = 0;
    int countAddedPoints = 0;
    IFieldPointIGC_t IFields[track->sizeIFields];
    while (fgets(line, sizeof(line), igcFile))
    {
        switch (line[0])
        {
        case 'B':
            if (maxSizeTrackPoints > track->sizeTrackPoints || countPoints % k == 0)
            {
                if (countAddedPoints == track->sizeTrackPoints)
                {
                    countPoints++;
                    break;
                }
                if (parseIFields)
                {
                    track->trackPoints[countAddedPoints].IFields = malloc(sizeof(IFieldPointIGC_t) * track->sizeIFields);
                    ERROR_CHECK(parseB(line, track, &track->trackPoints[countAddedPoints], track->trackPoints[countAddedPoints].IFields));
                }
                else
                {
                    track->trackPoints[countAddedPoints].IFields = NULL;
                    ERROR_CHECK(parseB(line, track, &track->trackPoints[countAddedPoints], IFields));
                }
                countPoints++;
                countAddedPoints++;
                break;
            }
            countPoints++;
            break;
        default:
            break;
        }
    }
    printf("End parse IGC \n");
    return 0;
}

int FreeTrackFile(Track_t *track)
{
    if (track->IFields != NULL)
    {
        free(track->IFields);
        for (size_t i = 0; i < track->sizeIFields; i++)
        {
            free(&track->trackPoints[i].IFields);
        }
    }
    if (!track->pointStackInit)
        free(track->trackPoints);
    return 0;
}

int SetPointCountFromFile(Track_t *track, FILE *igcFile, size_t maxSizeTrackPoints)
{
    char line[MAX_LINELENGTH];
    if (igcFile == NULL)
        return 1;
    // Determine total points in track, so that we can sample the points in the second run
    track->totalPoints = 0;
    memset(track, 0, sizeof(Track_t));
    while (fgets(line, sizeof(line), igcFile))
    {
        if (line[0] == 'B')
            track->totalPoints++;
    }
    if (maxSizeTrackPoints > track->totalPoints)
        track->sizeTrackPoints = track->totalPoints;
    else
        track->sizeTrackPoints = maxSizeTrackPoints;
    return 0;
}

void Get_duration_diff(TimeIGC_t start_time, TimeIGC_t end_time, uint8_t *hours, uint8_t *minutes)
{
    int total_start_minutes = start_time.hour * 60 + start_time.minute;
    int total_end_minutes = end_time.hour * 60 + end_time.minute;
    int total_diff_minutes = total_end_minutes - total_start_minutes;

    *hours = total_diff_minutes / 60;
    *minutes = total_diff_minutes % 60;
}

int Get_duration_diff_seconds(TimeIGC_t start_time, TimeIGC_t end_time)
{
    int seconds1 = start_time.hour * 3600 + start_time.minute * 60 + start_time.second;
    int seconds2 = end_time.hour * 3600 + end_time.minute * 60 + end_time.second;
    return seconds2 - seconds1;
}