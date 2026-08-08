// Host-target (linux) Unity tests for phone_home_poll_schedule.c.
// Exercises the absolute-deadline grid against a fake clock -- no real
// timer, no task, no queue involved. Regression coverage for #21: an
// early wake (advance() called before the deadline is reached) must
// leave the grid untouched, at any offset within the period.
#include "phone_home_poll_schedule.h"
#include "unity.h"
#include "unity_fixture.h"

#define TEST_PERIOD_US (5 * 1000000LL)  // 5s, matches PHONE_HOME_TASK_POLL_MS in spirit

TEST_GROUP(phone_home_poll_schedule);

static phone_home_poll_schedule_t s_sched;

TEST_SETUP(phone_home_poll_schedule)
{
    phone_home_poll_schedule_init(&s_sched, TEST_PERIOD_US, 0);
}

TEST_TEAR_DOWN(phone_home_poll_schedule)
{
}

TEST(phone_home_poll_schedule, initial_remaining_is_full_period)
{
    TEST_ASSERT_EQUAL_INT64(TEST_PERIOD_US, phone_home_poll_schedule_remaining_us(&s_sched, 0));
}

TEST(phone_home_poll_schedule, remaining_counts_down_linearly)
{
    TEST_ASSERT_EQUAL_INT64(TEST_PERIOD_US - 2000000, phone_home_poll_schedule_remaining_us(&s_sched, 2000000));
}

TEST(phone_home_poll_schedule, remaining_never_negative_past_deadline)
{
    TEST_ASSERT_EQUAL_INT64(0, phone_home_poll_schedule_remaining_us(&s_sched, TEST_PERIOD_US + 4000000));
}

TEST(phone_home_poll_schedule, advance_is_noop_before_deadline_reached_early_offset)
{
    // Early wake near the start of the period -- mirrors the issue's own
    // scenario shape (alert arrives well before the poll deadline).
    phone_home_poll_schedule_advance(&s_sched, 1000000);
    TEST_ASSERT_EQUAL_INT64(TEST_PERIOD_US - 1000000, phone_home_poll_schedule_remaining_us(&s_sched, 1000000));
}

TEST(phone_home_poll_schedule, advance_is_noop_before_deadline_reached_late_offset)
{
    // Early wake near the end of the period -- confirms the fix
    // generalizes across offsets, not just the one from the device log.
    phone_home_poll_schedule_advance(&s_sched, 4500000);
    TEST_ASSERT_EQUAL_INT64(TEST_PERIOD_US - 4500000, phone_home_poll_schedule_remaining_us(&s_sched, 4500000));
}

TEST(phone_home_poll_schedule, advance_across_multiple_early_wakes_in_same_period)
{
    phone_home_poll_schedule_advance(&s_sched, 1000000);
    phone_home_poll_schedule_advance(&s_sched, 2000000);
    phone_home_poll_schedule_advance(&s_sched, 3500000);
    TEST_ASSERT_EQUAL_INT64(TEST_PERIOD_US, phone_home_poll_schedule_remaining_us(&s_sched, 0));
}

TEST(phone_home_poll_schedule, advance_moves_deadline_by_exactly_one_period_at_normal_expiry)
{
    // A few ticks of scheduling jitter past the boundary still advances
    // from the *old* deadline, not from now -- the grid stays fixed.
    phone_home_poll_schedule_advance(&s_sched, TEST_PERIOD_US + 50);
    TEST_ASSERT_EQUAL_INT64(0, phone_home_poll_schedule_remaining_us(&s_sched, 2 * TEST_PERIOD_US));
    TEST_ASSERT_EQUAL_INT64(2 * TEST_PERIOD_US - (TEST_PERIOD_US + 50),
                             phone_home_poll_schedule_remaining_us(&s_sched, TEST_PERIOD_US + 50));
}

TEST(phone_home_poll_schedule, advance_catch_up_guard_snaps_forward_after_stall)
{
    // Stalled well past the deadline (>1 period missed) -- must snap
    // forward once, not replay every missed period into a near-zero
    // timeout burst.
    int64_t stalled_now = TEST_PERIOD_US + (TEST_PERIOD_US * 2);  // 3 periods in, deadline was at 1
    phone_home_poll_schedule_advance(&s_sched, stalled_now);
    TEST_ASSERT_EQUAL_INT64(TEST_PERIOD_US, phone_home_poll_schedule_remaining_us(&s_sched, stalled_now));
}

TEST(phone_home_poll_schedule, advance_catch_up_boundary_exactly_one_period_over)
{
    // Stall of just over one full period past the deadline still
    // triggers exactly one snap, not a partial/fractional catch-up.
    int64_t stalled_now = TEST_PERIOD_US + TEST_PERIOD_US + 1;
    phone_home_poll_schedule_advance(&s_sched, stalled_now);
    TEST_ASSERT_EQUAL_INT64(TEST_PERIOD_US, phone_home_poll_schedule_remaining_us(&s_sched, stalled_now));
}

TEST_GROUP_RUNNER(phone_home_poll_schedule)
{
    RUN_TEST_CASE(phone_home_poll_schedule, initial_remaining_is_full_period);
    RUN_TEST_CASE(phone_home_poll_schedule, remaining_counts_down_linearly);
    RUN_TEST_CASE(phone_home_poll_schedule, remaining_never_negative_past_deadline);
    RUN_TEST_CASE(phone_home_poll_schedule, advance_is_noop_before_deadline_reached_early_offset);
    RUN_TEST_CASE(phone_home_poll_schedule, advance_is_noop_before_deadline_reached_late_offset);
    RUN_TEST_CASE(phone_home_poll_schedule, advance_across_multiple_early_wakes_in_same_period);
    RUN_TEST_CASE(phone_home_poll_schedule, advance_moves_deadline_by_exactly_one_period_at_normal_expiry);
    RUN_TEST_CASE(phone_home_poll_schedule, advance_catch_up_guard_snaps_forward_after_stall);
    RUN_TEST_CASE(phone_home_poll_schedule, advance_catch_up_boundary_exactly_one_period_over);
}
