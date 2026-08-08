#include "phone_home_poll_schedule.h"

void phone_home_poll_schedule_init(phone_home_poll_schedule_t *sched, int64_t period_us, int64_t now_us)
{
    sched->period_us = period_us;
    sched->next_deadline_us = now_us + period_us;
}

int64_t phone_home_poll_schedule_remaining_us(const phone_home_poll_schedule_t *sched, int64_t now_us)
{
    int64_t remaining = sched->next_deadline_us - now_us;
    return remaining > 0 ? remaining : 0;
}

void phone_home_poll_schedule_advance(phone_home_poll_schedule_t *sched, int64_t now_us)
{
    if (now_us < sched->next_deadline_us) {
        return;
    }
    sched->next_deadline_us += sched->period_us;
    if (now_us >= sched->next_deadline_us) {
        sched->next_deadline_us = now_us + sched->period_us;
    }
}
