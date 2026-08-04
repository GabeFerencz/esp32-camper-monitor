# AC-presence debounce — host tests

Unit tests for `ac_presence_debounce.c`, built as a native binary for
ESP-IDF's `linux` target. No hardware required — this is a standalone
test project, independent of the root firmware build.

```bash
get_idf
cd components/ac_presence/host_test
idf.py --preview set-target linux build
./build/ac_presence_debounce_test.elf
```

Covers: a clean transition, a bouncy transition (rapid flicker that must
not fire the alert), and a brief-but-genuine outage (held longer than the
debounce window, so it must fire despite being short overall).
