#include "status_display.h"
#include "app_config.h"

#if defined(DEVICE_HAS_STATUS_TFT) && DEVICE_HAS_STATUS_TFT

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_timer.h"

#include <stdio.h>
#include <string.h>

/* Ideaspark ESP32-WROOM + 1.14" ST7789 (same pin map as openBPM / TFT_eSPI). */
#ifndef TFT_HOST
#define TFT_HOST SPI2_HOST
#endif
#ifndef TFT_MOSI
#define TFT_MOSI 23
#endif
#ifndef TFT_SCLK
#define TFT_SCLK 18
#endif
#ifndef TFT_CS
#define TFT_CS 15
#endif
#ifndef TFT_DC
#define TFT_DC 2
#endif
#ifndef TFT_RST
#define TFT_RST 4
#endif
#ifndef TFT_BL
#define TFT_BL 32
#endif
#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 135
#endif

/* Partial flush: one scan band + one glyph cell — not a full 65 KiB FB. */
#define BAND_H 16
#define GLYPH_MAX_SCALE 2
#define GLYPH_W (5 * GLYPH_MAX_SCALE)
#define GLYPH_H (7 * GLYPH_MAX_SCALE)

static const char *TAG = "tft";

static esp_lcd_panel_handle_t s_panel;
static bool s_ready;
static bool s_blank;
static unsigned s_blank_timeout_s = 30;
static int64_t s_last_activity_us;
static status_display_model_t s_last_model;
static bool s_have_model;

static uint16_t s_band[TFT_WIDTH * BAND_H];
static uint16_t s_glyph[GLYPH_W * GLYPH_H];

/* 5x7 font ASCII 32..90 (shared layout with OLED driver). */
static const uint8_t FONT5X7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x08, 0x2A, 0x1C, 0x2A, 0x08}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00}, {0x00, 0x08, 0x14, 0x22, 0x41}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x41, 0x22, 0x14, 0x08, 0x00}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x01, 0x01},
    {0x3E, 0x41, 0x41, 0x51, 0x32}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x51, 0x49, 0x45, 0x43},
};

#define COL_BG   0x0000
#define COL_FG   0xFFFF
#define COL_ACC  0x07E0 /* green */
#define COL_DIM  0x8410 /* grey */

static inline uint16_t swap565(uint16_t c)
{
    return (uint16_t)((c >> 8) | (c << 8));
}

static void fill_rect(int x, int y, int w, int h, uint16_t colour)
{
    if (w <= 0 || h <= 0 || x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        return;
    }
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > TFT_WIDTH) {
        w = TFT_WIDTH - x;
    }
    if (y + h > TFT_HEIGHT) {
        h = TFT_HEIGHT - y;
    }
    if (w <= 0 || h <= 0) {
        return;
    }

    const uint16_t c = swap565(colour);
    for (int row = y; row < y + h;) {
        int bh = BAND_H;
        if (row + bh > y + h) {
            bh = y + h - row;
        }
        for (int i = 0; i < w * bh; ++i) {
            s_band[i] = c;
        }
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel, x, row, x + w, row + bh, s_band));
        row += bh;
    }
}

static void draw_char(int x, int y, char c, uint16_t colour, int scale)
{
    if (scale < 1) {
        scale = 1;
    }
    if (scale > GLYPH_MAX_SCALE) {
        scale = GLYPH_MAX_SCALE;
    }
    if (c < 32 || c > 90) {
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        } else {
            c = '?';
        }
    }
    const int gw = 5 * scale;
    const int gh = 7 * scale;
    if (x + gw > TFT_WIDTH || y + gh > TFT_HEIGHT || x < 0 || y < 0) {
        return;
    }

    const uint16_t fg = swap565(colour);
    const uint16_t bg = swap565(COL_BG);
    const uint8_t *g = FONT5X7[(int)c - 32];
    for (int col = 0; col < 5; ++col) {
        uint8_t bits = g[col];
        for (int row = 0; row < 7; ++row) {
            const uint16_t px = ((bits >> row) & 1) ? fg : bg;
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    s_glyph[(row * scale + sy) * gw + (col * scale + sx)] = px;
                }
            }
        }
    }
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel, x, y, x + gw, y + gh, s_glyph));
}

