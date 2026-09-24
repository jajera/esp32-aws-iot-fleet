# This file was automatically generated for ESP-IDF PlatformIO projects.
# Uncomment lines below if you need to add extra source files or include dirs.

set(BOARD_REQUIRES
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

# esp32-camera is ESP32/S2/S3 only — pull it solely for the cam env.
if(DEFINED ENV{PIOENV} AND "$ENV{PIOENV}" STREQUAL "esp32-cam")
    list(APPEND BOARD_REQUIRES fatfs sdmmc esp_driver_sdmmc esp_driver_spi esp32-camera)
endif()

idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "." "../include"
    REQUIRES ${BOARD_REQUIRES}
)
