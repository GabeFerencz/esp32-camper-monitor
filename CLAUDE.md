# Project instructions for Claude Code

## What this is
ESP32-based AC/DC power monitor for a camper at a remote site. Full design
is in `SPEC.md` — read it before starting any feature work. This file is
for standing conventions and constraints; don't duplicate the spec here.

This is a public hobby project, but it should read like the hobby work of
a professional — clear commits, real documentation, no filler or sloppy
placeholder comments — since a potential client or employer may actually
read it.

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
   phone-home hostname, Home Assistant webhook ID) are provisioned at
   runtime into NVS over a serial console (see SPEC.md's Provisioning
   section) — never written to disk
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
6. **No session links in anything that reaches GitHub.** Never include a
   `claude.ai/code/session_...` link in commit messages, PR bodies, PR/issue
   comments, or any tracked file — regardless of what the current default
   commit/PR template does. `Co-Authored-By: Claude ...` trailers are
   welcome and should stay; I'm not trying to hide that this is
   AI-assisted work. The "🤖 Generated with Claude Code" line with its
   icon/hyperlink can go too — skip it by default, it's template filler.
   But the session link is different in kind: it's a pointer to the
   private conversation itself, which may contain things I was
   comfortable discussing with you but never intended for a public
   repo. If a future default template reintroduces it, strip it before
   the commit/PR goes out — don't assume the template is safe to trust
   as-is.

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
check-in. Sign-off before anything reaching GitHub follows the rule under
"Autonomy vs. review" above.

**Commit at each verified checkpoint, not once at the end.** "No
per-step check-in" is about not needing permission to commit — it isn't
license to squash a whole multi-part implementation into a single commit
once everything's done. If work is broken into discrete steps that each
get their own build/test verification (whether tracked as a task list or
just planned that way), commit after each one passes. This isn't a call
for maximal granularity — a typo fix or a one-line rename doesn't need
its own commit — the signal is whether a step was independently verified,
not how small a diff can get. A useful proxy: if a step is worth its own
bullet in an issue's acceptance criteria or its own entry in a task list,
it's worth its own commit.

PRs reference and close their issue (`Closes #N`) rather than closing the
issue directly.

**Never write a bare `#<number>` for anything but an actual issue/PR
reference.** GitHub auto-links any `#<number>` inside issue bodies, PR
bodies, comments, and commit messages to an issue or PR in this repo —
regardless of what it was meant to refer to. Numbering acceptance
criteria, list items, or steps as "AC #5", "item #3", "step #2" in
anything that reaches GitHub creates a spurious, misleading
cross-reference link to whatever issue/PR happens to hold that number.
Spell those out instead — "AC 5", "criterion 5", "item 3" — and reserve
`#<number>` strictly for real issue/PR references (`Closes #9`, "see
#18", etc.).

**Partial-scope PRs auto-close their issue too — watch for this.**
Because branches are created via `gh issue develop`, GitHub links the
branch to its issue, and merging closes that issue automatically. This
happens regardless of the PR body's wording — writing "Refs #N" instead
of "Closes #N" does *not* prevent it, since the close is driven by the
branch link, not by closing keywords in the text. If a PR is explicitly
a partial slice of an issue (some acceptance criteria intentionally left
for follow-up), reopen the issue immediately after merge as a deliberate
step — don't rely on PR phrasing to keep it open.

**A PR that changes what's implemented, working, or in scope updates the
docs in the same PR — not as a follow-up.** Update README.md's
status/current-state description, and SPEC.md too if architecture or scope
changed. If a PR has no user-facing or status-relevant change, say so
explicitly in the PR description rather than silently skipping docs. This
is what catches status drift before it ships — e.g. a README still calling
a feature "next" after it's already built.

## Workflow preference
Use plan mode for anything beyond a trivial fix — draft the approach against
the relevant `SPEC.md` section first, wait for approval, then implement.
Don't skip straight to code on anything touching the hard constraints above.

## Scope
Don't implement anything `SPEC.md` lists as deferred or out of scope without
checking in first — that's a deliberate sequencing decision, not an
oversight.
