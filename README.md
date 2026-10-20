# ESP32-S3 Wi-Fi Control Panel

An ESP-IDF SoftAP application with an embedded HTTP dashboard, device-status API, and onboard WS2812-compatible RGB LED control.

## Project structure

- `main/softap_example_main.c`: application startup and subsystem wiring.
- `main/app_config.c`: Wi-Fi settings sourced from ESP-IDF Kconfig.
- `main/networking/`: SoftAP lifecycle and access-point status.
- `main/server/`: HTTP API routes and the embedded dashboard.
- `main/hardware/led/`: addressable RGB LED driver using `espressif/led_strip`.
- `main/Kconfig.projbuild`: Wi-Fi settings and configurable RGB LED GPIO.

## Configure, build, and flash

1. Open `idf.py menuconfig`.
2. Under **Example Configuration**, choose the SoftAP SSID, password, channel, and client limit.
3. Under **Hardware Configuration**, the onboard addressable RGB LED GPIO defaults to GPIO 48, the onboard RGB LED data pin on this ESP32-S3 setup.
4. Build and flash with ESP-IDF. The first build downloads `espressif/led_strip` through the ESP Component Manager.

The default example credentials are `ESP32 S3` / `123456789`. Change them before using the AP beyond local development. A connected client can open **http://192.168.4.1/**. The responsive dashboard includes Home, LED Controls, Network Status, and System Settings sections.

## HTTP API

- `GET /api/status` — returns the configured SSID, AP IP, channel, connected-client count, uptime, free heap, configured LED GPIO, driver availability, on/off state, and last applied RGB color.
- `PUT /api/led` — sets the LED state and optional RGB color. For example: `{"on":true,"color":{"r":16,"g":48,"b":128}}` or `{"on":false}`. Color channels must be integers from 0 to 255.

## Hardware notes

The LED driver initializes one WS2812-compatible pixel through `espressif/led_strip` and the ESP-IDF RMT backend at a 10 MHz resolution. The configured default is GPIO 48. The API and dashboard report the last successfully transmitted LED state; the ESP32 cannot read back the physical LED color. If initialization or transmission fails, the SoftAP and dashboard remain available and report the driver/output error state rather than claiming the LED changed.
