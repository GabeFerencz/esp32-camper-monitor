// Pure debounce / transition-detection state machine for the AC-presence
// GPIO input. No ESP-IDF or hardware dependencies — the caller supplies
// raw samples and monotonic timestamps, which is what makes this
// host-testable (see host_test/) independent of the real GPIO driver.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AC_PRESENCE_LOST = 0,
    AC_PRESENCE_PRESENT = 1,
} ac_presence_state_t;

// Invoked synchronously, exactly once per confirmed transition, from
// within ac_presence_debounce_process().
typedef void (*ac_presence_alert_cb_t)(ac_presence_state_t new_state, void *ctx);

typedef struct {
    ac_presence_state_t state;   // last confirmed (debounced) state
    bool raw_level;              // most recent raw sample fed in
    int64_t stable_since_us;     // timestamp raw_level last changed
    int64_t debounce_us;         // required continuous-stability window
    ac_presence_alert_cb_t alert_cb;
    void *alert_cb_ctx;
} ac_presence_debounce_t;

// Real-app default (30ms) — filters electrical/contact noise without
// filtering genuinely short outages. Widen here if field data shows it's
// too chatty; tests pass their own window explicitly.
#define AC_PRESENCE_DEBOUNCE_DEFAULT_US (30 * 1000)

void ac_presence_debounce_init(ac_presence_debounce_t *db,
                                ac_presence_state_t initial_state,
                                int64_t debounce_us);

void ac_presence_set_alert_cb(ac_presence_debounce_t *db,
                               ac_presence_alert_cb_t cb, void *ctx);

// Feed one raw sample and its monotonic timestamp (microseconds).
// Returns true iff this call confirmed a genuine transition, in which
// case the alert callback (if set) has already run.
bool ac_presence_debounce_process(ac_presence_debounce_t *db,
                                   bool raw_level, int64_t now_us);

#ifdef __cplusplus
}
#endif
