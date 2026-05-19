/**
 * ota_nhatanh_update.c - OTA pull qua esp_https_ota (theo manifest URL)
 *
 * Manifest format (ESPHome compatible):
 *   { "name": "...", "version": "...", "builds": [ { "ota": { "path": "...", "md5": "..." } } ] }
 *
 * Implementation: GET manifest.json → parse JSON → tải bin từ builds[0].ota.path.
 */
#include "ota_nhatanh.h"
#include "ota_nhatanh_internal.h"

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "cJSON.h"

static const char *TAG = "ota_nhatanh_update";

static esp_err_t http_get_body(const char *url, char **out_buf, int *out_len) {
    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 15000,
        .crt_bundle_attach = NULL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return ESP_FAIL;

    esp_err_t r = esp_http_client_open(client, 0);
    if (r != ESP_OK) {
        esp_http_client_cleanup(client);
        return r;
    }
    int cl = esp_http_client_fetch_headers(client);
    if (cl <= 0 || cl > 32 * 1024) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    char *buf = malloc(cl + 1);
    if (!buf) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }
    int got = esp_http_client_read(client, buf, cl);
    buf[got > 0 ? got : 0] = '\0';
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (got <= 0) {
        free(buf);
        return ESP_FAIL;
    }
    *out_buf = buf;
    *out_len = got;
    return ESP_OK;
}

esp_err_t ota_nhatanh_check_ota_now(void) {
    if (!g_state.cfg.ota_manifest_url) return ESP_OK;
    ESP_LOGI(TAG, "Check manifest %s", g_state.cfg.ota_manifest_url);

    char *body = NULL;
    int len = 0;
    if (http_get_body(g_state.cfg.ota_manifest_url, &body, &len) != ESP_OK) {
        ESP_LOGW(TAG, "Fail tai manifest");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) return ESP_FAIL;

    cJSON *builds = cJSON_GetObjectItem(root, "builds");
    cJSON *first = builds ? cJSON_GetArrayItem(builds, 0) : NULL;
    cJSON *ota = first ? cJSON_GetObjectItem(first, "ota") : NULL;
    cJSON *path = ota ? cJSON_GetObjectItem(ota, "path") : NULL;
    cJSON *version = cJSON_GetObjectItem(root, "version");

    if (!path || !cJSON_IsString(path)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // So sanh version voi version hien tai (skip neu trung)
    const esp_app_desc_t *cur = esp_app_get_description();
    if (version && cJSON_IsString(version) && strcmp(version->valuestring, cur->version) == 0) {
        ESP_LOGI(TAG, "Da o version moi nhat (%s)", cur->version);
        cJSON_Delete(root);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Tai firmware: %s", path->valuestring);

    esp_http_client_config_t hcfg = {
        .url = path->valuestring,
        .timeout_ms = 60000,
        .crt_bundle_attach = NULL,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ocfg = { .http_config = &hcfg };
    esp_err_t r = esp_https_ota(&ocfg);

    cJSON_Delete(root);

    if (r == ESP_OK) {
        ESP_LOGI(TAG, "OTA xong, restart...");
        ota_nhatanh_publish("ota/ket-qua",
                          "{\"ket_qua\":\"thanh_cong\"}", 0);
        vTaskDelay(pdMS_TO_TICKS(1500));
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA fail: %s", esp_err_to_name(r));
        ota_nhatanh_publish("ota/ket-qua",
                          "{\"ket_qua\":\"that_bai\"}", 0);
    }
    return r;
}
