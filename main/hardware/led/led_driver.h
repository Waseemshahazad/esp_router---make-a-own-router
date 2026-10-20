#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} led_driver_color_t;

esp_err_t led_driver_init(void);
esp_err_t led_driver_set(bool on, const led_driver_color_t *color);
bool led_driver_is_available(void);
bool led_driver_is_on(void);
void led_driver_get_color(led_driver_color_t *color);
