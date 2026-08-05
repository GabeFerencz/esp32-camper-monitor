idf_component_register(SRCS "provisioning_validate.c" "provisioning_store.c" "provisioning_console.c" "provisioning.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES nvs_flash console esp_system)
