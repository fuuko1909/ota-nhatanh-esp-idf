/**
 * ota_nhatanh_mqtt.c - esp-mqtt TLS client
 */
#include "ota_nhatanh_internal.h"

#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "ota_nhatanh_mqtt";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                              int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t evt = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        g_state.mqtt_connected = true;
        {
            char topic[160];
            snprintf(topic, sizeof(topic), "%s/trang-thai", g_state.topic_prefix);
            esp_mqtt_client_publish(g_state.mqtt, topic, "online", 6, 1, 1);
            snprintf(topic, sizeof(topic), "%s/lenh/#", g_state.topic_prefix);
            esp_mqtt_client_subscribe(g_state.mqtt, topic, 1);
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        g_state.mqtt_connected = false;
        break;
    case MQTT_EVENT_DATA:
        {
            char *t = strndup(evt->topic, evt->topic_len);
            char *p = strndup(evt->data, evt->data_len);
            // Handle lenh/*
            if (t && strstr(t, "/lenh/")) {
                char *last = strrchr(t, '/');
                if (last && strcmp(last + 1, "ota") == 0) {
                    ota_nhatanh_check_ota_now();
                } else if (last && strcmp(last + 1, "reboot") == 0) {
                    vTaskDelay(pdMS_TO_TICKS(500));
                    esp_restart();
                }
            }
            if (g_state.cfg.on_mqtt_message) {
                g_state.cfg.on_mqtt_message(t, evt->data, evt->data_len);
            }
            free(t); free(p);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;
    default:
        break;
    }
}

esp_err_t ota_nhatanh_mqtt_start(void) {
    char broker_uri[160];
    snprintf(broker_uri, sizeof(broker_uri), "mqtts://%s:%u",
             g_state.cfg.mqtt_host, g_state.cfg.mqtt_port);

    char lwt_topic[160];
    snprintf(lwt_topic, sizeof(lwt_topic), "%s/trang-thai", g_state.topic_prefix);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
        .credentials.username = g_state.cfg.mqtt_user,
        .credentials.client_id = g_state.cfg.device_id,
        .credentials.authentication.password = g_state.cfg.mqtt_password,
        .session.last_will = {
            .topic = lwt_topic,
            .msg = "offline",
            .msg_len = 7,
            .qos = 1,
            .retain = 1,
        },
        .session.keepalive = 60,
    };
    if (g_state.cfg.insecure_tls) {
        mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
        mqtt_cfg.broker.verification.use_global_ca_store = false;
    } else {
        // Sử dụng built-in cert bundle (Let's Encrypt nằm trong default bundle)
        mqtt_cfg.broker.verification.crt_bundle_attach = NULL;
    }

    g_state.mqtt = esp_mqtt_client_init(&mqtt_cfg);
    if (!g_state.mqtt) return ESP_FAIL;
    esp_mqtt_client_register_event(g_state.mqtt, ESP_EVENT_ANY_ID,
                                  mqtt_event_handler, NULL);
    return esp_mqtt_client_start(g_state.mqtt);
}
