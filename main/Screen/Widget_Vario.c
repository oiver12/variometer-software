#include "Widget_Vario.h"
#include "DeviceContext.h"
#include "E_Ink/GUI_Paint.h"
#include "Widget_Vario_Common.h"
#include "assets/assets.h"

#include <stdio.h>
#include <math.h>
#include <time.h>

#define BUFFER_TEXT_SIZE 20
#define Compass_Method1

#define COMPASS_1METHOD
#define DEGREE_TO_RADIAN 0.01745329251
#define COMPASS_PLACE_SIDE 15
#define CHART_POINT_COUNT 20

#ifdef Compass_Method1
const unsigned char gImage_Compass[800] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0, 0x0, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0, 0x0, 0x7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x0, 0x0, 0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x0, 0x0, 0x0, 0x0, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x0, 0x7F, 0xFF, 0x0, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x3, 0xFF, 0xFF, 0xE0, 0x3, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1F, 0xFF, 0xFF, 0xFC, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x7F, 0xFF, 0xFF, 0xFF, 0x0, 0xFF, 0xFF, 0xFF, 0xFE, 0x1, 0xFF, 0xE7, 0x8F, 0xFF, 0xC0, 0x3F, 0xFF, 0xFF, 0xFC, 0x3, 0xFF, 0xF3, 0xDF, 0xFF, 0xE0, 0x1F, 0xFF, 0xFF, 0xF8, 0xF, 0xFF, 0xF5, 0xDF, 0xFF, 0xF8, 0xF, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xF5, 0xDF, 0xFF, 0xFC, 0x7, 0xFF, 0xFF, 0xE0, 0x3F, 0xFF, 0xF6, 0xDF, 0xFF, 0xFE, 0x3, 0xFF, 0xFF, 0xC0, 0x7F, 0xFF, 0xF7, 0x5F, 0xFF, 0xFF, 0x1, 0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0xF7, 0x9F, 0xFF, 0xFF, 0x81, 0xFF, 0xFF, 0x81, 0xFF, 0xFF, 0xF7, 0x9F, 0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0x3, 0xFF, 0xFF, 0xE3, 0xDF, 0xFF, 0xFF, 0xE0, 0x7F, 0xFE, 0x7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x3F, 0xFE, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x3F, 0xFC, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x1F, 0xFC, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x1F, 0xF8, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF, 0xF8, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF, 0xF0, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7, 0xF0, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x87, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x83, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x83, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC1, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC1, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xC3, 0x8D, 0xC7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x3, 0xE1, 0xC3, 0xDC, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7B, 0xE1, 0xC3, 0xDA, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xE1, 0xC3, 0xEA, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x77, 0xE1, 0xC3, 0xEB, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7, 0xE1, 0xC3, 0xEB, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x77, 0xE1, 0xC3, 0xE7, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xE1, 0xC3, 0xF7, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7B, 0xE1, 0xC3, 0xF7, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x3, 0xE1, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC1, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC1, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x83, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x83, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x87, 0xF0, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7, 0xF0, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7, 0xF8, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF, 0xF8, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF, 0xFC, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x1F, 0xFC, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x1F, 0xFE, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x3F, 0xFE, 0x7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x3F, 0xFF, 0x3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x7F, 0xFF, 0x81, 0xFF, 0xFF, 0xFC, 0x3F, 0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0xFB, 0xDF, 0xFF, 0xFF, 0x81, 0xFF, 0xFF, 0xC0, 0x7F, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0x1, 0xFF, 0xFF, 0xE0, 0x3F, 0xFF, 0xF8, 0xFF, 0xFF, 0xFE, 0x3, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFE, 0x3F, 0xFF, 0xFC, 0x7, 0xFF, 0xFF, 0xF8, 0xF, 0xFF, 0xFF, 0x9F, 0xFF, 0xF8, 0xF, 0xFF, 0xFF, 0xFC, 0x3, 0xFF, 0xFF, 0xDF, 0xFF, 0xE0, 0x1F, 0xFF, 0xFF, 0xFE, 0x1, 0xFF, 0xFB, 0xDF, 0xFF, 0xC0, 0x3F, 0xFF, 0xFF, 0xFF, 0x80, 0x7F, 0xFC, 0x3F, 0xFF, 0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x1F, 0xFF, 0xFF, 0xFC, 0x1, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x3, 0xFF, 0xFF, 0xE0, 0x3, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x0, 0x7F, 0xFF, 0x0, 0xF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x0, 0x0, 0x0, 0x0, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x0, 0x0, 0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x0, 0x0, 0x7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0, 0x0, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#else
const unsigned char gImage_Compass_Zahlen[512] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCF, 0x8F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEB, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xED, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xED, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEE, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x9F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0x78, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x3, 0xEF, 0x7D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFB, 0xF6, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xF6, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xF6, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF7, 0xF6, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x7, 0xFA, 0xD7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF7, 0xFA, 0xD7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFA, 0xD7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xF9, 0xD7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFD, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFB, 0xFD, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#endif
gps_t oldGpsObject;

sFONT *__FontStack[] =
    {
        &Font12, // 8px
        &Font12, // 12px
        &Font16, // 16px
        &Font20, // 20px
        &Font20, // 22px
        &Font24, // 24px
        &Font24, // 26px
        &Font24, // 28px
        &Font24, // 30px
        &Font24  // 32px
};

static uint16_t getTextWidth(const char *pString, sFONT *font)
{
    return font->Width * strlen(pString);
}

