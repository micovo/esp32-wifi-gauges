#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "nvs_flash.h"

extern uint8_t Value1GaugeWarning;
extern uint8_t Value2GaugeWarning;
extern float Value3GaugeWarning;

extern uint8_t Value1Alarm;
extern uint8_t Value2Alarm;
extern float Value3Alarm;

static const char *TAG = "nvs";

void nvs_init(void)
{
    ESP_LOGI(TAG, "NVS initialization");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else 
    {
        ESP_LOGI(TAG, "Reading values from NVS");

        uint8_t intValue = 0; 

        err = nvs_get_u8(my_handle, "Value1GaugeWarning", &intValue);
        if (err == ESP_OK) Value1GaugeWarning = intValue;
        
        err = nvs_get_u8(my_handle, "Value2GaugeWarning", &intValue);
        if (err == ESP_OK) Value2GaugeWarning = intValue;

        err = nvs_get_u8(my_handle, "value3GaugeWarning", &intValue);
        if (err == ESP_OK) Value3GaugeWarning = intValue / 10.0f;

        err = nvs_get_u8(my_handle, "Value1Alarm", &intValue);
        if (err == ESP_OK) Value1Alarm = intValue;

        err = nvs_get_u8(my_handle, "Value2Alarm", &intValue);
        if (err == ESP_OK) Value2Alarm = intValue;
        
        err = nvs_get_u8(my_handle, "Value3Alarm", &intValue);
        if (err == ESP_OK) Value3Alarm = intValue / 10.0f;

        nvs_close(my_handle);
    }
}

void nvs_write(void)
{
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else 
    {
        err = nvs_set_u8(my_handle, "Value1GaugeWarning", Value1GaugeWarning);
        err = nvs_set_u8(my_handle, "Value3GaugeWarning", Value2GaugeWarning);
        err = nvs_set_u8(my_handle, "Value1Alarm", Value1Alarm);
        err = nvs_set_u8(my_handle, "Value2Alarm", Value2Alarm);

        uint8_t int_value;
        
        int_value = Value3Alarm * 10;
        err = nvs_set_u8(my_handle, "Value3Alarm", int_value);
        int_value = Value2GaugeWarning * 10;
        err = nvs_set_u8(my_handle, "Value2GaugeWarning", int_value);

        err = nvs_commit(my_handle);
        if(err != ESP_OK) 
        {
            ESP_LOGE(TAG, "NVS Commit Failed");
        }
        else
        {
            ESP_LOGI(TAG, "NVS Commit OK");
        }

        // Close
        nvs_close(my_handle);
    }
 }