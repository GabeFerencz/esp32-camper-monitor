// Pure exponential-backoff delay calculation for WiFi reconnect attempts.
// No ESP-IDF dependency -- the esp_event glue in wifi_station.c calls
// this to decide how long to wait before retrying esp_wifi_connect(),
// but the math itself is host-testable in isolation (see host_test/).
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_RECONNECT_BASE_DELAY_MS 1000
#define WIFI_RECONNECT_MAX_DELAY_MS 60000
// The shift itself is capped before being applied, not just the
// resulting delay -- keeps the math well-defined no matter how large
// `attempt` grows over an extended outage, rather than relying on an
// unbounded left-shift to happen to saturate correctly.
#define WIFI_RECONNECT_BACKOFF_MAX_SHIFT 6

// attempt is 0-based; the caller resets it to 0 on a successful connect.
// Returns BASE_DELAY_MS * 2^min(attempt, MAX_SHIFT), capped at MAX_DELAY_MS.
uint32_t wifi_station_backoff_delay_ms(uint32_t attempt);

#ifdef __cplusplus
}
#endif
