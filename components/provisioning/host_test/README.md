# Provisioning — host tests

Unit tests for `provisioning_validate.c`, `provisioning_sha256.c`, and
`provisioning_fingerprint.c`, built as a native binary for ESP-IDF's
`linux` target. No hardware required — this is a standalone test project,
independent of the root firmware build.

```bash
get_idf
cd components/provisioning/host_test
idf.py --preview set-target linux build
./build/provisioning_test.elf
```

Covers:
- `provisioning_validate`: a complete config, each field missing
  individually, an empty phone-home host, and a phone-home host with a
  `http://`/`https://` scheme prefix (rejected — the firmware builds the
  request URL itself).
- `provisioning_sha256`: the hand-rolled digest against known FIPS 180-4
  test vectors, including an input long enough to force the two-block
  padding path.
- `provisioning_fingerprint`: fixed output length, determinism, distinct
  inputs producing distinct fingerprints, and no plaintext substring of
  the input leaking into the output — the properties the console's
  `show` command actually depends on.

`provisioning_store.c` and `provisioning_console.c` are real-NVS/real-console
code, not host-tested here — same split as `ac_presence_gpio.c` in the
sibling `ac_presence` component.
