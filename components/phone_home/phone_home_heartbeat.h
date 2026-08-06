// Pure "is a heartbeat due" check, decoupled from actually sending one.
// The caller supplies the clock (real esp_timer_get_time() on target, a
// fake counter in tests) and decides what "due" means for it -- this
// module only tracks the interval and the last-sent timestamp. No
// ESP-IDF or hardware dependency, what makes this host-testable (see
// host_test/).
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int64_t interval_us;
    int64_t last_sent_us;
    bool has_sent;  // false until the first mark_sent() -- a heartbeat is
                     // always due before the first one has ever gone out
} phone_home_heartbeat_t;

void phone_home_heartbeat_init(phone_home_heartbeat_t *hb, int64_t interval_us);

// True iff no heartbeat has been sent yet, or interval_us has elapsed
// since the last one was marked sent.
bool phone_home_heartbeat_is_due(const phone_home_heartbeat_t *hb, int64_t now_us);

// Records that a heartbeat was just sent (or buffered for send) at
// now_us, resetting the interval clock. Call only once per heartbeat
// attempt, not per retry.
void phone_home_heartbeat_mark_sent(phone_home_heartbeat_t *hb, int64_t now_us);

#ifdef __cplusplus
}
#endif
