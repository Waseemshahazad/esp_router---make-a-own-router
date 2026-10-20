#include "led_driver.h"

#include "esp_check.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "led_driver";
static led_strip_handle_t s_led_strip;
static bool s_led_available;
static bool s_led_on;
static led_driver_color_t s_led_color = {
    .red = 16,
    .green = 48,
    .blue = 128,
};

esp_err_t led_driver_init(void)
{
    const led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_RGB_LED_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .flags.invert_out = false,
    };
    const led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10U * 1000U * 1000U,
        .mem_block_symbols = 0,
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WS2812 LED: %s", esp_err_to_name(err));
        s_led_strip = NULL;
        return err;
    }

    err = led_strip_clear(s_led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear WS2812 LED: %s", esp_err_to_name(err));
        esp_err_t delete_err = led_strip_del(s_led_strip);
        if (delete_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to release WS2812 LED resources: %s",
                     esp_err_to_name(delete_err));
        }
        s_led_strip = NULL;
        return err;
    }

    s_led_available = true;
    s_led_on = false;
    s_led_color = (led_driver_color_t) {
        .red = 16,
        .green = 48,
        .blue = 128,
    };

    ESP_LOGI(TAG, "WS2812 LED initialized on GPIO %d", CONFIG_RGB_LED_GPIO);
    return ESP_OK;
}

bool led_driver_is_available(void)
{
    return s_led_available;
}

esp_err_t led_driver_set(bool on, const led_driver_color_t *color)
{
    if (!s_led_available || s_led_strip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (on && color == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_led_on == on && (color == NULL ||
        (s_led_color.red == color->red &&
         s_led_color.green == color->green &&
         s_led_color.blue == color->blue))) {
        return ESP_OK;
    }

    esp_err_t err = ESP_OK;
    if (on) {
        err = led_strip_set_pixel(s_led_strip, 0, color->red, color->green, color->blue);
    } else {
        err = led_strip_clear(s_led_strip);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update WS2812 LED: %s", esp_err_to_name(err));
        return err;
    }

    if (on) {
        err = led_strip_refresh(s_led_strip);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to refresh WS2812 LED: %s", esp_err_to_name(err));
        return err;
    }

    s_led_on = on;
    if (color != NULL) {
        s_led_color = *color;
    }
    return ESP_OK;
}

bool led_driver_is_on(void)
{
    return s_led_on;
}

void led_driver_get_color(led_driver_color_t *color)
{
    if (color != NULL) {
        *color = s_led_color;
    }
}