static const char *get_Info_Text(WidgetContentType contentType)
{
    switch (contentType)
    {
    case WidgetContent_Speed_Ground:
        return "Ground Speed";
    case WidgetContent_Vario_Active:
        return "Instant Vario";
    case WidgetContent_Vario_Lazy:
        return "Vario integrated (2s)";
    case WidgetContent_VertAcc:
        return "Vertical Acceleration";
    case WidgetContent_Pressure:
        return "Pressure";
    case WidgetContent_Temperature:
        return "Temperature";
    case WidgetContent_Altitude_Baro:
        return "Barometer Altitude";
    case WidgetContent_Altitude_GPS:
        return "GPS Altitude";
    case WidgetContent_Altitude_AGL:
        return "AGL";
    case WidgetContent_Time_GPS:
        return "Time";
    default:
        return "";
    }
}

float get_Value_Text(WidgetContentType contentType, char *const buffer, size_t bufferSize, bool shouldRender, bool shouldReset)
{
    gps_t tempGps;
    if (contentType == WidgetContent_Time_GPS)
    {
        static int countGPS = 20;
        vario_state_get_member(&tempGps, vario_state_gps_data, shouldReset);
        if (countGPS % 20 == 0)
        {
            snprintf(buffer, bufferSize, "%i:%i", tempGps.tim.hour, tempGps.tim.minute);
            printf("GPS Time Update \n");
        }
        else if (bufferSize != 0)
            buffer[0] = 0;
        countGPS++;
        return true;
    }
    float temp;
    bool returnValue = false;
    switch (contentType)
    {
    case WidgetContent_Speed_Ground:
        returnValue = vario_state_get_member(&temp, vario_state_groundSpeed, shouldReset);
        break;
    case WidgetContent_Vario_Active:
        returnValue = vario_state_get_member(&temp, vario_state_vertVel, shouldReset);
        break;
    case WidgetContent_Vario_Lazy:
        returnValue = vario_state_get_member(&temp, vario_state_vertVelLazy, shouldReset);
        break;
    case WidgetContent_VertAcc:
        returnValue = vario_state_get_member(&temp, vario_state_vertAcc, shouldReset);
        break;
    case WidgetContent_Pressure:
        returnValue = vario_state_get_member(&temp, vario_state_pressure, shouldReset);
        break;
    case WidgetContent_Temperature:
        returnValue = vario_state_get_member(&temp, vario_state_temperature, shouldReset);
        break;
    case WidgetContent_Altitude_Baro:
        returnValue = vario_state_get_member(&temp, vario_state_altitudeBaro, shouldReset);
        break;
    case WidgetContent_Altitude_GPS:
        returnValue = vario_state_get_member(&temp, vario_state_altitudeGPS, shouldReset);
        break;
    case WidgetContent_Altitude_AGL:
        returnValue = vario_state_get_member(&temp, vario_state_agl, shouldReset);
        break;
    default:
        return 0;
    }
    if ((returnValue | shouldRender) && bufferSize != 0)
    {
        // Wenn zwischen -0.1f und 0.0f, setzte auf 0.0f, weil sonst -0.0 angezeigt wird
        if (temp > -0.05f && temp < 0.0f)
            temp = 0.0f;
        snprintf(buffer, bufferSize, "%.1f", temp);
    }
    else if (bufferSize != 0)
        buffer[0] = 0;
    return temp;
}

static const char *get_Unit_Text(WidgetContentType contentType)
{
    switch (contentType)
    {
    case WidgetContent_Speed_Ground:
        return "km/h";
    case WidgetContent_Vario_Active:
        return "m/s";
    case WidgetContent_Vario_Lazy:
        return "m/s";
    case WidgetContent_VertAcc:
        return "m/s2";
    case WidgetContent_Pressure:
        return "hPa";
    case WidgetContent_Temperature:
        return "C°";
    case WidgetContent_Altitude_Baro:
    case WidgetContent_Altitude_GPS:
    case WidgetContent_Altitude_AGL:
        return "m";
    default:
        return "";
    }
}

static void add_TextBox_Widget(WidgetData *widget)
{
    // TODO wenn Widget gelöscht wird, muss auch das hiet gelöscht werden
    TextBox_Widget_Data *extr_data = malloc(sizeof(TextBox_Widget_Data));
    widget->extr_data = (void *)extr_data;

    const char *informationString = get_Info_Text(widget->type);
    DrawWidgetBone(widget, informationString);

    const char *unitText = get_Unit_Text(widget->type);
    uint16_t textWidth = getTextWidth(unitText, &Font16);
    // StartX: Abstand rechts vom Fram ist 5 Pixel
    DrawTextWithOffset(widget, unitText, widget->w - textWidth - 5, (widget->h / 2) + 5, &Font16, true);

    sFONT *fontValue = __FontStack[(widget->extraStyle & WS_FONT_MASK_CONTENT) >> 4];
    // Draw value Text
    ((TextBox_Widget_Data *)widget->extr_data)->linkerRandUnitText = widget->x + widget->w - textWidth;
    switch (widget->extraStyle & WS_FONT_MASK_CONTENT)
    {
    case WS_FONT_8_CONTENT:
        ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText = widget->h / 2 + 7;
        break;
    case WS_FONT_12_CONTENT:
        ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText = widget->h / 2 + 7;
        break;
    case WS_FONT_16_CONTENT:
        ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText = widget->h / 2 + 5;
        break;
    case WS_FONT_20_CONTENT:
        ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText = widget->h / 2 + 3;
        break;
    case WS_FONT_24_CONTENT:
        ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText = widget->h / 2;
        break;
    case WS_FONT_30_CONTENT:
        ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText = widget->h / 2;
        break;
    case WS_FONT_32_CONTENT:
        ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText = widget->h / 2;
        break;
    }
    char tempBuffer[10];
    get_Value_Text(widget->type, tempBuffer, 10, true, true);
    textWidth = getTextWidth(tempBuffer, __FontStack[(widget->extraStyle & WS_FONT_MASK_CONTENT) >> 4]);
    // StartX: Abstand zwischen Unit rechts und Frame links soll gleich gross sein
    // Paint_DrawString_EN(widget->x + ((((TextBox_Widget_Data*)widget->extr_data)->linkerRandUnitText - widget->x) / 2) - textWidth / 2, ((TextBox_Widget_Data*)widget->extr_data)->yStartValueText, tempBuffer, fontValue, WHITE, BLACK, TRUE);
    DrawTextWithOffset(widget, tempBuffer, ((((TextBox_Widget_Data *)widget->extr_data)->linkerRandUnitText - widget->x) / 2) - textWidth / 2, ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText, fontValue, true);
}

