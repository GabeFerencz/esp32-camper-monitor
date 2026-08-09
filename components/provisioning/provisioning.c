#include "provisioning.h"

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "provisioning_console.h"
#include "provisioning_schema.h"
#include "provisioning_store.h"
#include "provisioning_validate.h"

static const char *TAG = "provisioning";

void provisioning_run_if_needed(void)
{
    provisioning_config_t cfg;
    ESP_ERROR_CHECK(provisioning_store_load(&cfg));

    if (provisioning_config_is_complete(&cfg)) {
        ESP_LOGI(TAG, "provisioning complete");
        return;
    }

    ESP_LOGW(TAG, "provisioning incomplete -- entering provisioning console");
    SemaphoreHandle_t commit_sem = xSemaphoreCreateBinary();
    provisioning_console_start(commit_sem);

    xSemaphoreTake(commit_sem, portMAX_DELAY);
    ESP_LOGI(TAG, "provisioning committed -- rebooting into normal operation");
    // The console's "commit" confirmation and this log line are written
    // asynchronously by the UART driver; esp_restart() doesn't wait for
    // pending TX to drain, so without this delay the operator's terminal
    // loses the confirmation mid-transmission (observed on real hardware).
    vTaskDelay(pdMS_TO_TICKS(300));
    esp_restart();
}
