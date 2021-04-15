#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <font6x9.h>
#include <font5x7.h>
#include <font10x20.h>
#include <aps.h>
#include <fps.h>
#include <hagl_hal.h>
#include <hagl.h>
#include <math.h>

static const char *TAG = "display";
static EventGroupHandle_t event;

static const uint8_t RENDER_FINISHED = (1 << 0);
static const uint8_t FLUSH_STARTED = (1 << 1);

wchar_t message[128];
uint8_t blink = 0;

color_t blue;
color_t green;
color_t black;
color_t white;
color_t gray;
color_t red;
color_t yellow;
color_t orange;

static uint8_t Value1GaugeMin = 80;
static uint8_t Value1GaugeMax = 140;
static uint8_t Value2GaugeMin = 80;
static uint8_t Value2GaugeMax = 140;
static float Value3GaugeMin = 0.0f;
static float Value3GaugeMax = 9.9f;

#define GAUGES_MIN_HEIGHT    4
#define GAUGES_MAX_HEIGHT    44

uint8_t display_calculate_gauge_height(float min, float max, float value)
{
    if (value < min) value = min;
    if (value > max) value = max;

    float height = (GAUGES_MAX_HEIGHT - GAUGES_MIN_HEIGHT) * ((value - min) / (max-min)) ;
    height += GAUGES_MIN_HEIGHT;

    return (uint8_t)ceil(height);
}

/*
 * Flushes the backbuffer to the display. Needed when using
 * double or triple buffering.
 */
void flush_task(void *params)
{
    while (1) 
    {
        EventBits_t bits = xEventGroupWaitBits(
            event,
            RENDER_FINISHED,
            pdTRUE,
            pdFALSE,
            0
        );

        /* Flush only when RENDER_FINISHED is set. */
        if ((bits & RENDER_FINISHED) != 0 ) {
            xEventGroupSetBits(event, FLUSH_STARTED);
            hagl_flush();
        }
    }

    vTaskDelete(NULL);
}

uint8_t getNumberOfDigits(uint8_t value)
{
    if (value < 10)
    {
        return 1;
    }
    else if (value < 100)
    {
        return 2;
    }
    
    return 3;
}

