// Single Unity entry point covering all three phone_home host-testable
// modules -- one binary, three TEST_GROUPs (report building, ring
// buffer, heartbeat scheduling), each defined in its own *_test.c file.
#include "unity.h"
#include "unity_fixture.h"

static void run_all_tests(void)
{
    RUN_TEST_GROUP(phone_home_report);
    RUN_TEST_GROUP(phone_home_buffer);
    RUN_TEST_GROUP(phone_home_heartbeat);
}

int main(int argc, char **argv)
{
    UNITY_MAIN_FUNC(run_all_tests);
    return 0;
}
