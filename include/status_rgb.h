#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(DEVICE_HAS_STATUS_RGB) && DEVICE_HAS_STATUS_RGB

void status_rgb_init(void);
void status_rgb_set(uint8_t r, uint8_t g, uint8_t b);
void status_rgb_off(void);
/** Dim green when MQTT up; amber when Wi-Fi only; red when down. */
void status_rgb_show_link(bool wifi_ok, bool mqtt_ok);
/** Brief blue pulse after a successful telemetry publish. */
void status_rgb_flash_telemetry(void);
void status_rgb_tick(void);

#else

static inline void status_rgb_init(void) {}
static inline void status_rgb_set(uint8_t r, uint8_t g, uint8_t b)
{
    (void)r;
    (void)g;
    (void)b;
}
static inline void status_rgb_off(void) {}
static inline void status_rgb_show_link(bool wifi_ok, bool mqtt_ok)
{
    (void)wifi_ok;
    (void)mqtt_ok;
}
static inline void status_rgb_flash_telemetry(void) {}
static inline void status_rgb_tick(void) {}

#endif
