// Host-target (linux) Unity tests for wifi_station_backoff.c. Pure
// integer math, no ESP-IDF/esp_wifi/esp_event dependency involved.
#include "wifi_station_backoff.h"
#include "unity.h"
#include "unity_fixture.h"

TEST_GROUP(wifi_station_backoff);

TEST_SETUP(wifi_station_backoff)
{
}

TEST_TEAR_DOWN(wifi_station_backoff)
{
}

TEST(wifi_station_backoff, first_attempt_is_base_delay)
{
    TEST_ASSERT_EQUAL_UINT32(WIFI_RECONNECT_BASE_DELAY_MS,
                              wifi_station_backoff_delay_ms(0));
}

TEST(wifi_station_backoff, delay_doubles_each_attempt_until_cap)
{
    TEST_ASSERT_EQUAL_UINT32(1000, wifi_station_backoff_delay_ms(0));
    TEST_ASSERT_EQUAL_UINT32(2000, wifi_station_backoff_delay_ms(1));
    TEST_ASSERT_EQUAL_UINT32(4000, wifi_station_backoff_delay_ms(2));
    TEST_ASSERT_EQUAL_UINT32(8000, wifi_station_backoff_delay_ms(3));
    TEST_ASSERT_EQUAL_UINT32(16000, wifi_station_backoff_delay_ms(4));
    TEST_ASSERT_EQUAL_UINT32(32000, wifi_station_backoff_delay_ms(5));
}

TEST(wifi_station_backoff, delay_caps_at_max_delay)
{
    TEST_ASSERT_EQUAL_UINT32(WIFI_RECONNECT_MAX_DELAY_MS,
                              wifi_station_backoff_delay_ms(WIFI_RECONNECT_BACKOFF_MAX_SHIFT));
}

TEST(wifi_station_backoff, delay_stays_capped_for_an_extended_outage)
{
    TEST_ASSERT_EQUAL_UINT32(WIFI_RECONNECT_MAX_DELAY_MS,
                              wifi_station_backoff_delay_ms(1000));
    TEST_ASSERT_EQUAL_UINT32(WIFI_RECONNECT_MAX_DELAY_MS,
                              wifi_station_backoff_delay_ms(UINT32_MAX));
}

TEST_GROUP_RUNNER(wifi_station_backoff)
{
    RUN_TEST_CASE(wifi_station_backoff, first_attempt_is_base_delay);
    RUN_TEST_CASE(wifi_station_backoff, delay_doubles_each_attempt_until_cap);
    RUN_TEST_CASE(wifi_station_backoff, delay_caps_at_max_delay);
    RUN_TEST_CASE(wifi_station_backoff, delay_stays_capped_for_an_extended_outage);
}
