#include "ac_presence_gpio.h"

#include "driver/gpio.h"
#include "esp_err.h"

void ac_presence_gpio_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << AC_PRESENCE_GPIO_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

bool ac_presence_gpio_read(void)
{
    return gpio_get_level(AC_PRESENCE_GPIO_PIN) != 0;
}
