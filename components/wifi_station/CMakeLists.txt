idf_component_register(SRCS "wifi_station_backoff.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES esp_wifi esp_netif esp_event nvs_flash provisioning)
