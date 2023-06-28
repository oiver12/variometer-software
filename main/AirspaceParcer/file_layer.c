#include "file_layer.h"

// File ended
static int getToBuffer(FileLayer_t *layer)
{
    layer->lastChunkPos += BUFFERSIZELAYER;
    // fseek(layer->f, BUFFERSIZELAYER, SEEK_SET);
    long currPos = ftell(layer->f);
    // Normaler Fall: Einlesen vom Buffer
    if (currPos + BUFFERSIZELAYER < layer->fileLength)
    {
        fread(layer->buffer, BUFFERSIZELAYER, 1, layer->f);
        layer->endInBuffer = -1;
    }
    // Nichts mehr einzulesen
    else if (currPos >= layer->fileLength)
    {
        printf("File has ended\n");
        return 1;
    }
    // Weniger als BUFFERSIZELAYER einzulesen
    else
    {
        fread(layer->buffer, layer->fileLength - currPos, 1, layer->f);
        layer->endInBuffer = layer->fileLength - currPos;
    }
    layer->currPosBuffer = 0;
    return 0;
}

static int checkAndMakeBuffer(FileLayer_t *layer)
{
    if (layer->currPosBuffer >= BUFFERSIZELAYER)
    {
        if (getToBuffer(layer) != 0)
            return 1;
    }
    if (layer->currPosBuffer >= layer->endInBuffer)
        return 1;
    return 0;
}

int startFileLayer(FileLayer_t *layer, long fileStartPos, FILE *f)
{
    if (f == NULL)
    {
        printf("No file opend\n");
        return 1;
    }
    layer->f = f;
    fseek(layer->f, 0, SEEK_END);
    layer->fileLength = ftell(layer->f);
    fseek(layer->f, fileStartPos, SEEK_SET);
    layer->lastChunkPos = fileStartPos;
    layer->currPosBuffer = 0;
    layer->endInBuffer = BUFFERSIZELAYER;
    if (getToBuffer(layer) != 0)
        return 1;
    return 0;
}

bool readTilCharFileLayer(FileLayer_t *layer, int character)
{
    int c;
    while (1)
    {
        c = fgetCFileLayer(layer);
        if (c == EOF)
            break;
        if (c == character)
            break;
    }
    if (c == EOF)
        return false;
    return true;
}

bool readTilCharTimesFileLayer(FileLayer_t *layer, int character, uint8_t times)
{
    for (size_t i = 0; i < times; i++)
    {
        readTilCharFileLayer(layer, character);
    }
    return 0;
}

int fgetCFileLayer(FileLayer_t *layer)
{
    if (checkAndMakeBuffer(layer) != 0)
        return EOF;
    return layer->buffer[layer->currPosBuffer++];
}

int getFloatTilCharacter(FileLayer_t *layer, char character, float *value)
{
    if (checkAndMakeBuffer(layer) != 0)
        return 1;
    char tempBuffer[20];
    uint8_t currPos = 0;
    while (1)
    {
        if (currPos > 20)
            return 1;
        tempBuffer[currPos] = fgetCFileLayer(layer);
        if (tempBuffer[currPos] == character)
        {
            // Wenn letzter Character gerade der letzte vom Buffer war
            if (layer->currPosBuffer == 0)
            {
                printf("Scheissseeeee!\n");
                return 1;
            }
            layer->currPosBuffer--;
            break;
        }
        currPos++;
    }
    tempBuffer[currPos + 1] = '\n';
    return sscanf(tempBuffer, "%f", value);
}

int getFloatSize(FileLayer_t *layer, uint8_t size, float *value)
{
    if (checkAndMakeBuffer(layer) != 0)
        return 1;
    char tempBuffer[size + 1];
    for (size_t i = 1; i < size; i++)
    {
        tempBuffer[i] = fgetCFileLayer(layer);
    }
    tempBuffer[size + 1] = '\n';
    return sscanf(tempBuffer, "%f", value);
}

int getIntTilCharacter(FileLayer_t *layer, char character, int *value)
{
    if (checkAndMakeBuffer(layer) != 0)
        return 1;
    char tempBuffer[20];
    uint8_t currPos = 0;
    while (1)
    {
        if (currPos > 20)
            return 1;
        tempBuffer[currPos] = fgetCFileLayer(layer);
        if (tempBuffer[currPos] == character)
            break;
        currPos++;
    }
    tempBuffer[currPos + 1] = '\n';
    return sscanf(tempBuffer, "%i", value);
}

int skipNextFileLayer(FileLayer_t *layer)
{
    if (checkAndMakeBuffer(layer) != 0)
        return 1;
    layer->currPosBuffer++;
    return 0;
}

long ftellFileLayer(FileLayer_t *layer)
{
    return layer->lastChunkPos + layer->currPosBuffer;
}

int readToBufferTillNextCharFileLayer(FileLayer_t *layer, char character, char *buffer, int bufferSize)
{
    if (checkAndMakeBuffer(layer) != 0)
        return 1;
    int c;
    int i = 0;
    while ((c = fgetCFileLayer(layer)) != character)
    {
        if (checkAndMakeBuffer(layer) != 0)
            return 1;
        if (i < bufferSize - 1)
        {
            buffer[i] = c;
            i++;
        }
        else
        {
            printf("Buffer to small\n");
        }
    }
    buffer[i] = '\0';
    return 0;
}