static void draw_text(int x, int y, const char *s, uint16_t colour, int scale)
{
    const int advance = 6 * scale;
    while (*s && x + 5 * scale <= TFT_WIDTH) {
        draw_char(x, y, *s++, colour, scale);
        x += advance;
    }
}

static void draw_bar(int x, int y, int w, int h, int filled, uint16_t colour)
{
    if (filled < 0) {
        filled = 0;
    }
    if (filled > w) {
        filled = w;
    }
    if (filled > 0) {
        fill_rect(x, y, filled, h, colour);
    }
    if (filled < w) {
        fill_rect(x + filled, y, w - filled, h, COL_DIM);
    }
}

bool status_display_init(void)
{
    gpio_config_t bk = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << TFT_BL,
    };
    if (gpio_config(&bk) != ESP_OK) {
        return false;
    }
    gpio_set_level(TFT_BL, 1);

    spi_bus_config_t buscfg = {
        .sclk_io_num = TFT_SCLK,
        .mosi_io_num = TFT_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TFT_WIDTH * BAND_H * sizeof(uint16_t),
    };
    esp_err_t err = spi_bus_initialize(TFT_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "spi bus failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = TFT_DC,
        .cs_gpio_num = TFT_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TFT_HOST, &io_cfg, &io);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "panel io failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = TFT_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    err = esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "st7789 new failed: %s", esp_err_to_name(err));
        return false;
    }

    if (esp_lcd_panel_reset(s_panel) != ESP_OK || esp_lcd_panel_init(s_panel) != ESP_OK) {
        ESP_LOGW(TAG, "st7789 init failed");
        return false;
    }
    /* 135x240 panel: gaps + swap for landscape 240x135 (Ideaspark / TTGO-style). */
    esp_lcd_panel_invert_color(s_panel, true);
    esp_lcd_panel_swap_xy(s_panel, true);
    esp_lcd_panel_mirror(s_panel, false, true);
    esp_lcd_panel_set_gap(s_panel, 40, 53);
    esp_lcd_panel_disp_on_off(s_panel, true);

    s_ready = true;
    s_blank = false;
    s_last_activity_us = esp_timer_get_time();
    ESP_LOGI(TAG, "ST7789 ready %dx%d mosi=%d sclk=%d cs=%d dc=%d rst=%d bl=%d (banded)",
             TFT_WIDTH, TFT_HEIGHT, TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL);

    status_display_model_t boot = {0};
    snprintf(boot.thing_name, sizeof(boot.thing_name), "booting");
    snprintf(boot.status_line, sizeof(boot.status_line), "%s", DEVICE_MODEL);
    status_display_update(&boot);
    return true;
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
        gpio_set_level(TFT_BL, 1);
        esp_lcd_panel_disp_on_off(s_panel, true);
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
        gpio_set_level(TFT_BL, 0);
        esp_lcd_panel_disp_on_off(s_panel, false);
        s_blank = true;
        ESP_LOGI(TAG, "display blank after %us idle", s_blank_timeout_s);
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

    fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, COL_BG);
    draw_text(8, 8, model->thing_name[0] ? model->thing_name : "fleet", COL_FG, 2);

    char line[40];
    if (model->wifi_connected) {
        snprintf(line, sizeof(line), "WIFI %s", model->wifi_ssid[0] ? model->wifi_ssid : "up");
    } else {
        snprintf(line, sizeof(line), "WIFI down");
    }
    draw_text(8, 36, line, COL_FG, 2);

    int bars = 0;
    if (model->wifi_connected) {
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
    draw_text(8, 64, line, COL_FG, 2);
    draw_bar(140, 68, 80, 10, bars * 8, COL_ACC);

    draw_text(8, 92, model->iot_connected ? "IOT CONNECTED" : "IOT WAITING",
              model->iot_connected ? COL_ACC : COL_FG, 2);
    if (model->status_line[0]) {
        draw_text(8, 116, model->status_line, COL_DIM, 1);
    }
    s_last_activity_us = esp_timer_get_time();
}

#endif /* DEVICE_HAS_STATUS_TFT */
