/**
 * ota_nhatanh_internal.h - shared state giữa các file internal
 */
#pragma once

#include "ota_nhatanh.h"
#include "mqtt_client.h"
#include <stdbool.h>

typedef struct {
    ota_nhatanh_config_t cfg;
    char topic_prefix[96];
    bool wifi_connected;
    bool mqtt_connected;
    esp_mqtt_client_handle_t mqtt;
} ota_nhatanh_state_t;

extern ota_nhatanh_state_t g_state;

esp_err_t ota_nhatanh_wifi_start(void);
esp_err_t ota_nhatanh_mqtt_start(void);
