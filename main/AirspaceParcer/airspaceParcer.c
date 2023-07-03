#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "airspaceParcer.h"
#include "file_layer.h"
#include <freertos/FreeRTOS.h>
#include "../SDCard.h"
#include <esp_timer.h>
#include "../DeviceContext.h"
#include "ErrorHandling.h"

#define BOUNDINGBOX_OPTIMIZATION

#define MAXDISTANCETOSEARCH 10000

AirspaceParcer_State_t instanceAirspace;

static float VertDistanceToAirspace(float lowerLimit, heightType lowerType, float upperLimit, heightType upperType, float gnssAlt, float stdAlt, float aglH)
{
    float lowerDistance = 0; // positive when heigher and negativ when lower
    switch (lowerType)
    {
    case HeightType_mAGL:
        lowerDistance = aglH - lowerLimit;
        break;
    case HeightType_mQNH:
        lowerDistance = gnssAlt - lowerLimit;
        break;
    case HeightType_mSTD:
        lowerDistance = stdAlt - lowerLimit;
        break;
    default:
        printf("Can't find Height Type\n");
        break;
    }
    float upperDistance = 0; // positive when heigher and negative when lower
    switch (upperType)
    {
    case HeightType_mAGL:
        upperDistance = aglH - upperLimit;
        break;
    case HeightType_mQNH:
        upperDistance = gnssAlt - upperLimit;
        break;
    case HeightType_mSTD:
        upperDistance = stdAlt - upperLimit;
        break;
    default:
        printf("Can't find Height Type\n");
        break;
    }
    // we are in the airspace
    if (lowerDistance > 0 && upperDistance < 0)
        return 0;
    else if (lowerDistance < 0)
        return lowerDistance;
    else
        return upperDistance;
}

static void airspaceCalculateValuesToLine(double myLat, double myLon, float gnssAlt, float stdAlt, float aglH)
{
    float nearestLat, nearestLon = 0;
    NearestPointOnLine(instanceAirspace.nearestAirspaceInfo.nearestLineStart.lat, instanceAirspace.nearestAirspaceInfo.nearestLineStart.lon, instanceAirspace.nearestAirspaceInfo.nearestLineEnd.lat, instanceAirspace.nearestAirspaceInfo.nearestLineEnd.lon, myLat, myLon, &nearestLat, &nearestLon);
    float HorDist = GET_DISTANCEFAST(myLat, myLon, nearestLat, nearestLon);
    float VertDist = VertDistanceToAirspace(instanceAirspace.currentNearestAirspace->lowerLimit, instanceAirspace.currentNearestAirspace->heightTypeLower, instanceAirspace.currentNearestAirspace->upperLimit, instanceAirspace.currentNearestAirspace->heightTypeUpper, gnssAlt, stdAlt, aglH);
    if (instanceAirspace.nearestAirspaceInfo.distanceToAirspaceVert != VertDist || instanceAirspace.nearestAirspaceInfo.distanceToAirspaceHor != HorDist || strcmp(instanceAirspace.currentNearestAirspace->name, instanceAirspace.nearestAirspaceInfo.name) != 0)
    {
        strcpy(instanceAirspace.nearestAirspaceInfo.name, instanceAirspace.currentNearestAirspace->name);
        instanceAirspace.nearestAirspaceInfo.upperLimt = instanceAirspace.currentNearestAirspace->upperLimit;
        instanceAirspace.nearestAirspaceInfo.heightTypeUpper = instanceAirspace.currentNearestAirspace->heightTypeUpper;
        instanceAirspace.nearestAirspaceInfo.lowerLimit = instanceAirspace.currentNearestAirspace->lowerLimit;
        instanceAirspace.nearestAirspaceInfo.heightTypeLower = instanceAirspace.currentNearestAirspace->heightTypeLower;
        instanceAirspace.nearestAirspaceInfo.distanceToAirspaceHor = HorDist;
        instanceAirspace.nearestAirspaceInfo.distanceToAirspaceVert = VertDist;
        vario_state_set_member(&instanceAirspace.nearestAirspaceInfo, vario_state_airspaceInfo);
    }
    printf("Found nearest: %s\n", instanceAirspace.nearestAirspaceInfo.name);
}

