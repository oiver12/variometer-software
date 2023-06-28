#ifndef __FILELAYER__H
#define __FILELAYER__H

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

#define BUFFERSIZELAYER 3072

typedef struct FileLayer_t
{
    FILE *f;
    uint32_t currPosBuffer;
    long lastChunkPos;
    char buffer[BUFFERSIZELAYER];
    long fileLength;
    int endInBuffer;
} FileLayer_t;

int startFileLayer(FileLayer_t *layer, long fileStartPos, FILE *f);
bool readTilCharFileLayer(FileLayer_t *layer, int character);
bool readTilCharTimesFileLayer(FileLayer_t *layer, int character, uint8_t times);
int fgetCFileLayer(FileLayer_t *layer);
int skipNextFileLayer(FileLayer_t *layer);
int getFloatTilCharacter(FileLayer_t *layer, char character, float *value);
int getFloatSize(FileLayer_t *layer, uint8_t size, float *value);
int getIntTilCharacter(FileLayer_t *layer, char character, int *value);
long ftellFileLayer(FileLayer_t *layer);
int readToBufferTillNextCharFileLayer(FileLayer_t *layer, char character, char *buffer, int bufferSize);

#endif