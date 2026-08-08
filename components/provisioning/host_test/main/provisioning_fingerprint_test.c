// Host-target (linux) Unity tests for provisioning_sha256.c and
// provisioning_fingerprint.c. The SHA-256 group checks the hand-rolled
// digest against known FIPS 180-4 test vectors (reference values from
// Python's hashlib) since it isn't backed by a library implementation.
// The fingerprint group checks the properties `show` actually depends
// on: fixed length, determinism, distinct values for distinct inputs,
// and no plaintext substring leaking through. Entry point is
// provisioning_test_main.c.
#include <stdio.h>
#include <string.h>

#include "provisioning_fingerprint.h"
#include "provisioning_sha256.h"
#include "unity.h"
#include "unity_fixture.h"

static void digest_to_hex(const uint8_t digest[PROVISIONING_SHA256_DIGEST_LEN], char out[65])
{
    for (int i = 0; i < PROVISIONING_SHA256_DIGEST_LEN; i++) {
        snprintf(out + i * 2, 3, "%02x", digest[i]);
    }
}

static void assert_sha256_hex(const char *input, const char *expected_hex)
{
    uint8_t digest[PROVISIONING_SHA256_DIGEST_LEN];
    provisioning_sha256((const uint8_t *)input, strlen(input), digest);
    char hex[65];
    digest_to_hex(digest, hex);
    TEST_ASSERT_EQUAL_STRING(expected_hex, hex);
}

TEST_GROUP(provisioning_sha256);

TEST_SETUP(provisioning_sha256)
{
}

TEST_TEAR_DOWN(provisioning_sha256)
{
}

TEST(provisioning_sha256, empty_string)
{
    assert_sha256_hex("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(provisioning_sha256, abc)
{
    assert_sha256_hex("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(provisioning_sha256, hello_world)
{
    assert_sha256_hex("hello world", "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
}

// Deliberately >55 bytes so the single-block padding path (data + 0x80 +
// zero pad + length all fit in one 64-byte block) can't be taken --
// exercises the two-block spillover in provisioning_sha256.c.
TEST(provisioning_sha256, input_spans_two_padding_blocks)
{
    char input[128];
    memset(input, 'a', sizeof(input) - 1);
    input[sizeof(input) - 1] = '\0';
    assert_sha256_hex(input, "c57e9278af78fa3cab38667bef4ce29d783787a2f731d4e12200270f0c32320a");
}

TEST_GROUP_RUNNER(provisioning_sha256)
{
    RUN_TEST_CASE(provisioning_sha256, empty_string);
    RUN_TEST_CASE(provisioning_sha256, abc);
    RUN_TEST_CASE(provisioning_sha256, hello_world);
    RUN_TEST_CASE(provisioning_sha256, input_spans_two_padding_blocks);
}

TEST_GROUP(provisioning_fingerprint);

TEST_SETUP(provisioning_fingerprint)
{
}

TEST_TEAR_DOWN(provisioning_fingerprint)
{
}

TEST(provisioning_fingerprint, fixed_length)
{
    char fp[PROVISIONING_FINGERPRINT_LEN + 1];
    provisioning_fingerprint("hunter2hunter2", fp);
    TEST_ASSERT_EQUAL_UINT(PROVISIONING_FINGERPRINT_LEN, strlen(fp));
}

TEST(provisioning_fingerprint, deterministic)
{
    char fp_a[PROVISIONING_FINGERPRINT_LEN + 1];
    char fp_b[PROVISIONING_FINGERPRINT_LEN + 1];
    provisioning_fingerprint("subdom.example.com", fp_a);
    provisioning_fingerprint("subdom.example.com", fp_b);
    TEST_ASSERT_EQUAL_STRING(fp_a, fp_b);
}

TEST(provisioning_fingerprint, distinct_values_differ)
{
    char fp_a[PROVISIONING_FINGERPRINT_LEN + 1];
    char fp_b[PROVISIONING_FINGERPRINT_LEN + 1];
    provisioning_fingerprint("correct-horse-battery-staple", fp_a);
    provisioning_fingerprint("correct-horse-battery-staply", fp_b);
    TEST_ASSERT_NOT_EQUAL(0, strcmp(fp_a, fp_b));
}

TEST(provisioning_fingerprint, contains_no_plaintext_substring)
{
    static const char *values[] = {
        "hunter2hunter2",
        "abc123.access",
        "supersecrethex",
        "subdom.example.com",
        "a1b2c3d4e5f6",
    };
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        char fp[PROVISIONING_FINGERPRINT_LEN + 1];
        provisioning_fingerprint(values[i], fp);
        size_t value_len = strlen(values[i]);
        for (size_t start = 0; start + 3 <= value_len; start++) {
            char substr[4];
            memcpy(substr, values[i] + start, 3);
            substr[3] = '\0';
            TEST_ASSERT_NULL_MESSAGE(strstr(fp, substr), "fingerprint leaked a plaintext substring");
        }
    }
}

TEST_GROUP_RUNNER(provisioning_fingerprint)
{
    RUN_TEST_CASE(provisioning_fingerprint, fixed_length);
    RUN_TEST_CASE(provisioning_fingerprint, deterministic);
    RUN_TEST_CASE(provisioning_fingerprint, distinct_values_differ);
    RUN_TEST_CASE(provisioning_fingerprint, contains_no_plaintext_substring);
}
