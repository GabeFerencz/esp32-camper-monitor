# Hardware

Current state of the physical build. This is a dev-bench project — no
custom PCB yet. See "Planned / not yet built" for what's coming.

## Dev board

| | |
|---|---|
| Board | Generic "ESP32 DEVKIT V1" — 30-pin, DOIT-style clone, unbranded |
| Module | ESP32-WROOM-32 |
| Chip | ESP32-D0WD-V3, silicon rev 3.1 |
| Flash | 4 MB, Boya-brand |

The Boya flash requires an explicit sdkconfig flag — it isn't the default
assumption in ESP-IDF's flash driver:

```
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_SPI_FLASH_SUPPORT_BOYA_CHIP=y
```

Already set in `sdkconfig.defaults`. Recorded here so the *why* doesn't
get rediscovered from a cryptic flash-ID mismatch on the next fresh
checkout.

**On the "DEVKITV1" name:** this board has no single vendor and no
canonical schematic. DOIT published the original design; nearly everything
sold as "ESP32 DevKit V1" today is an unbranded clone of it, with the
regulator and USB-serial chip varying by manufacturer batch. Treat the
chip- and module-level datasheets below as authoritative. Treat any
specific DEVKITV1 pinout diagram as "matches most boards, verify against
the actual unit" — not as a guarantee.

### References

- [ESP32 Series Datasheet](https://documentation.espressif.com/esp32_datasheet_en.html) — chip-level: D0WD-V3 electricals, pin list, strapping pins
- [ESP32-WROOM-32 Module Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf) — module-level pinout and dimensions
- [ESP32-DevKitC V4 User Guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html) — Espressif's own dev board, same module family; closest *official* reference, not this board's actual schematic
- [Generic 30-pin DevKit pinout (community-maintained)](https://www.espboards.dev/esp32/esp32-30pin-devkit-generic/) — since no vendor publishes one for the clone family

## Sensing wiring (current state)

### AC-presence input — GPIO27

| | |
|---|---|
| Pin | GPIO27 (`AC_PRESENCE_GPIO_PIN`, `ac_presence_gpio.h`) |
| Direction | digital input, internal pull-down enabled |
| Disconnected/floating state | reads LOW → "AC absent" (fail-safe) |
| Debounce | 30 ms continuous-stability window (`AC_PRESENCE_DEBOUNCE_DEFAULT_US`) |
| Why GPIO27 | ordinary I/O pin — not a strapping pin (0/2/5/12/15), not input-only (34–39) |

Two interchangeable fixtures currently stand in for the real sensing
circuitry — no custom hardware exists beyond these:

1. **Static jumper, 3.3V → GPIO27.** Mocks a permanent "AC present"
   state. Fastest sanity check of the GPIO/debounce path; zero real-world
   timing behavior.
2. **Wall-wart (isolated low-voltage DC output) → GPIO27**, unplugged and
   replugged from AC to simulate a real outage. Closer to production
   signal behavior — a decaying/ramping edge instead of a clean digital
   toggle — and the only current way to exercise real transition timing.

Neither is the production circuit. SPEC.md's design calls for a
rectified/filtered wall-wart output through a protective divider + clamp
into the GPIO; that protection stage doesn't exist on this bench yet.
**Everything here today is bare 3.3V/DC straight onto GPIO27 — no
protection components in the path.** Fine for bench iteration, not
representative of what ships.

### Battery voltage sensing — not yet built

SPEC.md calls for a resistor divider into an ADC1 channel, calibrated via
IDF's eFuse-based ADC calibration API (issue #2). No components, wiring,
or code exist for this yet. Update this section once a pin and divider
values are chosen.

## Planned / not yet built

- Protective divider + clamp ahead of GPIO27 (real AC-sensing front end)
- Resistor divider for the battery-voltage ADC input
- Custom carrier board — deferred until both sensing circuits above are
  validated on breadboard/perfboard first
- v2 (explicitly deferred, see `SPEC.md`): Bluetooth integration with the
  existing camper power watchdog device

## Relevance to HIL testing

Captured here because it's already true today, not because a HIL design
exists yet:

- The jumper/wall-wart swap is manual. There's no programmatic way to
  toggle AC-presence state without a human moving a wire.
- The 30 ms debounce sets a floor: any automated stimulus needs to hold
  state longer than that to register as a confirmed transition, and any
  timing-sensitive HIL test needs to budget for it.

No harness, relay, or GPIO-driving-GPIO rig exists. This section is a
factual note for whoever designs that later, not a design doc itself.
