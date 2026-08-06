#include "ac_presence.h"

#include <stdbool.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ac_presence_debounce.h"
#include "ac_presence_gpio.h"

static const char *TAG = "ac_presence";

// 10ms matches the default FreeRTOS tick rate (100Hz); combined with the
// 30ms debounce window, a genuine transition confirms within ~3 samples.
#define AC_PRESENCE_SAMPLE_PERIOD_MS 10

static ac_presence_debounce_t s_debounce;
static volatile ac_presence_state_t s_last_state;

typedef struct {
    ac_presence_alert_cb_t cb;
    void *ctx;
} ac_presence_start_args_t;

static ac_presence_start_args_t s_start_args;

// Fallback used when ac_presence_start() is passed a NULL callback (e.g.
// no real phone-home wiring exists yet, or a test rig). Runs
// synchronously from the sampling task the instant a transition is
// confirmed — AC-loss must never wait for the next periodic cycle, and
// this is called from exactly that path.
static void ac_presence_alert_log_cb(ac_presence_state_t new_state, void *ctx)
{
    (void)ctx;
    ESP_LOGW(TAG, "AC presence %s -- immediate alert (stub: console only)",
              new_state == AC_PRESENCE_PRESENT ? "RESTORED" : "LOST");
}

static void ac_presence_task(void *arg)
{
    ac_presence_start_args_t *args = (ac_presence_start_args_t *)arg;

    ac_presence_gpio_init();

    bool initial_raw = ac_presence_gpio_read();
    ac_presence_state_t initial_state = initial_raw ? AC_PRESENCE_PRESENT : AC_PRESENCE_LOST;
    s_last_state = initial_state;
    ac_presence_debounce_init(&s_debounce, initial_state, AC_PRESENCE_DEBOUNCE_DEFAULT_US);
    ac_presence_set_alert_cb(&s_debounce,
                              args->cb != NULL ? args->cb : ac_presence_alert_log_cb,
                              args->ctx);
    ESP_LOGI(TAG, "AC presence initial state: %s", initial_raw ? "PRESENT" : "LOST");

    while (1) {
        bool raw = ac_presence_gpio_read();
        ac_presence_debounce_process(&s_debounce, raw, esp_timer_get_time());
        s_last_state = s_debounce.state;
        vTaskDelay(pdMS_TO_TICKS(AC_PRESENCE_SAMPLE_PERIOD_MS));
    }
}

void ac_presence_start(ac_presence_alert_cb_t alert_cb, void *alert_cb_ctx)
{
    s_start_args.cb = alert_cb;
    s_start_args.ctx = alert_cb_ctx;
    xTaskCreate(ac_presence_task, "ac_presence", 2048, &s_start_args, 5, NULL);
}

ac_presence_state_t ac_presence_get_state(void)
{
    return s_last_state;
}
