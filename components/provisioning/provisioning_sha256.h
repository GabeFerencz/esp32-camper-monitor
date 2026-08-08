// Minimal, self-contained SHA-256 (FIPS 180-4), used only to compute the
// non-reversible display fingerprint in provisioning_fingerprint.c. Hand-
// rolled instead of pulling in ESP-IDF's mbedtls: this project's mbedtls
// version exposes SHA-256 only through the PSA Crypto API (psa_crypto_init()
// plus friends), which is a lot of surface for a UI display aid that isn't
// a security boundary, and it would drag a hardware-crypto dependency into
// the host-testable side of provisioning. No streaming API -- every caller
// hashes a single short provisioning field in one call.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROVISIONING_SHA256_DIGEST_LEN 32

void provisioning_sha256(const uint8_t *data, size_t len, uint8_t digest[PROVISIONING_SHA256_DIGEST_LEN]);

#ifdef __cplusplus
}
#endif
