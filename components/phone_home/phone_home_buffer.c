#include "phone_home_buffer.h"

#include <string.h>

void phone_home_buffer_init(phone_home_buffer_t *buf)
{
    memset(buf, 0, sizeof(*buf));
}

void phone_home_buffer_push(phone_home_buffer_t *buf, const phone_home_report_t *report)
{
    size_t tail;
    if (buf->count < PHONE_HOME_BUFFER_CAPACITY) {
        tail = (buf->head + buf->count) % PHONE_HOME_BUFFER_CAPACITY;
        buf->count++;
    } else {
        // Full -- evict the oldest by overwriting its slot and advancing head.
        tail = buf->head;
        buf->head = (buf->head + 1) % PHONE_HOME_BUFFER_CAPACITY;
    }
    buf->reports[tail] = *report;
}

bool phone_home_buffer_pop(phone_home_buffer_t *buf, phone_home_report_t *out)
{
    if (buf->count == 0) {
        return false;
    }
    *out = buf->reports[buf->head];
    buf->head = (buf->head + 1) % PHONE_HOME_BUFFER_CAPACITY;
    buf->count--;
    return true;
}

bool phone_home_buffer_pop_alert_first(phone_home_buffer_t *buf, phone_home_report_t *out)
{
    size_t match = buf->count;  // sentinel: no alert found
    for (size_t i = 0; i < buf->count; i++) {
        size_t phys = (buf->head + i) % PHONE_HOME_BUFFER_CAPACITY;
        if (buf->reports[phys].type == PHONE_HOME_REPORT_AC_ALERT) {
            match = i;
            break;
        }
    }
    if (match == buf->count) {
        return phone_home_buffer_pop(buf, out);
    }

    size_t match_phys = (buf->head + match) % PHONE_HOME_BUFFER_CAPACITY;
    *out = buf->reports[match_phys];
    // Shift everything after the match back by one logical slot, closing
    // the gap while preserving the relative order of what's left.
    for (size_t i = match; i + 1 < buf->count; i++) {
        size_t dst = (buf->head + i) % PHONE_HOME_BUFFER_CAPACITY;
        size_t src = (buf->head + i + 1) % PHONE_HOME_BUFFER_CAPACITY;
        buf->reports[dst] = buf->reports[src];
    }
    buf->count--;
    return true;
}

size_t phone_home_buffer_count(const phone_home_buffer_t *buf)
{
    return buf->count;
}

bool phone_home_buffer_is_full(const phone_home_buffer_t *buf)
{
    return buf->count == PHONE_HOME_BUFFER_CAPACITY;
}
