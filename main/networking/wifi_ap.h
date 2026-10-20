#pragma once

#include <stdint.h>

#include "app_config.h"
#include "esp_err.h"

typedef struct {
    char ip[16];
    uint16_t connected_stations;
} wifi_ap_status_t;

esp_err_t wifi_ap_start(const app_wifi_config_t *config);
esp_err_t wifi_ap_get_status(wifi_ap_status_t *status);
