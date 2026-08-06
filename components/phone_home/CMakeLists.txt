idf_component_register(SRCS "phone_home_report.c" "phone_home_buffer.c" "phone_home_heartbeat.c"
                             "phone_home_sender.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES provisioning ac_presence esp_http_client mbedtls)
