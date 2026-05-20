/**
 * ota_nhatanh_internal.h - shared state giữa các file internal
 */
#pragma once

#include "ota_nhatanh.h"
#include "mqtt_client.h"
#include <stdbool.h>

#define OTA_MAX_ENTITIES 32

typedef struct {
    char key[32];
    char platform[16];
    char name[64];
    char unit[16];
    char device_class[32];
    float num_min, num_max, num_step;
    char last_state[64];
    void *cb;       // dạng cast về callback tương ứng theo platform
} ota_entity_t;

typedef struct {
    ota_nhatanh_config_t cfg;
    char topic_prefix[96];
    bool wifi_connected;
    bool mqtt_connected;
    esp_mqtt_client_handle_t mqtt;
    ota_entity_t entities[OTA_MAX_ENTITIES];
    int entity_count;
} ota_nhatanh_state_t;

extern ota_nhatanh_state_t g_state;

esp_err_t ota_nhatanh_wifi_start(void);
esp_err_t ota_nhatanh_mqtt_start(void);

ota_entity_t *_ota_find_entity(const char *key);
void _ota_publish_entity_config(ota_entity_t *e);
void _ota_publish_entity_state(ota_entity_t *e, const char *value);
void _ota_publish_all_entity_configs(void);
void _ota_handle_entity_set(const char *key, const char *value);
