idf_component_register(
  SRCS
    "src/ota_nhatanh.c"
    "src/ota_nhatanh_wifi.c"
    "src/ota_nhatanh_mqtt.c"
    "src/ota_nhatanh_update.c"
  INCLUDE_DIRS
    "include"
  REQUIRES
    nvs_flash esp_wifi esp_event esp_netif
    mqtt esp_https_ota esp_http_client app_update
    json
)
