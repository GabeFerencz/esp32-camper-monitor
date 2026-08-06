#include "wifi_station_backoff.h"

uint32_t wifi_station_backoff_delay_ms(uint32_t attempt)
{
    uint32_t shift = attempt < WIFI_RECONNECT_BACKOFF_MAX_SHIFT
                          ? attempt
                          : WIFI_RECONNECT_BACKOFF_MAX_SHIFT;
    uint32_t delay = WIFI_RECONNECT_BASE_DELAY_MS << shift;
    return delay < WIFI_RECONNECT_MAX_DELAY_MS ? delay : WIFI_RECONNECT_MAX_DELAY_MS;
}
