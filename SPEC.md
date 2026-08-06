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
See [`docs/adr/0001-phone-home-transport.md`](docs/adr/0001-phone-home-transport.md)
for the full reasoning behind this design, alternatives considered, and why
they were rejected. This section states the decision as it applies to
firmware behavior.

- **Transport:** a plain HTTPS POST from the ESP32 to a webhook trigger on
  a home-automation platform (Home Assistant) already running on existing
  personal infrastructure — reuses proven infra rather than standing up a
  new receiver. Specific host, domain, and webhook path are kept out of
  this repo — see `CLAUDE.local.md` (gitignored) for the real values.
- **Routing:** the existing self-hosted Cloudflare Tunnel already in use
  for other services on the same host — a dedicated subdomain added as an
  additional Tunnel ingress rule + DNS record. Existing routes on the same
  tunnel are untouched — purely additive.
- **Auth:** dedicated Cloudflare Access application for the new subdomain,
  using a **Service Auth policy + Service Token** (not browser/SSO login —
  the ESP32 is headless and can't do an interactive auth flow). Firmware
  sends `CF-Access-Client-Id` / `CF-Access-Client-Secret` as headers on
  every request. Unchanged from the prior design.
- **MQTT is not used on the WAN leg.** It is used locally: the Home
  Assistant automation that receives the webhook republishes parsed state
  to the local MQTT broker already running there, so any future local
  consumer (dashboard, other automations, a future local-only sensor) can
  subscribe without depending on the webhook path at all. The firmware
  itself has no MQTT involvement — it only ever POSTs to the webhook. See
  the ADR for why MQTT was rejected for the WAN leg specifically.
- **No device-specific secret (WiFi credentials, CF Access service token,
  phone-home hostname, webhook ID) exists in any file, tracked or
  untracked, at any point.** Rather than compiling credentials into
  firmware and relying on `.gitignore`/secret-scanning discipline to keep
  them off GitHub, the device is provisioned at runtime: on boot, if
  required config is missing from NVS, it drops into a blocking
  `esp_console` REPL over the same USB/UART connection `idf.py monitor`
  already uses. No new hardware or transport. See "Provisioning" below for
  the current schema and flow. The webhook ID is a new credential this
  transport introduces — Home Assistant's own guidance is to treat it like
  a password — so it must extend the provisioning schema rather than be
  hardcoded; see the ADR for that decision and the not-yet-created
  implementation issue for the actual schema change. Project context
  that isn't a firmware secret (real host/path details for the *receiver*
  side) still lives in the gitignored `CLAUDE.local.md`.

### Provisioning
- **NVS namespace:** `provision`. Fields (all plain strings, `nvs_get_str`/
  `nvs_set_str`): `wifi_ssid`, `wifi_pass`, `cf_id` (CF Access client ID),
  `cf_secret` (CF Access client secret), `phone_host` (no URL scheme
  prefix — firmware builds the request URL itself), `webhook_id` (Home
  Assistant webhook ID — treated like a password per the ADR below, since
  HA's own docs say anyone holding it can trigger the automation).
- **Trigger:** at boot, after `nvs_flash_init()`, the device checks
  whether all six fields are present and non-empty. If not, it starts
  the provisioning console and blocks there — provisioning mode and
  normal operation never run in the same boot.
- **Console commands:** `set-wifi <ssid> <password>`, `set-cf-token
  <client-id> <client-secret>`, `set-host <hostname>`, `set-webhook
  <webhook-id>`, `show` (prints current values, masking the password,
  client secret, and webhook ID), `commit` (validates completeness, then
  reboots into normal operation).
- **Re-provisioning** (new WiFi network, rotated token) doesn't require a
  rebuild/reflash: erase just the NVS partition (`parttool.py
  erase_partition --partition-name=nvs`) to force the device back into
  provisioning mode on next boot, then run the console flow again.
- **NVS partition is separate from `ota_0`/`ota_1`/`otadata`** in the
  default partition table, so provisioned values are expected to survive
  OTA updates without extra work — a design expectation, not yet tested,
  since OTA doesn't exist in this project yet. Verification rides along
  with the OTA checkpoint below once that work starts.
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
- [ ] Provisioning console tested end-to-end (enter WiFi/CF-token/host via
      `set-wifi`/`set-cf-token`/`set-host`, `commit`, confirm it reboots
      and reconnects using the provisioned values)
- [ ] Confirmed no credential, hostname, or token appears in any tracked
      or untracked file in the repo
- [ ] Status reported successfully to the endpoint over the private
      cellular WiFi, authenticated via Cloudflare Access service token
- [ ] Survives a WiFi drop/reconnect with no manual intervention
- [ ] OTA update pushed successfully; rollback deliberately tested (push a
      broken build, confirm auto-rollback recovers it) *before* first field
      install (confirm NVS-provisioned config also survives this test)
- [ ] README documents the AI-augmented workflow used to build it