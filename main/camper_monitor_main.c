#include "ac_presence.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "phone_home_task.h"
#include "provisioning.h"
#include "provisioning_schema.h"
#include "provisioning_store.h"

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    init_nvs();
    provisioning_run_if_needed();

    provisioning_config_t cfg;
    ESP_ERROR_CHECK(provisioning_store_load(&cfg));

    QueueHandle_t alert_queue = phone_home_task_start(&cfg);
    ac_presence_start(phone_home_task_alert_cb, (void *)alert_queue);
}
