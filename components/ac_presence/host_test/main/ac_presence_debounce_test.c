// Host-target (linux) Unity tests for the AC-presence debounce state
// machine. Fabricates raw sample sequences — clean, bouncy, and a brief
// genuine blip — and asserts the confirmed-transition event fires
// exactly once per genuine change, never once per bounce.
#include <string.h>

#include "ac_presence_debounce.h"
#include "unity.h"
#include "unity_fixture.h"

#define TEST_DEBOUNCE_US 10000  // 10ms; short window keeps test timestamps readable

typedef struct {
    int call_count;
    ac_presence_state_t states[8];
    void *ctxs[8];
} alert_spy_t;

static alert_spy_t s_spy;

static void alert_spy_cb(ac_presence_state_t new_state, void *ctx)
{
    TEST_ASSERT_LESS_THAN(8, s_spy.call_count);
    s_spy.states[s_spy.call_count] = new_state;
    s_spy.ctxs[s_spy.call_count] = ctx;
    s_spy.call_count++;
}

TEST_GROUP(ac_presence_debounce);

TEST_SETUP(ac_presence_debounce)
{
    memset(&s_spy, 0, sizeof(s_spy));
}

TEST_TEAR_DOWN(ac_presence_debounce)
{
}

// Steady HIGH, then LOW held well past the debounce window: exactly one
// confirmed transition, firing the instant the window elapses.
TEST(ac_presence_debounce, clean_transition)
{
    ac_presence_debounce_t db;
    int dummy_ctx;
    ac_presence_debounce_init(&db, AC_PRESENCE_PRESENT, TEST_DEBOUNCE_US);
    ac_presence_set_alert_cb(&db, alert_spy_cb, &dummy_ctx);

    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, true, 0));
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 2000));
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 4000));
    TEST_ASSERT_EQUAL(0, s_spy.call_count);

    // Window elapses exactly here (stable since t=2000, debounce=10000).
    TEST_ASSERT_TRUE(ac_presence_debounce_process(&db, false, 12000));
    TEST_ASSERT_EQUAL(1, s_spy.call_count);
    TEST_ASSERT_EQUAL(AC_PRESENCE_LOST, s_spy.states[0]);
    TEST_ASSERT_EQUAL_PTR(&dummy_ctx, s_spy.ctxs[0]);
    TEST_ASSERT_EQUAL(AC_PRESENCE_LOST, db.state);

    // Further samples in the same (now-confirmed) state fire nothing more.
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 14000));
    TEST_ASSERT_EQUAL(1, s_spy.call_count);
}

// Rapid flicker within the debounce window before settling: zero events
// during the bounce, exactly one once it actually settles.
TEST(ac_presence_debounce, bouncy_transition)
{
    ac_presence_debounce_t db;
    ac_presence_debounce_init(&db, AC_PRESENCE_PRESENT, TEST_DEBOUNCE_US);
    ac_presence_set_alert_cb(&db, alert_spy_cb, NULL);

    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, true, 0));
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 2000));  // bounce start
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, true, 4000));   // bounce back
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 6000));  // bounce again
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 8000));  // starting to settle
    TEST_ASSERT_EQUAL(0, s_spy.call_count);

    // Settled at t=6000; window elapses at t=16000.
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 15000));
    TEST_ASSERT_TRUE(ac_presence_debounce_process(&db, false, 16000));

    TEST_ASSERT_EQUAL(1, s_spy.call_count);
    TEST_ASSERT_EQUAL(AC_PRESENCE_LOST, s_spy.states[0]);
}

// A real outage that's brief overall but held continuously longer than
// the debounce window must still be reported — both the loss and the
// restore, neither suppressed just because the whole thing was short.
TEST(ac_presence_debounce, brief_genuine_blip)
{
    ac_presence_debounce_t db;
    ac_presence_debounce_init(&db, AC_PRESENCE_PRESENT, TEST_DEBOUNCE_US);
    ac_presence_set_alert_cb(&db, alert_spy_cb, NULL);

    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, true, 0));
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 2000));
    TEST_ASSERT_TRUE(ac_presence_debounce_process(&db, false, 14000));  // loss confirmed
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 16000));
    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, true, 18000));
    TEST_ASSERT_TRUE(ac_presence_debounce_process(&db, true, 30000));  // restore confirmed

    TEST_ASSERT_EQUAL(2, s_spy.call_count);
    TEST_ASSERT_EQUAL(AC_PRESENCE_LOST, s_spy.states[0]);
    TEST_ASSERT_EQUAL(AC_PRESENCE_PRESENT, s_spy.states[1]);
    TEST_ASSERT_EQUAL(AC_PRESENCE_PRESENT, db.state);
}

// No callback registered must not crash and must still track state.
TEST(ac_presence_debounce, no_callback_registered_is_safe)
{
    ac_presence_debounce_t db;
    ac_presence_debounce_init(&db, AC_PRESENCE_PRESENT, TEST_DEBOUNCE_US);

    TEST_ASSERT_FALSE(ac_presence_debounce_process(&db, false, 0));
    TEST_ASSERT_TRUE(ac_presence_debounce_process(&db, false, TEST_DEBOUNCE_US));
    TEST_ASSERT_EQUAL(AC_PRESENCE_LOST, db.state);
}

TEST_GROUP_RUNNER(ac_presence_debounce)
{
    RUN_TEST_CASE(ac_presence_debounce, clean_transition);
    RUN_TEST_CASE(ac_presence_debounce, bouncy_transition);
    RUN_TEST_CASE(ac_presence_debounce, brief_genuine_blip);
    RUN_TEST_CASE(ac_presence_debounce, no_callback_registered_is_safe);
}

static void run_all_tests(void)
{
    RUN_TEST_GROUP(ac_presence_debounce);
}

int main(int argc, char **argv)
{
    UNITY_MAIN_FUNC(run_all_tests);
    return 0;
}
