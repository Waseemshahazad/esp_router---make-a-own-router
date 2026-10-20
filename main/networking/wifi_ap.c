#include "wifi_ap.h"

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"

static const char *TAG = "wifi_ap";
static esp_netif_t *s_ap_netif;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_base;

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        const wifi_event_ap_staconnected_t *event = event_data;
        ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        const wifi_event_ap_stadisconnected_t *event = event_data;
        ESP_LOGI(TAG, "Station " MACSTR " left, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

esp_err_t wifi_ap_start(const app_wifi_config_t *config)
{
    if (config == NULL || config->ssid == NULL || config->password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const size_t ssid_length = strlen(config->ssid);
    const size_t password_length = strlen(config->password);
    if (ssid_length == 0 || ssid_length > 32 ||
        (password_length != 0 && (password_length < 8 || password_length > 63))) {
        ESP_LOGE(TAG, "SSID must be 1-32 bytes and password must be empty or 8-63 bytes");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize network interface");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Failed to create event loop");

    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create AP network interface");
        return ESP_ERR_NO_MEM;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), TAG, "Failed to initialize Wi-Fi");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   wifi_event_handler, NULL),
                        TAG, "Failed to register Wi-Fi event handler");

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = {0},
            .ssid_len = ssid_length,
            .channel = config->channel,
            .password = {0},
            .max_connection = config->max_connections,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                .required = true,
            },
        },
    };

    memcpy(wifi_config.ap.ssid, config->ssid, ssid_length);
    memcpy(wifi_config.ap.password, config->password, password_length);
    if (password_length == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        wifi_config.ap.pmf_cfg.required = false;
    }

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "Failed to set AP mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config),
                        TAG, "Failed to configure AP");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start AP");

    ESP_LOGI(TAG, "SoftAP started: SSID=%s channel=%u",
             config->ssid, (unsigned int)config->channel);
    return ESP_OK;
}

esp_err_t wifi_ap_get_status(wifi_ap_status_t *status)
{
    if (status == NULL || s_ap_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_netif_ip_info_t ip_info;
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(s_ap_netif, &ip_info),
                        TAG, "Failed to get AP IP address");

    wifi_sta_list_t stations = {0};
    ESP_RETURN_ON_ERROR(esp_wifi_ap_get_sta_list(&stations),
                        TAG, "Failed to get connected station list");

    snprintf(status->ip, sizeof(status->ip), IPSTR, IP2STR(&ip_info.ip));
    status->connected_stations = stations.num;
    return ESP_OK;
}
