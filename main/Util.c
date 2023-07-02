#include "Util.h"
#include "math.h"

#include <string.h>
#include <stdio.h>

#define R_SPECIFIC 287.053
#define GRAVITY_G 9.80665
#define TEMPGRAD_L 0.0065
#define Pressure_SeaLewel 101325

static double TO_RADIAN(double deg)
{
	return deg / 180.0 * PI;
}

static double TO_DEGREE(double rad)
{
	return rad * (180.0 / PI);
}

static void cross(const vectorEarth *v1, const vectorEarth *v2, vectorEarth *v3)
{
	v3->x = v1->y * v2->z - v1->z * v2->y;
	v3->y = v1->z * v2->x - v1->x * v2->z;
	v3->z = v1->x * v2->y - v1->y * v2->x;
}

static float dot(const vectorEarth *v1, const vectorEarth *v2)
{
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

static float angleTo(vectorEarth *from, vectorEarth *to, vectorEarth *n)
{
	vectorEarth temp;
	cross(from, to, &temp);
	const int sign = (dot(&temp, n) < 0) ? -1 : 1;
	const float sinV = sqrt(temp.x * temp.x + temp.y * temp.y + temp.z * temp.z) * sign;
	const float cosV = dot(from, to);
	return atan2(sinV, cosV);
}

void getVectorRep(double lat1, double lon1, vectorEarth *v)
{
	const double sinLat = sin(TO_RADIAN(lat1));
	const double cosLat = cos(TO_RADIAN(lat1));
	const double sinLon = sin(TO_RADIAN(lon1));
	const double cosLon = cos(TO_RADIAN(lon1));
	const double x = cosLat * cosLon;
	const double y = cosLat * sinLon;
	const double z = sinLat;
	v->x = x;
	v->y = y;
	v->z = z;
}

bool isEnclosedByPolygon(polygonSide *polyArray, int numPolygons, float lat, float lon)
{
	if (numPolygons < 3)
		return false;
	vectorEarth point;
	getVectorRep(lat, lon, &point);
	float sumAngle = 0;
	vectorEarth polySideVec;
	getVectorRep(polyArray[0].latitude, polyArray[0].longitude, &polySideVec);
	vectorEarth lastPoint = {.x = point.x - polySideVec.x, .y = point.y - polySideVec.y, .z = point.z - polySideVec.z};
	char buffer[5000];
	snprintf(buffer, 5000, "const testArray = [");
	char tempBuffer[100];
	for (size_t i = 1; i < numPolygons + 1; i++)
	{
		snprintf(tempBuffer, 100, "{lat: %f, lon: %f}, ", polyArray[i - 1].latitude, polyArray[i - 1].longitude);
		strcat(buffer, tempBuffer);
		if (i == numPolygons)
		{
			getVectorRep(polyArray[0].latitude, polyArray[0].longitude, &polySideVec);
		}
		else
		{
			getVectorRep(polyArray[i].latitude, polyArray[i].longitude, &polySideVec);
		}
		vectorEarth vecTemp = {.x = point.x - polySideVec.x, .y = point.y - polySideVec.y, .z = point.z - polySideVec.z};
		sumAngle += angleTo(&lastPoint, &vecTemp, &point);
		lastPoint = vecTemp;
	}
	strcat(buffer, "];");
	return fabs(sumAngle) > PI;
}

int16_t GET_BEARING(double lat1, double lon1, double lat2, double lon2)
{
	double teta1 = TO_RADIAN(lat1);
	double teta2 = TO_RADIAN(lat2);

	double delta2 = TO_RADIAN(lon2 - lon1);

	double y = sin(delta2) * cos(teta2);
	double x = cos(teta1) * sin(teta2) - sin(teta1) * cos(teta2) * cos(delta2);

	double bearing = atan2(y, x);
	bearing = TO_DEGREE(bearing); // radians to degrees

	return (((int16_t)bearing + 360) % 360);

#if 0
    float dx = (lon2 - lon1);
    float dy = (lat2 - lat1);

    return (-(int16_t)TO_DEGREE(atan2(dx, dy)) + 360) % 360;
#endif
}

double GET_DISTANCE(double lat1, double lon1, double lat2, double lon2)
{
	double theta, dist;

	if ((lat1 == lat2) && (lon1 == lon2))
		return 0;

	theta = lon1 - lon2;
	dist = sin(TO_RADIAN(lat1)) * sin(TO_RADIAN(lat2)) + cos(TO_RADIAN(lat1)) * cos(TO_RADIAN(lat2)) * cos(TO_RADIAN(theta));
	dist = acos(dist);
	dist = TO_DEGREE(dist);
	dist = dist * 60 * 1.1515;

	return dist * 1609.344; // meters
}

double GET_DISTANCEFAST(double lat1, double lon1, double lat2, double lon2)
{
	double x = TO_RADIAN(lon1 - lon2) * cos(TO_RADIAN(lat1 + lat2) / 2);
	double y = TO_RADIAN(lat1 - lat2);
	return sqrt(x * x + y * y) * 6371e3; // meters
}

double GET_DISTANCEFASTSQ(double lat1, double lon1, double lat2, double lon2)
{
	double x = TO_RADIAN(lon1 - lon2) * cos(TO_RADIAN(lat1 + lat2) / 2);
	double y = TO_RADIAN(lat1 - lat2);
	return (x * x + y * y) * 6371e3 * 6371e3;
}

bool overlappBoundingBox(BoundingBox_LatLong_t *box1, BoundingBox_LatLong_t *box2)
{
	return !(box1->minLat < box2->maxLat && box2->minLat < box1->maxLat && box1->minLong < box2->maxLong && box2->minLong < box1->maxLong);
}

void boundingBoxFromPoint(BoundingBox_LatLong_t *box, float lat, float lon, float distanceMeters)
{
	double latAccuracy = 180.0 * distanceMeters / 40075017;
	double lngAccuracy = latAccuracy / cos((PI / 180.0) * lat);

	double lat1 = lat - latAccuracy;
	double lon1 = lon - lngAccuracy;
	double lat2 = lat + latAccuracy;
	double lon2 = lon + lngAccuracy;
	if (lat1 > lat2)
	{
		box->maxLat = lat1;
		box->minLat = lat2;
	}
	else
	{
		box->maxLat = lat2;
		box->minLat = lat1;
	}
	if (lon1 > lon2)
	{
		box->maxLong = lon1;
		box->minLong = lon2;
	}
	else
	{
		box->maxLong = lon2;
		box->minLong = lon1;
	}
}

float DistancePointToLineSqr(float startLat, float startLon, float endLat, float endLon, float pointLat, float pointLon)
{
	float xLine = endLon - startLon;
	float yLine = endLat - startLat;
	float xLTP = pointLon - startLon;
	float yLTP = pointLat - startLat;
	float mag = xLine * xLine + yLine * yLine;
	float r = xLine * xLTP + yLine * yLTP;
	r /= mag;
	if (r < 0)
		return GET_DISTANCEFASTSQ(startLat, startLon, pointLat, pointLon);
	else if (r > 1)
		return GET_DISTANCEFASTSQ(endLat, endLon, pointLat, pointLon);
	else
	{
		float lonM = startLon + xLine * r;
		float latM = startLat + yLine * r;
		return GET_DISTANCEFASTSQ(latM, lonM, pointLat, pointLon);
	}
}
// TODO: not working https://maps.co/gis/ oder doch?????
void NearestPointOnLine(float startLat, float startLon, float endLat, float endLon, float pointLat, float pointLon, float *nearestLat, float *nearestLon)
{
	float xLine = endLon - startLon;
	float yLine = endLat - startLat;
	float xLTP = pointLon - startLon;
	float yLTP = pointLat - startLat;
	float mag = xLine * xLine + yLine * yLine;
	float r = xLine * xLTP + yLine * yLTP;
	r /= mag;
	if (r < 0)
	{
		*nearestLat = startLat;
		*nearestLon = startLon;
		return;
	}
	else if (r > 1)
	{
		*nearestLat = endLat;
		*nearestLon = endLon;
		return;
	}
	else
	{
		*nearestLon = startLon + xLine * r;
		*nearestLat = startLat + yLine * r;
		return;
	}
}
void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

void list_add_head(struct list_head *new_tile, struct list_head *head)
{
	list_insert(new_tile, head, head->next);
}

void list_add_tail(struct list_head *new_tile, struct list_head *head)
{
	list_insert(new_tile, head->prev, head);
}

void list_insert(struct list_head *new_tile, struct list_head *prev, struct list_head *next)
{
	next->prev = new_tile;
	new_tile->next = next;
	new_tile->prev = prev;
	prev->next = new_tile;
}

float pressureToSTDHeight(float pressure, float startTemp)
{

	return ((startTemp + 273.15) / TEMPGRAD_L) * (pow(pressure / Pressure_SeaLewel, -TEMPGRAD_L * R_SPECIFIC / GRAVITY_G) - 1);
}