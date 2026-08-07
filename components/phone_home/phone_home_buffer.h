// Fixed-size FIFO ring buffer of pending phone-home reports, used to
// carry readings across a network outage instead of dropping them (see
// SPEC.md's local-buffering requirement). Pure in-memory structure --
// no ESP-IDF or hardware dependency, what makes this host-testable (see
// host_test/) independent of the real send path.
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "phone_home_report.h"

#ifdef __cplusplus
extern "C" {
#endif

// At the 15-minute heartbeat cadence (see phone_home_task.c), 32 entries
// covers roughly 8 hours of continuous outage before oldest-eviction
// kicks in -- reasonable for a "transient wobble" per SPEC.md; revisit
// if real-world outage duration proves this insufficient.
#define PHONE_HOME_BUFFER_CAPACITY 32

typedef struct {
    phone_home_report_t reports[PHONE_HOME_BUFFER_CAPACITY];
    size_t head;   // index of the oldest pending report
    size_t count;  // number of valid entries, 0..PHONE_HOME_BUFFER_CAPACITY
} phone_home_buffer_t;

void phone_home_buffer_init(phone_home_buffer_t *buf);

// Pushes a report onto the tail. If the buffer is already full, evicts
// the oldest entry first -- never blocks, never drops the newest report
// in favor of an older one.
void phone_home_buffer_push(phone_home_buffer_t *buf, const phone_home_report_t *report);

// Pops the oldest pending report into *out, in FIFO order. Returns false
// (leaving *out untouched) if the buffer is empty.
bool phone_home_buffer_pop(phone_home_buffer_t *buf, phone_home_report_t *out);

// Pops the oldest pending PHONE_HOME_REPORT_AC_ALERT report into *out,
// regardless of how many older heartbeats are backlogged ahead of it --
// per SPEC.md's alert-priority requirement, an alert must never wait
// behind heartbeat traffic. Falls back to plain phone_home_buffer_pop()
// (oldest overall) when no alert is pending, so a heartbeat-only backlog
// behaves exactly like today's plain FIFO.
//
// Safe to combine with a caller that re-pushes a failed send: push()
// always appends to the tail, but because this function re-scans for
// the oldest alert on every call rather than trusting buffer position,
// a re-queued failing alert is found and retried again on the very next
// call regardless of where it physically landed.
bool phone_home_buffer_pop_alert_first(phone_home_buffer_t *buf, phone_home_report_t *out);

size_t phone_home_buffer_count(const phone_home_buffer_t *buf);
bool phone_home_buffer_is_full(const phone_home_buffer_t *buf);

#ifdef __cplusplus
}
#endif
