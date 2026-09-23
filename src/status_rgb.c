#include "status_rgb.h"

#if defined(DEVICE_HAS_STATUS_RGB) && DEVICE_HAS_STATUS_RGB

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#ifndef STATUS_RGB_GPIO
#define STATUS_RGB_GPIO 48
#endif
#ifndef STATUS_RGB_PROBE_ALT
#define STATUS_RGB_PROBE_ALT -1
#endif

static const char *TAG = "rgb";
static led_strip_handle_t s_strip;
static int s_gpio = -1;
static int64_t s_flash_until_us;
static bool s_flash_active;
static uint8_t s_last_r = 0xff, s_last_g = 0xff, s_last_b = 0xff;
static bool s_have_last;

static bool open_strip(int gpio)
{
    if (gpio < 0) {
        return false;
    }
    if (s_strip != NULL) {
        led_strip_del(s_strip);
        s_strip = NULL;
        s_gpio = -1;
    }

    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {.invert_out = false},
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {.with_dma = true},
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK || s_strip == NULL) {
        ESP_LOGW(TAG, "rmt+dma failed gpio=%d (%s) — retry without dma", gpio, esp_err_to_name(err));
        rmt_config.flags.with_dma = false;
        err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    }
    if (err != ESP_OK || s_strip == NULL) {
        ESP_LOGW(TAG, "rmt init failed gpio=%d err=%s", gpio, esp_err_to_name(err));
        s_strip = NULL;
        return false;
    }
    s_gpio = gpio;
    s_have_last = false;
    esp_rom_delay_us(300);
    led_strip_clear(s_strip);
    return true;
}

static void write_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_strip == NULL) {
        return;
    }
    /* Only refresh when the color changes — continuous RMT frames glitch WS2812. */
    if (s_have_last && s_last_r == r && s_last_g == g && s_last_b == b) {
        return;
    }
    if (r == 0 && g == 0 && b == 0) {
        led_strip_clear(s_strip);
    } else {
        led_strip_set_pixel(s_strip, 0, r, g, b);
        led_strip_refresh(s_strip);
    }
    s_last_r = r;
    s_last_g = g;
    s_last_b = b;
    s_have_last = true;
}

static void probe_gpio(int gpio, const char *label)
{
    if (gpio < 0 || !open_strip(gpio)) {
        return;
    }
    ESP_LOGI(TAG, "probe %s white 1s GPIO%d", label, gpio);
    write_rgb(80, 80, 80);
    vTaskDelay(pdMS_TO_TICKS(1000));
    write_rgb(0, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(150));
}

void status_rgb_init(void)
{
    const int prefer = STATUS_RGB_GPIO;
    const int alt = STATUS_RGB_PROBE_ALT;

    probe_gpio(prefer, "primary");
    if (alt >= 0 && alt != prefer) {
        probe_gpio(alt, "alt");
    }

    if (!open_strip(prefer)) {
        if (alt >= 0 && alt != prefer && open_strip(alt)) {
            ESP_LOGW(TAG, "preferred GPIO%d failed — using GPIO%d", prefer, alt);
        } else {
            ESP_LOGE(TAG, "no RGB strip");
            return;
        }
    }

    /* No rainbow — solid green means ready after MQTT connects. */
    ESP_LOGI(TAG, "status RGB ready on GPIO%d", s_gpio);
    write_rgb(60, 0, 0); /* red until link */
}

void status_rgb_set(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_flash_active) {
        return;
    }
    write_rgb(r, g, b);
}

void status_rgb_off(void)
{
    s_flash_active = false;
    write_rgb(0, 0, 0);
}

void status_rgb_show_link(bool wifi_ok, bool mqtt_ok)
{
    if (s_flash_active) {
        return;
    }
    if (mqtt_ok) {
        write_rgb(0, 60, 0);
    } else if (wifi_ok) {
        write_rgb(60, 36, 0);
    } else {
        write_rgb(60, 0, 0);
    }
}

void status_rgb_flash_telemetry(void)
{
    if (s_strip == NULL) {
        return;
    }
    s_have_last = false; /* force blue even if last was blue */
    write_rgb(0, 0, 90);
    s_flash_active = true;
    s_flash_until_us = esp_timer_get_time() + 500LL * 1000LL;
}

void status_rgb_tick(void)
{
    if (!s_flash_active) {
        return;
    }
    if (esp_timer_get_time() >= s_flash_until_us) {
        s_flash_active = false;
        s_have_last = false; /* force restore via show_link */
    }
}

#endif /* DEVICE_HAS_STATUS_RGB */
