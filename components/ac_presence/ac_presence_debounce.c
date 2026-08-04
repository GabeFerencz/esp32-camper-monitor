#include <stddef.h>

#include "ac_presence_debounce.h"

void ac_presence_debounce_init(ac_presence_debounce_t *db,
                                ac_presence_state_t initial_state,
                                int64_t debounce_us)
{
    db->state = initial_state;
    db->raw_level = (initial_state == AC_PRESENCE_PRESENT);
    db->stable_since_us = 0;
    db->debounce_us = debounce_us;
    db->alert_cb = NULL;
    db->alert_cb_ctx = NULL;
}

void ac_presence_set_alert_cb(ac_presence_debounce_t *db,
                               ac_presence_alert_cb_t cb, void *ctx)
{
    db->alert_cb = cb;
    db->alert_cb_ctx = ctx;
}

bool ac_presence_debounce_process(ac_presence_debounce_t *db,
                                   bool raw_level, int64_t now_us)
{
    if (raw_level != db->raw_level) {
        // Raw input changed — (re)start the stability clock. Covers both
        // a fresh bounce and the start of a genuine transition; only
        // time will tell them apart.
        db->raw_level = raw_level;
        db->stable_since_us = now_us;
        return false;
    }

    bool raw_matches_confirmed = (raw_level == (db->state == AC_PRESENCE_PRESENT));
    if (raw_matches_confirmed) {
        return false;
    }

    if (now_us - db->stable_since_us < db->debounce_us) {
        return false;
    }

    db->state = raw_level ? AC_PRESENCE_PRESENT : AC_PRESENCE_LOST;
    if (db->alert_cb != NULL) {
        db->alert_cb(db->state, db->alert_cb_ctx);
    }
    return true;
}
