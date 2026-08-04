#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ac_presence_gpio.h"

static const char *TAG = "camper_monitor";

// Stage-2 bring-up only: logs raw GPIO level changes so the pin can be
// verified against a known reference (a jumper to 3.3V/GND) on real
// hardware. Replaced in stage 3 by a task that feeds samples through the
// debounce state machine instead of logging raw level changes directly.
static void ac_presence_bringup_task(void *arg)
{
    (void)arg;

    bool last = ac_presence_gpio_read();
    ESP_LOGI(TAG, "AC-presence GPIO%d initial raw level: %s",
             AC_PRESENCE_GPIO_PIN, last ? "HIGH" : "LOW");

    while (1) {
        bool level = ac_presence_gpio_read();
        if (level != last) {
            ESP_LOGI(TAG, "AC-presence GPIO%d raw level changed: %s -> %s",
                     AC_PRESENCE_GPIO_PIN, last ? "HIGH" : "LOW", level ? "HIGH" : "LOW");
            last = level;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    ac_presence_gpio_init();
    xTaskCreate(ac_presence_bringup_task, "ac_presence_bringup", 2048, NULL, 5, NULL);
}
