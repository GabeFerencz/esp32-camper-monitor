#include "phone_home_sender.h"

#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "phone_home_sender";

#define PHONE_HOME_SEND_TIMEOUT_MS 10000

esp_err_t phone_home_sender_send(const phone_home_request_t *req)
{
    if (req == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_http_client_config_t config = {
        .url = req->url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = PHONE_HOME_SEND_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "CF-Access-Client-Id", req->cf_client_id);
    esp_http_client_set_header(client, "CF-Access-Client-Secret", req->cf_client_secret);
    esp_http_client_set_post_field(client, req->body, (int)strlen(req->body));

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    // Never log req->url (embeds the webhook_id credential), cf_client_id,
    // or cf_client_secret -- CLAUDE.md constraint #2. Status/err alone are
    // enough to act on a failure.
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "send failed: %s", esp_err_to_name(err));
        return err;
    }
    if (status < 200 || status >= 300) {
        ESP_LOGW(TAG, "send rejected: HTTP %d", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}
