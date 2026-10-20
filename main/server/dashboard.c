#include "dashboard.h"

#include <stdint.h>

extern const uint8_t dashboard_html_start[] asm("_binary_dashboard_html_start");
extern const uint8_t dashboard_html_end[] asm("_binary_dashboard_html_end");

static esp_err_t dashboard_get_handler(httpd_req_t *request)
{
    const size_t html_size = dashboard_html_end - dashboard_html_start;
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, (const char *)dashboard_html_start, html_size);
}

esp_err_t dashboard_register_handler(httpd_handle_t server)
{
    const httpd_uri_t dashboard_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = dashboard_get_handler,
    };
    return httpd_register_uri_handler(server, &dashboard_uri);
}
