/**
 * ota_nhatanh_entity.c - MQTT discovery entity API
 */
#include "ota_nhatanh.h"
#include "ota_nhatanh_internal.h"
#include "mqtt_client.h"
#include <string.h>
#include <stdio.h>

ota_entity_t *_ota_find_entity(const char *key) {
    for (int i = 0; i < g_state.entity_count; i++) {
        if (strcmp(g_state.entities[i].key, key) == 0) return &g_state.entities[i];
    }
    return NULL;
}

static ota_entity_t *_alloc_entity(const char *key, const char *platform, const char *name) {
    if (g_state.entity_count >= OTA_MAX_ENTITIES) return NULL;
    if (_ota_find_entity(key)) return NULL;
    ota_entity_t *e = &g_state.entities[g_state.entity_count++];
    memset(e, 0, sizeof(*e));
    strlcpy(e->key, key, sizeof(e->key));
    strlcpy(e->platform, platform, sizeof(e->platform));
    if (name) strlcpy(e->name, name, sizeof(e->name));
    e->num_min = 0; e->num_max = 100; e->num_step = 1;
    return e;
}

void _ota_publish_entity_config(ota_entity_t *e) {
    if (!g_state.mqtt) return;
    char payload[512];
    int n = snprintf(payload, sizeof(payload), "{\"platform\":\"%s\"", e->platform);
    if (e->name[0])         n += snprintf(payload+n, sizeof(payload)-n, ",\"name\":\"%s\"", e->name);
    if (e->unit[0])         n += snprintf(payload+n, sizeof(payload)-n, ",\"unit\":\"%s\"", e->unit);
    if (e->device_class[0]) n += snprintf(payload+n, sizeof(payload)-n, ",\"device_class\":\"%s\"", e->device_class);
    if (strcmp(e->platform, "number") == 0) {
        n += snprintf(payload+n, sizeof(payload)-n,
                      ",\"min\":%.3f,\"max\":%.3f,\"step\":%.3f",
                      e->num_min, e->num_max, e->num_step);
    }
    n += snprintf(payload+n, sizeof(payload)-n, "}");

    char topic[160];
    snprintf(topic, sizeof(topic), "%s/entity/%s/config", g_state.topic_prefix, e->key);
    esp_mqtt_client_publish(g_state.mqtt, topic, payload, n, 1, 1);
}

void _ota_publish_entity_state(ota_entity_t *e, const char *value) {
    strlcpy(e->last_state, value, sizeof(e->last_state));
    if (!g_state.mqtt) return;
    char topic[160];
    snprintf(topic, sizeof(topic), "%s/entity/%s/state", g_state.topic_prefix, e->key);
    esp_mqtt_client_publish(g_state.mqtt, topic, value, strlen(value), 1, 1);
}

void _ota_publish_all_entity_configs(void) {
    for (int i = 0; i < g_state.entity_count; i++) {
        _ota_publish_entity_config(&g_state.entities[i]);
        if (g_state.entities[i].last_state[0]) {
            _ota_publish_entity_state(&g_state.entities[i], g_state.entities[i].last_state);
        }
    }
}

void _ota_handle_entity_set(const char *key, const char *value) {
    ota_entity_t *e = _ota_find_entity(key);
    if (!e || !e->cb) return;
    if (strcmp(e->platform, "switch") == 0) {
        bool on = (strcmp(value, "ON") == 0 || strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
        ((ota_switch_cb_t)e->cb)(on);
        ota_nhatanh_update_switch(key, on);
    } else if (strcmp(e->platform, "number") == 0) {
        float v = (float)atof(value);
        ((ota_number_cb_t)e->cb)(v);
        ota_nhatanh_update_number(key, v);
    } else if (strcmp(e->platform, "button") == 0) {
        ((ota_button_cb_t)e->cb)();
    } else if (strcmp(e->platform, "text") == 0) {
        ((ota_text_cb_t)e->cb)(value);
        ota_nhatanh_update_text(key, value);
    }
}

esp_err_t ota_nhatanh_add_sensor(const char *key, const char *name, const char *unit) {
    ota_entity_t *e = _alloc_entity(key, "sensor", name);
    if (!e) return ESP_FAIL;
    if (unit) strlcpy(e->unit, unit, sizeof(e->unit));
    return ESP_OK;
}
esp_err_t ota_nhatanh_add_binary_sensor(const char *key, const char *name) {
    return _alloc_entity(key, "binary_sensor", name) ? ESP_OK : ESP_FAIL;
}
esp_err_t ota_nhatanh_add_switch(const char *key, const char *name, ota_switch_cb_t cb) {
    ota_entity_t *e = _alloc_entity(key, "switch", name);
    if (!e) return ESP_FAIL;
    e->cb = (void *)cb;
    return ESP_OK;
}
esp_err_t ota_nhatanh_add_number(const char *key, const char *name, float mn, float mx, float st, ota_number_cb_t cb) {
    ota_entity_t *e = _alloc_entity(key, "number", name);
    if (!e) return ESP_FAIL;
    e->num_min = mn; e->num_max = mx; e->num_step = st; e->cb = (void *)cb;
    return ESP_OK;
}
esp_err_t ota_nhatanh_add_button(const char *key, const char *name, ota_button_cb_t cb) {
    ota_entity_t *e = _alloc_entity(key, "button", name);
    if (!e) return ESP_FAIL;
    e->cb = (void *)cb;
    return ESP_OK;
}
esp_err_t ota_nhatanh_add_text(const char *key, const char *name) {
    return _alloc_entity(key, "text", name) ? ESP_OK : ESP_FAIL;
}

esp_err_t ota_nhatanh_update_sensor(const char *key, float v) {
    ota_entity_t *e = _ota_find_entity(key);
    if (!e) return ESP_FAIL;
    char buf[32]; snprintf(buf, sizeof(buf), "%.3f", v);
    _ota_publish_entity_state(e, buf);
    return ESP_OK;
}
esp_err_t ota_nhatanh_update_binary_sensor(const char *key, bool on) {
    ota_entity_t *e = _ota_find_entity(key);
    if (!e) return ESP_FAIL;
    _ota_publish_entity_state(e, on ? "ON" : "OFF");
    return ESP_OK;
}
esp_err_t ota_nhatanh_update_switch(const char *key, bool on) {
    return ota_nhatanh_update_binary_sensor(key, on);
}
esp_err_t ota_nhatanh_update_number(const char *key, float v) {
    return ota_nhatanh_update_sensor(key, v);
}
esp_err_t ota_nhatanh_update_text(const char *key, const char *value) {
    ota_entity_t *e = _ota_find_entity(key);
    if (!e) return ESP_FAIL;
    _ota_publish_entity_state(e, value);
    return ESP_OK;
}
