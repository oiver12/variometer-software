#include "Widget_Vario_Common.h"

#include <stdio.h>

static uint16_t getTextWidth(const char* pString, sFONT* font)
{
    return font->Width * strlen(pString);
}

void DrawWidgetBone(WidgetData* widget, const char* title)
{
    //TODO: Add rounded Corners to Frame
    Paint_DrawRectangle(widget->x, widget->y, widget->x + widget->w, widget->y + widget->h, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    if(title != NULL)
    {
        uint16_t textWidth = getTextWidth(title, &Font12);
        //TODO: was tuen, wenn Text grösser ist? (sollte nicht passieren)
        if (widget->w < textWidth)
            printf("Text passt nicht rein!\n");
        //Text ist oben in der Mitte vom Frame
        Paint_DrawString_EN(widget->x + ((widget->w - textWidth) / 2), widget->y + 4, title, &Font12, WHITE, BLACK, TRUE);
    }
}

void DrawTextWithOffset(WidgetData* widget, const char* text, uint16_t offsetX, uint16_t offsetY,sFONT* font, bool redrawBackground)
{
    Paint_DrawString_EN(widget->x + offsetX, widget->y + offsetY, text, font, WHITE, BLACK, redrawBackground);
}

void DrawTextWithAlign(const char* text, uint16_t offsetX, uint16_t offsetY,sFONT* font, bool redrawBackground, AlignementX_t alignX, AlignementY_t alignY)
{
    uint16_t textWidth = getTextWidth(text, font);
    uint16_t textHeight = font->Height;
    switch(alignX)
    {
        case ALIGN_CENTER:
            offsetX -= textWidth / 2;
            break;
        case ALIGN_RIGHT:
            offsetX -= textWidth;
            break;
        default:
            break;
        //Case ALIGN_LEFT is without changing
    }
    switch(alignY)
    {
        case ALIGN_MID:
            offsetY -= textHeight / 2;
            break;
        case ALIGN_BOTTOM:
            offsetY -= textHeight;
            break;
        default:
            break;
    }
    Paint_DrawString_EN(offsetX, offsetY, text, font, WHITE, BLACK, redrawBackground);
}

void DrawTextWithAlignOffset(WidgetData* widget, const char* text, uint16_t offsetX, uint16_t offsetY, sFONT* font, bool redrawBackground, AlignementX_t alignX, AlignementY_t alignY)
{
    uint16_t posX = widget->x + offsetX;
    uint16_t posY = widget->y + offsetY;

    uint16_t textWidth = getTextWidth(text, font);
    uint16_t textHeight = font->Height;
    switch(alignX)
    {
        case ALIGN_CENTER:
            posX -= textWidth / 2;
            break;
        case ALIGN_RIGHT:
            posX -= textWidth;
            break;
        default:
            break;
        //Case ALIGN_LEFT is without changing
    }
    switch(alignY)
    {
        case ALIGN_MID:
            posY -= textHeight / 2;
            break;
        case ALIGN_BOTTOM:
            posY -= textHeight;
            break;
        default:
            break;
    }
    Paint_DrawString_EN(posX, posY, text, font, WHITE, BLACK, redrawBackground);
}