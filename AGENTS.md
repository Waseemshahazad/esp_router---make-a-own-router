# AGENTS.md

This repository is a minimal ESP-IDF Wi-Fi SoftAP project that turns an ESP device into an access point. The app entry point is in `main/softap_example_main.c`, and the project follows the standard ESP-IDF CMake layout.

## Project layout
- Root `CMakeLists.txt` loads the ESP-IDF project CMake script and sets the project name.
- `main/CMakeLists.txt` registers the app component and source file.
- `main/softap_example_main.c` contains the Wi-Fi AP setup and startup logic.
- Configurable Wi-Fi settings come from ESP-IDF Kconfig values such as `CONFIG_ESP_WIFI_SSID`, `CONFIG_ESP_WIFI_PASSWORD`, `CONFIG_ESP_WIFI_CHANNEL`, and `CONFIG_ESP_MAX_STA_CONN`.

## Working conventions
- Keep the application logic aligned with ESP-IDF patterns: `ESP_ERROR_CHECK(...)`, NVS initialization, and event registration using the ESP Wi-Fi APIs.
- Prefer changes in `main/softap_example_main.c` for access-point behavior; do not replace the standard ESP-IDF project structure unless there is a clear reason.
- When changing Wi-Fi configuration, prefer `idf.py menuconfig` over hard-coding credentials in source unless the task explicitly requires a fixed example configuration.
- This project is a hardware-focused embedded application, so compile-time verification is the primary validation path.

## Build and validation commands
Run these from the repository root:

```bash
idf.py build
idf.py -p PORT flash monitor
idf.py menuconfig
idf.py reconfigure
```

There are no unit tests in this repo; compile and runtime logs are the main checks for correctness.

## References
- [README.md](README.md)
- [main/softap_example_main.c](main/softap_example_main.c)
- [main/CMakeLists.txt](main/CMakeLists.txt)