static void nearestLineAirspace(double myLat, double myLon, float gnssAlt, float stdAlt, float aglH)
{
    uint32_t tempStartIndex = 0;
    uint32_t tempEndIndex = 0;
    float minDistance = INFINITY;
    if (!instanceAirspace.currentNearestAirspace)
    {
        printf("Current nearest is null\n");
        return;
    }
    for (size_t i = 0; i < instanceAirspace.currentNearestAirspace->polygonSize; i++)
    {
        size_t nextI = i + 1;
        if (nextI >= instanceAirspace.currentNearestAirspace->polygonSize)
            nextI = 0;
        float dist = DistancePointToLineSqr(instanceAirspace.currentNearestAirspace->polygons[i].latitude, instanceAirspace.currentNearestAirspace->polygons[i].longitude, instanceAirspace.currentNearestAirspace->polygons[nextI].latitude, instanceAirspace.currentNearestAirspace->polygons[nextI].longitude, myLat, myLon);
        if (dist < minDistance)
        {
            tempStartIndex = i;
            tempEndIndex = nextI;
            minDistance = dist;
        }
    }
    instanceAirspace.nearestAirspaceInfo.nearestLineStart.lat = instanceAirspace.currentNearestAirspace->polygons[tempStartIndex].latitude;
    instanceAirspace.nearestAirspaceInfo.nearestLineStart.lon = instanceAirspace.currentNearestAirspace->polygons[tempStartIndex].longitude;
    instanceAirspace.nearestAirspaceInfo.nearestLineEnd.lat = instanceAirspace.currentNearestAirspace->polygons[tempEndIndex].latitude;
    instanceAirspace.nearestAirspaceInfo.nearestLineEnd.lon = instanceAirspace.currentNearestAirspace->polygons[tempEndIndex].longitude;
    instanceAirspace.lastInNearestAirspaceUpdatePoint.lat = myLat;
    instanceAirspace.lastInNearestAirspaceUpdatePoint.lon = myLon;
}

static void searchInNearestAirspace(double myLat, double myLon, float gnssAlt, float stdAlt, float aglH)
{
    airspace *nearestAirspace = NULL;
    float nearestDistanceToAirspace = MAXDISTANCETOSEARCH * MAXDISTANCETOSEARCH;
    float secondNearestDistanceToAirspace = INFINITY;
    airspace *airspaces = instanceAirspace.nearestAirspaceList;
    if (airspaces == NULL)
        printf("Is null!!!!\n");
    while (airspaces != NULL)
    {
        float minDistance = INFINITY;
        for (size_t i = 0; i < airspaces->polygonSize; i++)
        {
            size_t nextI = i + 1;
            if (nextI >= airspaces->polygonSize)
                nextI = 0;
            float dist = DistancePointToLineSqr(airspaces->polygons[i].latitude, airspaces->polygons[i].longitude, airspaces->polygons[nextI].latitude, airspaces->polygons[nextI].longitude, myLat, myLon);
            if (dist < minDistance)
            {
                minDistance = dist;
            }
        }
        bool hasUpdated = false;
        if (minDistance < nearestDistanceToAirspace)
        {
            float VertDistance = VertDistanceToAirspace(airspaces->lowerLimit, airspaces->heightTypeLower, airspaces->upperLimit, airspaces->heightTypeUpper, gnssAlt, stdAlt, aglH);
            minDistance = minDistance + VertDistance * VertDistance; // Pythagoras and shit
            if (minDistance < nearestDistanceToAirspace)
            {
                hasUpdated = true;
                if (secondNearestDistanceToAirspace > nearestDistanceToAirspace)
                    secondNearestDistanceToAirspace = nearestDistanceToAirspace;
                nearestDistanceToAirspace = minDistance;
                nearestAirspace = airspaces;
            }
        }
        if (!hasUpdated && minDistance < secondNearestDistanceToAirspace)
        {
            secondNearestDistanceToAirspace = minDistance;
        }
        airspaces = airspaces->next;
    }
    nearestDistanceToAirspace = sqrtf(nearestDistanceToAirspace);
    secondNearestDistanceToAirspace = sqrtf(secondNearestDistanceToAirspace);
    instanceAirspace.currentNearestAirspace = nearestAirspace;
    instanceAirspace.diffFirstToSecondAirspace = MAXDISTANCETOSEARCH;
    if (secondNearestDistanceToAirspace != INFINITY)
        instanceAirspace.diffFirstToSecondAirspace = secondNearestDistanceToAirspace - nearestDistanceToAirspace;
    instanceAirspace.lastInNearAirspacesUpdatePoint.lat = myLat;
    instanceAirspace.lastInNearAirspacesUpdatePoint.lon = myLon;
}

