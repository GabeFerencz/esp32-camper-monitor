// Pure completeness/sanity checks for a provisioning_config_t. No
// ESP-IDF or hardware dependencies -- what makes this host-testable
// (see host_test/) independent of the real NVS storage.
#pragma once

#include <stdbool.h>

#include "provisioning_schema.h"

#ifdef __cplusplus
extern "C" {
#endif

// Rejects an empty host or one with a URL scheme prefix -- the firmware
// builds the request URL itself, so a stored "https://..." would double up.
bool provisioning_host_is_valid(const char *host);

// True iff every field is non-empty and phone_host passes
// provisioning_host_is_valid(). Used both to decide whether to enter
// provisioning mode at boot and to gate the console's `commit` command.
bool provisioning_config_is_complete(const provisioning_config_t *cfg);

#ifdef __cplusplus
}
#endif
