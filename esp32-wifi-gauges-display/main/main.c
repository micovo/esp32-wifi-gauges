#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include <math.h>
#include "driver/gpio.h"
#include "gauge.h"
#include "display.h"
#include "_config.h"

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t alarm = 0;
uint8_t alarmBuzzerTimer = 0;
uint8_t selectedValue = 0;
GaugeData_t GaugeData;

uint32_t alarmBuzzerMuteTimer = 0;

// Default values are used if not values are stored in NVS

uint8_t Value1GaugeWarning = 110;
uint8_t Value2GaugeWarning = 110;
float Value3GaugeWarning = 7.0;

uint8_t Value1Alarm = 140;
uint8_t Value2Alarm = 140;
float Value3Alarm = 1.0f;

/////////////////////////////////////////////////////////////////////////////////////////

#define ALARM_BUZZER_MUTE_TICKS     (30000 / portTICK_PERIOD_MS)

/////////////////////////////////////////////////////////////////////////////////////////

void nvs_init(void);

void udp_client_init(void);
void wifi_client_init(void);

uint8_t wifi_is_connected(void);

uint8_t menu_is_active(void);
void menu_check_buttons(void);
void menu_iteration(void);

/////////////////////////////////////////////////////////////////////////////////////////

#define LIMIT(value, min, max) { value = value > max ? max : value; value = value < min ? min : value; }

/////////////////////////////////////////////////////////////////////////////////////////

void process_sensor_data(void)
{
    if (wifi_is_connected() == 0)
    {
        display_update(255, 255, 255, 255, 9.9f, 9.9f, 1, 0, u"NO DATA");
        return;
    }
    
    if (GaugeData.Magic == GAUGE_DATA_MAGIC_VALUE)
    {
        float WaterTemperature = GaugeData.WaterTemperature;
        float OilTemperature = GaugeData.OilTemperature;
        float OilPressure = GaugeData.OilPressure;
        
        LIMIT(WaterTemperature, 0.0f, 150.0f);
        LIMIT(OilTemperature, 0.0f, 150.0f);
        LIMIT(OilPressure, 0.0f, 9.9f);

        alarm = 0;
        if ((Value1Alarm != 0) && (WaterTemperature >= Value1Alarm))
        {
            alarm = 1;
        }
        
        if ((Value2Alarm != 0) && (OilTemperature >= Value2Alarm))
        {
            alarm = 1;
        }
        
        if ((Value3Alarm > 0.0f) && (OilPressure <= Value3Alarm))
        {
            alarm = 1;
        }

        display_update(
            (uint8_t)round(GaugeData.WaterTemperature), Value1GaugeWarning,
            (uint8_t)round(GaugeData.OilTemperature), Value2GaugeWarning, 
            GaugeData.OilPressure, Value3GaugeWarning,
            alarm, selectedValue,
            u"");

        if (alarm)
        {
            if (alarmBuzzerMuteTimer < xTaskGetTickCount())
            {
                if (alarmBuzzerTimer == 0)
                {
                    gpio_set_level(BUZZER_GPIO, 1);
                }

                alarmBuzzerTimer++;
                if (alarmBuzzerTimer == 5)
                {
                    alarmBuzzerTimer = 0;
                }
            }
            else
            {
                gpio_set_level(BUZZER_GPIO, 0);
            }

            if ((gpio_get_level(BUTTON_TOP_GPIO) == 0) || (gpio_get_level(BUTTON_BOTTOM_GPIO) == 0))
            {
                alarmBuzzerMuteTimer = xTaskGetTickCount() + ALARM_BUZZER_MUTE_TICKS;
            }
        }
        else
        {
            gpio_set_level(BUZZER_GPIO, 0);
            alarmBuzzerTimer = 0;
        }
        
        //Mark buffer as invalid
        GaugeData.Magic = 0;
    }
}

void app_main()
{
    // Load stored alarm and warning values
    nvs_init();

    display_init();

    wifi_client_init();
    udp_client_init();

    //Postpone alarm on device power-up
    alarmBuzzerMuteTimer = xTaskGetTickCount() + ALARM_BUZZER_MUTE_TICKS;

    gpio_pad_select_gpio(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(BUTTON_TOP_GPIO);
    gpio_set_direction(BUTTON_TOP_GPIO, GPIO_MODE_INPUT);

    gpio_pad_select_gpio(BUTTON_BOTTOM_GPIO);
    gpio_set_direction(BUTTON_BOTTOM_GPIO, GPIO_MODE_INPUT);

    while (1) 
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (menu_is_active() == 0)
        {
            process_sensor_data();

            menu_check_buttons();
        }
        else
        {
            menu_iteration();
        }
    }
}
