# ESP32 Camper Power Monitor

Status: **early stage — environment and spec complete, firmware not yet started.**

An ESP32-based monitor for a camper at a remote site: tracks house battery
voltage and AC/shore-power presence, and reports status home. Built to run
unattended for extended periods with no guarantee of physical access.

This is also a deliberate learning project — the goal is to build current,
AI-augmented embedded development practice on real hardware, and document
that process in the open as it happens. Expect commits that show the actual
process, not just polished end states.

## Full spec
See [`SPEC.md`](./SPEC.md) for the complete design: hardware, firmware
architecture, OTA strategy, and the phone-home design (self-hosted,
Cloudflare Tunnel + Access, isolated from other services on the same host).

## Hardware
- ESP32-WROOM-32 (DEVKITV1 dev board)
- ESP-IDF v6.0.2, FreeRTOS-native (task-based, not an Arduino-style loop)

## Building
```bash
get_idf                         # or: source /path/to/esp-idf/export.sh
cp secrets.h.example main/secrets.h   # then fill in your own values
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

`main/secrets.h` is gitignored — it holds WiFi credentials and the
Cloudflare Access service token for this project's own phone-home endpoint.
Use `secrets.h.example` as the template.

## A note on the repo's security setup
This repo intentionally documents its own security practices as part of the
learning goal above:
- Credentials are never committed — see `secrets.h.example` for the pattern.
- A pre-commit [gitleaks](https://github.com/gitleaks/gitleaks) hook blocks
  credential-shaped secrets automatically.
- Personal infrastructure details (real hostnames, the phone-home domain)
  are kept out of this repo entirely — not just the credentials that point
  at them. `.gitleaks.toml.example` and `CLAUDE.local.md`'s absence here
  (it's gitignored) are both part of that: a plain hostname isn't
  credential-shaped, so a generic scanner won't catch it on its own, and
  the fix is keeping it out of tracked files rather than trying to scan
  for it after the fact.

## License
MIT — see [`LICENSE`](./LICENSE).