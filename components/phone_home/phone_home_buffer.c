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

size_t phone_home_buffer_count(const phone_home_buffer_t *buf)
{
    return buf->count;
}

bool phone_home_buffer_is_full(const phone_home_buffer_t *buf)
{
    return buf->count == PHONE_HOME_BUFFER_CAPACITY;
}