static airspace *createAirspaceOnHeapOrInArray(size_t polygonSize)
{
    airspace *tempAirspace;
    if (instanceAirspace.currentIndexAirspaces < PREALLOCATEDAIRSPACES)
    {
        tempAirspace = &instanceAirspace.nearestAirspaceList[instanceAirspace.currentIndexAirspaces];
        tempAirspace->isAllocated = false;
        instanceAirspace.currentIndexAirspaces++;
    }
    else
    {
        tempAirspace = calloc(sizeof(airspace), 1);
        tempAirspace->isAllocated = true;
    }
    if (polygonSize > PREALLOCATEDPOLYGONS)
    {
        tempAirspace->allocatedPolygons = calloc(sizeof(polygonSide), polygonSize - PREALLOCATEDPOLYGONS);
    }
    return tempAirspace;
}

static airspace *readPolygonsFromFile(bool *hasNext, BoundingBox_LatLong_t *myBoundingBox, char *nameAirspace, FILE *cachedFile, long airspaceStartPos)
{
    char nextChar = ',';
    int polygonSize = 0;
    long posPolyStart = ftellFileLayer(&instanceAirspace.layer);
    BoundingBox_LatLong_t boundingBoxAirspace = {.maxLat = 0, .maxLong = 0, .minLat = INFINITY, .minLong = INFINITY};
    while (nextChar == ',')
    {
        polygonSize++;
        readTilCharFileLayer(&instanceAirspace.layer, '[');
        float lonitude;
        getFloatTilCharacter(&instanceAirspace.layer, ',', &lonitude);
        boundingBoxAirspace.maxLong = fmax(boundingBoxAirspace.maxLong, lonitude);
        boundingBoxAirspace.minLong = fmin(boundingBoxAirspace.minLong, lonitude);
        float latitude;
        getFloatTilCharacter(&instanceAirspace.layer, ']', &latitude);
        boundingBoxAirspace.maxLat = fmax(boundingBoxAirspace.maxLat, latitude);
        boundingBoxAirspace.minLat = fmin(boundingBoxAirspace.minLat, latitude);
        nextChar = fgetCFileLayer(&instanceAirspace.layer);
    }
    if (cachedFile != NULL)
    {
        // printf("Write: %.2f, %.2f, %.2f, %.2f %i\n", boundingBoxAirspace.maxLat, boundingBoxAirspace.maxLong, boundingBoxAirspace.minLat, boundingBoxAirspace.minLong, (int)airspaceStartPos);
        fwrite(&boundingBoxAirspace.maxLat, sizeof(float), 1, cachedFile);
        fwrite(&boundingBoxAirspace.maxLong, sizeof(float), 1, cachedFile);
        fwrite(&boundingBoxAirspace.minLat, sizeof(float), 1, cachedFile);
        fwrite(&boundingBoxAirspace.minLong, sizeof(float), 1, cachedFile);
        fwrite(&airspaceStartPos, sizeof(long), 1, cachedFile);
    }
    if (overlappBoundingBox(myBoundingBox, &boundingBoxAirspace))
    {
        *hasNext = readTilCharFileLayer(&instanceAirspace.layer, '{');
        return NULL;
    }
    airspace *tempAirspace = &instanceAirspace.nearestAirspaceList[0];
    while (tempAirspace)
    {
        // Already exists
        if (strcmp(nameAirspace, tempAirspace->name) == 0)
        {
            tempAirspace->isValid = true;
            *hasNext = readTilCharFileLayer(&instanceAirspace.layer, '{');
            return tempAirspace;
        }
        // Airspace does not yet exist append to end
        if (!tempAirspace->next)
        {
            airspace *newAirspace = createAirspaceOnHeapOrInArray(polygonSize);
            tempAirspace->next = newAirspace;
            tempAirspace = tempAirspace->next;
            tempAirspace->next = NULL;
            break;
        }
        tempAirspace = tempAirspace->next;
    }
    // No Airspace does yet exist at all
    if (!tempAirspace)
    {
        tempAirspace = createAirspaceOnHeapOrInArray(polygonSize);
        tempAirspace->next = NULL;
    }
    tempAirspace->isValid = true;
    tempAirspace->polygonSize = polygonSize;

    startFileLayer(&instanceAirspace.layer, posPolyStart, instanceAirspace.layer.f);

    int i = 0;
    nextChar = ',';
    while (nextChar == ',')
    {
        polygonSide *tempPolygon;
        if (i < PREALLOCATEDPOLYGONS)
            tempPolygon = &tempAirspace->polygons[i];
        else
            tempPolygon = &tempAirspace->allocatedPolygons[i - PREALLOCATEDPOLYGONS];
        i++;
        readTilCharFileLayer(&instanceAirspace.layer, '[');
        getFloatSize(&instanceAirspace.layer, 16, &tempPolygon->longitude);
        skipNextFileLayer(&instanceAirspace.layer);
        getFloatSize(&instanceAirspace.layer, 16, &tempPolygon->latitude);

        skipNextFileLayer(&instanceAirspace.layer);
        nextChar = fgetCFileLayer(&instanceAirspace.layer);
    }
    *hasNext = true;
    return tempAirspace;
}

