#include "phone_home_report.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char *report_type_str(phone_home_report_type_t type)
{
    return (type == PHONE_HOME_REPORT_HEARTBEAT) ? "heartbeat" : "ac_alert";
}

static const char *ac_state_str(ac_presence_state_t state)
{
    return (state == AC_PRESENCE_PRESENT) ? "present" : "lost";
}

// snprintf() returns the length the output *would* have needed; a
// negative or too-large result means the field didn't fit.
static bool snprintf_fits(int written, size_t buf_size)
{
    return written >= 0 && (size_t)written < buf_size;
}

bool phone_home_report_build_request(const provisioning_config_t *cfg,
                                      const phone_home_report_t *report,
                                      phone_home_request_t *out)
{
    if (cfg == NULL || report == NULL || out == NULL) {
        return false;
    }

    int n = snprintf(out->url, sizeof(out->url), "https://%s/api/webhook/%s",
                      cfg->phone_host, cfg->webhook_id);
    if (!snprintf_fits(n, sizeof(out->url))) {
        return false;
    }

    n = snprintf(out->cf_client_id, sizeof(out->cf_client_id), "%s", cfg->cf_client_id);
    if (!snprintf_fits(n, sizeof(out->cf_client_id))) {
        return false;
    }

    n = snprintf(out->cf_client_secret, sizeof(out->cf_client_secret), "%s", cfg->cf_client_secret);
    if (!snprintf_fits(n, sizeof(out->cf_client_secret))) {
        return false;
    }

    n = snprintf(out->body, sizeof(out->body),
                 "{\"type\":\"%s\",\"ac_state\":\"%s\",\"uptime_us\":%lld}",
                 report_type_str(report->type), ac_state_str(report->ac_state),
                 (long long)report->uptime_us);
    if (!snprintf_fits(n, sizeof(out->body))) {
        return false;
    }

    return true;
}
