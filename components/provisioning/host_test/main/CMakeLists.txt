idf_component_register(SRCS "provisioning_test_main.c"
                             "provisioning_validate_test.c"
                             "provisioning_fingerprint_test.c"
                             "../../provisioning_validate.c"
                             "../../provisioning_fingerprint.c"
                             "../../provisioning_sha256.c"
                        INCLUDE_DIRS "../.."
                        PRIV_REQUIRES unity)
