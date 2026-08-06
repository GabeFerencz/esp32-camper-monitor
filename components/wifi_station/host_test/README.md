# WiFi station — host tests

Unit tests for `wifi_station_backoff.c`, built as a native binary for
ESP-IDF's `linux` target. No hardware or network required.

```bash
get_idf
cd components/wifi_station/host_test
idf.py --preview set-target linux build
./build/wifi_station_test.elf
```

Covers:
- **Reconnect backoff** — first attempt returns the base delay, delay
  doubles each subsequent attempt, caps at the configured maximum, and
  stays capped indefinitely for an extended outage (no overflow at large
  attempt counts).

The real `esp_wifi`/`esp_netif`/`esp_event` connectivity glue in
`wifi_station.c` is not covered here — it's ESP-IDF-dependent and is only
exercised by the compile-only `idf.py build` (esp32 target) and manual
on-hardware verification. See the parent issue (#9) for that scope.
