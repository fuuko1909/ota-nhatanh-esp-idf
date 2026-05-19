#include <stdio.h>
#include "ota_nhatanh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DEVICE_ID  "demo-esp32-idf"
#define WIFI_SSID  "Wifi-cua-ban"
#define WIFI_PASS  "wifi-pass"
#define MQTT_PASS  "doi-mat-khau-tu-UI"
#define OTA_TOKEN  "doi-token-tu-UI"

void app_main(void) {
    ota_nhatanh_config_t cfg = OTA_NHATANH_CONFIG_DEFAULT();
    cfg.device_id = DEVICE_ID;
    cfg.wifi_ssid = WIFI_SSID;
    cfg.wifi_password = WIFI_PASS;
    cfg.mqtt_host = "mqtt.nhatanh.tech";
    cfg.mqtt_port = 8883;
    cfg.mqtt_user = DEVICE_ID;
    cfg.mqtt_password = MQTT_PASS;
    cfg.ota_manifest_url = "https://ota.nhatanh.tech/fw/" OTA_TOKEN "/manifest.json";

    ota_nhatanh_init(&cfg);
    ota_nhatanh_start();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        ota_nhatanh_publish_sensor("nhiet-do", 25.5);
    }
}
