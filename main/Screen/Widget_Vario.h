#ifndef __WIDGETVARIO_H
#define __WIDGETVARIO_H

#include <stdint.h>
#include "FIFO.h"
#include <stdio.h>
#include <esp_err.h>
#include "DeviceContext.h"

#define ChartNumDataPoints 20

#define WS_FONT_MASK_LABEL 0x0000000F
#define WS_FONT_8_LABEL 0x00000000
#define WS_FONT_12_LABEL 0x00000001
#define WS_FONT_16_LABEL 0x00000002
#define WS_FONT_20_LABEL 0x00000003
#define WS_FONT_24_LABEL 0x00000004

#define WS_FONT_MASK_CONTENT 0x000000F0
#define WS_FONT_8_CONTENT 0x00000000
#define WS_FONT_12_CONTENT 0x00000010
#define WS_FONT_16_CONTENT 0x00000020
#define WS_FONT_20_CONTENT 0x00000030
#define WS_FONT_22_CONTENT 0x00000040
#define WS_FONT_24_CONTENT 0x00000050
#define WS_FONT_26_CONTENT 0x00000060
#define WS_FONT_28_CONTENT 0x00000070
#define WS_FONT_30_CONTENT 0x00000080
#define WS_FONT_32_CONTENT 0x00000090

typedef enum
{
    Button_Direction_up,
    Button_Direction_down,
} Button_Direction;

typedef enum
{
    Settings_Page_PageButtons,
    Settings_Page_Audio,
    Settings_Page_Config,
    Settings_Page_Flights,
    Settings_Page_FlightStatistics,
} Settings_Page_t;

// Wie sieht das Widget aus (TextBox, Compass, etc.)
typedef enum
{
    Widget_Empty,
    Widget_TextBox,
    Widget_Compass,
    Widget_Chart,
    Widget_SdCardInfo,
    Widget_GPSInfo,
    Widget_Airspace,
    Widget_TrackPoints,
    /*Settings Widget*/
    Widget_Settings_PageButton,
    Widget_settings_BackButton,
    Widget_Settings_Slider,
    Widget_Settings_Checkbox,
    Widget_Settings_SoundThreshold,
    Widget_Settings_CalibrationPageButton,
    Widget_Settings_FlightListElement,
} WidgetStyle;

// was ist im Widget dargestellt (z.B. Vario_Active in TextBox)
typedef enum
{
    /* Empty Widget */
    WidgetContent_Empty,

    /* Text Box Widgets / Chart Widgets */
    WidgetContent_Speed_Ground,
    WidgetContent_Vario_Active,
    WidgetContent_Vario_Lazy,
    WidgetContent_VertAcc,
    WidgetContent_Pressure,
    WidgetContent_Temperature,
    WidgetContent_Altitude_Baro, // QNE
    WidgetContent_Altitude_GPS,  // QNH
    WidgetContent_Altitude_AGL,  // QFE
    WidgetContent_Time_GPS,

    /* Sd Card Info Widget */
    WidgetContent_SDCardInfo,

    /* Compass Widgets */
    WidgetContent_CompassSensor,

    /* GPSInfo Widgets */
    WidgetContent_GPSInfo,

    /* Airspace Widget */
    WidgetContent_Airspace,

    /* Trackpoint Widgets */
    WidgetContent_TrackPoints,

    /* Settings Widget */
    WidgetContent_Settings_PageButton,
    WidgetContent_Settings_PageButton_Audio,
    WidgetContent_Settings_PageButton_Config,
    WidgetContent_Settings_PageButton_Flights,
    WidgetContent_Settings_VolumeSlider,
    WidgetContent_Settings_VertVelLazySlider,
    WidgetContent_Settings_SoundLiftThreshold,
    WidgetContent_Settings_SoundSinkThreshold,
    WidgetContent_Settings_MagnCalibrationPageButton,
    WidgetContent_Settings_FlightListElement,
} WidgetContentType;

typedef struct
{
    WidgetStyle style;
    uint32_t extraStyle;
    WidgetContentType type;
    uint16_t x, y, w, h;
    void *extr_data; // Widget_specific_data
} WidgetData;

/***** Text Box Widget Data *****/
typedef struct
{
    uint32_t linkerRandUnitText;
    uint32_t yStartValueText;
} TextBox_Widget_Data;

/***** Chart Widget Data ******/
typedef struct
{
    uint32_t differenzToShow; // in Meters
    int32_t startValue;       // Start Height from Chart in Meters (height at the bottom)
    uint32_t timePeriod;      // In Seconds
    uint64_t lastPointTime;   // When a point was added last Time
    fifo_t data;              // Fifo with all the data from Chart_point
    uint32_t startXPosition;  // most right Position, where new Points will be spawned
    uint32_t xMoveDelta;      // how much each Point will be moved left, when a new Point is spawned
    uint32_t heightChart;     // In Pixel
    uint32_t yStartPosChart;  // in Pixel, where the Chart does start
    uint32_t pointsAdded;     // How many Points are added tp the chart
} Chart_Widget_Data;
typedef struct
{
    int yPoint;
    int lastXPoint;
    float heightValue;
} Chart_Point;

/***** Compass Widget Data *****/
typedef struct
{
    int32_t lastAngle;
    int32_t angleDifferenceToShow;
} Compass_Widget_Data;

/***** Flight Widget Settings Data *****/
typedef struct FlightList_Widget_Data
{
    char FlightName[255];
} FlightList_Widget_Data;

typedef enum
{
    PopUpType_GPSFix = 0,
    PopUpType_BatteryWarning = 1,
    PopUpType_WidgetSettings = 2,
    PopUpType_TakeOff = 3,
    PopUpType_Landing = 4,
} PopUpType;

typedef struct
{
    uint16_t x, y, w, h;
    float timeToDisplay; // ms
    PopUpType type;
    WidgetData *widgetToEdit; // needs to be set, if PopUpType_WidgetSettings
} PopUpConfig_t;

typedef struct
{
    void (*returnFunction)(WidgetData *);
    WidgetData *widget;
} ReturnFunctionData_t;

void vario_display_add_widget(WidgetData *widget);
void vario_update_widget(WidgetData *widget);

/*** Settings Implemented in "Widget_Vario_Settings.c" ***/
void vario_display_add_settings_widget(WidgetData *widget, Settings_Page_t currPage, bool hovered, bool selected, bool reRenderRectangle);
void vario_display_select_settings_widget(WidgetData *widget, Settings_Page_t currPage, bool hovered, bool selected);
void vario_display_settings_widget_handle_input(WidgetData *widget, Button_Direction direction);

/*** PopUps Implemented in "Widget_Vario_PopUp.c" ***/
esp_err_t vario_display_showPopUp(PopUpConfig_t configData);

/*** Screens Implemented in "Widget_Vario_Screens.c" ***/
void vario_display_PowerOffScreen();
void vario_display_FlightStatistics(void *params);

#endif
