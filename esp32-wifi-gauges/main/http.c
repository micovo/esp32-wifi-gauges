#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include <sys/param.h>
#include <esp_http_server.h>
#include "gauge.h"

static const char *TAG = "http";

extern GaugeData_t GaugeData;

char * storage_file_read(char * filename);
esp_err_t get_handler(httpd_req_t *req);
esp_err_t get_data_handler(httpd_req_t *req);

static char ajaxcontent[128];

httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

httpd_uri_t jquery_uri_get = {
    .uri      = "/pureknob.js",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

httpd_uri_t justgage_uri_get = {
    .uri      = "/justgage.min.js",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

httpd_uri_t data_uri_get = {
    .uri      = "/data",
    .method   = HTTP_GET,
    .handler  = get_data_handler,
    .user_ctx = NULL
};

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req)
{
    char * content = NULL;

    ESP_LOGI(TAG, "HTTP request URI (%s)", req->uri);

    if ((strlen(req->uri) == 0) || (strcmp(req->uri, "/")==0) || (strcmp(req->uri, "index.html")==0))
    {
        content = storage_file_read("index.html");
    }
    else
    {
        content = storage_file_read(req->uri);
    }

    httpd_resp_send(req, content, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t get_data_handler(httpd_req_t *req)
{    
    sprintf(ajaxcontent, "{\"data\":{\"waterTemp\":\"%.01f\",\"oilTemp\":\"%.01f\",\"oilPres\":\"%.02f\"}}", 
        GaugeData.WaterTemperature, 
        GaugeData.OilTemperature, 
        GaugeData.OilPressure);

    httpd_resp_send(req, ajaxcontent, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &data_uri_get);
        httpd_register_uri_handler(server, &justgage_uri_get);
        httpd_register_uri_handler(server, &jquery_uri_get);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}