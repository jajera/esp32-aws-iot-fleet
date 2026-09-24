#include "status_display.h"
#include "app_config.h"

#if !defined(DEVICE_HAS_STATUS_TFT) || !DEVICE_HAS_STATUS_TFT

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_timer.h"

#include <stdio.h>
#include <string.h>

#ifndef LCD_SDA_GPIO
#define LCD_SDA_GPIO 21
#endif
#ifndef LCD_SCL_GPIO
#define LCD_SCL_GPIO 22
#endif
#ifndef LCD_I2C_ADDR
#define LCD_I2C_ADDR 0x3C
#endif
#ifndef LCD_COLUMN_OFFSET
#define LCD_COLUMN_OFFSET 0
#endif
#ifndef LCD_WIDTH
#define LCD_WIDTH 128
#endif
#ifndef LCD_HEIGHT
#define LCD_HEIGHT 64
#endif

static const char *TAG = "lcd";

static esp_lcd_panel_handle_t s_panel;
static bool s_ready;
static bool s_blank;
static unsigned s_blank_timeout_s = 30;
static int64_t s_last_activity_us;
static uint8_t s_fb[LCD_WIDTH * LCD_HEIGHT / 8];
static status_display_model_t s_last_model;
static bool s_have_model;

/* 5x7 font for ASCII 32..127 (bit0 = top). */
static const uint8_t FONT5X7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

static void fb_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

static void fb_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) {
        return;
    }
    const size_t i = (size_t)x + ((size_t)(y / 8) * LCD_WIDTH);
    const uint8_t bit = (uint8_t)(1u << (y & 7));
    if (on) {
        s_fb[i] |= bit;
    } else {
        s_fb[i] &= (uint8_t)~bit;
    }
}

static void fb_char(int x, int y, char c)
{
    if (c < 32 || c > 90) {
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        } else {
            c = '?';
        }
    }
    const uint8_t *g = FONT5X7[(int)c - 32];
    for (int col = 0; col < 5; ++col) {
        uint8_t bits = g[col];
        for (int row = 0; row < 7; ++row) {
            fb_pixel(x + col, y + row, (bits >> row) & 1);
        }
    }
}

static void fb_text(int x, int y, const char *s)
{
    while (*s && x < LCD_WIDTH - 5) {
        fb_char(x, y, *s++);
        x += 6;
    }
}

static void fb_bar(int x, int y, int w, int h, int filled)
{
    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            fb_pixel(x + xx, y + yy, xx < filled);
        }
    }
}

static void fb_flush(void)
{
    // SSD1306 / SH1106: SH1106 1.3" panels often need a 2-column RAM offset.
    const int x0 = LCD_COLUMN_OFFSET;
    const int x1 = LCD_COLUMN_OFFSET + LCD_WIDTH;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel, x0, 0, x1, LCD_HEIGHT, s_fb));
}

bool status_display_init(void)
{
#if !defined(DEVICE_HAS_STATUS_LCD) || !DEVICE_HAS_STATUS_LCD
    ESP_LOGI(TAG, "status LCD disabled for this model");
    return false;
#else
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = -1,
        .sda_io_num = LCD_SDA_GPIO,
        .scl_io_num = LCD_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c bus failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_i2c_config_t io_cfg = {
        .dev_addr = LCD_I2C_ADDR,
        .scl_speed_hz = 100000,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_bit_offset = 6,
    };
    err = esp_lcd_new_panel_io_i2c(bus, &io_cfg, &io);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "panel io failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = -1,
        .bits_per_pixel = 1,
    };
    esp_lcd_panel_ssd1306_config_t ssd_cfg = {
        .height = LCD_HEIGHT,
    };
    panel_cfg.vendor_config = &ssd_cfg;
    err = esp_lcd_new_panel_ssd1306(io, &panel_cfg, &s_panel);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ssd1306 new failed (addr=0x%02x sda=%d scl=%d): %s", LCD_I2C_ADDR,
                 LCD_SDA_GPIO, LCD_SCL_GPIO, esp_err_to_name(err));
        return false;
    }

    err = esp_lcd_panel_reset(s_panel);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "panel reset failed: %s", esp_err_to_name(err));
        return false;
    }
    err = esp_lcd_panel_init(s_panel);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "panel init failed (addr=0x%02x sda=%d scl=%d) — continuing without LCD: %s",
                 LCD_I2C_ADDR, LCD_SDA_GPIO, LCD_SCL_GPIO, esp_err_to_name(err));
        s_panel = NULL;
        return false;
    }
    err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "disp_on failed: %s", esp_err_to_name(err));
        s_panel = NULL;
        return false;
    }

    s_ready = true;
    s_blank = false;
    s_last_activity_us = esp_timer_get_time();
    ESP_LOGI(TAG, "OLED ready addr=0x%02x sda=%d scl=%d col_off=%d blank_timeout=%us", LCD_I2C_ADDR,
             LCD_SDA_GPIO, LCD_SCL_GPIO, LCD_COLUMN_OFFSET, s_blank_timeout_s);

    status_display_model_t boot = {0};
    snprintf(boot.thing_name, sizeof(boot.thing_name), "booting");
    snprintf(boot.status_line, sizeof(boot.status_line), "%s", DEVICE_MODEL);
    status_display_update(&boot);
    return true;
