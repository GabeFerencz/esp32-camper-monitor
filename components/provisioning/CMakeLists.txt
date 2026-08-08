idf_component_register(SRCS "provisioning_validate.c" "provisioning_store.c" "provisioning_console.c" "provisioning.c"
                             "provisioning_fingerprint.c" "provisioning_sha256.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES nvs_flash console esp_system)
