// Thin wrapper around the AC-presence sensing GPIO. Real hardware only
// (driver/gpio.h) — deliberately kept out of ac_presence_debounce.c so
// that file stays host-testable.
#pragma once

#include <stdbool.h>

#include "hal/gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// GPIO27: a regular I/O-capable pin, not a strapping pin (0/2/5/12/15)
// and not input-only (34-39, which have no internal pull). Confirm
// against actual wiring before connecting anything.
#define AC_PRESENCE_GPIO_PIN GPIO_NUM_27

// Configures AC_PRESENCE_GPIO_PIN as a digital input with the internal
// pull-down enabled, so a floating/disconnected pin reads LOW (fail-safe
// "AC absent") rather than an undefined or falsely-HIGH level.
void ac_presence_gpio_init(void);

// Raw (undebounced) level: true = HIGH, false = LOW.
bool ac_presence_gpio_read(void);

#ifdef __cplusplus
}
#endif
