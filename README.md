# ESP32 Camper Power Monitor

Status: **paused (October 2026) — working prototype in personal use,
tagged `v0.1.0`.** Development is deliberately on hold, not abandoned; the
remaining work below is planned, not dropped.

### What works (verified on real hardware)
- AC/shore-power presence detection via GPIO, software-debounced
- Runtime serial-console provisioning into NVS — no secrets in firmware
  or in any file (see "Provisioning" below)
- WiFi station mode with exponential-backoff reconnect; recovers from a
  dropped AP with no manual intervention
- Phone-home over HTTPS through Cloudflare Access to a Home Assistant
  webhook ([ADR 0001](./docs/adr/0001-phone-home-transport.md)): immediate
  out-of-cycle alert on AC loss, periodic heartbeat on an absolute-deadline
  schedule, local buffering during outages with alerts drained ahead of
  heartbeats
- Brownout detector explicitly configured; OTA-ready dual-slot partition
  table in place
- CI: host-side unit tests plus a full ESP32 build on every push and PR

### Remaining before an unattended field install
SPEC.md treats these as hard requirements for a device with no guaranteed
physical access. The current build is suitable for personal use where a
hang would surface as a missing heartbeat, not yet for a hands-off
deployment.
- Battery voltage sensing via ADC (issue #2)
- Task watchdog (issue #37)
- Boot-loop protection with a minimal safe-mode fallback
- OTA updates with self-validation and automatic rollback

An ESP32-based monitor for a camper at a remote site: tracks house battery
voltage and AC/shore-power presence, and reports status home. Built to run
unattended for extended periods with no guarantee of physical access.

This is a hobby project, built in whatever time is actually available, not
against unlimited free time. AI assistance (Claude Code, used throughout)
is part of that: it means more gets done in the time available, and it
doubles as practice with AI-augmented development workflows worth carrying
into other work. Expect commits that show the actual process, not just
polished end states.

## Full spec
See [`SPEC.md`](./SPEC.md) for the complete design: hardware, firmware
architecture, OTA strategy, and the phone-home design (self-hosted,
Cloudflare Tunnel + Access, isolated from other services on the same host).

## Hardware
- ESP32-WROOM-32 (DEVKITV1 dev board)
- ESP-IDF v6.0.2, FreeRTOS-native (task-based, not an Arduino-style loop)

See [`docs/hardware.md`](./docs/hardware.md) for full part details, wiring,
and datasheet links.

## Building
```bash
get_idf                         # or: source /path/to/esp-idf/export.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

No credentials file to copy — see "Provisioning" below for how the device
gets its WiFi/phone-home config.

## Provisioning
The firmware ships with no WiFi SSID, no Cloudflare Access service token,
no phone-home hostname, and no Home Assistant webhook ID anywhere in
source. On boot, if that config is missing from NVS, the device drops
into a serial-console provisioning mode over the same USB/UART connection
`idf.py monitor` already uses:

```bash
idf.py -p /dev/ttyUSB0 monitor
# at the "provisioning>" prompt:
set-wifi <ssid>    # then enter the password at the masked prompt
set-cf-token       # enter client ID and secret at masked prompts
set-host           # enter the hostname at a masked prompt
set-webhook        # enter the webhook ID at a masked prompt
show      # confirm the values (secret-class fields shown as <hidden>, never plaintext)
commit    # validates and reboots into normal operation
```

Only the SSID is ever typed as part of a command line — every other field
(WiFi password, Cloudflare Access client ID/secret, phone-home hostname,
webhook ID) is entered at a masked prompt: characters echo as `*` instead
of the value typed, so nothing plaintext ever appears in the serial
stream, and `show` displays `<hidden>` rather than any part of the real
value (or a digest of it — a digest looks safe but isn't a reliable
guarantee for a human-chosen field like the WiFi password, which is
guessable enough that a captured digest could be dictionary-attacked
about as easily as the plaintext). This closes a gap the original design
left open: passing a secret as a command-line argument still echoed it
in full as it was typed, even before `show` was involved.

**Re-provisioning** an already-deployed device (new WiFi network, rotated
token) doesn't need a rebuild or reflash — erase just the NVS partition to
force the device back into provisioning mode on next boot:

```bash
python $IDF_PATH/components/partition_table/parttool.py \
    -p /dev/ttyUSB0 erase_partition --partition-name=nvs
```

## A note on the repo's security setup
This repo intentionally documents its own security practices as part of the
learning goal above:
- No device-specific secret (WiFi credentials, Cloudflare Access service
  token, phone-home hostname, Home Assistant webhook ID) exists in any
  file in this repo, tracked or untracked, at any point — see
  "Provisioning" above. This is a deliberate choice, not just
  `.gitignore` discipline: Claude Code (used throughout
  this project's development, see below) has read access to the working
  directory, so a gitignored file only keeps a secret off GitHub, not out
  of the AI agent's context. Removing the secret from the filesystem
  entirely closes that gap instead of trying to fence off part of it.
- A pre-commit [gitleaks](https://github.com/gitleaks/gitleaks) hook blocks
  credential-shaped secrets automatically, as a general safety net.

## License
MIT — see [`LICENSE`](./LICENSE).