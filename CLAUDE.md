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
  idf.py set-target esp32
  idf.py build
  idf.py -p /dev/ttyUSB0 flash monitor
  ```
  No secrets file to copy first — WiFi/CF-Access/phone-home config is
  provisioned at runtime over the serial console (see SPEC.md's
  Provisioning section), never compiled into firmware.
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
2. **No device credentials in any tracked *or* untracked file — ever.**
   Firmware secrets (WiFi SSID/password, Cloudflare Access service token,
   phone-home hostname) are provisioned at runtime into NVS over a serial
   console (see SPEC.md's Provisioning section) — never written to disk
   in any form, gitignored or not. This is deliberate, not just
   `.gitignore` discipline: Claude Code has read access to the working
   directory, so a gitignored file only keeps a secret off GitHub, not out
   of the AI agent's context.

   This is narrower than a blanket "no personal infrastructure details"
   rule — `CLAUDE.local.md` may still hold real infra context for
   not-yet-built *server*-side work (phone-home host/port/tunnel details),
   but must never be read from or written into firmware source, code,
   comments, commit messages, or any tracked documentation/workflow
   write-up (e.g. anything under `docs/` describing the AI-assisted
   process). If you generate example output, terminal captures, or
   screenshots for documentation, sanitize them the same way — check for
   real hostnames, IPs, MACs, and tokens before they're written to a
   tracked file. If you need a real infrastructure detail to complete a
   task and it isn't in `CLAUDE.local.md`, ask rather than guess or invent
   one.
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

## Autonomy vs. review
Local, reversible work — reading, building, testing, editing files,
committing — proceed on your own judgment without checking in at each
step. I trust you the way I'd trust a competent colleague: expected to
try to do the right thing, understood to occasionally get something
wrong.

Anything reaching GitHub needs my review first: pushes, PR or issue
creation, merges, and any issue/PR edit that adds new written content.
Toggling an already-agreed checkbox in an issue body is fine on your own
judgment; rewriting the surrounding text is not.

Why: this repo is public, and it's already had a real near-miss with
personal infrastructure details landing in a tracked file (see constraint
2 above). Automated tooling catches credential-shaped secrets well, but a
plain hostname or a personal detail described in prose doesn't look like
a "secret" to a pattern scanner — that's the gap a human check closes.

I've found `settings.local.json`'s enforcement inconsistent in practice,
so treat this note as the real checkpoint, not the permission file —
tell me plainly what's about to reach GitHub and why, even if a technical
rule would currently let it through silently. When genuinely unsure
whether something needs my review, ask rather than guess.

## Git workflow
Every unit of implementation work starts as a GitHub issue. Exception:
small, non-functional companion changes with no independent scope of
their own (e.g., a doc-only ADR write-up) may skip the issue and use a
`docs/<slug>` branch instead — a deliberate, named exception, not a
shortcut.

Branch creation for issue-backed work always goes through
`gh issue develop <issue-number> --checkout`, never a hand-typed branch
name.

Local work (edits, commits, builds, tests) proceeds without per-step
check-in, per the human-review rule above. Sign-off is still required
before: `gh issue create`, `git push`, `gh pr create`, or any issue/PR
edit that adds new written content.

PRs reference and close their issue (`Closes #N`) rather than closing the
issue directly.

## Workflow preference
Use plan mode for anything beyond a trivial fix — draft the approach against
the relevant `SPEC.md` section first, wait for approval, then implement.
Don't skip straight to code on anything touching the hard constraints above.

## Scope
Don't implement anything `SPEC.md` lists as deferred or out of scope without
checking in first — that's a deliberate sequencing decision, not an
oversight.
