// Host-target (linux) Unity tests for phone_home_buffer.c. Exercises
// FIFO push/pop order, count/is_full accounting, oldest-eviction on
// overflow, and alert-first priority pop -- pure in-memory structure,
// no networking involved.
#include "phone_home_buffer.h"
#include "unity.h"
#include "unity_fixture.h"

static phone_home_report_t make_report(int64_t id)
{
    phone_home_report_t report = {
        .type = PHONE_HOME_REPORT_HEARTBEAT,
        .ac_state = AC_PRESENCE_PRESENT,
        .uptime_us = id,  // used purely as an identifier in these tests
    };
    return report;
}

static phone_home_report_t make_alert(int64_t id)
{
    phone_home_report_t report = make_report(id);
    report.type = PHONE_HOME_REPORT_AC_ALERT;
    return report;
}

TEST_GROUP(phone_home_buffer);

static phone_home_buffer_t s_buf;

TEST_SETUP(phone_home_buffer)
{
    phone_home_buffer_init(&s_buf);
}

TEST_TEAR_DOWN(phone_home_buffer)
{
}

TEST(phone_home_buffer, starts_empty)
{
    TEST_ASSERT_EQUAL_UINT(0, phone_home_buffer_count(&s_buf));
    TEST_ASSERT_FALSE(phone_home_buffer_is_full(&s_buf));

    phone_home_report_t out;
    TEST_ASSERT_FALSE(phone_home_buffer_pop(&s_buf, &out));
}