void deInitAirspaceList()
{
}

static airspace *readSingleAirspaceFromLayer(esp_err_t *error, bool *hasNext, BoundingBox_LatLong_t *myBoundingBox, FILE *cachedFile, long airspaceStarPos)
{
    char nameBuffer[50];
    char heightBuffer[10];
    readTilCharFileLayer(&instanceAirspace.layer, '{');
    readToBufferTillNextCharFileLayer(&instanceAirspace.layer, ':', nameBuffer, 50);
    if (strcmp(nameBuffer, "\"Name\"") != 0)
    {
        printf("Not in the right position\n");
        *error = ESP_ERR_INVALID_STATE;
        return NULL;
    }
    skipNextFileLayer(&instanceAirspace.layer);
    readToBufferTillNextCharFileLayer(&instanceAirspace.layer, '"', nameBuffer, 50);
    char nextChar = fgetCFileLayer(&instanceAirspace.layer);
    while (1)
    {
        skipNextFileLayer(&instanceAirspace.layer);
        nextChar = fgetCFileLayer(&instanceAirspace.layer);
        if (nextChar == 'V')
        {
            // TODO: parse Validity
            readTilCharFileLayer(&instanceAirspace.layer, '{');
            readTilCharFileLayer(&instanceAirspace.layer, '{');
            break;
        }
        if (nextChar == 'U')
        {
            readTilCharFileLayer(&instanceAirspace.layer, '{');
            break;
        }
        readTilCharFileLayer(&instanceAirspace.layer, ',');
    }
    readTilCharTimesFileLayer(&instanceAirspace.layer, '{', 2);
    readTilCharFileLayer(&instanceAirspace.layer, ':');
    skipNextFileLayer(&instanceAirspace.layer);
    heightType heightTypeUpper;
    readToBufferTillNextCharFileLayer(&instanceAirspace.layer, '"', heightBuffer, 10);
    if (strcmp(heightBuffer, "m QNH") == 0)
    {
        heightTypeUpper = HeightType_mQNH;
    }
    else if (strcmp(heightBuffer, "m AGL") == 0)
    {
        heightTypeUpper = HeightType_mAGL;
    }
    else if (strcmp(heightBuffer, "m STD") == 0)
    {
        heightTypeUpper = HeightType_mSTD;
    }
    else
    {
        printf("Unknown height type %s\n", heightBuffer);
        *error = ESP_ERR_INVALID_STATE;
        return NULL;
    }
    readTilCharFileLayer(&instanceAirspace.layer, ':');
    int uppperLimit;
    getIntTilCharacter(&instanceAirspace.layer, '}', &uppperLimit);
    readTilCharFileLayer(&instanceAirspace.layer, '{');
    skipNextFileLayer(&instanceAirspace.layer);
    char temp = fgetCFileLayer(&instanceAirspace.layer);
    // TODO: Save Alt2
    if (temp == 'T')
    {
        readTilCharFileLayer(&instanceAirspace.layer, '{');
        readTilCharFileLayer(&instanceAirspace.layer, '}');
    }
    readTilCharFileLayer(&instanceAirspace.layer, '}');
    readTilCharTimesFileLayer(&instanceAirspace.layer, '{', 3);
    readTilCharFileLayer(&instanceAirspace.layer, ':');
    skipNextFileLayer(&instanceAirspace.layer);
    readToBufferTillNextCharFileLayer(&instanceAirspace.layer, '"', heightBuffer, 10);
    heightType heightTypeLower;
    if (strcmp(heightBuffer, "m QNH") == 0)
    {
        heightTypeLower = HeightType_mQNH;
    }
    else if (strcmp(heightBuffer, "m AGL") == 0)
    {
        heightTypeLower = HeightType_mAGL;
    }
    else if (strcmp(heightBuffer, "m STD") == 0)
    {
        heightTypeLower = HeightType_mSTD;
    }
    else
    {
        printf("Unknown height type %s\n", heightBuffer);
        *error = ESP_ERR_INVALID_STATE;
        return NULL;
    }
    readTilCharFileLayer(&instanceAirspace.layer, ':');
    int lowerLimit;
    getIntTilCharacter(&instanceAirspace.layer, '}', &lowerLimit);
    readTilCharTimesFileLayer(&instanceAirspace.layer, '[', 2);
    airspace *tempAirspace = readPolygonsFromFile(hasNext, myBoundingBox, nameBuffer, cachedFile, airspaceStarPos);
    if (!tempAirspace)
    {
        *error = ESP_OK;
        return NULL;
    }

    tempAirspace->lowerLimit = lowerLimit;
    tempAirspace->upperLimit = uppperLimit;
    tempAirspace->heightTypeLower = heightTypeLower;
    tempAirspace->heightTypeUpper = heightTypeUpper;
    if (strlen(nameBuffer) > AIRSPACE_NAMEBUFFERSIZE)
    {
        printf("Can not save NameBuffer: To Large for buffer\n");
        *error = ESP_ERR_NO_MEM;
        return NULL;
    }
    strcpy(tempAirspace->name, nameBuffer);
    *error = ESP_OK;
    return tempAirspace;
}

