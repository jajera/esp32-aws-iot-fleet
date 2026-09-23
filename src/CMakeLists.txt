# This file was automatically generated for ESP-IDF PlatformIO projects.
# Uncomment lines below if you need to add extra source files or include dirs.

idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "." "../include"
    REQUIRES
        mqtt
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        esp_timer
        driver
        esp_lcd
        led_strip
        esp_hw_support
)
