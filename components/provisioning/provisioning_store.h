// NVS-facing half of provisioning: real flash I/O only. Deliberately kept
// out of provisioning_validate.c so that file stays host-testable (same
// split as ac_presence_gpio.c vs ac_presence_debounce.c).
#pragma once

#include "esp_err.h"
#include "provisioning_schema.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reads every field present in NVS into *out; fields absent from NVS are
// left as empty strings, including the case where the namespace itself
// hasn't been created yet (nothing provisioned).
esp_err_t provisioning_store_load(provisioning_config_t *out);

// Writes one field (use a PROVISIONING_KEY_* constant) and commits it.
esp_err_t provisioning_store_save_field(const char *nvs_key, const char *value);

// Erases the entire provisioning namespace, forcing re-entry into
// provisioning mode on next boot.
esp_err_t provisioning_store_erase(void);

#ifdef __cplusplus
}
#endif
