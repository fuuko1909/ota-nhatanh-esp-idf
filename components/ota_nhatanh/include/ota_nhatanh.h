/**
 * ota_nhatanh.h - ESP-IDF component cho ota.nhatanh.tech
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ota_nhatanh_mqtt_cb_t)(const char *topic, const char *payload, int len);

typedef struct {
    const char *device_id;
    const char *wifi_ssid;
    const char *wifi_password;
    const char *mqtt_host;
    uint16_t mqtt_port;
    const char *mqtt_user;
    const char *mqtt_password;
    const char *ota_manifest_url;
    uint8_t ota_check_hours;
    uint16_t heartbeat_seconds;
    ota_nhatanh_mqtt_cb_t on_mqtt_message;
    bool insecure_tls;        // bỏ verify cert TLS (chỉ test)
} ota_nhatanh_config_t;

#define OTA_NHATANH_CONFIG_DEFAULT() (ota_nhatanh_config_t){ \
    .mqtt_port = 8883, \
    .ota_check_hours = 6, \
    .heartbeat_seconds = 60, \
    .insecure_tls = false, \
}

esp_err_t ota_nhatanh_init(const ota_nhatanh_config_t *cfg);
esp_err_t ota_nhatanh_start(void);
esp_err_t ota_nhatanh_publish(const char *sub_topic, const char *payload, int retain);
esp_err_t ota_nhatanh_publish_sensor(const char *sensor_name, float value);
esp_err_t ota_nhatanh_subscribe(const char *sub_topic);
esp_err_t ota_nhatanh_check_ota_now(void);
bool ota_nhatanh_is_connected(void);

void ota_nhatanh_log(char level, const char *fmt, ...);

#define OTA_LOGD(fmt, ...) ota_nhatanh_log('D', fmt, ##__VA_ARGS__)
#define OTA_LOGI(fmt, ...) ota_nhatanh_log('I', fmt, ##__VA_ARGS__)
#define OTA_LOGW(fmt, ...) ota_nhatanh_log('W', fmt, ##__VA_ARGS__)
#define OTA_LOGE(fmt, ...) ota_nhatanh_log('E', fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
