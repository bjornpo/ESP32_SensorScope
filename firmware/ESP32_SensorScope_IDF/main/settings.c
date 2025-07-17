#include "settings.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#ifdef CONFIG_IDF_TARGET_ESP32S3
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#endif // CONFIG_IDF_TARGET_ESP32S3

#include "serial_analyzer.h"

AppSettings g_settings;

#define SETTINGS_NAMESPACE "settings"
#define SETTINGS_VERSION 2

static const char *TAG = "settings";

void nvs_init(void) {
    #ifdef CONFIG_IDF_TARGET_ESP32S3
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t nvs_handle;
    err = nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    //if (err == ESP_ERR_NVS_NOT_FOUND) {
    //    ESP_LOGI(TAG, "NVS namespace %s not found. generating new default settings", SETTINGS_NAMESPACE);
    //    //return err;
    //}

    size_t size = sizeof(g_settings);
    err = nvs_get_blob(nvs_handle, SETTINGS_NAMESPACE, &g_settings, &size);
    if (err != ESP_OK || size != sizeof(g_settings) || g_settings.version != SETTINGS_VERSION)
    {
        // Settings are incompatible or missing – use defaults
        ESP_LOGI(TAG, "Settings not found or incompatible, initializing with defaults");
        init_default_settings(&g_settings);
        ESP_ERROR_CHECK (nvs_set_blob(nvs_handle, "settings", &g_settings, sizeof(g_settings)));
    }
    nvs_close(nvs_handle);
    #else
    init_default_settings(&g_settings);
    LV_LOG_USER("Initialicing default settings");
    #endif
}

void init_default_settings(AppSettings *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->version = SETTINGS_VERSION;

    serial_analyzer_init_defaults(&cfg->serial_analyzer);
    //modbus_init_defaults(&cfg->modbus);
    //sdi12_init_defaults(&cfg->sdi12);
    // Add more as you include more modules
}

void nvs_save_settings() {
    #ifdef CONFIG_IDF_TARGET_ESP32S3
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace %s: %s", SETTINGS_NAMESPACE, esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(nvs_handle, SETTINGS_NAMESPACE, &g_settings, sizeof(g_settings));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save settings: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Settings saved successfully");
    }
    nvs_close(nvs_handle);
    #else
    LV_LOG_USER("Saving current settings");
    #endif // CONFIG_IDF_TARGET_ESP32S3
}