static void add_Compass_widget(WidgetData *widget)
{
    Compass_Widget_Data *extr_data = malloc(sizeof(Compass_Widget_Data));
    widget->extr_data = (void *)extr_data;
    extr_data->lastAngle = 0;
    extr_data->angleDifferenceToShow = 5;

    DrawWidgetBone(widget, NULL);

    Paint_DrawBitMap_Paste(gImage_Arrow_down, widget->x + widget->w / 2 - 5, widget->y + 28, 10, 10, FALSE);
#ifdef Compass_Method1
    Paint_DrawBitMap_Paste(gImage_Compass, widget->x + widget->w / 2 - 40, widget->y + widget->h - 80 - 10, 80, 80, FALSE);
#else
    Paint_DrawCircle(widget->x + widget->w / 2, widget->y + widget->h - 40 - 8, 40, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
#endif
}

static void add_Chart_Widget(WidgetData *widget)
{
    Chart_Widget_Data *extr_data = malloc(sizeof(Chart_Widget_Data));
    widget->extr_data = (void *)extr_data;
    extr_data->data = fifo_create(ChartNumDataPoints, sizeof(Chart_Point));
    extr_data->lastPointTime = 0;
    extr_data->startXPosition = widget->x + widget->w - 10;
    extr_data->timePeriod = 120;
    extr_data->differenzToShow = 200;
    extr_data->heightChart = widget->h - 45;
    extr_data->yStartPosChart = widget->y + 30;
    extr_data->pointsAdded = 0;
    uint32_t chartXWidth = widget->w - 35;
    extr_data->xMoveDelta = chartXWidth / ChartNumDataPoints;
    // Draw Chart
    const char *informationString = get_Info_Text(widget->type);
    DrawWidgetBone(widget, informationString);
    Paint_DrawLine(widget->x + 8, widget->y + 25, widget->x + 8, widget->y + widget->h - 10, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    char buffer[6];
    snprintf(buffer, 6, "%lu%s", (unsigned long)extr_data->differenzToShow, get_Unit_Text(widget->type));
    DrawTextWithOffset(widget, buffer, 12, widget->h / 2, &Font12, true);
    snprintf(buffer, 6, "%lus", (unsigned long)extr_data->timePeriod);
    DrawTextWithOffset(widget, buffer, widget->w - 30, widget->h - 14, &Font12, true);
}

static void add_sdCardInfo_Widget(WidgetData *widget)
{
    sd_card_state_t sdcard_state;
    device_state_get_member(&sdcard_state, device_state_statusSDCard, TRUE);
    DrawWidgetBone(widget, "SD Card usage");
    if (sdcard_state.status & SD_CARD_STATE_ERROR_INIT)
    {
        char *buffer = "SD Card not mounted";
        int16_t textWidth = getTextWidth(buffer, &Font12);
        Paint_DrawString_EN(widget->x + ((widget->w - textWidth) / 2), widget->y + widget->h / 2, buffer, &Font12, WHITE, BLACK, TRUE);
        return;
    }
    uint32_t startPos = widget->x + 10;
    uint32_t widthProgressBar = widget->w - 60;
    Paint_DrawRectangle(startPos, widget->y + 25, startPos + widthProgressBar, widget->y + 45, BLACK, DOT_PIXEL_1X1, FALSE);
    double percentage = 1.0f - ((double)sdcard_state.freeSpace / (double)sdcard_state.maxSpace);
    uint32_t filledPartWidth = percentage * widthProgressBar;
    Paint_DrawRectangle(startPos, widget->y + 25, startPos + filledPartWidth, widget->y + 45, BLACK, DOT_PIXEL_1X1, TRUE);
    char buffer[50];
    snprintf(buffer, 10, "%i%%", (int)(percentage * 100));
    Paint_DrawString_EN(startPos + widthProgressBar + 7, widget->y + 29, buffer, &Font16, WHITE, BLACK, true);
    if (sdcard_state.status & SD_CARD_STATE_FLIGHTLOGGING)
        snprintf(buffer, 50, "Flight Logger started");
    else
        snprintf(buffer, 50, "Flight Logger disabled");
    int16_t textWidth = getTextWidth(buffer, &Font12);
    DrawTextWithOffset(widget, buffer, widget->w / 2 - textWidth / 2, 55, &Font12, true);
    if (sdcard_state.status & SD_CARD_STATE_IGC_LOGGING)
        snprintf(buffer, 50, "IGC Logger started");
    else
        snprintf(buffer, 50, "IGC Logger disabled");
    textWidth = getTextWidth(buffer, &Font12);
    DrawTextWithOffset(widget, buffer, widget->w / 2 - textWidth / 2, 70, &Font12, true);
}

static void add_gpsInfo_Widget(WidgetData *widget)
{
    gps_t gps_data;
    vario_state_get_member(&gps_data, vario_state_gps_data, true);
    DrawWidgetBone(widget, "GPS Info");
    char buffer[50];
    snprintf(buffer, 49, "Lat: %.2f", gps_data.latitude);
    DrawTextWithOffset(widget, buffer, 5, 25, &Font12, true);
    snprintf(buffer, 49, "Lon: %.2f", gps_data.longitude);
    DrawTextWithOffset(widget, buffer, 90, 25, &Font12, true);
    snprintf(buffer, 49, "Height: %.2fm", gps_data.altitude);
    DrawTextWithAlignOffset(widget, buffer, widget->w - 5, 25, &Font12, true, ALIGN_RIGHT, ALIGN_TOP);
    DrawTextWithOffset(widget, gps_data.valid ? "1" : "0", 5, 50, &Font12, true);
    switch (gps_data.fix)
    {
    case GPS_FIX_INVALID:
        snprintf(buffer, 50, "Fix: Invalid");
        break;
    case GPS_FIX_GPS:
        snprintf(buffer, 50, "Fix: GPS");
        break;
    case GPS_FIX_DGPS:
        snprintf(buffer, 50, "Fix: DGPS");
        break;
    default:
        snprintf(buffer, 50, "None");
        break;
    }
    DrawTextWithOffset(widget, buffer, 40, 50, &Font12, true);
    switch (gps_data.fix_mode)
    {
    case GPS_MODE_INVALID:
        snprintf(buffer, 50, "Fix M: Invalid");
        break;
    case GPS_MODE_2D:
        snprintf(buffer, 50, "Fix M: 2D");
        break;
    case GPS_MODE_3D:
        snprintf(buffer, 50, "Fix: 3D");
        break;
    default:
        snprintf(buffer, 50, "Fix M: Invalid");
        break;
    }
    DrawTextWithAlignOffset(widget, buffer, widget->w - 20, 50, &Font12, true, ALIGN_RIGHT, ALIGN_TOP);
    DrawTextWithOffset(widget, "--Satellits--", 5, 65, &Font12, true);
    uint16_t currentPosY = 80;

    gps_data.sats_in_view = 12;
    for (size_t i = 0; i < gps_data.sats_in_view; i++)
    {
        // gps_satellite_t sat = gps_data.sats_desc_in_view[i];
        gps_satellite_t sat = {
            .num = i,
        };
        snprintf(buffer, 50, "ID: %u", sat.num);
        for (size_t y = 0; y < gps_data.sats_in_use; y++)
        {
            // TODO: Check if this is right, perhaps in num of satellit
            if (gps_data.sats_id_in_use[y] == i)
            {
                strcat(buffer, "(in use)");
            }
        }
        if (i % 2 == 0)
        {
            strcat(buffer, "(in use)");
        }
        DrawTextWithOffset(widget, buffer, 20, currentPosY, &Font8, true);
        currentPosY += 9;
    }

    DrawTextWithOffset(widget, "Dilution:", 130, 86, &Font12, true);
    snprintf(buffer, 50, "H: %.2f", gps_data.dop_h);
    DrawTextWithOffset(widget, buffer, 200, 75, &Font12, true);
    snprintf(buffer, 50, "V: %.2f", gps_data.dop_v);
    DrawTextWithOffset(widget, buffer, 200, 87, &Font12, true);
    snprintf(buffer, 50, "P: %.2f", gps_data.dop_p);
    DrawTextWithOffset(widget, buffer, 200, 99, &Font12, true);

    snprintf(buffer, 50, "COG: %.2f", gps_data.cog);
    DrawTextWithOffset(widget, buffer, 130, 130, &Font12, true);
    snprintf(buffer, 50, "Variation: %.2f", gps_data.variation);
    DrawTextWithOffset(widget, buffer, 130, 150, &Font12, true);
    snprintf(buffer, 50, "Speed: %.2f", gps_data.speed);
    DrawTextWithOffset(widget, buffer, 130, 170, &Font12, true);
    oldGpsObject = gps_data;
}

static void add_Airspace_Widget(WidgetData *widget)
{
    Airspace_Info_t airspaceInfo;
    vario_state_get_member(&airspaceInfo, vario_state_airspaceInfo, true);
    if (airspaceInfo.error)
    {
        DrawWidgetBone(widget, "Airspace");
        DrawTextWithAlignOffset(widget, airspaceInfo.name, widget->w / 2, widget->h / 2, &Font12, true, ALIGN_CENTER, ALIGN_MID);
        return;
    }
    if (airspaceInfo.name[0] == '\0')
    {
        DrawWidgetBone(widget, "No Airspace nearby found");
        return;
    }
    DrawWidgetBone(widget, airspaceInfo.name);
    char buffer[100];
    snprintf(buffer, 100, "%im", (int)airspaceInfo.lowerLimit);
    switch (airspaceInfo.heightTypeLower)
    {
    case HeightType_mQNH:
        strcat(buffer, " QNH");
        break;
    case HeightType_mSTD:
        strcat(buffer, " STD");
        break;
    case HeightType_mAGL:
        if (airspaceInfo.lowerLimit == 0)
        {
            strcpy(buffer, "GND");
            break;
        }
        strcat(buffer, " AGL");
        break;
    }
    char tempBuffer[25];
    snprintf(tempBuffer, 25, " - %im", (int)airspaceInfo.upperLimt);
    strcat(buffer, tempBuffer);
    switch (airspaceInfo.heightTypeUpper)
    {
    case HeightType_mQNH:
        strcat(buffer, " QHN");
        break;
    case HeightType_mSTD:
        strcat(buffer, " STD");
        break;
    case HeightType_mAGL:
        strcat(buffer, " AGL");
        break;
    }
    DrawTextWithAlignOffset(widget, buffer, widget->w / 2, 20, &Font12, true, ALIGN_CENTER, ALIGN_TOP);
    snprintf(buffer, 100, "Hor: %i m", (int)airspaceInfo.distanceToAirspaceHor);
    DrawTextWithAlignOffset(widget, buffer, widget->w / 2, 40, &Font12, true, ALIGN_CENTER, ALIGN_TOP);
    snprintf(buffer, 100, "Vert: %i m", (int)airspaceInfo.distanceToAirspaceVert);
    DrawTextWithAlignOffset(widget, buffer, widget->w / 2, 60, &Font12, true, ALIGN_CENTER, ALIGN_TOP);
}

static void add_TrackPoint_Widget(WidgetData *widget)
{
    DrawWidgetBone(widget, "");
}

/************************ Update Widgets ************************/

static void update_TextBox_Widget(WidgetData *widget)
{
    char tempBuffer[10];
    get_Value_Text(widget->type, tempBuffer, 10, false, true);
    // Update Value Text
    if (tempBuffer[0] != '\0')
    {
        sFONT *font = __FontStack[(widget->extraStyle & WS_FONT_MASK_CONTENT) >> 4];
        uint16_t textWidth = getTextWidth(tempBuffer, font);
        // Clear surface in frame
        Paint_ClearWindows(widget->x + 5, widget->y + ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText - 2, ((TextBox_Widget_Data *)widget->extr_data)->linkerRandUnitText - 8, widget->y + (widget->h / 2) + 21, WHITE);
        // Paint_DrawRectangle(widget->x + 5, ((TextBox_Widget_Data*)widget->extr_data)->yStartValueText - 2, ((TextBox_Widget_Data*)widget->extr_data)->linkerRandUnitText - 8, widget->y + (widget->h / 2) + 21, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        DrawTextWithOffset(widget, tempBuffer, ((((TextBox_Widget_Data *)widget->extr_data)->linkerRandUnitText - widget->x) / 2) - textWidth / 2, ((TextBox_Widget_Data *)widget->extr_data)->yStartValueText, font, FALSE);
    }
}

static void update_Compass_Widget(WidgetData *widget)
{
    // Compass_Method2: Optimisation: Draw Circle once and rotate only letters in the middle. In the rotated function, only display pixels in circle (corners would "overdraw" circle)
    int heading;
    if (vario_state_get_member(&heading, vario_state_heading, TRUE) && abs(((Compass_Widget_Data *)widget->extr_data)->lastAngle - heading) > ((Compass_Widget_Data *)widget->extr_data)->angleDifferenceToShow)
    {
        ((Compass_Widget_Data *)widget->extr_data)->lastAngle = heading;
#ifdef Compass_Method1
        Paint_DrawBitMap_Paste_Rotated(gImage_Compass, widget->x + widget->w / 2 - 40, widget->y + widget->h - 80 - 10, 80, 80, (int)heading);
#else
        Paint_DrawBitMap_Paste_Rotated_Circle(gImage_Compass_Zahlen, widget->x + widget->w / 2 - 32, widget->y + widget->h - 80, 64, 64, (int)heading, 31);
#endif
        char tempBuffer[9];
        if (heading > 337 || heading < 23)
            snprintf(tempBuffer, 9, "%iN", heading);
        else if (heading >= 23 && heading <= 67)
            snprintf(tempBuffer, 9, "%iNE", heading);
        else if (heading > 67 && heading < 113)
            snprintf(tempBuffer, 9, "%iE", heading);
        else if (heading >= 113 && heading < 157)
            snprintf(tempBuffer, 9, "%iSE", heading);
        else if (heading >= 157 && heading <= 201)
            snprintf(tempBuffer, 9, "%iS", heading);
        else if (heading > 201 && heading < 247)
            snprintf(tempBuffer, 9, "%iSW", heading);
        else if (heading >= 247 && heading < 293)
            snprintf(tempBuffer, 9, "%iW", heading);
        else
            snprintf(tempBuffer, 9, "%iNW", heading);
        Paint_ClearWindows(widget->x + 10, widget->y + 10, widget->x + (widget->w / 2) + 30, widget->y + 25, WHITE);
        DrawTextWithOffset(widget, tempBuffer, widget->w / 2, 10, &Font16, FALSE);
    }
}

// TODO: Program Mode, where it auto. zooms to see all Points
static void update_Chart_Widget(WidgetData *widget)
{
    float t1 = (float)clock() / CLOCKS_PER_SEC; // in seconds
    // Update Chart
    Chart_Widget_Data *chart_data = (Chart_Widget_Data *)widget->extr_data;
    // Place a point all x Seconds (e.g. when timePeriod is 20s and Point Count is 20, place all 1s a new point and shift all Point to the left)
    if (chart_data->timePeriod / CHART_POINT_COUNT <= t1 - chart_data->lastPointTime)
    {
        chart_data->lastPointTime = t1;
        if (chart_data->pointsAdded == 0)
        {
            /*float height;
            vario_state_get_member(&height, vario_state_altitudeBaro, FALSE);*/
            float value = get_Value_Text(widget->type, NULL, 0, false, false);
            // Place first Point in the middle
            chart_data->startValue = value - chart_data->differenzToShow / 2;
            float percentageFromHeight = ((float)(value - chart_data->startValue)) / ((float)chart_data->startValue);
            Chart_Point point = {
                .lastXPoint = chart_data->startXPosition,
                .yPoint = (uint16_t)((1 - percentageFromHeight) * (float)chart_data->heightChart) + chart_data->yStartPosChart,
                .heightValue = value,
            };
            fifo_add(chart_data->data, &point);
            Paint_DrawPoint(point.lastXPoint, point.yPoint, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
            chart_data->pointsAdded++;
            return;
        }
        float value = get_Value_Text(widget->type, NULL, 0, false, false);
        bool startValueChanged = false;
        // when new height can't is lower or higher than the range in the chart, shift the range, so the new point is in the chart
        if (value < chart_data->startValue || value > chart_data->startValue + chart_data->differenzToShow)
        {
            chart_data->startValue = value < chart_data->startValue ? value : (value - chart_data->differenzToShow);
            startValueChanged = true;
        }
        float percentageFromHeight = ((float)(value - chart_data->startValue)) / ((float)chart_data->differenzToShow);
        Chart_Point newPoint = {
            .lastXPoint = chart_data->startXPosition,
            .yPoint = (uint16_t)((1 - percentageFromHeight) * (float)chart_data->heightChart) + chart_data->yStartPosChart,
            .heightValue = value,
        };
        Chart_Point lastpoint;
        if (chart_data->pointsAdded < CHART_POINT_COUNT)
        {
            chart_data->pointsAdded++;
            if (fifo_is_full(chart_data->data))
                printf("Test scheisse\n");
        }
        else
        {
            fifo_get(chart_data->data, &lastpoint);
            if (lastpoint.yPoint != -1)
                Paint_DrawPoint(lastpoint.lastXPoint, lastpoint.yPoint, WHITE, DOT_PIXEL_2X2, DOT_STYLE_DFT);
        }

        fifo_add(chart_data->data, &newPoint);

        // Paint_ClearWindows(chart_data->startXPosition - chart_data->xMoveDelta * CHART_POINT_COUNT, chart_data->yStartPosChart - 2, chart_data->startXPosition + 2, chart_data->yStartPosChart + chart_data->heightChart + 2, WHITE);
        // Paint_DrawRectangle(chart_data->startXPosition - chart_data->xMoveDelta * CHART_POINT_COUNT , chart_data->yStartPosChart - 2, chart_data->startXPosition + 2, chart_data->yStartPosChart + chart_data->heightChart + 2, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // shift all Points left and render (dont shift last one)
        // TODO: there are buggs, where the last point isn't rendered full.
        // TODO: also fifo_peekItem shouldn't make a for loop everytime. State could be remembered in the fifo and just call getNext() on fifo
        for (size_t i = 0; i < chart_data->pointsAdded - 1; i++)
        {
            Chart_Point *itemPoint = fifo_peekItem(chart_data->data, i);
            // Clear old Point
            if (itemPoint->yPoint != -1)
                Paint_DrawPoint(itemPoint->lastXPoint, itemPoint->yPoint, WHITE, DOT_PIXEL_2X2, DOT_STYLE_DFT);
            if (startValueChanged)
            {
                float percentageFromHeight = ((float)(itemPoint->heightValue - chart_data->startValue)) / ((float)chart_data->differenzToShow);
                if (percentageFromHeight > 1 || percentageFromHeight < 0)
                    itemPoint->yPoint = -1;
                else
                    itemPoint->yPoint = (uint16_t)((1 - percentageFromHeight) * (float)chart_data->heightChart) + chart_data->yStartPosChart;
            }
            itemPoint->lastXPoint -= chart_data->xMoveDelta;
            if (itemPoint->yPoint != -1)
                Paint_DrawPoint(itemPoint->lastXPoint, itemPoint->yPoint, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
        }
        Paint_DrawPoint(newPoint.lastXPoint, newPoint.yPoint, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    }
}

static void update_sdCardInfo_Widget(WidgetData *widget)
{
    sd_card_state_t sdcard_state;
    if (device_state_get_member(&sdcard_state, device_state_statusSDCard, true))
    {
        if (sdcard_state.status & SD_CARD_STATE_ERROR_INIT)
        {
            char *buffer = "SD Card not mounted";
            uint16_t textWidth = getTextWidth(buffer, &Font12);
            DrawTextWithOffset(widget, buffer, (widget->w - textWidth) / 2, widget->h / 2, &Font12, TRUE);
            return;
        }
        uint32_t startPos = widget->x + 10;
        uint32_t widthProgressBar = widget->w - 60;
        Paint_ClearWindows(widget->x + 5, widget->y + 25, widget->x + widget->w - 5, widget->y + widget->h - 5, WHITE);
        Paint_DrawRectangle(startPos, widget->y + 25, startPos + widthProgressBar, widget->y + 45, BLACK, DOT_PIXEL_1X1, FALSE);
        double percentage = 1.0f - ((double)sdcard_state.freeSpace / (double)sdcard_state.maxSpace);
        uint32_t filledPartWidth = percentage * widthProgressBar;
        Paint_DrawRectangle(startPos, widget->y + 25, startPos + filledPartWidth, widget->y + 45, BLACK, DOT_PIXEL_1X1, TRUE);
        char buffer[50];
        snprintf(buffer, 10, "%i%%", (int)(percentage * 100));
        Paint_DrawString_EN(startPos + widthProgressBar + 7, widget->y + 29, buffer, &Font16, WHITE, BLACK, true);
        if (sdcard_state.status & SD_CARD_STATE_FLIGHTLOGGING)
            snprintf(buffer, 50, "Flight Logger started");
        else
            snprintf(buffer, 50, "Flight Logger disabled");
        uint16_t textWidth = getTextWidth(buffer, &Font12);
        Paint_DrawString_EN(widget->x + widget->w / 2 - textWidth / 2, widget->y + 55, buffer, &Font12, WHITE, BLACK, true);
        if (sdcard_state.status & SD_CARD_STATE_IGC_LOGGING)
            snprintf(buffer, 50, "IGC Logger started");
        else
            snprintf(buffer, 50, "IGC Logger disabled");
        textWidth = getTextWidth(buffer, &Font12);
        DrawTextWithOffset(widget, buffer, widget->w / 2 - textWidth / 2, 70, &Font12, true);
        Paint_DrawString_EN(widget->x + widget->w / 2 - textWidth / 2, widget->y + 70, buffer, &Font12, WHITE, BLACK, true);
    }
}

static void update_gpsInfo_Widget(WidgetData *widget)
{
    gps_t gps_data;
    vario_state_get_member(&gps_data, vario_state_gps_data, true);

    char buffer[50];
    if (fabsf(gps_data.latitude - oldGpsObject.latitude) > 0.01f)
    {
        snprintf(buffer, 49, "Lat: %.2f", gps_data.latitude);
        DrawTextWithOffset(widget, buffer, 5, 25, &Font12, true);
    }
    if (fabsf(gps_data.longitude - oldGpsObject.longitude) > 0.01f)
    {
        snprintf(buffer, 49, "Lon: %.2f", gps_data.longitude);
        DrawTextWithOffset(widget, buffer, 90, 25, &Font12, true);
    }
    if (fabsf(gps_data.altitude - oldGpsObject.altitude) > 0.01f)
    {
        snprintf(buffer, 49, "Height: %.2fm", gps_data.altitude);
        DrawTextWithAlignOffset(widget, buffer, widget->w - 5, 25, &Font12, true, ALIGN_RIGHT, ALIGN_TOP);
    }
    if (gps_data.fix != oldGpsObject.fix || gps_data.fix_mode != oldGpsObject.fix_mode || gps_data.valid != oldGpsObject.valid)
    {
        DrawTextWithOffset(widget, gps_data.valid ? "1" : "0", 5, 50, &Font12, true);
        switch (gps_data.fix)
        {
        case GPS_FIX_INVALID:
            snprintf(buffer, 50, "Fix: Invalid");
            break;
        case GPS_FIX_GPS:
            snprintf(buffer, 50, "Fix: GPS");
            break;
        case GPS_FIX_DGPS:
            snprintf(buffer, 50, "Fix: DGPS");
            break;
        default:
            snprintf(buffer, 50, "None");
            break;
        }
        DrawTextWithOffset(widget, buffer, 40, 50, &Font12, true);
        switch (gps_data.fix_mode)
        {
        case GPS_MODE_INVALID:
            snprintf(buffer, 50, "Fix M: Invalid");
            break;
        case GPS_MODE_2D:
            snprintf(buffer, 50, "Fix M: 2D");
            break;
        case GPS_MODE_3D:
            snprintf(buffer, 50, "Fix: 3D");
            break;
        default:
            snprintf(buffer, 50, "Fix M: Invalid");
            break;
        }
        DrawTextWithAlignOffset(widget, buffer, widget->w - 20, 50, &Font12, true, ALIGN_RIGHT, ALIGN_TOP);
    }
    if (gps_data.sats_in_view != oldGpsObject.sats_in_view || gps_data.sats_in_use != oldGpsObject.sats_in_use)
    {
        DrawTextWithOffset(widget, "--Satellits--", 5, 65, &Font12, true);
        uint16_t currentPosY = 80;

        gps_data.sats_in_view = 12;
        for (size_t i = 0; i < gps_data.sats_in_view; i++)
        {
            // gps_satellite_t sat = gps_data.sats_desc_in_view[i];
            gps_satellite_t sat = {
                .num = i,
            };
            snprintf(buffer, 50, "ID: %u", sat.num);
            for (size_t y = 0; y < gps_data.sats_in_use; y++)
            {
                // TODO: Check if this is right, perhaps in num of satellit
                if (gps_data.sats_id_in_use[y] == i)
                {
                    strcat(buffer, "(in use)");
                }
            }
            if (i % 2 == 0)
            {
                strcat(buffer, "(in use)");
            }
            DrawTextWithOffset(widget, buffer, 20, currentPosY, &Font8, true);
            currentPosY += 9;
        }
    }

    if (fabsf(gps_data.dop_h - oldGpsObject.dop_h) > 0.1f)
    {
        snprintf(buffer, 50, "H: %.2f", gps_data.dop_h);
        DrawTextWithOffset(widget, buffer, 200, 75, &Font12, true);
    }
    if (fabsf(gps_data.dop_v - oldGpsObject.dop_v) > 0.1f)
    {
        snprintf(buffer, 50, "V: %.2f", gps_data.dop_v);
        DrawTextWithOffset(widget, buffer, 200, 87, &Font12, true);
    }
    if (fabsf(gps_data.dop_p - oldGpsObject.dop_p) > 0.1f)
    {
        snprintf(buffer, 50, "P: %.2f", gps_data.dop_p);
        DrawTextWithOffset(widget, buffer, 200, 99, &Font12, true);
    }

    if (fabsf(gps_data.cog - oldGpsObject.cog) > 0.1f)
    {
        snprintf(buffer, 50, "COG: %.2f", gps_data.cog);
        DrawTextWithOffset(widget, buffer, 130, 130, &Font12, true);
    }
    if (fabsf(gps_data.variation - oldGpsObject.variation) > 0.1f)
    {
        snprintf(buffer, 50, "Variation: %.2f", gps_data.variation);
        DrawTextWithOffset(widget, buffer, 130, 150, &Font12, true);
    }
    if (fabsf(gps_data.speed - oldGpsObject.speed) > 0.1f)
    {
        snprintf(buffer, 50, "Speed: %.2f", gps_data.speed);
        DrawTextWithOffset(widget, buffer, 130, 170, &Font12, true);
    }
    oldGpsObject = gps_data;
}

static void update_Airspace_Widget(WidgetData *widget)
{
    Airspace_Info_t airspaceInfo;
    bool stateChange = vario_state_get_member(&airspaceInfo, vario_state_airspaceInfo, true);
    if (!stateChange)
        return;
    add_Airspace_Widget(widget);
}

static void update_TrackPoint_Widget(WidgetData *widget)
{
    const float ZOOM_FACTOR = 0.6;
    int heading;
    vario_state_get_member(&heading, vario_state_heading, false);
    float theta = DEGREE_TO_RADIAN * (180 - heading);
    flight_stats_t *flightStats = flight_stats_take_trackPoints();
    if (flightStats == NULL)
        return;
    Paint_ClearWindows(widget->x + 1, widget->y + 1, widget->x + widget->w - 1, widget->y + widget->h - 1, WHITE);
    float cosT = cosf(theta);
    float sinT = sinf(theta);
    for (int i = flightStats->rearPoint; i != flightStats->frontPoint;)
    {
#if 1 // 0 clock is heading
        float x0 = flightStats->trackDistance[i].dx * ZOOM_FACTOR;
        float y0 = flightStats->trackDistance[i].dy * ZOOM_FACTOR;

        int16_t x = (widget->x + widget->w / 2) - (int16_t)(x0 * cosT - y0 * sinT);
        int16_t y = (widget->y + widget->h / 2) + (int16_t)(x0 * sinT + y0 * cosT);
#else // 0 clock is north
        int16_t x = widget->x + widget->w / 2;
        int16_t y = widget->y + widget->h / 2;

        x -= flightStats->trackDistance[i].dx * ZOOM_FACTOR;
        y += flightStats->trackDistance[i].dy * ZOOM_FACTOR;
#endif

        if (widget->x + 2 < x && widget->y + 2 < y && x < widget->x + widget->w - 2 && y < widget->y + widget->h - 2)
        {
            if (flightStats->trackPoints[i].vario < 0)
            {
                Paint_DrawRectangle(x - 1, y - 1, x + 1, y + 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
            }
            else
            {
                int16_t r = 2;

                if (flightStats->trackPoints[i].vario > 1)
                    r = (flightStats->trackPoints[i].vario > 2) ? 4 : 3;

                Paint_DrawCircle(x, y, r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
            }
        }

        i = (i + 1) % MAX_TRACKPOINT_SIZE;
    }

    // draw glider
    {
        int16_t cx = widget->x + widget->w / 2;
        int16_t cy = widget->y + widget->h / 2;

        Paint_DrawLine(cx - 10, cy, cx, cy - 18, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(cx, cy - 18, cx + 10, cy, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
}

void vario_display_add_widget(WidgetData *widget)
{
    switch (widget->style)
    {
    case Widget_Empty:
        break;
    case Widget_TextBox:
        add_TextBox_Widget(widget);
        break;
    case Widget_Compass:
        add_Compass_widget(widget);
        break;
    case Widget_Chart:
        add_Chart_Widget(widget);
        break;
    case Widget_SdCardInfo:
        add_sdCardInfo_Widget(widget);
        break;
    case Widget_GPSInfo:
        add_gpsInfo_Widget(widget);
        break;
    case Widget_Airspace:
        add_Airspace_Widget(widget);
        break;
    case Widget_TrackPoints:
        add_TrackPoint_Widget(widget);
    default:
        break;
    }
}

void vario_update_widget(WidgetData *widget)
{
    switch (widget->style)
    {
    case Widget_Empty:
        break;
    case Widget_TextBox:
        update_TextBox_Widget(widget);
        break;
    case Widget_Compass:
        update_Compass_Widget(widget);
        break;
    case Widget_Chart:
        update_Chart_Widget(widget);
        break;
    case Widget_SdCardInfo:
        update_sdCardInfo_Widget(widget);
        break;
    case Widget_GPSInfo:
        update_gpsInfo_Widget(widget);
        break;
    case Widget_Airspace:
        update_Airspace_Widget(widget);
        break;
    case Widget_TrackPoints:
        update_TrackPoint_Widget(widget);
        break;
    default:
        break;
    }
}