static esp_err_t ParseAirspaceJSON(float myLat, float myLon, FILE *cachedFile, FILE *jsonFile)
{
    startFileLayer(&instanceAirspace.layer, 0, jsonFile);
    airspace *listNearest = instanceAirspace.nearestAirspaceList;
    while (listNearest)
    {
        listNearest->isValid = false;
        listNearest = listNearest->next;
    }

    // FILE *fileptr = openFile_sdcard("/airspace", "airspaces.json", "r");
    if (!jsonFile)
    {
        printf("Error opening file\n");
        return ESP_ERR_NOT_FOUND;
    }
    readTilCharFileLayer(&instanceAirspace.layer, '{');
    readTilCharFileLayer(&instanceAirspace.layer, '{');
    bool hasNext = true;
    uint32_t airspacesCount = 0;

    uint32_t airspacesCounted = 0;
    BoundingBox_LatLong_t myBoundingBox;
    boundingBoxFromPoint(&myBoundingBox, myLat, myLon, MAXDISTANCETOSEARCH);
    while (hasNext)
    {
        airspacesCounted++;
        long airspaceStartPos = ftellFileLayer(&instanceAirspace.layer);
        esp_err_t errorValue;
        airspace *tempAirspace = readSingleAirspaceFromLayer(&errorValue, &hasNext, &myBoundingBox, cachedFile, airspaceStartPos);
        if (errorValue != ESP_OK)
            return errorValue;
        if (!tempAirspace)
            continue;
        airspacesCount++;
        hasNext = readTilCharFileLayer(&instanceAirspace.layer, '{');
    }
    listNearest = instanceAirspace.nearestAirspaceList;
    airspace *prev = NULL;
    while (listNearest)
    {
        if (!listNearest->isValid)
        {
            // Delete from LinkedList
            // if (!prev) // First Item to delete
            //     instanceAirspace.nearestAirspaceList = listNearest->next;
            // else // Delete other item
            prev->next = listNearest->next;
            airspace *oldObj = listNearest;
            listNearest = listNearest->next;
            oldObj->next = NULL;
            if (oldObj->allocatedPolygons != NULL)
                free(oldObj->allocatedPolygons);
            if (oldObj->isAllocated)
                free(oldObj);
            continue;
        }
        prev = listNearest;
        listNearest = listNearest->next;
    }
    return ESP_OK;
}

