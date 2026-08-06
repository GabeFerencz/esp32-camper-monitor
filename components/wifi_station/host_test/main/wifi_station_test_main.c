// Single Unity entry point for the wifi_station host-testable module
// (currently just the backoff delay calculation -- the esp_wifi/esp_event
// connectivity glue in wifi_station.c is ESP-IDF-dependent and is only
// exercised by the compile-only esp32 build, per this repo's convention).
#include "unity.h"
#include "unity_fixture.h"

static void run_all_tests(void)
{
    RUN_TEST_GROUP(wifi_station_backoff);
}

int main(int argc, char **argv)
{
    UNITY_MAIN_FUNC(run_all_tests);
    return 0;
}
