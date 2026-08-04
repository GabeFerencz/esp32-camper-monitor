idf_component_register(SRCS "ac_presence_debounce.c" "ac_presence_gpio.c" "ac_presence.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES esp_driver_gpio esp_timer)
