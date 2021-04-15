#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <sys/param.h>
#include <esp_http_server.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include <math.h>
#include "gauge.h"
#include "_config.h"

/////////////////////////////////////////////////////////////////////////////////////////

static const adc_channel_t channels[] = {ADC_WATER_TEMP, ADC_OIL_TEMP, ADC_OIL_PRESSURE};   

GaugeData_t GaugeData;

float waterTempSensorVoltage;
float oilTempSensorVoltage;
float oilPressureSensorVoltage;

float waterTemp;
float oilTemp;
float oilPressure;
/////////////////////////////////////////////////////////////////////////////////////////

void spiffs_init(void);
void wifi_init_softap(void);
httpd_handle_t start_webserver(void);

void udp_sender_init(void);
void udp_sender_set_payload(const char * payloadBuffer, uint32_t payloadBufferSize);

void adc_init(void);
float adc_measure(adc_channel_t channel);

/////////////////////////////////////////////////////////////////////////////////////////

void nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

float getResistance(float refVoltage, float refResistor, float sensorVoltage)
{
    return (-sensorVoltage * refResistor) / (sensorVoltage - refVoltage);
}

float calcTemp(float sensorVoltage)
{
    float resistivity = getResistance(3.3, 47000.0, sensorVoltage);
    float temperature = log(resistivity / 242463.0) / -0.037;
    return temperature;
}

float calcPress(float sensorVoltage)
{
    float resistivity = getResistance(3.3, 68.0, sensorVoltage);
    float pressure = (resistivity - 12.079) / 18.303;
    
    if (pressure < 0.0) pressure = 0.0;
    if (pressure > 9.9) pressure = 9.9;

    return pressure;
}

void app_main(void)
{       
    nvs_init();

    spiffs_init();

    wifi_init_softap();

    start_webserver();

    udp_sender_init();
    adc_init();
    
    while (1) 
    {
        waterTempSensorVoltage = adc_measure(channels[0]);
        oilTempSensorVoltage = adc_measure(channels[1]);
        oilPressureSensorVoltage = adc_measure(channels[2]);

        waterTemp = calcTemp(waterTempSensorVoltage);
        oilTemp = calcTemp(oilTempSensorVoltage);
        oilPressure = calcPress(oilPressureSensorVoltage);

        printf("%.03fV\t%.03fV\t%.03fV\t\t%0.1f°C\t%.01f°C\t%.01f bar\n", 
            waterTempSensorVoltage, 
            oilTempSensorVoltage, 
            oilPressureSensorVoltage,
            waterTemp, 
            oilTemp, 
            oilPressure);

        GaugeData.Magic = GAUGE_DATA_MAGIC_VALUE;
        GaugeData.WaterTemperature = waterTemp;
        GaugeData.OilTemperature = oilTemp;
        GaugeData.OilPressure = oilPressure;

        udp_sender_set_payload((const char*)&GaugeData, sizeof(GaugeData_t));  

        vTaskDelay(100 / portTICK_PERIOD_MS);   
    }
}