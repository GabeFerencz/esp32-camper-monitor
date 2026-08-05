#include "provisioning_store.h"

#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "provisioning_store";

esp_err_t provisioning_store_load(provisioning_config_t *out)
{
    memset(out, 0, sizeof(*out));

    nvs_handle_t handle;
    esp_err_t err = nvs_open(PROVISIONING_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Namespace doesn't exist yet -- nothing provisioned; fields stay empty.
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    const struct {
        const char *key;
        char *dst;
        size_t dst_size;
    } fields[] = {
        { PROVISIONING_KEY_WIFI_SSID,  out->wifi_ssid,        sizeof(out->wifi_ssid) },
        { PROVISIONING_KEY_WIFI_PASS,  out->wifi_pass,        sizeof(out->wifi_pass) },
        { PROVISIONING_KEY_CF_ID,      out->cf_client_id,     sizeof(out->cf_client_id) },
        { PROVISIONING_KEY_CF_SECRET,  out->cf_client_secret, sizeof(out->cf_client_secret) },
        { PROVISIONING_KEY_PHONE_HOST, out->phone_host,       sizeof(out->phone_host) },
    };

    esp_err_t first_err = ESP_OK;
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        size_t len = fields[i].dst_size;
        esp_err_t field_err = nvs_get_str(handle, fields[i].key, fields[i].dst, &len);
        if (field_err == ESP_ERR_NVS_NOT_FOUND) {
            continue;  // left empty by the memset above
        }
        if (field_err != ESP_OK) {
            ESP_LOGE(TAG, "failed to read '%s' from NVS: %s", fields[i].key, esp_err_to_name(field_err));
            if (first_err == ESP_OK) {
                first_err = field_err;
            }
        }
    }

    nvs_close(handle);
    return first_err;
}

esp_err_t provisioning_store_save_field(const char *nvs_key, const char *value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(PROVISIONING_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, nvs_key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t provisioning_store_erase(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(PROVISIONING_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}
