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

// Placeholder capacity, sized generously for a transient outage; revisit
// once the real heartbeat cadence (and therefore worst-case backlog) is
// decided alongside the WiFi task that will actually drive this buffer.
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

size_t phone_home_buffer_count(const phone_home_buffer_t *buf);
bool phone_home_buffer_is_full(const phone_home_buffer_t *buf);

#ifdef __cplusplus
}
#endif