#endif
}

bool status_display_ready(void)
{
    return s_ready;
}

void status_display_set_blank_timeout_s(unsigned timeout_s)
{
    s_blank_timeout_s = timeout_s;
}

bool status_display_is_blank(void)
{
    return s_blank;
}

void status_display_wake(void)
{
    if (!s_ready) {
        return;
    }
    s_last_activity_us = esp_timer_get_time();
    if (s_blank) {
        if (esp_lcd_panel_disp_on_off(s_panel, true) != ESP_OK) {
            return;
        }
        s_blank = false;
        ESP_LOGI(TAG, "display wake (BOOT)");
        if (s_have_model) {
            status_display_update(&s_last_model);
        }
    }
}

void status_display_tick(void)
{
    if (!s_ready || s_blank || s_blank_timeout_s == 0) {
        return;
    }
    const int64_t idle_us = (int64_t)s_blank_timeout_s * 1000000LL;
    if (esp_timer_get_time() - s_last_activity_us >= idle_us) {
        if (esp_lcd_panel_disp_on_off(s_panel, false) == ESP_OK) {
            s_blank = true;
            ESP_LOGI(TAG, "display blank after %us idle", s_blank_timeout_s);
        }
    }
}

void status_display_update(const status_display_model_t *model)
{
    if (!s_ready || model == NULL) {
        return;
    }

    s_last_model = *model;
    s_have_model = true;

    if (s_blank) {
        return;
    }

    fb_clear();
    fb_text(0, 0, model->thing_name[0] ? model->thing_name : "fleet");

    char line[22];
    if (model->wifi_connected) {
        snprintf(line, sizeof(line), "WIFI %s", model->wifi_ssid[0] ? model->wifi_ssid : "up");
    } else {
        snprintf(line, sizeof(line), "WIFI down");
    }
    fb_text(0, 12, line);

    int bars = 0;
    if (model->wifi_connected) {
        // Map typical RSSI (-90..-30) to 0..10
        int r = model->wifi_rssi;
        if (r >= -50) {
            bars = 10;
        } else if (r >= -60) {
            bars = 8;
        } else if (r >= -70) {
            bars = 6;
        } else if (r >= -80) {
            bars = 4;
        } else if (r >= -90) {
            bars = 2;
        } else {
            bars = 1;
        }
        snprintf(line, sizeof(line), "RSSI %d", (int)model->wifi_rssi);
    } else {
        snprintf(line, sizeof(line), "RSSI --");
    }
    fb_text(0, 24, line);
    fb_bar(64, 26, 60, 6, bars * 6);

    fb_text(0, 38, model->iot_connected ? "IOT  CONNECTED" : "IOT  WAITING");
    if (model->status_line[0]) {
        fb_text(0, 52, model->status_line);
    }
    fb_flush();
}

#endif /* !DEVICE_HAS_STATUS_TFT */
