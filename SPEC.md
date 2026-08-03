# ESP32 Camper Power Monitor — v1 Spec

## Goal
Monitor camper house battery voltage and AC/shore-power presence at a remote,
rarely-accessed site. Report status home reliably and autonomously. No
assumption of physical access after initial install.

## Hardware
- **MCU:** ESP32-WROOM-32 (DEVKITV1 dev board for v1)
- **Battery voltage:** resistor divider into an ADC1 channel, calibrated via
  IDF's eFuse-based ADC calibration API — not raw ADC counts
- **AC presence:** isolated wall-wart (3–5V DC) plugged into any AC outlet —
  no inverter installed, so every outlet is active only on shore power;
  no risk of a false "AC present" reading from an inverter-fed circuit.
  Rectified/filtered DC output through a protective divider + clamp into a
  digital GPIO. Software-debounced. HIGH = AC present.
- **Connectivity:** WiFi station mode, connecting to a private
  cellular-backed WiFi at the site (not public campground WiFi). The
  cellular router itself is battery-backed and always plugged in — its
  uptime is independent of the camper's own AC/battery state, so the
  report path itself isn't the weak link.
- **Power source for the ESP32 itself:** confirmed — in-wall USB ports, fed
  from the house battery (DC), not AC-derived. This is intentional, not a
  limitation: the ESP32 keeps running through an AC outage for as long as
  the battery lasts, which is the entire point — that window is the
  alerting opportunity. The device eventually losing power once the
  battery empties is an expected end state, not a failure to design
  around. See "Alert priority & reporting cadence" below.

## Firmware architecture (ESP-IDF, FreeRTOS-native)
- Separate FreeRTOS tasks: sensor sampling, network/report, watchdog-feed
  (OTA-check task added when OTA work starts)
- Hardware watchdog enabled, fed only by a dedicated health-check task — a
  hang anywhere else should trigger a reset, not get silently fed through
- WiFi reconnect with backoff; never require a manual reset to recover from
  a dropped AP
- Brownout detection explicitly configured, not left at IDF default
- Local buffering of readings during network outages — cellular backhaul
  means drops are expected, not exceptional; no data gaps for a transient
  wobble
- Boot-loop protection: repeated crash/failure falls back to a minimal safe
  mode rather than an infinite reboot loop

## OTA (in scope for v1)
- OTA-capable partition table (dual `ota_0`/`ota_1` + `otadata` — not the
  single-factory layout used in the `hello_world` validation build)
- **Rollback safety is the core requirement, not a nice-to-have:** a new
  image must self-validate (confirm WiFi connect + successful report) before
  marking itself valid. Automatic rollback to the last known-good image on
  failure. A bad OTA push must never stand a device down at a site with no
  physical access.
- Secure OTA transport — HTTPS at minimum; signed images worth serious
  consideration given the repo is public and OTA is inherently a
  remote-code-execution surface

## Alert priority & reporting cadence
- **AC-loss transition (HIGH→LOW) triggers an immediate, out-of-cycle
  report attempt** — not a wait for the next periodic poll. This is the
  single most important message the device will ever send; there's no
  second chance once the battery is exhausted.
- **Periodic heartbeat, separate from event-driven alerts.** The receiving
  side must treat a missing heartbeat as its own alert condition, distinct
  from an explicit AC-loss message — this is what catches a silent crash,
  hang, or faster-than-expected battery death, where an event-only design
  would go silent with no signal at all. Carries forward as a hard
  requirement into the phone-home design below.
- **Battery voltage doubles as the runway indicator.** Since device death
  is an expected outcome once the battery empties, the trend in reported
  voltage is literally the countdown to losing visibility — not just a
  background data point.
- **Deliberately not attempting a brownout "last gasp" transmission.**
  Considered and rejected: no reliable guarantee of enough time or stable
  power during brownout to complete a network round-trip. The immediate-
  alert-on-loss plus heartbeat-silence-detection combination already
  covers this without the added complexity and unreliability.

## Phone-home
- **Host:** self-hosted on existing personal infrastructure (a home server
  already running a Cloudflare Tunnel) — reuses proven infra rather than
  standing up something new. Specific host and domain details are kept out
  of this repo — see `CLAUDE.local.md` (gitignored) for the real values.
- **Isolation from other services on the same host:** separate systemd
  service, separate local port, separate SQLite database file. No shared
  state or shared failure surface with anything else already running
  there.
- **Transport:** a dedicated subdomain added as an additional Cloudflare
  Tunnel ingress rule + DNS record. Existing routes on the same tunnel are
  untouched — purely additive.
- **Auth:** dedicated Cloudflare Access application for the new subdomain,
  using a **Service Auth policy + Service Token** (not browser/SSO login —
  the ESP32 is headless and can't do an interactive auth flow). Firmware
  sends `CF-Access-Client-Id` / `CF-Access-Client-Secret` as headers on
  every request.
- **No real hostnames, service names, or infrastructure details appear in
  this repo, in commit messages, or in any documentation here.** Real
  values live only in the gitignored `secrets.h` (firmware) and
  `CLAUDE.local.md` (project context). `secrets.h.example` shows the
  pattern with a placeholder (`your-endpoint.example.com`).
- **Defense specific to this leak category:** a generic secret scanner
  catches credential-shaped strings but won't reliably flag a plain
  hostname. `.gitleaks.toml.example` shows the pattern for a personal
  rule; the real, gitignored `.gitleaks.toml` fills in the actual value
  locally, so it's never committed either.
- Must support: immediate event alerts, periodic heartbeats, and
  heartbeat-silence detection on the receiving side (see above)
- Must tolerate the local-buffering behavior above (delayed/batched
  delivery after a connectivity gap)

## Explicitly deferred to v2 (captured, not dropped)
- Bluetooth integration with the existing camper power watchdog device —
  richer voltage/current/diagnostic data, reuses hardware already in place.
  Deferred because it adds a second wireless stack and a dependency on an
  external device's protocol, working against v1's core constraint:
  minimize failure surface on an unattended device. Revisit once the v1
  checkpoint (below) is solid.

## v1 "done" checkpoint
- [ ] Battery voltage and AC-presence both read reliably on real hardware
- [ ] Phone-home endpoint live on personal infrastructure, isolated from
      other services on the same host (separate service, port, database)
- [ ] Personal gitleaks rule in place locally and verified (deliberately
      try to commit the real hostname, confirm the hook blocks it)
- [ ] Status reported successfully to the endpoint over the private
      cellular WiFi, authenticated via Cloudflare Access service token
- [ ] Survives a WiFi drop/reconnect with no manual intervention
- [ ] OTA update pushed successfully; rollback deliberately tested (push a
      broken build, confirm auto-rollback recovers it) *before* first field
      install
- [ ] README documents the AI-augmented workflow used to build it