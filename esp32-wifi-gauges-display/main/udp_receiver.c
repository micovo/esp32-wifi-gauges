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

#include "gauge.h"

#define PORT 2222
#define HOST_IP_ADDR "255.255.255.255"

static const char *TAG = "udp";

extern GaugeData_t GaugeData;

void main_sync_data(void);

static void udp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) 
    {
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

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

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) 
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        while (1) 
        {
            struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6¨
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) 
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            else 
            {
                rx_buffer[len] = 0;

                if (len == sizeof(GaugeData_t))
                {
                    memcpy((void *)&GaugeData, (void *)rx_buffer, sizeof(GaugeData_t));
                }
            }

            vTaskDelay(100  /portTICK_PERIOD_MS);
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

void udp_client_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
}
