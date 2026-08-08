// Non-reversible display fingerprint for secret-class provisioning fields.
// `show` prints this instead of any substring of the real value, so a
// captured serial transcript is still useful for confirming two sessions
// entered the same (or different) value without ever containing plaintext.
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hex chars in the fingerprint text, excluding the NUL terminator.
#define PROVISIONING_FINGERPRINT_LEN 12

// Writes a PROVISIONING_FINGERPRINT_LEN-char hex fingerprint of value into
// out (out must be at least PROVISIONING_FINGERPRINT_LEN + 1 bytes). Same
// value always yields the same fingerprint; different values yield
// different fingerprints with overwhelming probability.
void provisioning_fingerprint(const char *value, char out[PROVISIONING_FINGERPRINT_LEN + 1]);

#ifdef __cplusplus
}
#endif
