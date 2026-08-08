// Single Unity entry point covering all phone_home host-testable
// modules -- one binary, one TEST_GROUP per module (report building, ring
// buffer, heartbeat scheduling, poll-loop deadline scheduling), each
// defined in its own *_test.c file.
#include "unity.h"
#include "unity_fixture.h"

static void run_all_tests(void)
{
    RUN_TEST_GROUP(phone_home_report);
    RUN_TEST_GROUP(phone_home_buffer);
    RUN_TEST_GROUP(phone_home_heartbeat);
    RUN_TEST_GROUP(phone_home_poll_schedule);
}

int main(int argc, char **argv)
{
    UNITY_MAIN_FUNC(run_all_tests);
    return 0;
}
