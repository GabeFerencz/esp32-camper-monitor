#include "wifi_station.h"
#include "wifi_station_backoff.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"

static const char *TAG = "wifi_station";

static wifi_station_state_cb_t s_state_cb;
static void *s_state_cb_ctx;
static esp_timer_handle_t s_reconnect_timer;
static volatile bool s_connected;
static uint32_t s_reconnect_attempt;

static void reconnect_timer_cb(void *arg)
{
    (void)arg;
    esp_wifi_connect();
}

// Schedules the next esp_wifi_connect() retry via a one-shot esp_timer
// (its own timer-task context) rather than vTaskDelay() inline in the
// event handler -- the default event-loop task must stay free to keep
// delivering other WIFI/IP events while a retry is pending.
static void schedule_reconnect(void)
{
    uint32_t delay_ms = wifi_station_backoff_delay_ms(s_reconnect_attempt);
    s_reconnect_attempt++;
    ESP_LOGW(TAG, "disconnected, retrying in %u ms (attempt %u)",
             (unsigned)delay_ms, (unsigned)s_reconnect_attempt);
    esp_timer_start_once(s_reconnect_timer, (uint64_t)delay_ms * 1000);
}

static void event_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        if (s_state_cb != NULL) {
            s_state_cb(false, s_state_cb_ctx);
        }
        schedule_reconnect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_reconnect_attempt = 0;
        s_connected = true;
        ESP_LOGI(TAG, "connected");
        if (s_state_cb != NULL) {
            s_state_cb(true, s_state_cb_ctx);
        }
    }
}

esp_err_t wifi_station_connect_start(const provisioning_config_t *cfg,
                                      wifi_station_state_cb_t cb, void *cb_ctx)
{
    if (cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_state_cb = cb;
    s_state_cb_ctx = cb_ctx;

    const esp_timer_create_args_t timer_args = {
        .callback = reconnect_timer_cb,
        .name = "wifi_reconnect",
    };
    esp_err_t err = esp_timer_create(&timer_args, &s_reconnect_timer);
    if (err != ESP_OK) {
        return err;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, cfg->wifi_ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, cfg->wifi_pass, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Deliberately not logging cfg->wifi_ssid/wifi_pass in full -- SSID is
    // low-sensitivity but password is a provisioned credential (CLAUDE.md
    // constraint #2); nothing here needs it beyond what esp_wifi_set_config
    // already consumed.
    ESP_LOGI(TAG, "station starting");
    return ESP_OK;
}

bool wifi_station_is_connected(void)
{
    return s_connected;
}
