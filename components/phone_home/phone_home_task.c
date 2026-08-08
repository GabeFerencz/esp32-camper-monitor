#include "phone_home_task.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

#include "ac_presence.h"
#include "phone_home_buffer.h"
#include "phone_home_heartbeat.h"
#include "phone_home_poll_schedule.h"
#include "phone_home_report.h"
#include "phone_home_sender.h"
#include "wifi_station.h"

static const char *TAG = "phone_home_task";

static provisioning_config_t s_cfg;
static QueueHandle_t s_alert_queue;
static phone_home_buffer_t s_buffer;
static phone_home_heartbeat_t s_heartbeat;

static void push_report(phone_home_report_type_t type, ac_presence_state_t ac_state)
{
    phone_home_report_t report = {
        .type = type,
        .ac_state = ac_state,
        .uptime_us = esp_timer_get_time(),
    };
    phone_home_buffer_push(&s_buffer, &report);
}

// Drains as many buffered reports as can be sent right now, stopping at
// the first failure and re-pushing that report -- avoids busy-looping
// against a down network. Always attempts the oldest pending alert
// before any backlogged heartbeat (phone_home_buffer_pop_alert_first()),
// per SPEC.md's alert-priority requirement. Re-pushing a failed report
// appends it to the tail rather than restoring its original position,
// but that's safe here: the alert-first scan re-checks every call, so a
// requeued alert is found and retried again next time regardless of
// where it physically landed -- it never ends up stuck behind newer
// heartbeats.
static void drain_buffer(void)
{
    phone_home_report_t report;
    while (phone_home_buffer_pop_alert_first(&s_buffer, &report)) {
        phone_home_request_t request;
        if (!phone_home_report_build_request(&s_cfg, &report, &request)) {
            ESP_LOGE(TAG, "failed to build request for report type %d -- dropping", report.type);
            continue;
        }
        if (phone_home_sender_send(&request, report.type) != ESP_OK) {
            phone_home_buffer_push(&s_buffer, &report);
            break;
        }
    }
}

static void phone_home_task(void *arg)
{
    (void)arg;

    phone_home_poll_schedule_t poll_sched;
    phone_home_poll_schedule_init(&poll_sched, PHONE_HOME_TASK_POLL_MS * 1000LL, esp_timer_get_time());

    while (1) {
        // Remaining-time-to-deadline, not the full period, is what keeps
        // this grid absolute -- see phone_home_poll_schedule.h. Ceiling
        // the ms conversion so a near-zero remainder still blocks at
        // least one tick instead of spinning until advance() catches up.
        int64_t remaining_us = phone_home_poll_schedule_remaining_us(&poll_sched, esp_timer_get_time());
        TickType_t timeout_ticks = pdMS_TO_TICKS((remaining_us + 999) / 1000);

        ac_presence_state_t alert_state;
        if (xQueueReceive(s_alert_queue, &alert_state, timeout_ticks) == pdTRUE) {
            push_report(PHONE_HOME_REPORT_AC_ALERT, alert_state);
        }

        if (phone_home_heartbeat_is_due(&s_heartbeat, esp_timer_get_time())) {
            push_report(PHONE_HOME_REPORT_HEARTBEAT, ac_presence_get_state());
            phone_home_heartbeat_mark_sent(&s_heartbeat, esp_timer_get_time());
        }

        if (wifi_station_is_connected()) {
            drain_buffer();
        }

        phone_home_poll_schedule_advance(&poll_sched, esp_timer_get_time());
    }
}

QueueHandle_t phone_home_task_start(const provisioning_config_t *cfg)
{
    memcpy(&s_cfg, cfg, sizeof(s_cfg));
    phone_home_buffer_init(&s_buffer);
    phone_home_heartbeat_init(&s_heartbeat, PHONE_HOME_HEARTBEAT_INTERVAL_US);

    s_alert_queue = xQueueCreate(PHONE_HOME_ALERT_QUEUE_DEPTH, sizeof(ac_presence_state_t));
    if (s_alert_queue == NULL) {
        ESP_LOGE(TAG, "failed to create alert queue");
        return NULL;
    }

    // Synchronous use only -- wifi_station_connect_start() reads cfg's
    // fields immediately and doesn't retain the pointer, so the caller's
    // cfg (possibly stack-local) is safe to pass directly here.
    esp_err_t err = wifi_station_connect_start(cfg, NULL, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_station_connect_start failed: %s", esp_err_to_name(err));
        return NULL;
    }

    xTaskCreate(phone_home_task, "phone_home", 4096, NULL, 4, NULL);
    return s_alert_queue;
}

void phone_home_task_alert_cb(ac_presence_state_t new_state, void *ctx)
{
    QueueHandle_t queue = (QueueHandle_t)ctx;
    xQueueSend(queue, &new_state, 0);
}
