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

typedef struct {
    char wifi_ssid[33];         // 802.11 SSID max is 32 bytes
    char wifi_pass[64];         // WPA2-PSK max is 63 chars
    char cf_client_id[128];     // Cloudflare Access service token client ID
    char cf_client_secret[128]; // Cloudflare Access service token client secret
    char phone_host[128];       // phone-home hostname, no URL scheme prefix
    char webhook_id[128];       // Home Assistant webhook ID -- treated like a
                                 // password per ADR 0001 / HA's own guidance
} provisioning_config_t;

#ifdef __cplusplus
}
#endif