void display_update(uint8_t value1, uint8_t value1GaugeWarning, 
                    uint8_t value2, uint8_t value2GaugeWarning, 
                    float value3, float value3GaugeWarning, 
                    uint8_t alarm, uint8_t selectedValue,
                    wchar_t * menuMessage)
{
    color_t fcolor = white;
    color_t bgcolor = black;

    uint8_t column1 = 15;
    uint8_t column2 = column1 + 60 + 15;
    uint8_t column3 = column2 + 60 + 15;

    uint8_t valuesFontWidth = 20;
    uint8_t valuesFontHeight = 50;
    const unsigned char * valuesFont = font10x20;

    if (blink == 0)
    {
        blink = 1;
    }
    else
    {
        blink = 0;
    }

    if ((alarm) && (blink == 0) && (selectedValue == 0))
    {
        bgcolor = red; 
    }
    else
    {  
        bgcolor = black; 
    }
        
    //Background
    hagl_fill_rectangle(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, bgcolor);
    
    //Offset for center alignement
    uint8_t value1LeftOffset = ((3 - getNumberOfDigits(value1)) * 10);
    uint8_t value2LeftOffset = ((3 - getNumberOfDigits(value2)) * 10);

    //Values
    if ((selectedValue == 0) || (selectedValue == 1))
    {
        if (value1 < 1)
        {
            value1LeftOffset = 0;
            swprintf(message, sizeof(message), u"OFF");
        }
        else
        {
            swprintf(message, sizeof(message), u"%u ", (uint32_t)value1);
        }
        
        hagl_put_text(message, column1 + value1LeftOffset, 0, fcolor, bgcolor, valuesFont, valuesFontWidth, valuesFontHeight);
    }

    if ((selectedValue == 0) || (selectedValue == 2))
    {
        if (value2 < 1)
        {
            value2LeftOffset = 0;
            swprintf(message, sizeof(message), u"OFF");
        }
        else
        {
            swprintf(message, sizeof(message), u"%u ", (uint32_t)value2);
        }

        hagl_put_text(message, column2 + value2LeftOffset, 0, fcolor, bgcolor, valuesFont, valuesFontWidth, valuesFontHeight);
    }

    if ((selectedValue == 0) || (selectedValue == 3))
    {
        if (value3 < 0.0f)
        {
            swprintf(message, sizeof(message), u"OFF");
        }
        else
        {
            swprintf(message, sizeof(message), u"%.01f", value3);
        }

        hagl_put_text(message, column3, 0, fcolor, bgcolor, valuesFont, valuesFontWidth, valuesFontHeight);
    }

    //Description
    hagl_put_text(u"WATER", column1 + 5, 44, fcolor, bgcolor, font10x20, 10, 20);
    hagl_put_text(u" OIL ", column2 + 5, 44, fcolor, bgcolor, font10x20, 10, 20);
    hagl_put_text(u" OIL ", column3 + 5, 44, fcolor, bgcolor, font10x20, 10, 20);
    hagl_put_text(u"TEMP", column1 + 10, 64, fcolor, bgcolor, font10x20, 10, 20);
    hagl_put_text(u"TEMP", column2 + 10, 64, fcolor, bgcolor, font10x20, 10, 20);
    hagl_put_text(u"PRES", column3 + 10, 64, fcolor, bgcolor, font10x20, 10, 20);

    uint8_t len = wcslen(menuMessage);
    if (len == 0)
    {
        //Gauges background 
        hagl_fill_rectangle(column1, DISPLAY_HEIGHT - (GAUGES_MAX_HEIGHT + 2), column1 + 60, DISPLAY_HEIGHT - 2, black);
        hagl_fill_rectangle(column2, DISPLAY_HEIGHT - (GAUGES_MAX_HEIGHT + 2), column2 + 60, DISPLAY_HEIGHT - 2, black);
        hagl_fill_rectangle(column3, DISPLAY_HEIGHT - (GAUGES_MAX_HEIGHT + 2), column3 + 60, DISPLAY_HEIGHT - 2, black);

        //Gauges border 
        hagl_draw_rectangle(column1, DISPLAY_HEIGHT - (GAUGES_MAX_HEIGHT + 2), column1 + 60, DISPLAY_HEIGHT - 2, gray);
        hagl_draw_rectangle(column2, DISPLAY_HEIGHT - (GAUGES_MAX_HEIGHT + 2), column2 + 60, DISPLAY_HEIGHT - 2, gray);
        hagl_draw_rectangle(column3, DISPLAY_HEIGHT - (GAUGES_MAX_HEIGHT + 2), column3 + 60, DISPLAY_HEIGHT - 2, gray);

        //Gauges
        color_t gauge1Color = (value1 < value1GaugeWarning) ? ((value1 < value1GaugeWarning/2) ? blue: green) : orange;
        color_t gauge2Color = (value2 < value2GaugeWarning) ? ((value2 < value2GaugeWarning/2) ? blue: green) : orange;
        color_t gauge3Color = (value3 < value3GaugeWarning) ? ((value3 < value3GaugeWarning/2) ? blue: green) : orange;

        uint8_t gauge1HeightPixels = display_calculate_gauge_height(Value1GaugeMin, Value1GaugeMax, value1);
        uint8_t gauge2HeightPixels = display_calculate_gauge_height(Value2GaugeMin, Value2GaugeMax, value2);
        uint8_t gauge3HeightPixels = display_calculate_gauge_height(Value3GaugeMin, Value3GaugeMax, value3);
        
        hagl_fill_rectangle(column1 + 2, DISPLAY_HEIGHT - gauge1HeightPixels, column1 + 60 - 2, DISPLAY_HEIGHT - 4, gauge1Color);
        hagl_fill_rectangle(column2 + 2, DISPLAY_HEIGHT - gauge2HeightPixels, column2 + 60 - 2, DISPLAY_HEIGHT - 4, gauge2Color);
        hagl_fill_rectangle(column3 + 2, DISPLAY_HEIGHT - gauge3HeightPixels, column3 + 60 - 2, DISPLAY_HEIGHT - 4, gauge3Color);
    }
    else
    {
        uint8_t len = wcslen(menuMessage);
        if (len <= 12)
        {
            uint8_t menuMessageOffset = (12 - len) * (valuesFontWidth / 2);
            hagl_put_text(menuMessage, menuMessageOffset, 85, fcolor, bgcolor, valuesFont, valuesFontWidth, valuesFontHeight);
        }
        else
        {
            ESP_LOGE(TAG, "Menu message is too long");
        }
    }

    // Notify flush task that rendering has finished.
    xEventGroupSetBits(event, RENDER_FINISHED);
}

void display_init()
{
    blue = hagl_color(0, 50, 200);
    green = hagl_color(0, 200, 50);
    black = hagl_color(0, 0, 0);
    white = hagl_color(255, 255, 255);
    gray = hagl_color(127, 127, 127);
    red = hagl_color(255, 0, 0);
    yellow = hagl_color(255, 255, 0);
    orange = hagl_color(255, 127, 0);

    ESP_LOGI(TAG, "Heap when starting: %d", esp_get_free_heap_size());

    event = xEventGroupCreate();

    hagl_init();
    hagl_set_clip_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

    ESP_LOGI(TAG, "Heap after HAGL init: %d", esp_get_free_heap_size());

    xTaskCreatePinnedToCore(flush_task, "Flush", 4096, NULL, 1, NULL, 0);
}