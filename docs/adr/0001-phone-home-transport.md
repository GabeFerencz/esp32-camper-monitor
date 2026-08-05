# 1. Phone-home WAN transport

## Status
Accepted

## Context
`SPEC.md`'s original Phone-home section called for a self-hosted receiver:
a purpose-built FastAPI/SQLite service, reachable through an existing
Cloudflare Tunnel + Access setup already used for other services on the
same host. That design was never implemented — no receiver code exists in
this repo. Firmware development had also not yet reached the networking
stage (WiFi, HTTP client, buffering), so this decision changes nothing
already built; it only changes what gets built next.

Separately, a home-automation platform (Home Assistant) is already running
on the same personal infrastructure the original design would have reused.
It supports inbound webhook triggers natively and already has an MQTT
broker running for local integration. Building a second, bespoke receiver
alongside a platform that already does most of what's needed was hard to
justify.

The device itself only ever needs to do three things over the WAN leg:
send an immediate alert on AC loss, send a periodic heartbeat, and do both
reliably enough that a receiver can tell "no news" apart from "device is
dead." Nothing about that requirement implies a specific protocol.

Two hard constraints from `CLAUDE.md` shape every option considered here:
- No device-specific credential exists in any file, tracked or untracked,
  ever — provisioned into NVS at runtime instead.
- AC-loss is the single message in this system with no second chance: no
  brownout last-gasp transmission is attempted (see `SPEC.md`'s Alert
  priority section), so whatever carries that message needs to be the
  most boring, well-understood option available, not the newest one.

## Decision
**WAN transport is a plain HTTPS POST from the ESP32 to a webhook trigger
on Home Assistant**, routed through the existing Cloudflare Tunnel, secured
by the same Cloudflare Access Service Auth / Service Token mechanism
already in use for other services on that host (`CF-Access-Client-Id` /
`CF-Access-Client-Secret` headers). This replaces the FastAPI/SQLite
receiver design. No code changes accompany that removal — the prior design
never had an implementation to remove, only a spec description, now
retired in favor of this one.

Home Assistant's own documentation treats a webhook ID as equivalent to a
password: anyone who has it can trigger the automation. Given this
project's existing rule against any device credential living in a file,
the webhook ID is being treated as a new provisioning field, alongside
`wifi_ssid`, `wifi_pass`, `cf_id`, `cf_secret`, and `phone_host` — not
hardcoded into firmware source. The actual NVS schema change and console
support for it are implementation work, scoped in the follow-up issue this
ADR accompanies; this ADR fixes the decision that it must be provisioned,
not the field name or exact console syntax.

**MQTT is not used on the WAN leg.** It is used locally: the Home
Assistant automation that receives the webhook republishes parsed state to
the local MQTT broker already running there, so any future local consumer
(a dashboard, other Home Assistant automations, a future local-only
sensor) can subscribe without depending on the webhook path. The ESP32
firmware itself has no MQTT client and no MQTT dependency at all — it only
ever POSTs to one HTTPS endpoint.

This mirrors how the market-leading product in this device category
(RV/marine power monitoring) is architected: HTTPS for WAN telemetry to
its own cloud service, MQTT reserved for local integration. That's
supporting precedent, not the reason for the decision — the reasoning
below is what actually drove it for this project.

## Alternatives considered

### MQTT end-to-end over the existing tunnel
Rejected. The free tier of the tunnel provider already in use doesn't
cleanly support raw MQTT/TCP ingress. The documented workaround is MQTT
over WebSockets, which would work, but it trades a boring, ubiquitous
protocol (HTTPS, which every ESP-IDF HTTP client example already handles,
with mature TLS and retry semantics) for a less common combination
(MQTT-over-WS client, on-device) that's simply less exercised in the wild.
That's an untested failure mode for a protocol upgrade that doesn't
actually add anything the webhook doesn't already provide — there's no
requirement here for pub/sub fanout, QoS-tiered delivery, or a persistent
broker connection on the WAN leg. Given that the one message riding on
this path with zero retry budget is the AC-loss alert, taking on that risk
for no functional gain wasn't worth it.

### MQTT over a mesh VPN (e.g. Tailscale/WireGuard-style overlay) instead of the tunnel
Rejected. This would sidestep the tunnel's MQTT/TCP limitation, but at the
cost of a second piece of always-on infrastructure on the device side (a
mesh client alongside the WiFi stack) and a second thing that can fail
independently of the WiFi connection itself — working against the same
"minimize failure surface on an unattended device" principle that already
ruled out Bluetooth for v1 (see `SPEC.md`'s deferred-to-v2 section). It
also would have meant standing up and maintaining a receiver process
listening on that overlay network, which is exactly the "build a bespoke
receiver" work this decision is trying to avoid by reusing Home Assistant.

### Keep the original self-hosted FastAPI/SQLite receiver
Rejected, though not because anything was wrong with it — it was a
reasonable design. It just duplicates a webhook + persistence + automation
surface that Home Assistant already provides on the same infrastructure,
for a project whose real complexity budget belongs on the device side
(reliability, watchdog, OTA rollback), not on standing up and maintaining
a second small service.

## Consequences
- Firmware only needs a standard HTTPS client (ESP-IDF's `esp_http_client`
  is the expected fit) plus the existing Cloudflare Access header pair —
  no MQTT client, no MQTT dependency, on the device at all.
- The provisioning NVS schema gains a new field for the webhook ID/path,
  which must be added with the same "never hardcoded, never written to
  any tracked or untracked file" treatment as the existing four fields.
  This is a small, additive change to the provisioning console
  (`set-webhook` or equivalent) and store — it does not change the
  provisioning trigger/blocking logic already implemented.
- Heartbeat-silence detection remains entirely a receiver-side
  responsibility: an explicit time-based check against a last-seen
  timestamp in the Home Assistant automation, independent of transport.
  MQTT's Last Will and Testament was considered implicitly by virtue of
  MQTT being rejected above, but is also explicitly not a substitute for
  this check even where MQTT is used locally — LWT only fires on a clean
  TCP disconnect and doesn't reliably catch a hung device, which is
  exactly the failure mode heartbeat-silence detection exists to catch.
- Local buffering during a network outage remains a firmware-side
  requirement regardless of transport — a ring buffer with retry-on-
  reconnect, unaffected by this decision.
- Retiring a receiver design that was never implemented has no code-level
  consequence in this repo; it's purely a `SPEC.md` documentation change.
- Real infrastructure values (Home Assistant's address, the actual webhook
  path once created) live only in the gitignored `CLAUDE.local.md`, never
  in this ADR, `SPEC.md`, or any other tracked file — the same boundary
  the prior Phone-home design already respected.
