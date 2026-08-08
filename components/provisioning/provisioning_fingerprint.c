#include "provisioning_fingerprint.h"

#include <stdio.h>
#include <string.h>

#include "provisioning_sha256.h"

void provisioning_fingerprint(const char *value, char out[PROVISIONING_FINGERPRINT_LEN + 1])
{
    uint8_t digest[PROVISIONING_SHA256_DIGEST_LEN];
    provisioning_sha256((const uint8_t *)value, strlen(value), digest);

    // PROVISIONING_FINGERPRINT_LEN/2 bytes of the digest is plenty to tell
    // values apart at a glance without printing the full 64-char digest.
    for (int i = 0; i < PROVISIONING_FINGERPRINT_LEN / 2; i++) {
        snprintf(out + i * 2, 3, "%02x", digest[i]);
    }
}
