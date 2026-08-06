idf_component_register(SRCS "phone_home_report.c" "phone_home_buffer.c" "phone_home_heartbeat.c"
                             "phone_home_sender.c" "phone_home_task.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES provisioning ac_presence esp_http_client mbedtls
                                      wifi_station esp_timer)
