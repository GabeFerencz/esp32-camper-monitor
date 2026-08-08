// Single Unity entry point covering all provisioning host-testable
// modules -- one binary, one TEST_GROUP per module (config completeness,
// fingerprint/digest), each defined in its own *_test.c file.
#include "unity.h"
#include "unity_fixture.h"

static void run_all_tests(void)
{
    RUN_TEST_GROUP(provisioning_validate);
    RUN_TEST_GROUP(provisioning_sha256);
    RUN_TEST_GROUP(provisioning_fingerprint);
}

int main(int argc, char **argv)
{
    UNITY_MAIN_FUNC(run_all_tests);
    return 0;
}
