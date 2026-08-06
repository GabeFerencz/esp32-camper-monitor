// Pure request-building for a phone-home report: given a provisioned
// config and a report, produces the URL, CF-Access header values, and
// JSON body -- no network, no ESP-IDF dependency. What makes this
// host-testable (see host_test/) independent of the real HTTP client,
// same split as provisioning_validate.c / ac_presence_debounce.c.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ac_presence_debounce.h"
#include "provisioning_schema.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PHONE_HOME_REPORT_HEARTBEAT = 0,
    PHONE_HOME_REPORT_AC_ALERT = 1,
} phone_home_report_type_t;

typedef struct {
    phone_home_report_type_t type;
    ac_presence_state_t ac_state;  // AC-presence state at report time
    int64_t uptime_us;             // esp_timer_get_time() at report time --
                                    // no wall clock needed: heartbeat-silence
                                    // detection is a receiver-side, last-seen
                                    // timestamp check (see ADR 0001)
} phone_home_report_t;

typedef struct {
    char url[256];              // full HTTPS URL, phone_host + webhook path
    char cf_client_id[128];     // CF-Access-Client-Id header value
    char cf_client_secret[128]; // CF-Access-Client-Secret header value
    char body[256];             // JSON request body
} phone_home_request_t;

// Builds *out from *cfg and *report. Returns false (leaving *out contents
// unspecified) if any argument is NULL or any formatted field would
// overflow its buffer -- callers must check the return value rather than
// assume a fixed-size buffer always fits.
bool phone_home_report_build_request(const provisioning_config_t *cfg,
                                      const phone_home_report_t *report,
                                      phone_home_request_t *out);

#ifdef __cplusplus
}
#endif
