#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include "gauge.h"
#include "display.h"
#include "_config.h"

#define MENU_VALUE_STEP  5

extern uint8_t Value1GaugeWarning;
extern uint8_t Value2GaugeWarning;
extern float Value3GaugeWarning;

extern uint8_t Value1Alarm;
extern uint8_t Value2Alarm;
extern float Value3Alarm;

static uint8_t menuModeActive = 0;
static uint8_t selectedValue = 0;

void nvs_write(void);

static void wait_for_buttons_release(void)
{
    vTaskDelay(50 / portTICK_PERIOD_MS);

    while ((gpio_get_level(BUTTON_TOP_GPIO) == 0) || (gpio_get_level(BUTTON_BOTTOM_GPIO) == 0))
    {
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void SaveSettings(void)
{
    nvs_write();
}

uint8_t menu_is_active(void)
{
    return menuModeActive != 0;
}

void menu_check_buttons(void)
{
    if ((gpio_get_level(BUTTON_TOP_GPIO) == 0) && (gpio_get_level(BUTTON_BOTTOM_GPIO) == 0))
    {
        menuModeActive = 1;
        selectedValue = 1;

        wait_for_buttons_release();
    }
}

void menu_iteration(void)
{
    if (menuModeActive == 1)
    {
        display_update(Value1Alarm, 255, Value2Alarm, 255, Value3Alarm, 0.0f, 0, selectedValue, u"Set Alarm");

        if ((gpio_get_level(BUTTON_TOP_GPIO) == 0))
        {
            selectedValue++;

            if (selectedValue >= 4)
            {
                menuModeActive = 2;
                selectedValue = 1;
            }
            wait_for_buttons_release();
        }

        if ((gpio_get_level(BUTTON_BOTTOM_GPIO) == 0))
        {
            if (selectedValue == 1)
            {
                if (Value1Alarm == 0)
                {
                    Value1Alarm = 80 - MENU_VALUE_STEP;
                }

                Value1Alarm += MENU_VALUE_STEP;

                if (Value1Alarm > 150)
                {
                    Value1Alarm = 0;
                }
            }
            else if (selectedValue == 2)
            {
                if (Value2Alarm == 0)
                {
                    Value2Alarm = 80 - MENU_VALUE_STEP;
                }

                Value2Alarm += MENU_VALUE_STEP;

                if (Value2Alarm > 150)
                {
                    Value2Alarm = 0;
                }
            }
            else if (selectedValue == 3)
            {
                if (Value3Alarm < 0)
                {
                    Value3Alarm = 0.0f;
                }

                Value3Alarm += 0.1f;

                if (Value3Alarm > 9.9f)
                {
                    Value3Alarm = -1.0f;
                }
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
    else if (menuModeActive == 2)
    {
        display_update(Value1GaugeWarning, 255, Value2GaugeWarning, 255, Value3GaugeWarning, 0.0f, 0, selectedValue, u"Set Warning");

        if ((gpio_get_level(BUTTON_TOP_GPIO) == 0))
        {
            selectedValue++;

            if (selectedValue >= 4)
            {
                menuModeActive = 0;
                selectedValue = 0;
                
                display_update(255, 255, 255, 255, 0.0f, 0.0f, 0, 4, u"Saving");
                vTaskDelay(500 / portTICK_PERIOD_MS);
                SaveSettings();
            }
            wait_for_buttons_release();
        }

        if ((gpio_get_level(BUTTON_BOTTOM_GPIO) == 0))
        {
            if (selectedValue == 1)
            {
                Value1GaugeWarning += 5;

                if (Value1GaugeWarning > 150)
                {
                    Value1GaugeWarning = 80;
                }
            }
            else if (selectedValue == 2)
            {
                Value2GaugeWarning += 5;

                if (Value2GaugeWarning > 150)
                {
                    Value2GaugeWarning = 80;
                }
            }
            else if (selectedValue == 3)
            {
                Value3GaugeWarning += 0.1f;

                if (Value3GaugeWarning > 9.9f)
                {
                    Value3GaugeWarning = 0.0;
                }
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}