// Sends a pre-built phone_home_request_t over HTTPS. ESP-IDF-dependent
// (esp_http_client) -- deliberately separate from phone_home_report.c so
// request-building stays pure/host-testable while this stays untested
// except by compile and on-hardware verification, same split as
// provisioning_store.c relative to provisioning_validate.c.
#pragma once

#include "esp_err.h"
#include "phone_home_report.h"

#ifdef __cplusplus
extern "C" {
#endif

// Blocking POST of *req. Returns ESP_OK only on a 2xx HTTP status; any
// transport error or non-2xx response is reported identically -- from
// the caller's perspective both mean "failed, re-buffer and retry,"
// so the distinction doesn't change what happens next.
//
// `type` is caller-supplied metadata (already known from the report that
// produced *req) used only to tag failure/rejection log lines as
// heartbeat vs. alert -- it plays no part in the request itself.
esp_err_t phone_home_sender_send(const phone_home_request_t *req, phone_home_report_type_t type);

#ifdef __cplusplus
}
#endif
