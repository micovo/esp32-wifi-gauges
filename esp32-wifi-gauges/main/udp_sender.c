/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "driver/gpio.h"

#define HOST_IP_ADDR "255.255.255.255"
#define PORT 2222

#define BLINK_GPIO 2

static const char *TAG = "udp";

static char payload[64];
static uint32_t payloadSize = 0;
static TaskHandle_t udpSenderTask;

void udp_sender_set_payload(const char * payloadBuffer, uint32_t payloadBufferSize)
{
    memcpy(payload, payloadBuffer, payloadBufferSize);
    payloadSize = payloadBufferSize;

    vTaskResume(udpSenderTask);
}

static void udp_client_task(void *pvParameters)
{
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    int addr_family = 0;
    int ip_protocol = 0;

    while (1) 
    {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) 
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        while (1) 
        {
            vTaskSuspend(NULL);
            
            if (payloadSize > 0)
            {
                gpio_set_level(BLINK_GPIO, 1);

                int err = sendto(sock, payload, payloadSize, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                if (err < 0) 
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                }

                vTaskDelay(50 / portTICK_PERIOD_MS);
                gpio_set_level(BLINK_GPIO, 0);
                payloadSize = 0;
            }

            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        if (sock != -1) 
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(NULL);
}

void udp_sender_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, &udpSenderTask);
}
