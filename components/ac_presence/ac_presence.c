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

// Stands in for the real out-of-cycle phone-home call until networking
// exists (separate issue). Runs synchronously from the sampling task the
// instant a transition is confirmed — AC-loss must never wait for the
// next periodic cycle, and this is called from exactly that path.
static void ac_presence_alert_log_cb(ac_presence_state_t new_state, void *ctx)
{
    (void)ctx;
    ESP_LOGW(TAG, "AC presence %s -- immediate alert (stub: console only)",
              new_state == AC_PRESENCE_PRESENT ? "RESTORED" : "LOST");
}

static void ac_presence_task(void *arg)
{
    (void)arg;

    ac_presence_gpio_init();

    bool initial_raw = ac_presence_gpio_read();
    ac_presence_debounce_init(&s_debounce,
                               initial_raw ? AC_PRESENCE_PRESENT : AC_PRESENCE_LOST,
                               AC_PRESENCE_DEBOUNCE_DEFAULT_US);
    ac_presence_set_alert_cb(&s_debounce, ac_presence_alert_log_cb, NULL);
    ESP_LOGI(TAG, "AC presence initial state: %s", initial_raw ? "PRESENT" : "LOST");

    while (1) {
        bool raw = ac_presence_gpio_read();
        ac_presence_debounce_process(&s_debounce, raw, esp_timer_get_time());
        vTaskDelay(pdMS_TO_TICKS(AC_PRESENCE_SAMPLE_PERIOD_MS));
    }
}

void ac_presence_start(void)
{
    xTaskCreate(ac_presence_task, "ac_presence", 2048, NULL, 5, NULL);
}
