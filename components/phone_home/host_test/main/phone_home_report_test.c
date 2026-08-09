// Host-target (linux) Unity tests for phone_home_report.c. Exercises
// request building for both report types, that CF-Access header values
// pass through untouched, and that an oversized field is rejected
// instead of silently truncated, without touching real networking.
#include <string.h>

#include "phone_home_report.h"
#include "provisioning_schema.h"
#include "unity.h"
#include "unity_fixture.h"

static provisioning_config_t make_cfg(void)
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

static void fill_chars(char *buf, size_t buf_size, char c)
{
    memset(buf, c, buf_size - 1);
    buf[buf_size - 1] = '\0';
}

TEST_GROUP(phone_home_report);

TEST_SETUP(phone_home_report)
{
}

TEST_TEAR_DOWN(phone_home_report)
{
}

TEST(phone_home_report, heartbeat_builds_expected_url_and_body)
{
    provisioning_config_t cfg = make_cfg();
    phone_home_report_t report = {
        .type = PHONE_HOME_REPORT_HEARTBEAT,
        .ac_state = AC_PRESENCE_PRESENT,
        .uptime_us = 123456789,
    };
    phone_home_request_t out;

    TEST_ASSERT_TRUE(phone_home_report_build_request(&cfg, &report, &out));
    TEST_ASSERT_EQUAL_STRING("https://subdom.example.com/api/webhook/a1b2c3d4e5f6", out.url);
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"heartbeat\",\"ac_state\":\"present\",\"uptime_us\":123456789}", out.body);
}

TEST(phone_home_report, ac_alert_builds_expected_body)
{
    provisioning_config_t cfg = make_cfg();
    phone_home_report_t report = {
        .type = PHONE_HOME_REPORT_AC_ALERT,
        .ac_state = AC_PRESENCE_LOST,
        .uptime_us = 42,
    };
    phone_home_request_t out;

    TEST_ASSERT_TRUE(phone_home_report_build_request(&cfg, &report, &out));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"ac_alert\",\"ac_state\":\"lost\",\"uptime_us\":42}", out.body);
}

TEST(phone_home_report, large_uptime_preserves_full_precision)
{
    // Exercises the reason build_body() renders uptime_us as a raw token
    // instead of a cJSON number: cJSON's integer mirror is a 32-bit int,
    // so a naive cJSON_AddNumberToObject() would wrap a value like this
    // one (well past INT32_MAX) instead of printing it exactly.
    provisioning_config_t cfg = make_cfg();
    phone_home_report_t report = {
        .type = PHONE_HOME_REPORT_HEARTBEAT,
        .ac_state = AC_PRESENCE_PRESENT,
        .uptime_us = 5000000000LL,
    };
    phone_home_request_t out;

    TEST_ASSERT_TRUE(phone_home_report_build_request(&cfg, &report, &out));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"heartbeat\",\"ac_state\":\"present\",\"uptime_us\":5000000000}", out.body);
}

TEST(phone_home_report, cf_access_header_values_pass_through)
{
    provisioning_config_t cfg = make_cfg();
    phone_home_report_t report = {
        .type = PHONE_HOME_REPORT_HEARTBEAT,
        .ac_state = AC_PRESENCE_PRESENT,
        .uptime_us = 0,
    };
    phone_home_request_t out;

    TEST_ASSERT_TRUE(phone_home_report_build_request(&cfg, &report, &out));
    TEST_ASSERT_EQUAL_STRING(cfg.cf_client_id, out.cf_client_id);
    TEST_ASSERT_EQUAL_STRING(cfg.cf_client_secret, out.cf_client_secret);
}

TEST(phone_home_report, oversized_url_fields_are_rejected)
{
    provisioning_config_t cfg = make_cfg();
    fill_chars(cfg.phone_host, sizeof(cfg.phone_host), 'h');
    fill_chars(cfg.webhook_id, sizeof(cfg.webhook_id), 'w');
    phone_home_report_t report = {
        .type = PHONE_HOME_REPORT_HEARTBEAT,
        .ac_state = AC_PRESENCE_PRESENT,
        .uptime_us = 0,
    };
    phone_home_request_t out;

    TEST_ASSERT_FALSE(phone_home_report_build_request(&cfg, &report, &out));
}

TEST(phone_home_report, null_arguments_are_rejected)
{
    provisioning_config_t cfg = make_cfg();
    phone_home_report_t report = {
        .type = PHONE_HOME_REPORT_HEARTBEAT,
        .ac_state = AC_PRESENCE_PRESENT,
        .uptime_us = 0,
    };
    phone_home_request_t out;

    TEST_ASSERT_FALSE(phone_home_report_build_request(NULL, &report, &out));
    TEST_ASSERT_FALSE(phone_home_report_build_request(&cfg, NULL, &out));
    TEST_ASSERT_FALSE(phone_home_report_build_request(&cfg, &report, NULL));
}

TEST_GROUP_RUNNER(phone_home_report)
{
    RUN_TEST_CASE(phone_home_report, heartbeat_builds_expected_url_and_body);
    RUN_TEST_CASE(phone_home_report, ac_alert_builds_expected_body);
    RUN_TEST_CASE(phone_home_report, large_uptime_preserves_full_precision);
    RUN_TEST_CASE(phone_home_report, cf_access_header_values_pass_through);
    RUN_TEST_CASE(phone_home_report, oversized_url_fields_are_rejected);
    RUN_TEST_CASE(phone_home_report, null_arguments_are_rejected);
}
