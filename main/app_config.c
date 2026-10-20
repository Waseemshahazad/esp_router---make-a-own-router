#include "app_config.h"

#include "sdkconfig.h"

void app_config_get_wifi(app_wifi_config_t *config)
{
    *config = (app_wifi_config_t) {
        .ssid = CONFIG_ESP_WIFI_SSID,
        .password = CONFIG_ESP_WIFI_PASSWORD,
        .channel = CONFIG_ESP_WIFI_CHANNEL,
        .max_connections = CONFIG_ESP_MAX_STA_CONN,
    };
}
