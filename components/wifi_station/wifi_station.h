// WiFi station-mode connectivity: init, connect, and reconnect-with-
// backoff. ESP-IDF-dependent (esp_wifi/esp_netif/esp_event) -- the pure
// backoff-delay math lives separately in wifi_station_backoff.h so it
// can be host-tested without any of this.
#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "provisioning_schema.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wifi_station_state_cb_t)(bool connected, void *ctx);

// Initializes esp_netif/esp_event/esp_wifi, starts station mode using the
// provisioned SSID/password, and begins connecting. Call once, from
// app_main() or equivalent -- not safe to call concurrently or more than
// once. cb (optional, NULL allowed) fires from the default event loop
// task's context on each connectivity transition; keep it fast and
// non-blocking, same rule as any ESP-IDF event handler.
// Named wifi_station_connect_start (not wifi_station_start) -- the
// shorter name collides with a symbol already defined inside ESP-IDF's
// precompiled libnet80211.a, which the linker won't tolerate.
esp_err_t wifi_station_connect_start(const provisioning_config_t *cfg,
                                      wifi_station_state_cb_t cb, void *cb_ctx);

bool wifi_station_is_connected(void);

#ifdef __cplusplus
}
#endif
