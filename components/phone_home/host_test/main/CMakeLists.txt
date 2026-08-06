idf_component_register(SRCS "phone_home_test_main.c"
                             "phone_home_report_test.c"
                             "phone_home_buffer_test.c"
                             "phone_home_heartbeat_test.c"
                             "../../phone_home_report.c"
                             "../../phone_home_buffer.c"
                             "../../phone_home_heartbeat.c"
                        INCLUDE_DIRS "../.." "../../../provisioning" "../../../ac_presence"
                        PRIV_REQUIRES unity)
