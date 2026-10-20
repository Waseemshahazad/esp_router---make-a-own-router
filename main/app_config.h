#pragma once

#include <stdint.h>

typedef struct {
    const char *ssid;
    const char *password;
    uint8_t channel;
    uint8_t max_connections;
} app_wifi_config_t;

void app_config_get_wifi(app_wifi_config_t *config);
