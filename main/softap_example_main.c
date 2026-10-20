#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_config.h"
#include "http_server.h"
#include "led_driver.h"
#include "wifi_ap.h"

static const char *TAG = "app";

static esp_err_t initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "Failed to erase NVS");
        err = nvs_flash_init();
    }
    return err;
}

void app_main(void)
{
    ESP_ERROR_CHECK(initialize_nvs());

    app_wifi_config_t wifi_config;
    app_config_get_wifi(&wifi_config);
    ESP_ERROR_CHECK(wifi_ap_start(&wifi_config));

    const esp_err_t led_err = led_driver_init();
    if (led_err != ESP_OK) {
        ESP_LOGW(TAG, "Starting without LED control; check the WS2812 wiring and GPIO configuration");
    }

    ESP_ERROR_CHECK(http_server_start());

    ESP_LOGI(TAG, "Dashboard available at http://192.168.4.1/");
    
}
