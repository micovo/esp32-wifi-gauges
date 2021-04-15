#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <sys/param.h>

static const char *TAG = "spiffs";

// This awefull fileread approach works only for files up to 32KB
// This approach was not working for bigger files and I didn't have time to bother with that...
static char storageBuffer[32*1024];

void spiffs_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

char * storage_file_read(char * filename)
{
    char path[32];

    char * filenameFixed = filename;

    if (*filenameFixed == '/')
    {
        filenameFixed++;
    }

    if (strlen(filenameFixed) > 16)
    {
        storageBuffer[0] = 0;
        return NULL;
    }

    sprintf(path, "/spiffs/%s", filenameFixed);

    ESP_LOGI(TAG, "Reading %s", path);

    // Open for reading hello.txt
    FILE* f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        return NULL;
    }

    memset(storageBuffer, 0, sizeof(storageBuffer));
    int bytes = fread(storageBuffer, 1, sizeof(storageBuffer), f);
    fclose(f);

    
    ESP_LOGI(TAG, "%u bytes read", bytes);

    return storageBuffer;
}