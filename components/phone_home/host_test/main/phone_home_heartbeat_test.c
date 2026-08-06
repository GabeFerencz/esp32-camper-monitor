// Host-target (linux) Unity tests for phone_home_heartbeat.c. Exercises
// the due/not-due decision against a fake clock -- no real timer, no
// task, no networking involved.
#include "phone_home_heartbeat.h"
#include "unity.h"
#include "unity_fixture.h"

#define TEST_INTERVAL_US (60 * 1000000LL)  // 60s, arbitrary for these tests

TEST_GROUP(phone_home_heartbeat);

static phone_home_heartbeat_t s_hb;

TEST_SETUP(phone_home_heartbeat)
{
    phone_home_heartbeat_init(&s_hb, TEST_INTERVAL_US);
}

TEST_TEAR_DOWN(phone_home_heartbeat)
{
}

TEST(phone_home_heartbeat, due_immediately_before_first_send)
{
    TEST_ASSERT_TRUE(phone_home_heartbeat_is_due(&s_hb, 0));
    TEST_ASSERT_TRUE(phone_home_heartbeat_is_due(&s_hb, TEST_INTERVAL_US * 10));
}

TEST(phone_home_heartbeat, not_due_before_interval_elapses)
{
    phone_home_heartbeat_mark_sent(&s_hb, 1000);
    TEST_ASSERT_FALSE(phone_home_heartbeat_is_due(&s_hb, 1000 + TEST_INTERVAL_US - 1));
}

TEST(phone_home_heartbeat, due_once_interval_elapses)
{
    phone_home_heartbeat_mark_sent(&s_hb, 1000);
    TEST_ASSERT_TRUE(phone_home_heartbeat_is_due(&s_hb, 1000 + TEST_INTERVAL_US));
}

TEST(phone_home_heartbeat, mark_sent_resets_the_clock)
{
    phone_home_heartbeat_mark_sent(&s_hb, 1000);
    TEST_ASSERT_TRUE(phone_home_heartbeat_is_due(&s_hb, 1000 + TEST_INTERVAL_US));

    phone_home_heartbeat_mark_sent(&s_hb, 1000 + TEST_INTERVAL_US);
    TEST_ASSERT_FALSE(phone_home_heartbeat_is_due(&s_hb, 1000 + TEST_INTERVAL_US + 1));
}

TEST_GROUP_RUNNER(phone_home_heartbeat)
{
    RUN_TEST_CASE(phone_home_heartbeat, due_immediately_before_first_send);
    RUN_TEST_CASE(phone_home_heartbeat, not_due_before_interval_elapses);
    RUN_TEST_CASE(phone_home_heartbeat, due_once_interval_elapses);
    RUN_TEST_CASE(phone_home_heartbeat, mark_sent_resets_the_clock);
}
