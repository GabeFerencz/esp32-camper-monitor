#include "phone_home_report.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"

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

// cJSON's number type stores values as a double, mirrored into a 32-bit
// int for integer printing (cJSON_SetNumberValue) -- not enough to hold
// esp_timer_get_time() microsecond uptimes past ~35 minutes of runtime
// without silently wrapping. Rendering uptime_us as a pre-formatted raw
// token instead of a cJSON number keeps full int64_t precision without
// going through that lossy path.
static bool build_body(const phone_home_report_t *report, char *out, size_t out_size)
{
    char uptime_buf[32];
    int n = snprintf(uptime_buf, sizeof(uptime_buf), "%lld", (long long)report->uptime_us);
    if (!snprintf_fits(n, sizeof(uptime_buf))) {
        return false;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return false;
    }

    bool ok = cJSON_AddStringToObject(root, "type", report_type_str(report->type)) != NULL &&
              cJSON_AddStringToObject(root, "ac_state", ac_state_str(report->ac_state)) != NULL &&
              cJSON_AddRawToObject(root, "uptime_us", uptime_buf) != NULL;

    if (ok) {
        ok = cJSON_PrintPreallocated(root, out, (int)out_size, false) != 0;
    }

    cJSON_Delete(root);
    return ok;
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

    if (!build_body(report, out->body, sizeof(out->body))) {
        return false;
    }

    return true;
}
