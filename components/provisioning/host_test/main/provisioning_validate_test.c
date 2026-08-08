// Host-target (linux) Unity tests for provisioning_validate.c. Exercises
// completeness checks (each required field missing individually) and the
// phone-home host's scheme-prefix rejection, without touching real NVS.
#include <string.h>

#include "provisioning_schema.h"
#include "provisioning_validate.h"
#include "unity.h"
#include "unity_fixture.h"

static provisioning_config_t make_complete_config(void)
{
    provisioning_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strcpy(cfg.wifi_ssid, "site-network");
    strcpy(cfg.wifi_pass, "hunter2hunter2");
    strcpy(cfg.cf_client_id, "abc123.access");
    strcpy(cfg.cf_client_secret, "supersecrethex");
    strcpy(cfg.phone_host, "subdom.example.com");
    strcpy(cfg.webhook_id, "a1b2c3d4e5f6");
    return cfg;
}

TEST_GROUP(provisioning_validate);

TEST_SETUP(provisioning_validate)
{
}

TEST_TEAR_DOWN(provisioning_validate)
{
}

TEST(provisioning_validate, complete_config_is_valid)
{
    provisioning_config_t cfg = make_complete_config();
    TEST_ASSERT_TRUE(provisioning_config_is_complete(&cfg));
}

TEST(provisioning_validate, missing_wifi_ssid_is_incomplete)
{
    provisioning_config_t cfg = make_complete_config();
    cfg.wifi_ssid[0] = '\0';
    TEST_ASSERT_FALSE(provisioning_config_is_complete(&cfg));
}

TEST(provisioning_validate, missing_wifi_pass_is_incomplete)
{
    provisioning_config_t cfg = make_complete_config();
    cfg.wifi_pass[0] = '\0';
    TEST_ASSERT_FALSE(provisioning_config_is_complete(&cfg));
}

TEST(provisioning_validate, missing_cf_client_id_is_incomplete)
{
    provisioning_config_t cfg = make_complete_config();
    cfg.cf_client_id[0] = '\0';
    TEST_ASSERT_FALSE(provisioning_config_is_complete(&cfg));
}

TEST(provisioning_validate, missing_cf_client_secret_is_incomplete)
{
    provisioning_config_t cfg = make_complete_config();
    cfg.cf_client_secret[0] = '\0';
    TEST_ASSERT_FALSE(provisioning_config_is_complete(&cfg));
}

TEST(provisioning_validate, missing_webhook_id_is_incomplete)
{
    provisioning_config_t cfg = make_complete_config();
    cfg.webhook_id[0] = '\0';
    TEST_ASSERT_FALSE(provisioning_config_is_complete(&cfg));
}

TEST(provisioning_validate, empty_phone_host_is_incomplete)
{
    provisioning_config_t cfg = make_complete_config();
    cfg.phone_host[0] = '\0';
    TEST_ASSERT_FALSE(provisioning_host_is_valid(cfg.phone_host));
    TEST_ASSERT_FALSE(provisioning_config_is_complete(&cfg));
}

TEST(provisioning_validate, phone_host_with_http_scheme_is_rejected)
{
    provisioning_config_t cfg = make_complete_config();
    strcpy(cfg.phone_host, "http://subdom.example.com");
    TEST_ASSERT_FALSE(provisioning_host_is_valid(cfg.phone_host));
    TEST_ASSERT_FALSE(provisioning_config_is_complete(&cfg));
}

TEST(provisioning_validate, phone_host_with_https_scheme_is_rejected)
{
    TEST_ASSERT_FALSE(provisioning_host_is_valid("https://subdom.example.com"));
}

TEST(provisioning_validate, bare_hostname_is_valid)
{
    TEST_ASSERT_TRUE(provisioning_host_is_valid("subdom.example.com"));
}

TEST_GROUP_RUNNER(provisioning_validate)
{
    RUN_TEST_CASE(provisioning_validate, complete_config_is_valid);
    RUN_TEST_CASE(provisioning_validate, missing_wifi_ssid_is_incomplete);
    RUN_TEST_CASE(provisioning_validate, missing_wifi_pass_is_incomplete);
    RUN_TEST_CASE(provisioning_validate, missing_cf_client_id_is_incomplete);
    RUN_TEST_CASE(provisioning_validate, missing_cf_client_secret_is_incomplete);
    RUN_TEST_CASE(provisioning_validate, missing_webhook_id_is_incomplete);
    RUN_TEST_CASE(provisioning_validate, empty_phone_host_is_incomplete);
    RUN_TEST_CASE(provisioning_validate, phone_host_with_http_scheme_is_rejected);
    RUN_TEST_CASE(provisioning_validate, phone_host_with_https_scheme_is_rejected);
    RUN_TEST_CASE(provisioning_validate, bare_hostname_is_valid);
}

static void run_all_tests(void)
{
    RUN_TEST_GROUP(provisioning_validate);
}

int main(int argc, char **argv)
{
    UNITY_MAIN_FUNC(run_all_tests);
    return 0;
}
