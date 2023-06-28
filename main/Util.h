#ifndef __UTIL_H__
#define __UTIL_H__

#include "stdint.h"
#include <stdbool.h>

#define _MIN(x, y) ((x) < (y) ? (x) : (y))
#define _MAX(x, y) ((x) > (y) ? (x) : (y))
#define _CLAMP(x, min, max) (((x) < (min)) ? (min) : (((x) > (max) ? (max) : (x))))

#define PI 3.1415926535897932384626433832795

typedef struct
{
    float lon;
    float lat;
} Point2D_t;
struct list_head
{
    struct list_head *next, *prev;
};

typedef struct
{
    float x;
    float y;
    float z;
} vectorEarth;

typedef struct
{
    float minLat;
    float maxLat;
    float minLong;
    float maxLong;
} BoundingBox_LatLong_t;

typedef struct
{
    float latitude;
    float longitude;
} polygonSide;

#define LIST_HEAD_INIT(name) \
    {                        \
        &(name), &(name)     \
    }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

void INIT_LIST_HEAD(struct list_head *list);

/**
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 * @new: new entry to be added
 * @head: list head to add it after
 *
 */
void list_add_head(struct list_head *new_tile, struct list_head *head);

/**
 * Insert a new entry at the end
 */
void list_add_tail(struct list_head *new_tile, struct list_head *head);

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
void list_insert(struct list_head *new_tile, struct list_head *prev, struct list_head *next);

int16_t GET_BEARING(double lat1, double lon1, double lat2, double lon2);
double GET_DISTANCE(double lat1, double lon1, double lat2, double lon2);
double GET_DISTANCEFAST(double lat1, double lon1, double lat2, double lon2);
double GET_DISTANCEFASTSQ(double lat1, double lon1, double lat2, double lon2);
void getVectorRep(double lat1, double lon1, vectorEarth *v);
bool isEnclosedByPolygon(polygonSide *polyArray, int numPolygons, float lat, float lon);

float DistancePointToLineSqr(float startLat, float startLon, float endLat, float endLon, float pointLat, float pointLon);
void NearestPointOnLine(float startLat, float startLon, float endLat, float endLon, float pointLat, float pointLon, float *nearestLat, float *nearestLon);

bool overlappBoundingBox(BoundingBox_LatLong_t *box1, BoundingBox_LatLong_t *box2);
void boundingBoxFromPoint(BoundingBox_LatLong_t *box, float lat, float lon, float distanceMeters);

float pressureToSTDHeight(float pressure, float startTemp);
#endif
