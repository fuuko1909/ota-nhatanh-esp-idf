# ota-nhatanh-esp-idf

ESP-IDF component kết nối ESP32 vào hệ thống **ota.nhatanh.tech**.

## Tính năng

- WiFi station + provisioning (qua NVS, fallback SoftAP nếu chưa có config)
- MQTT TLS (esp-mqtt) tới `mqtt.nhatanh.tech:8883`
- OTA pull qua `esp_https_ota` theo manifest URL mỗi N giờ
- Topic chuẩn: `thiet-bi/<id>/...`
- Birth + Last Will tự động
- Heartbeat JSON mỗi 60s

## Cài đặt (PlatformIO ESP-IDF framework)

`platformio.ini`:

```ini
[env:my_esp32]
platform = espressif32
framework = espidf
board = esp32dev
lib_deps =
  https://github.com/fuuko1909/ota-nhatanh-esp-idf.git
```

## Cài đặt (ESP-IDF native)

Trong project của bạn:
```bash
cd components
git submodule add https://github.com/fuuko1909/ota-nhatanh-esp-idf.git ota_nhatanh
```

Hoặc copy thư mục `components/ota_nhatanh/` vào project.

## Cách dùng

```c
#include "ota_nhatanh.h"

void app_main(void) {
  ota_nhatanh_config_t cfg = OTA_NHATANH_CONFIG_DEFAULT();
  cfg.device_id = "kho1-cam-bien-1";
  cfg.wifi_ssid = "Wifi-cua-ban";
  cfg.wifi_password = "wifi-pass";
  cfg.mqtt_host = "mqtt.nhatanh.tech";
  cfg.mqtt_port = 8883;
  cfg.mqtt_user = "kho1-cam-bien-1";
  cfg.mqtt_password = "mqtt-pass-tu-DB";
  cfg.ota_manifest_url = "https://ota.nhatanh.tech/fw/<token>/manifest.json";
  cfg.ota_check_hours = 6;

  ota_nhatanh_init(&cfg);
  ota_nhatanh_start();

  // publish sensor moi 30s
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(30000));
    char val[16];
    snprintf(val, sizeof(val), "%.2f", 25.5);
    ota_nhatanh_publish("sensor/nhiet-do/state", val, 0);
  }
}
```

## License

MIT
