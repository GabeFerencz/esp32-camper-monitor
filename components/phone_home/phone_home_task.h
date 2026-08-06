// Owns the network/report FreeRTOS task (SPEC.md's "network/report"
// task): starts WiFi station connectivity, drains the report buffer over
// HTTPS, sends heartbeats on schedule, and receives AC-presence alerts
// via a non-blocking queue hand-off from the (fast, must-not-block)
// sampling task in ac_presence.c.
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "ac_presence_debounce.h"
#include "provisioning_schema.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PHONE_HOME_ALERT_QUEUE_DEPTH 1
// A second transition arriving before the first is drained supersedes
// rather than queues -- the debounce state machine only ever reports
// current state, and ac_presence_get_state() reconciles the true state
// on the next heartbeat regardless.

#define PHONE_HOME_TASK_POLL_MS 5000
// Bounds heartbeat-due-check and buffer-drain granularity only -- alert
// latency is unaffected, since xQueueReceive() returns immediately once
// an item arrives. This is just how long the loop can go between checks
// when no alert is pending.

#define PHONE_HOME_HEARTBEAT_INTERVAL_US (15LL * 60 * 1000000)

// Starts WiFi station connectivity and the network/report task. Copies
// *cfg by value before returning -- the caller does not need to keep it
// alive afterward (important since app_main()'s stack is torn down once
// it returns). Returns the queue handle for ac_presence's alert callback
// to send on; NULL on failure.
QueueHandle_t phone_home_task_start(const provisioning_config_t *cfg);

// ac_presence_alert_cb_t-compatible trampoline. ctx must be the
// QueueHandle_t returned by phone_home_task_start(), cast to void*.
// Zero-timeout, non-blocking send -- never blocks the sampling task.
void phone_home_task_alert_cb(ac_presence_state_t new_state, void *ctx);

#ifdef __cplusplus
}
#endif
