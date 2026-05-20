/**
 * ota_nhatanh.c - core init + publish API
 */
#include "ota_nhatanh.h"
#include "ota_nhatanh_internal.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "cJSON.h"

static const char *TAG = "ota_nhatanh";

ota_nhatanh_state_t g_state = {0};

static void heartbeat_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(g_state.cfg.heartbeat_seconds * 1000));
        if (!ota_nhatanh_is_connected()) continue;

        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "version", "0.1.0-idf");
        cJSON_AddNumberToObject(root, "uptime", esp_timer_get_time() / 1000000);
        char *out = cJSON_PrintUnformatted(root);
        ota_nhatanh_publish("he-thong/thong-tin", out, 0);
        free(out);
        cJSON_Delete(root);
    }
}

static void ota_check_task(void *arg) {
    uint32_t interval_ms = (uint32_t)g_state.cfg.ota_check_hours * 3600UL * 1000UL;
    vTaskDelay(pdMS_TO_TICKS(30000));
    while (1) {
        ota_nhatanh_check_ota_now();
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

esp_err_t ota_nhatanh_init(const ota_nhatanh_config_t *cfg) {
    if (!cfg || !cfg->device_id || !cfg->mqtt_host) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(&g_state.cfg, cfg, sizeof(ota_nhatanh_config_t));
    if (g_state.cfg.heartbeat_seconds == 0) g_state.cfg.heartbeat_seconds = 60;
    if (g_state.cfg.ota_check_hours == 0) g_state.cfg.ota_check_hours = 6;

    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    snprintf(g_state.topic_prefix, sizeof(g_state.topic_prefix),
             "thiet-bi/%s", cfg->device_id);
    ESP_LOGI(TAG, "init device=%s prefix=%s", cfg->device_id, g_state.topic_prefix);
    return ESP_OK;
}

esp_err_t ota_nhatanh_start(void) {
    ESP_ERROR_CHECK(ota_nhatanh_wifi_start());
    ESP_ERROR_CHECK(ota_nhatanh_mqtt_start());
    xTaskCreate(heartbeat_task, "ota_hb", 4096, NULL, 5, NULL);
    xTaskCreate(ota_check_task, "ota_chk", 8192, NULL, 5, NULL);
    return ESP_OK;
}

void ota_nhatanh_log(char level, const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("[%c] %s\n", level, buf);
    if (!g_state.mqtt_connected) return;

    // Escape JSON
    char payload[512];
    int n = snprintf(payload, sizeof(payload), "{\"l\":\"%c\",\"m\":\"", level);
    for (const char *p = buf; *p && n < (int)sizeof(payload) - 4; p++) {
        if (*p == '"' || *p == '\\') {
            payload[n++] = '\\';
            payload[n++] = *p;
        } else if (*p >= 32) {
            payload[n++] = *p;
        }
    }
    payload[n++] = '"';
    payload[n++] = '}';
    payload[n] = 0;
    ota_nhatanh_publish("log", payload, 0);
}

// ─────────── Live Serial stream ───────────
static char _raw_buf[256];
static int _raw_idx = 0;

static void _flush_raw(void) {
    if (_raw_idx == 0) return;
    _raw_buf[_raw_idx] = 0;
    if (g_state.mqtt_connected) {
        ota_nhatanh_publish("log/raw", _raw_buf, 0);
    }
    _raw_idx = 0;
}

void ota_nhatanh_print(const char *s) {
    printf("%s", s);
    for (; *s; s++) {
        if (*s == '\n' || _raw_idx >= 255) { _flush_raw(); continue; }
        if (*s == '\r') continue;
        _raw_buf[_raw_idx++] = *s;
    }
}

void ota_nhatanh_println(const char *s) {
    ota_nhatanh_print(s);
    _flush_raw();
}

void ota_nhatanh_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ota_nhatanh_print(buf);
}


esp_err_t ota_nhatanh_publish(const char *sub_topic, const char *payload, int retain) {
    if (!g_state.mqtt) return ESP_FAIL;
    char topic[160];
    snprintf(topic, sizeof(topic), "%s/%s", g_state.topic_prefix, sub_topic);
    int msg_id = esp_mqtt_client_publish(g_state.mqtt, topic, payload,
                                         (int)strlen(payload), 0, retain);
    return msg_id >= 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t ota_nhatanh_publish_sensor(const char *sensor_name, float value) {
    char sub[128];
    snprintf(sub, sizeof(sub), "sensor/%s/state", sensor_name);
    char val[32];
    snprintf(val, sizeof(val), "%.3f", value);
    return ota_nhatanh_publish(sub, val, 0);
}

esp_err_t ota_nhatanh_subscribe(const char *sub_topic) {
    if (!g_state.mqtt) return ESP_FAIL;
    char topic[160];
    snprintf(topic, sizeof(topic), "%s/%s", g_state.topic_prefix, sub_topic);
    int msg_id = esp_mqtt_client_subscribe(g_state.mqtt, topic, 1);
    return msg_id >= 0 ? ESP_OK : ESP_FAIL;
}

bool ota_nhatanh_is_connected(void) {
    return g_state.wifi_connected && g_state.mqtt_connected;
}
