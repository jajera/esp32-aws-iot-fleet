#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool wifi_connected;
    int8_t wifi_rssi;       // dBm when connected; 0 if unknown
    char wifi_ssid[33];
    bool iot_connected;
    char thing_name[64];
    char status_line[40];   // short extra (e.g. "pub ok" / error)
} status_display_model_t;

// Init I2C SSD1306. Returns false if no panel (firmware still runs).
bool status_display_init(void);

void status_display_update(const status_display_model_t *model);

bool status_display_ready(void);

// Blank panel after idle; BOOT press calls wake. Timeout in seconds (0 = never).
void status_display_set_blank_timeout_s(unsigned timeout_s);
void status_display_tick(void);
void status_display_wake(void);
bool status_display_is_blank(void);
