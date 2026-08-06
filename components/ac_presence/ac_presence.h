// Glue between the AC-presence GPIO and the debounce state machine:
// owns a dedicated FreeRTOS task that samples the pin and feeds it
// through ac_presence_debounce_process().
#pragma once

#include "ac_presence_debounce.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initializes the sensing GPIO and starts the sampling task. Call once
// from app_main(). alert_cb (optional, NULL allowed) is registered as
// the debounce state machine's alert callback -- it runs synchronously,
// inline in the sampling task, so it must not block. Passing NULL keeps
// the built-in console-log-only stub instead.
void ac_presence_start(ac_presence_alert_cb_t alert_cb, void *alert_cb_ctx);

// Returns the last confirmed (debounced) state. Safe to call from any
// task; backed by a single volatile read, so it's a benign race for a
// monitoring-only value, not used for control flow.
ac_presence_state_t ac_presence_get_state(void);

#ifdef __cplusplus
}
#endif
