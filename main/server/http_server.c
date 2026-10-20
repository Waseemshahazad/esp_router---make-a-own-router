#include "http_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "cJSON.h"
#include "dashboard.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "led_driver.h"
#include "sdkconfig.h"
#include "wifi_ap.h"

static const char *TAG = "http_server";

static esp_err_t send_json(httpd_req_t *request, cJSON *json)
{
    char *payload = cJSON_PrintUnformatted(json);
    if (payload == NULL) {
        cJSON_Delete(json);
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to create JSON response");
    }

    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    const esp_err_t err = httpd_resp_sendstr(request, payload);
    free(payload);
    cJSON_Delete(json);
    return err;
}

static esp_err_t status_get_handler(httpd_req_t *request)
{
    wifi_ap_status_t ap_status;
    const esp_err_t status_err = wifi_ap_get_status(&ap_status);
    if (status_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read AP status: %s", esp_err_to_name(status_err));
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to read AP status");
    }

    app_wifi_config_t ap_config;
    app_config_get_wifi(&ap_config);

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to allocate status response");
    }

    bool added =
        cJSON_AddStringToObject(json, "ssid", ap_config.ssid) != NULL &&
        cJSON_AddStringToObject(json, "ip", ap_status.ip) != NULL &&
        cJSON_AddNumberToObject(json, "channel", ap_config.channel) != NULL &&
        cJSON_AddNumberToObject(json, "connected_stations",
                                ap_status.connected_stations) != NULL &&
        cJSON_AddNumberToObject(json, "uptime_ms",
                                (double)(esp_timer_get_time() / 1000)) != NULL &&
        cJSON_AddNumberToObject(json, "free_heap_bytes", esp_get_free_heap_size()) != NULL &&
        cJSON_AddBoolToObject(json, "led_available", led_driver_is_available()) != NULL &&
        cJSON_AddBoolToObject(json, "led_on", led_driver_is_on()) != NULL &&
        cJSON_AddNumberToObject(json, "led_gpio", CONFIG_RGB_LED_GPIO) != NULL;
    led_driver_color_t led_color;
    led_driver_get_color(&led_color);
    char color_hex[8];
    snprintf(color_hex, sizeof(color_hex), "#%02X%02X%02X",
             led_color.red, led_color.green, led_color.blue);
    if (added) {
        added = cJSON_AddStringToObject(json, "led_color", color_hex) != NULL;
    }
    if (!added) {
        cJSON_Delete(json);
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to create status response");
    }

    return send_json(request, json);
}

static esp_err_t led_put_handler(httpd_req_t *request)
{
    enum { MAX_LED_REQUEST_BYTES = 64 };
    if (request->content_len == 0 || request->content_len >= MAX_LED_REQUEST_BYTES) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                                   "Expected a small JSON body with an 'on' boolean");
    }

    char body[MAX_LED_REQUEST_BYTES];
    size_t received_total = 0;
    while (received_total < request->content_len) {
        const int received = httpd_req_recv(request, body + received_total,
                                            request->content_len - received_total);
        if (received <= 0) {
            ESP_LOGE(TAG, "Failed to receive LED request body");
            return ESP_FAIL;
        }
        received_total += received;
    }
    body[received_total] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (json == NULL || !cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid JSON object");
    }

    const cJSON *on_item = cJSON_GetObjectItemCaseSensitive(json, "on");
    if (!cJSON_IsBool(on_item)) {
        cJSON_Delete(json);
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                                   "The 'on' field must be a boolean");
    }

    const bool on = cJSON_IsTrue(on_item);
    led_driver_color_t color;
    led_driver_get_color(&color);
    const cJSON *color_item = cJSON_GetObjectItemCaseSensitive(json, "color");
    if (color_item != NULL) {
        const cJSON *red_item = cJSON_GetObjectItemCaseSensitive(color_item, "r");
        const cJSON *green_item = cJSON_GetObjectItemCaseSensitive(color_item, "g");
        const cJSON *blue_item = cJSON_GetObjectItemCaseSensitive(color_item, "b");
        if (!cJSON_IsObject(color_item) ||
            !cJSON_IsNumber(red_item) || !cJSON_IsNumber(green_item) ||
            !cJSON_IsNumber(blue_item) ||
            red_item->valuedouble < 0 || red_item->valuedouble > 255 ||
            green_item->valuedouble < 0 || green_item->valuedouble > 255 ||
            blue_item->valuedouble < 0 || blue_item->valuedouble > 255 ||
            (double)(uint8_t)red_item->valuedouble != red_item->valuedouble ||
            (double)(uint8_t)green_item->valuedouble != green_item->valuedouble ||
            (double)(uint8_t)blue_item->valuedouble != blue_item->valuedouble) {
            cJSON_Delete(json);
            return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                                       "Color channels must be integers from 0 to 255");
        }
        color.red = (uint8_t)red_item->valuedouble;
        color.green = (uint8_t)green_item->valuedouble;
        color.blue = (uint8_t)blue_item->valuedouble;
    }
    cJSON_Delete(json);

    if (!led_driver_is_available()) {
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "LED hardware is unavailable");
    }

    const esp_err_t led_err = led_driver_set(on, &color);
    if (led_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LED state: %s", esp_err_to_name(led_err));
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to update LED");
    }

    cJSON *response = cJSON_CreateObject();
    char color_hex[8];
    snprintf(color_hex, sizeof(color_hex), "#%02X%02X%02X",
             color.red, color.green, color.blue);
    if (response == NULL ||
        cJSON_AddBoolToObject(response, "led_on", on) == NULL ||
        cJSON_AddStringToObject(response, "led_color", color_hex) == NULL) {
        cJSON_Delete(response);
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to allocate LED response");
    }
    return send_json(request, response);
}

static esp_err_t register_api_routes(httpd_handle_t server)
{
    const httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get_handler,
    };
    const httpd_uri_t led_uri = {
        .uri = "/api/led",
        .method = HTTP_PUT,
        .handler = led_put_handler,
    };

    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &status_uri),
                        TAG, "Failed to register status endpoint");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &led_uri),
                        TAG, "Failed to register LED endpoint");
    return ESP_OK;
}

esp_err_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 4;
    config.stack_size = 6144;

    httpd_handle_t server = NULL;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &config), TAG, "Failed to start HTTP server");

    esp_err_t err = dashboard_register_handler(server);
    if (err == ESP_OK) {
        err = register_api_routes(server);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register HTTP routes: %s", esp_err_to_name(err));
        ESP_RETURN_ON_ERROR(httpd_stop(server), TAG, "Failed to stop HTTP server after setup error");
        return err;
    }

    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}
