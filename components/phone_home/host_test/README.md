# Phone-home — host tests

Unit tests for `phone_home_report.c`, `phone_home_buffer.c`, and
`phone_home_heartbeat.c`, built as a single native binary for ESP-IDF's
`linux` target. No hardware or network required — this is a standalone
test project, independent of the root firmware build.

```bash
get_idf
cd components/phone_home/host_test
idf.py --preview set-target linux build
./build/phone_home_test.elf
```

Covers:
- **Request building** — correct URL and JSON body for a heartbeat and
  for an AC-alert report, CF-Access header values passed through
  unchanged, and an oversized field (e.g. an implausibly long
  `phone_host`/`webhook_id`) rejected rather than silently truncated.
- **Ring buffer** — FIFO push/pop order, `count`/`is_full` accounting,
  and overflow evicting the oldest entry while keeping the newest.
- **Heartbeat scheduling** — due before any heartbeat has ever been
  sent, not due before the interval elapses, due once it has, and
  `mark_sent` resetting the clock.

The real HTTP send path, WiFi task, and ring-buffer wiring into a live
network outage are not covered here — see the parent issue (#9) for the
hardware-dependent follow-up.
