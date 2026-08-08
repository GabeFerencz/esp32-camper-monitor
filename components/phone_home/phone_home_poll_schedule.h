// Absolute-deadline scheduling for phone_home_task's main loop poll.
// Decoupled from esp_timer/FreeRTOS the same way phone_home_heartbeat.c
// is -- the caller supplies now_us (real esp_timer_get_time() on target,
// a fake counter in tests) so the grid-tracking arithmetic is
// host-testable (see host_test/) without a working FreeRTOS scheduler.
//
// Exists to fix a phase-drift bug: driving the loop's xQueueReceive() off
// a relative timeout re-issued every iteration means an early return (an
// alert arriving on the same queue) rebases every subsequent iteration's
// timing. Tracking an absolute next_deadline_us instead, and only ever
// advancing it once that deadline is actually reached, keeps the polling
// grid fixed regardless of how often the loop wakes early.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int64_t period_us;
    int64_t next_deadline_us;
} phone_home_poll_schedule_t;

// Sets the first deadline to now_us + period_us.
void phone_home_poll_schedule_init(phone_home_poll_schedule_t *sched, int64_t period_us, int64_t now_us);

// Time remaining until next_deadline_us, clamped to zero if already past
// it. Intended for use as the caller's next xQueueReceive timeout. Pure
// query -- does not mutate *sched.
int64_t phone_home_poll_schedule_remaining_us(const phone_home_poll_schedule_t *sched, int64_t now_us);

// Advances the deadline once now_us has reached it; a no-op otherwise --
// an early wake (now_us still short of next_deadline_us) leaves the grid
// untouched, which is the fix this module exists for. On a normal wake,
// advances by exactly one period_us from the prior deadline, preserving
// the fixed grid. If the loop body itself stalled long enough that even
// the new deadline is already in the past (e.g. drain_buffer()'s HTTP
// send blocking past its own timeout, longer than one poll period), snaps
// forward once to now_us + period_us instead of replaying every missed
// period -- avoids a burst of near-zero timeouts hammering the loop to
// "catch up" on a device where that cadence hiccup carries no other
// consequence (is_due() tracks its own elapsed time independently).
void phone_home_poll_schedule_advance(phone_home_poll_schedule_t *sched, int64_t now_us);

#ifdef __cplusplus
}
#endif
