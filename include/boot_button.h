#pragma once

#include <stdbool.h>

// BOOT button (active-low). Override per board: classic ESP32=0, ESP32-C61=9.
#ifndef BOOT_BUTTON_GPIO
#define BOOT_BUTTON_GPIO 0
#endif

void boot_button_init(void);
// True once per press (debounced). Clears the pending flag.
bool boot_button_take_press(void);