static esp_err_t ParseFromCached(float myLat, float myLon, FILE *cachedFile, FILE *jsonFile)
{
    if (cachedFile == NULL || jsonFile == NULL)
    {
        printf("Cached File or json File was not opend\n");
        return ESP_ERR_NOT_FOUND;
    }
    airspace *listNearest = instanceAirspace.nearestAirspaceList;
    while (listNearest)
    {
        listNearest->isValid = false;
        listNearest = listNearest->next;
    }
    fseek(cachedFile, 0, SEEK_END);
    long pos = ftell(cachedFile);
    fseek(cachedFile, 0, SEEK_SET);
    if (pos % (sizeof(BoundingBox_LatLong_t) + sizeof(long)) != 0)
    {
        printf("This file is not valid\n");
        return ESP_FAIL;
    }
    uint32_t allAirspacesLength = pos / (sizeof(BoundingBox_LatLong_t) + sizeof(long));
    BoundingBox_LatLong_t myBoundingBox;
    boundingBoxFromPoint(&myBoundingBox, myLat, myLon, MAXDISTANCETOSEARCH);
    for (size_t i = 0; i < allAirspacesLength; i++)
    {
        BoundingBox_LatLong_t boundingBox;
        long airspacePos;
        fread(&boundingBox.maxLat, sizeof(float), 1, cachedFile);
        fread(&boundingBox.maxLong, sizeof(float), 1, cachedFile);
        fread(&boundingBox.minLat, sizeof(float), 1, cachedFile);
        fread(&boundingBox.minLong, sizeof(float), 1, cachedFile);
        fread(&airspacePos, sizeof(long), 1, cachedFile);
        // printf("%.2f, %.2f, %.2f, %.2f, %.2f\n", boundingBox.maxLat, boundingBox.maxLong, boundingBox.minLat, boundingBox.minLong, (float)airspacePos);
        if (overlappBoundingBox(&myBoundingBox, &boundingBox))
            continue;
        startFileLayer(&instanceAirspace.layer, airspacePos, jsonFile);
        bool hasNext;
        esp_err_t errorValue;
        airspace *tempAirspace = readSingleAirspaceFromLayer(&errorValue, &hasNext, &myBoundingBox, NULL, airspacePos);
        if (errorValue != ESP_OK)
            return errorValue;
        if (!tempAirspace)
        {
            printf("This sould definitly not happen!!\n");
            continue;
        }
        printf("Found: %s\n", tempAirspace->name);
    }
    listNearest = instanceAirspace.nearestAirspaceList;
    airspace *prev = NULL;
    while (listNearest)
    {
        if (!listNearest->isValid && prev)
        {
            prev->next = listNearest->next;
            airspace *oldObj = listNearest;
            listNearest = listNearest->next;
            oldObj->next = NULL;
            if (oldObj->allocatedPolygons != NULL)
                free(oldObj->allocatedPolygons);
            if (oldObj->isAllocated)
                free(oldObj);
            continue;
        }
        prev = listNearest;
        listNearest = listNearest->next;
    }
    return ESP_OK;
}

static void AirspaceParseError(char *error)
{
    instanceAirspace.nearestAirspaceInfo.error = true;
    strncpy(instanceAirspace.nearestAirspaceInfo.name, error, AIRSPACE_NAMEBUFFERSIZE);
    vario_state_set_member(&instanceAirspace.nearestAirspaceInfo, vario_state_airspaceInfo);
    vTaskDelete(NULL);
}

