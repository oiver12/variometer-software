#ifndef __AIRSPACEPARCER__H
#define __AIRSPACEPARCER__H

#include "../Util.h"
#include "file_layer.h"

#define AIRSPACE_NAMEBUFFERSIZE 100
#define PREALLOCATEDAIRSPACES 10
#define PREALLOCATEDPOLYGONS 60

typedef enum
{
    HeightType_mQNH = 1,
    HeightType_mSTD = 2,
    HeightType_mAGL = 3
} heightType;

typedef struct
{
    char name[100];
    uint16_t upperLimit;
    heightType heightTypeUpper;
    uint16_t lowerLimit;
    heightType heightTypeLower;
    polygonSide polygons[PREALLOCATEDPOLYGONS];
    polygonSide *allocatedPolygons;
    size_t currentIndexPolygons;
    size_t polygonSize;
    void *next;
    bool isValid;
    bool isAllocated;
} airspace;

typedef struct Airspace_Info_t
{
    char name[AIRSPACE_NAMEBUFFERSIZE];
    uint16_t upperLimt;
    heightType heightTypeUpper;
    uint16_t lowerLimit;
    heightType heightTypeLower;
    float distanceToAirspaceVert;
    float distanceToAirspaceHor;
    Point2D_t nearestLineStart;
    Point2D_t nearestLineEnd;
    bool error;
} Airspace_Info_t;

typedef struct AirspaceParcer_State_t
{
    airspace nearestAirspaceList[PREALLOCATEDAIRSPACES];
    size_t currentIndexAirspaces;
    airspace *currentNearestAirspace;
    Airspace_Info_t nearestAirspaceInfo;
    uint64_t lastLineUpdate;
    Point2D_t lastInNearestAirspaceUpdatePoint; // Point where nearestLine was found in the nearest Airspace
    Point2D_t lastInNearAirspacesUpdatePoint;   // Point where all near Airspaces where searched and the nearest was picked
    Point2D_t lastAllAirspaceUpdatePoint;       // Point where whole JSON File was last parsed
    float diffFirstToSecondAirspace;
    FileLayer_t layer;
} AirspaceParcer_State_t;

void StartAirspaceParse();

#endif