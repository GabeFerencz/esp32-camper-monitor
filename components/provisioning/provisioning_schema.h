// Shared NVS layout for provisioning: the config struct plus the
// namespace/key names, kept in one header so provisioning_store.c (NVS
// I/O), provisioning_console.c (console commands), and provisioning_validate.c
// (completeness checks) can't drift apart. Data-only -- no .c file.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define PROVISIONING_NVS_NAMESPACE "provision"

// NVS key names -- must stay <=15 chars (NVS_KEY_NAME_MAX_SIZE).
#define PROVISIONING_KEY_WIFI_SSID  "wifi_ssid"
#define PROVISIONING_KEY_WIFI_PASS  "wifi_pass"
#define PROVISIONING_KEY_CF_ID      "cf_id"
#define PROVISIONING_KEY_CF_SECRET  "cf_secret"
#define PROVISIONING_KEY_PHONE_HOST "phone_host"
#define PROVISIONING_KEY_WEBHOOK_ID "webhook_id"

// Max content length per field (excludes the NUL terminator) -- shared by
// the storage struct below and the console's masked-entry buffers so the
// two can't drift apart.
#define PROVISIONING_SSID_MAX_LEN        32  // 802.11 SSID max is 32 bytes
#define PROVISIONING_PASS_MAX_LEN        63  // WPA2-PSK max is 63 chars
#define PROVISIONING_CF_ID_MAX_LEN       127
#define PROVISIONING_CF_SECRET_MAX_LEN   127
#define PROVISIONING_HOST_MAX_LEN        127 // no URL scheme prefix
#define PROVISIONING_WEBHOOK_ID_MAX_LEN  127 // treated like a password per
                                              // ADR 0001 / HA's own guidance

typedef struct {
    char wifi_ssid[PROVISIONING_SSID_MAX_LEN + 1];
    char wifi_pass[PROVISIONING_PASS_MAX_LEN + 1];
    char cf_client_id[PROVISIONING_CF_ID_MAX_LEN + 1];
    char cf_client_secret[PROVISIONING_CF_SECRET_MAX_LEN + 1];
    char phone_host[PROVISIONING_HOST_MAX_LEN + 1];
    char webhook_id[PROVISIONING_WEBHOOK_ID_MAX_LEN + 1];
} provisioning_config_t;

#ifdef __cplusplus
}
#endif