void AirspaceTask()
{
    gps_t gpsData;
    vario_state_get_member(&gpsData, vario_state_gps_data, false);
    gpsData.valid = true;
    // while (!gpsData.valid)
    //{
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // vario_state_get_member(&gpsData, vario_state_gps_data, false);
    //}
    gpsData.latitude = 47.50733020688501f;
    gpsData.longitude = 7.525586773402665f;
    gpsData.date.year = 2023;
    gpsData.date.month = 05;
    gpsData.date.day = 26;
    char nameBuffer[50];
    snprintf(nameBuffer, 50, "airspace%i_%i_%i.txt", gpsData.date.year, gpsData.date.month, gpsData.date.day);
    if (!sdCard_Init())
    {
        AirspaceParseError("SDCard not found!");
        vTaskDelete(NULL);
        return;
    }
    FILE *cachedFile = openFile_sdcard("/cache", nameBuffer, "r");
    // FILE *cachedFile = NULL;
    FILE *jsonFile = openFile_sdcard("/airspace", "airspaces.json", "r");
    if (jsonFile == NULL)
    {
        printf("Cant open JSON File, please donwload it\n");
        AirspaceParseError("JSON File not downloaded");
        error_handle(ESP_ERR_NOT_FOUND, ERR_AIRSPACEPARCER, "Can't find Airspace JSON File");
        vTaskDelete(NULL);
        return;
    }
    if (cachedFile == NULL)
    {
        cachedFile = openFile_sdcard("/cache", nameBuffer, "w");
        if (cachedFile == NULL)
        {
            printf("Cached File should always be able to open\n");
            fclose(jsonFile);
            AirspaceParseError("Can't open Cache File");
            error_handle(ESP_ERR_NO_MEM, ERR_AIRSPACEPARCER, "Cant create cache file");
            vTaskDelete(NULL);
            return;
        }
        uint64_t timeStart = esp_timer_get_time();
        esp_err_t err = ParseAirspaceJSON(gpsData.latitude, gpsData.longitude, cachedFile, jsonFile);
        if (err != ESP_OK)
        {
            printf("Cant parse JSON File");
            fclose(jsonFile);
            fclose(cachedFile);
            AirspaceParseError("Can't parse JSON File");
            error_handle(err, ERR_AIRSPACEPARCER, "Can't parse JSON");
            vTaskDelete(NULL);
            return;
        }
        printf("Took to parse JSON: %i\n", (int)((esp_timer_get_time() - timeStart) / 1000));
        fclose(cachedFile);
        cachedFile = openFile_sdcard("/cache", nameBuffer, "r");
        if (cachedFile == NULL)
        {
            error_handle(ESP_ERR_NOT_FOUND, ERR_AIRSPACEPARCER, "Can't open created Cachefile");
            vTaskDelete(NULL);
            return;
        }
    }
    uint64_t startTime = esp_timer_get_time();
    esp_err_t err = ParseFromCached(gpsData.latitude, gpsData.longitude, cachedFile, jsonFile);
    printf("Took to parse with Cached: %i\n", (int)((esp_timer_get_time() - startTime) / 1000));
    fclose(cachedFile);
    fclose(jsonFile);
    if (err != ESP_OK)
    {
        error_handle(ESP_ERR_NOT_FOUND, ERR_AIRSPACEPARCER, "Can't Parse JSON with cached");
        vTaskDelete(NULL);
        return;
    }
    //   TODO: handle this case
    instanceAirspace.lastAllAirspaceUpdatePoint.lat = gpsData.latitude;
    instanceAirspace.lastAllAirspaceUpdatePoint.lon = gpsData.longitude;
    if (instanceAirspace.currentIndexAirspaces > 0)
    {
        float pressure;
        vario_state_get_member(&pressure, vario_state_pressure, false);
        pressure = 87765;
        float agl;
        vario_state_get_member(&agl, vario_state_agl, false);
        agl = 30;
        float heightSTD = pressureToSTDHeight(pressure, 17);
        startTime = esp_timer_get_time();
        searchInNearestAirspace(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
        printf("In Airspace took: %i\n", (int)((esp_timer_get_time() - startTime) / 1000));
        startTime = esp_timer_get_time();
        nearestLineAirspace(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
        printf("To all Lines took: %i\n", (int)(esp_timer_get_time() - startTime));
        startTime = esp_timer_get_time();
        airspaceCalculateValuesToLine(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
        printf("In Line took: %i\n", (int)(esp_timer_get_time() - startTime));
    }
    else
    {
        printf("Cant find airspace nearby\n");
        // Screen displays "No Airspace found"
        instanceAirspace.nearestAirspaceInfo.name[0] = '\0';
    }
    return;
    while (true)
    {
        // No Airspace Nearby
        if (!instanceAirspace.currentNearestAirspace)
        {
            instanceAirspace.nearestAirspaceInfo.name[0] = '\0';
            // Wait 10s and try again
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            vario_state_get_member(&gpsData, vario_state_gps_data, false);
            jsonFile = openFile_sdcard("/airspace", "airspaces.json", "r");
            cachedFile = openFile_sdcard("/cache", nameBuffer, "r");
            err = ParseFromCached(gpsData.latitude, gpsData.longitude, cachedFile, jsonFile);
            if (err != ESP_OK)
            {
                error_handle(ESP_ERR_NOT_FOUND, ERR_AIRSPACEPARCER, "Can't Parse JSON with cached");
                vTaskDelete(NULL);
                return;
            }
            continue;
        }
        vario_state_get_member(&gpsData, vario_state_gps_data, false);
        gpsData.latitude = 47.38407637662871,
        gpsData.longitude = 8.525115677931941f;
        float pressure;
        vario_state_get_member(&pressure, vario_state_pressure, false);
        float agl;
        vario_state_get_member(&agl, vario_state_agl, false);
        float heightSTD = pressureToSTDHeight(pressure, 17);

        float distanceToNearest = instanceAirspace.nearestAirspaceInfo.distanceToAirspaceVert;
        float distToAllParse = GET_DISTANCEFASTSQ(gpsData.latitude, gpsData.longitude, instanceAirspace.lastAllAirspaceUpdatePoint.lat, instanceAirspace.lastAllAirspaceUpdatePoint.lon);
        float distToAllNearest = GET_DISTANCEFASTSQ(gpsData.latitude, gpsData.longitude, instanceAirspace.lastAllAirspaceUpdatePoint.lat, instanceAirspace.lastAllAirspaceUpdatePoint.lon);
        float distToNearestLine = GET_DISTANCEFASTSQ(gpsData.latitude, gpsData.longitude, instanceAirspace.lastInNearestAirspaceUpdatePoint.lat, instanceAirspace.lastInNearestAirspaceUpdatePoint.lon);
        // Parse whole JSON with cached, when distance to last parsed of 5km
        if (distToAllParse > (5000 * 5000))
        {
            jsonFile = openFile_sdcard("/airspace", "airspaces.json", "r");
            cachedFile = openFile_sdcard("/cache", nameBuffer, "r");
            err = ParseFromCached(gpsData.latitude, gpsData.longitude, cachedFile, jsonFile);
            if (err != ESP_OK)
            {
                error_handle(ESP_ERR_NOT_FOUND, ERR_AIRSPACEPARCER, "Can't Parse JSON with cached");
                vTaskDelete(NULL);
                return;
            }
            instanceAirspace.lastAllAirspaceUpdatePoint.lat = gpsData.latitude;
            instanceAirspace.lastAllAirspaceUpdatePoint.lon = gpsData.longitude;
            searchInNearestAirspace(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
            nearestLineAirspace(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
            airspaceCalculateValuesToLine(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
        }
        // Search in all nearest Airspace for nearestAirspace, when distance to last calc is bigger than (diff1-2)/3)
        else if (distToAllNearest > ((instanceAirspace.diffFirstToSecondAirspace * instanceAirspace.diffFirstToSecondAirspace) / 3))
        {
            searchInNearestAirspace(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
            nearestLineAirspace(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
            airspaceCalculateValuesToLine(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
        }
        // Search for Line in nearest Airspace when distance to nearestAirspace is < 1km -> all 50m, when farer away all 100m
        else if ((distanceToNearest < 1000 && distToNearestLine > 25) || distToNearestLine > 100)
        {
            nearestLineAirspace(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
            airspaceCalculateValuesToLine(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
        }
        // Sonst all 1s updaten
        else
        {
            airspaceCalculateValuesToLine(gpsData.latitude, gpsData.longitude, gpsData.altitude, heightSTD, agl);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

void StartAirspaceParse()
{
    xTaskCreate(AirspaceTask, "Airspace Parcer", 1024 * 3, NULL, 0, NULL);
}