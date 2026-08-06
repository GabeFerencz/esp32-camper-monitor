idf_component_register(SRCS "wifi_station_test_main.c"
                             "wifi_station_backoff_test.c"
                             "../../wifi_station_backoff.c"
                        INCLUDE_DIRS "../.."
                        PRIV_REQUIRES unity)
