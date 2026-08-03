# Project instructions for Claude Code

## What this is
ESP32-based AC/DC power monitor for a camper at a remote site. Full design
is in `SPEC.md` — read it before starting any feature work. This file is
for standing conventions and constraints; don't duplicate the spec here.

This is a public hobby project, but it should read like the hobby work of
a professional: clear commits, real documentation, no filler, no sloppy
placeholder comments left behind. Treat code quality and documentation
quality as things a potential client or employer may actually read.

## Environment
- Target: ESP32 (WROOM-32, DEVKITV1 dev board)
- ESP-IDF v6.0.2, sourced via `get_idf` (WSL2)
- Build/flash:
  ```
  cp secrets.h.example main/secrets.h   # first time only, then fill in values
  idf.py set-target esp32
  idf.py build
  idf.py -p /dev/ttyUSB0 flash monitor
  ```
- Board profile (4MB flash, Boya flash chip) — apply via
  `sdkconfig.defaults`, don't rediscover these warnings on every new build:
  ```
  CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
  CONFIG_SPI_FLASH_SUPPORT_BOYA_CHIP=y
  ```
- Use `esptool` and `chip-id` (not the deprecated `esptool.py` / `chip_id`)

## Hard constraints — do not violate these
1. **FreeRTOS-native architecture.** Real tasks with defined responsibilities
   (sensor sampling, networking, watchdog-feed, OTA-check), not a single
   Arduino-style loop. See `SPEC.md` for the task breakdown.
2. **No credentials, hostnames, or personal infrastructure details in any
   tracked file — ever.** This includes code, comments, commit messages,
   and documentation/workflow write-ups (e.g. anything under `docs/`
   describing the AI-assisted process). Real values live only in the
   gitignored `main/secrets.h` (firmware secrets) and `CLAUDE.local.md`
   (project/infra context — real hostnames, host names, anything specific
   to the actual deployment). Tracked files use placeholders only:
   `secrets.h.example`, `.gitleaks.toml.example`. If you generate example
   output, terminal captures, or screenshots for documentation, sanitize
   them the same way — check for real hostnames, IPs, MACs, and tokens
   before they're written to a tracked file. If you need a real
   infrastructure detail to complete a task and it isn't in
   `CLAUDE.local.md`, ask rather than guess or invent one.
3. **Reliability requirements are non-negotiable, not aspirational** — this
   device runs unattended at a site with no guaranteed physical access:
   - Hardware watchdog enabled, fed only by a dedicated health-check task
   - WiFi reconnect with backoff — never require a manual reset to recover
   - Brownout detection explicitly configured
   - Boot-loop protection — repeated failure falls back to a minimal safe
     mode, not an infinite reboot loop
   - Local buffering of readings during network outages
4. **OTA rollback safety.** Any OTA-related work must implement
   self-validation (confirm WiFi connect + successful report) before
   marking a new image valid, with automatic rollback on failure. A broken
   OTA push must never strand the device.
5. **Alert priority.** AC-loss transitions trigger an immediate,
   out-of-cycle report — never wait for the next periodic cycle. Heartbeats
   are separate from event alerts; both matter.

## Workflow preference
Use plan mode for anything beyond a trivial fix — draft the approach against
the relevant `SPEC.md` section first, wait for approval, then implement.
Don't skip straight to code on anything touching the hard constraints above.

## Scope
Don't implement anything `SPEC.md` lists as deferred or out of scope without
checking in first — that's a deliberate sequencing decision, not an
oversight.
