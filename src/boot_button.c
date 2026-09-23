#include "boot_button.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_timer.h"

#include <stdatomic.h>

static atomic_bool s_pending;
static int64_t s_last_us;

static void IRAM_ATTR on_boot_isr(void *arg)
{
    (void)arg;
    atomic_store(&s_pending, true);
}

void boot_button_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BOOT_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    esp_err_t isr = gpio_install_isr_service(0);
    if (isr != ESP_OK && isr != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(isr);
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(BOOT_BUTTON_GPIO, on_boot_isr, NULL));
}

bool boot_button_take_press(void)
{
    if (!atomic_exchange(&s_pending, false)) {
        return false;
    }
    const int64_t now = esp_timer_get_time();
    if (now - s_last_us < 300000LL) {
        return false;
    }
    // Ignore while still held (noise / long press repeats via ISR edge only).
    if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
        // wait briefly for release is done in caller; accept edge
    }
    s_last_us = now;
    return true;
}
