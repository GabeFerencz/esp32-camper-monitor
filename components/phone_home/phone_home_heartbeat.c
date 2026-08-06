#include "phone_home_heartbeat.h"

void phone_home_heartbeat_init(phone_home_heartbeat_t *hb, int64_t interval_us)
{
    hb->interval_us = interval_us;
    hb->last_sent_us = 0;
    hb->has_sent = false;
}

bool phone_home_heartbeat_is_due(const phone_home_heartbeat_t *hb, int64_t now_us)
{
    if (!hb->has_sent) {
        return true;
    }
    return (now_us - hb->last_sent_us) >= hb->interval_us;
}

void phone_home_heartbeat_mark_sent(phone_home_heartbeat_t *hb, int64_t now_us)
{
    hb->last_sent_us = now_us;
    hb->has_sent = true;
}