TEST(phone_home_buffer, pops_in_fifo_order)
{
    phone_home_report_t a = make_report(1);
    phone_home_report_t b = make_report(2);
    phone_home_report_t c = make_report(3);
    phone_home_buffer_push(&s_buf, &a);
    phone_home_buffer_push(&s_buf, &b);
    phone_home_buffer_push(&s_buf, &c);
    TEST_ASSERT_EQUAL_UINT(3, phone_home_buffer_count(&s_buf));

    phone_home_report_t out;
    TEST_ASSERT_TRUE(phone_home_buffer_pop(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(1, out.uptime_us);
    TEST_ASSERT_TRUE(phone_home_buffer_pop(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(2, out.uptime_us);
    TEST_ASSERT_TRUE(phone_home_buffer_pop(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(3, out.uptime_us);
    TEST_ASSERT_EQUAL_UINT(0, phone_home_buffer_count(&s_buf));
}

TEST(phone_home_buffer, reports_full_at_capacity)
{
    for (int i = 0; i < PHONE_HOME_BUFFER_CAPACITY; i++) {
        phone_home_report_t r = make_report(i);
        phone_home_buffer_push(&s_buf, &r);
    }
    TEST_ASSERT_EQUAL_UINT(PHONE_HOME_BUFFER_CAPACITY, phone_home_buffer_count(&s_buf));
    TEST_ASSERT_TRUE(phone_home_buffer_is_full(&s_buf));
}

TEST(phone_home_buffer, overflow_evicts_oldest_not_newest)
{
    for (int i = 0; i < PHONE_HOME_BUFFER_CAPACITY; i++) {
        phone_home_report_t r = make_report(i);
        phone_home_buffer_push(&s_buf, &r);
    }
    // One more than capacity -- the oldest (id 0) must be evicted, and
    // the buffer must still hold exactly CAPACITY entries, not drop the
    // newest push in its place.
    phone_home_report_t overflow_report = make_report(PHONE_HOME_BUFFER_CAPACITY);
    phone_home_buffer_push(&s_buf, &overflow_report);
    TEST_ASSERT_EQUAL_UINT(PHONE_HOME_BUFFER_CAPACITY, phone_home_buffer_count(&s_buf));

    phone_home_report_t out;
    TEST_ASSERT_TRUE(phone_home_buffer_pop(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(1, out.uptime_us);  // id 0 was evicted

    // Draining the rest confirms FIFO order held and the newest (id
    // CAPACITY) survived through to the end.
    for (int i = 2; i <= PHONE_HOME_BUFFER_CAPACITY; i++) {
        TEST_ASSERT_TRUE(phone_home_buffer_pop(&s_buf, &out));
        TEST_ASSERT_EQUAL_INT64(i, out.uptime_us);
    }
    TEST_ASSERT_EQUAL_UINT(0, phone_home_buffer_count(&s_buf));
}

TEST(phone_home_buffer, alert_first_pops_before_older_heartbeat)
{
    phone_home_report_t heartbeat = make_report(1);
    phone_home_report_t alert = make_alert(2);
    phone_home_buffer_push(&s_buf, &heartbeat);
    phone_home_buffer_push(&s_buf, &alert);

    phone_home_report_t out;
    TEST_ASSERT_TRUE(phone_home_buffer_pop_alert_first(&s_buf, &out));
    TEST_ASSERT_EQUAL(PHONE_HOME_REPORT_AC_ALERT, out.type);
    TEST_ASSERT_EQUAL_INT64(2, out.uptime_us);
}

TEST(phone_home_buffer, alert_first_oldest_alert_wins_among_several)
{
    phone_home_report_t alert_a = make_alert(1);
    phone_home_report_t heartbeat = make_report(2);
    phone_home_report_t alert_b = make_alert(3);
    phone_home_buffer_push(&s_buf, &alert_a);
    phone_home_buffer_push(&s_buf, &heartbeat);
    phone_home_buffer_push(&s_buf, &alert_b);

    phone_home_report_t out;
    TEST_ASSERT_TRUE(phone_home_buffer_pop_alert_first(&s_buf, &out));
    TEST_ASSERT_EQUAL(PHONE_HOME_REPORT_AC_ALERT, out.type);
    TEST_ASSERT_EQUAL_INT64(1, out.uptime_us);
}

TEST(phone_home_buffer, alert_first_falls_back_to_fifo_with_no_alert_pending)
{
    phone_home_report_t a = make_report(1);
    phone_home_report_t b = make_report(2);
    phone_home_buffer_push(&s_buf, &a);
    phone_home_buffer_push(&s_buf, &b);

    phone_home_report_t out;
    TEST_ASSERT_TRUE(phone_home_buffer_pop_alert_first(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(1, out.uptime_us);
    TEST_ASSERT_TRUE(phone_home_buffer_pop_alert_first(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(2, out.uptime_us);
}

TEST(phone_home_buffer, alert_first_removal_preserves_order_of_remainder)
{
    phone_home_report_t h1 = make_report(1);
    phone_home_report_t a2 = make_alert(2);
    phone_home_report_t h3 = make_report(3);
    phone_home_buffer_push(&s_buf, &h1);
    phone_home_buffer_push(&s_buf, &a2);
    phone_home_buffer_push(&s_buf, &h3);

    phone_home_report_t out;
    TEST_ASSERT_TRUE(phone_home_buffer_pop_alert_first(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(2, out.uptime_us);  // the alert, removed from the middle

    // What's left (h1, h3) must still pop in their original relative order.
    TEST_ASSERT_TRUE(phone_home_buffer_pop_alert_first(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(1, out.uptime_us);
    TEST_ASSERT_TRUE(phone_home_buffer_pop_alert_first(&s_buf, &out));
    TEST_ASSERT_EQUAL_INT64(3, out.uptime_us);
    TEST_ASSERT_EQUAL_UINT(0, phone_home_buffer_count(&s_buf));
}

TEST_GROUP_RUNNER(phone_home_buffer)
{
    RUN_TEST_CASE(phone_home_buffer, starts_empty);
    RUN_TEST_CASE(phone_home_buffer, pops_in_fifo_order);
    RUN_TEST_CASE(phone_home_buffer, reports_full_at_capacity);
    RUN_TEST_CASE(phone_home_buffer, overflow_evicts_oldest_not_newest);
    RUN_TEST_CASE(phone_home_buffer, alert_first_pops_before_older_heartbeat);
    RUN_TEST_CASE(phone_home_buffer, alert_first_oldest_alert_wins_among_several);
    RUN_TEST_CASE(phone_home_buffer, alert_first_falls_back_to_fifo_with_no_alert_pending);
    RUN_TEST_CASE(phone_home_buffer, alert_first_removal_preserves_order_of_remainder);
}
