# Provisioning — host tests

Unit tests for `provisioning_validate.c`, built as a native binary for
ESP-IDF's `linux` target. No hardware required — this is a standalone
test project, independent of the root firmware build.

```bash
get_idf
cd components/provisioning/host_test
idf.py --preview set-target linux build
./build/provisioning_validate_test.elf
```

Covers: a complete config, each field missing individually, an empty
phone-home host, and a phone-home host with a `http://`/`https://` scheme
prefix (rejected — the firmware builds the request URL itself).

`provisioning_store.c` and `provisioning_console.c` are real-NVS/real-console
code, not host-tested here — same split as `ac_presence_gpio.c` in the
sibling `ac_presence` component.
