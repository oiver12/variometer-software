#ifndef __COMMON_WIDGETVARIO_H
#define __COMMON_WIDGETVARIO_H

#include "Widget_Vario.h"
#include "E_Ink/GUI_Paint.h"

typedef enum AlignementY
{
    ALIGN_TOP = 0,
    ALIGN_MID = 1,
    ALIGN_BOTTOM = 2,
} AlignementY_t;

typedef enum AlignementX
{
    ALIGN_LEFT = 0,
    ALIGN_CENTER = 1,
    ALIGN_RIGHT = 2,
} AlignementX_t;

void GetWidgetColor(WidgetData *widget, uint16_t *backgroundColor, uint16_t *textColor);
void DrawWidgetBone(WidgetData *widget, const char *title);
// Draw Standardtext with Alignement TOP, LEFT
void DrawTextWithOffset(WidgetData *widget, const char *text, uint16_t offsetX, uint16_t offsetY, sFONT *font, bool redrawBackground);
void DrawTextWithAlignOffset(WidgetData *widget, const char *text, uint16_t offsetX, uint16_t offsetY, sFONT *font, bool redrawBackground, AlignementX_t alignX, AlignementY_t alignY);
void DrawTextWithAlign(const char *text, uint16_t offsetX, uint16_t offsetY, sFONT *font, bool redrawBackground, AlignementX_t alignX, AlignementY_t alignY);

#endif