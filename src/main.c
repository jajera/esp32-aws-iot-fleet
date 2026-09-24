#include "app_config.h"
#include "board_camera.h"
#include "board_sd.h"
#include "boot_button.h"
#include "bootstrap.h"
#include "chip_sensors.h"
#include "iot_mqtt.h"
#include "status_display.h"
#include "status_rgb.h"
#include "wifi_net.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_sntp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "mbedtls/base64.h"

#if __has_include("device_certs.h")
#include "device_certs.h"
#define HAS_DEVICE_CERTS 1
#else
#define HAS_DEVICE_CERTS 0
#endif

static const char *TAG = "app";

static void refresh_display(const char *extra)
{
    status_display_model_t m = {0};
#if defined(THING_NAME)
    strncpy(m.thing_name, THING_NAME, sizeof(m.thing_name) - 1);
#else
    strncpy(m.thing_name, DEVICE_MODEL, sizeof(m.thing_name) - 1);
#endif
    m.wifi_connected = wifi_net_is_connected();
    m.wifi_rssi = wifi_net_rssi();
    wifi_net_copy_ssid(m.wifi_ssid, sizeof(m.wifi_ssid));
    m.iot_connected = iot_mqtt_is_connected();
    if (extra) {
        strncpy(m.status_line, extra, sizeof(m.status_line) - 1);
    }
    status_display_update(&m);
}

static uint32_t epoch_now(void)
{
    time_t now = 0;
    time(&now);
    if (now < 1700000000) {
        return 0;
    }
    return (uint32_t)now;
}

static const char *chip_model_str(void)
{
    /* CHIP_* are enum values, not macros — never gate cases with #if defined(CHIP_*). */
    esp_chip_info_t info;
    esp_chip_info(&info);
    switch (info.model) {
    case CHIP_ESP32:
        return "ESP32";
    case CHIP_ESP32S2:
        return "ESP32-S2";
    case CHIP_ESP32S3:
        return "ESP32-S3";
    case CHIP_ESP32C3:
        return "ESP32-C3";
    case CHIP_ESP32C6:
        return "ESP32-C6";
    case CHIP_ESP32C61:
        return "ESP32-C61";
    default:
        return "unknown";
    }
}

static int build_telemetry_json(char *out, size_t out_len)
{
    wifi_net_link_t link = {0};
    wifi_net_get_link(&link);

    char ssid[33] = {0};
    wifi_net_copy_ssid(ssid, sizeof(ssid));

    uint32_t flash_bytes = 0;
    esp_flash_get_size(NULL, &flash_bytes);

    const uint32_t heap_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    const uint32_t heap_min_free = (uint32_t)heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    const uint32_t psram_bytes = (uint32_t)heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    const uint32_t psram_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000LL);
    const uint16_t cpu_mhz = (uint16_t)(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);

    esp_chip_info_t chip = {0};
    esp_chip_info(&chip);

    uint8_t mac_raw[6] = {0};
    char mac_str[18] = {0};
    if (esp_wifi_get_mac(WIFI_IF_STA, mac_raw) == ESP_OK) {
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac_raw[0], mac_raw[1],
                 mac_raw[2], mac_raw[3], mac_raw[4], mac_raw[5]);
    }

    char bssid_str[18] = {0};
    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        snprintf(bssid_str, sizeof(bssid_str), "%02x:%02x:%02x:%02x:%02x:%02x", ap.bssid[0],
                 ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5]);
    }

    float temp_c = 0.0f;
    const bool have_temp = chip_sensors_read_temp_c(&temp_c);
    char temp_json[32] = "null";
    if (have_temp) {
        snprintf(temp_json, sizeof(temp_json), "%.1f", (double)temp_c);
    }

    const bool has_wifi = (chip.features & CHIP_FEATURE_WIFI_BGN) != 0;
    const bool has_ble = (chip.features & CHIP_FEATURE_BLE) != 0;
    const bool has_bt = (chip.features & CHIP_FEATURE_BT) != 0;
    const bool has_emb_flash = (chip.features & CHIP_FEATURE_EMB_FLASH) != 0;
    const bool has_emb_psram = (chip.features & CHIP_FEATURE_EMB_PSRAM) != 0;

    board_camera_status_t cam = {0};
    board_sd_status_t sd = {0};
    board_camera_get_status(&cam);
    board_sd_get_status(&sd);

    return snprintf(
        out, out_len,
        "{"
        "\"device_id\":\"%s\","
        "\"ts\":%lu,"
        "\"type\":\"connectivity\","
        "\"rssi\":%d,"
        "\"uptime_s\":%lu,"
        "\"heap_free\":%lu,"
        "\"heap_min_free\":%lu,"
        "\"chip_temp_c\":%s,"
        "\"chip_model\":\"%s\","
        "\"chip_cores\":%u,"
        "\"chip_revision\":%u,"
        "\"chip_features\":%lu,"
        "\"has_wifi\":%s,"
        "\"has_ble\":%s,"
        "\"has_bt\":%s,"
        "\"has_emb_flash\":%s,"
        "\"has_emb_psram\":%s,"
        "\"cpu_mhz\":%u,"
        "\"flash_bytes\":%lu,"
        "\"psram_bytes\":%lu,"
        "\"psram_free\":%lu,"
        "\"mac\":\"%s\","
        "\"wifi_bssid\":\"%s\","
        "\"wifi_ssid\":\"%s\","
        "\"wifi_status\":%d,"
        "\"wifi_channel\":%d,"
        "\"wifi_ip\":\"%s\","
        "\"wifi_gateway\":\"%s\","
        "\"wifi_dns\":\"%s\","
        "\"reset_reason\":%d,"
        "\"app_version\":\"%s\","
        "\"model\":\"%s\","
        "\"camera_ok\":%s,"
        "\"camera_sensor\":\"%s\","
        "\"camera_frame_bytes\":%lu,"
        "\"camera_captures\":%lu,"
        "\"camera_fails\":%lu,"
        "\"sd_ok\":%s,"
        "\"sd_total_mb\":%lu,"
        "\"sd_free_mb\":%lu,"
        "\"sd_writes\":%lu,"
        "\"sd_fails\":%lu,"
        "\"clock_offset_ms\":0"
        "}",
        THING_NAME, (unsigned long)epoch_now(), (int)wifi_net_rssi(), (unsigned long)uptime_s,
        (unsigned long)heap_free, (unsigned long)heap_min_free, temp_json, chip_model_str(),
        (unsigned)chip.cores, (unsigned)chip.revision, (unsigned long)chip.features,
        has_wifi ? "true" : "false", has_ble ? "true" : "false", has_bt ? "true" : "false",
        has_emb_flash ? "true" : "false", has_emb_psram ? "true" : "false", (unsigned)cpu_mhz,
        (unsigned long)flash_bytes, (unsigned long)psram_bytes, (unsigned long)psram_free, mac_str,
        bssid_str, ssid, (int)link.status, (int)link.channel, link.ip, link.gateway, link.dns,
        (int)esp_reset_reason(), APP_VERSION, DEVICE_MODEL,
        cam.ok ? "true" : "false", cam.sensor[0] ? cam.sensor : "",
        (unsigned long)cam.last_frame_bytes, (unsigned long)cam.capture_count,
        (unsigned long)cam.fail_count, sd.ok ? "true" : "false",
        (unsigned long)(sd.total_bytes / (1024 * 1024)), (unsigned long)(sd.free_bytes / (1024 * 1024)),
        (unsigned long)sd.write_count, (unsigned long)sd.fail_count);
}

static void cam_capture_publish(void)
{
#if DEVICE_HAS_CAMERA
    uint8_t *jpg = NULL;
    size_t jpg_len = 0;
    if (!board_camera_capture_jpeg(&jpg, &jpg_len) || !jpg || jpg_len == 0) {
        return;
    }

#if DEVICE_HAS_SD
    char path[80];
    snprintf(path, sizeof(path), "/sdcard/fleet/%s_%lu.jpg", THING_NAME, (unsigned long)epoch_now());
    if (!board_sd_write_file(path, jpg, jpg_len)) {
        ESP_LOGW(TAG, "sd write failed for capture");
    }
#endif

    if (iot_mqtt_is_connected()) {
        size_t b64_len = 0;
        mbedtls_base64_encode(NULL, 0, &b64_len, jpg, jpg_len);
        const size_t msg_est = b64_len + 160;
#ifndef MQTT_OUT_BUFFER_SIZE
#define MQTT_OUT_BUFFER_SIZE 8192
#endif
        if (msg_est >= (size_t)MQTT_OUT_BUFFER_SIZE) {
            ESP_LOGW(TAG, "camera frame too large for MQTT (%u jpeg → ~%u mqtt, out_buf=%d)",
                     (unsigned)jpg_len, (unsigned)msg_est, MQTT_OUT_BUFFER_SIZE);
        } else {
            char *b64 = (char *)malloc(b64_len + 1);
            char *msg = NULL;
            if (b64) {
                size_t out_len = 0;
                if (mbedtls_base64_encode((unsigned char *)b64, b64_len + 1, &out_len, jpg,
                                         jpg_len) == 0) {
                    b64[out_len] = '\0';
                    const size_t msg_cap = out_len + 160;
                    msg = (char *)malloc(msg_cap);
                    if (msg) {
                        int n = snprintf(msg, msg_cap,
                                         "{\"device_id\":\"%s\",\"ts\":%lu,\"type\":\"camera\","
                                         "\"jpeg_b64\":\"%s\"}",
                                         THING_NAME, (unsigned long)epoch_now(), b64);
                        if (n > 0 && (size_t)n < msg_cap &&
                            iot_mqtt_publish_camera(msg, (size_t)n)) {
                            ESP_LOGI(TAG, "camera frame published %u jpeg / %d mqtt bytes",
                                     (unsigned)jpg_len, n);
                        } else {
                            ESP_LOGW(TAG, "camera mqtt publish failed");
                        }
                    }
                }
            }
            free(msg);
            free(b64);
        }
    }

    board_camera_release_frame();
#endif
}

static void start_sntp(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    bootstrap_init();
    chip_sensors_init();
    boot_button_init();
    status_display_init();
    status_display_set_blank_timeout_s(30);
    status_rgb_init();
#if DEVICE_HAS_SD
    /* Mount SD before camera — CAM init touches shared straps/GPIOs. */
    if (board_sd_init()) {
        ESP_LOGI(TAG, "sd init ok");
    } else {
        ESP_LOGW(TAG, "sd init failed");
    }
#endif
#if DEVICE_HAS_CAMERA
    if (board_camera_init()) {
        ESP_LOGI(TAG, "camera init ok");
    } else {
        ESP_LOGW(TAG, "camera init failed");
    }
#endif
    refresh_display("init");
#if !HAS_DEVICE_CERTS
    ESP_LOGW(TAG, "no device_certs.h — run aws/provision-device.sh then rebuild");
    refresh_display("no certs");
    while (1) {
        if (boot_button_take_press()) {
            status_display_wake();
        }
        status_display_tick();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
#else

#if !defined(WIFI_SSID) || !defined(WIFI_PASSWORD)
#error "WIFI_SSID and WIFI_PASSWORD must come from secrets.ini custom_wifi_*"
#endif

    refresh_display("wifi...");
    if (!wifi_net_init(WIFI_SSID, WIFI_PASSWORD)) {
        ESP_LOGE(TAG, "wifi failed");
        refresh_display("wifi fail");
    } else {
        refresh_display("wifi ok");
        start_sntp();
    }

    refresh_display("iot...");
    if (!iot_mqtt_init(AWS_IOT_ENDPOINT, THING_NAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE)) {
        ESP_LOGE(TAG, "mqtt start failed");
        refresh_display("mqtt fail");
    }

    int64_t last_pub_us = 0;
    int64_t last_cam_us = 0;
    bool cam_boot_done = false;
    while (1) {
        if (boot_button_take_press()) {
            status_display_wake();
            if (iot_mqtt_is_connected()) {
                char ev[160];
                int n = snprintf(ev, sizeof(ev),
                                 "{\"device_id\":\"%s\",\"ts\":%lu,\"type\":\"button\",\"event\":\"press\"}",
                                 THING_NAME, (unsigned long)epoch_now());
                if (n > 0 && iot_mqtt_publish_event(ev, (size_t)n)) {
                    refresh_display("btn ok");
                    ESP_LOGI(TAG, "button event published");
                } else {
                    refresh_display("btn fail");
                }
            } else {
                refresh_display("btn wake");
            }
        }

        const bool wifi_ok = wifi_net_is_connected();
        const bool iot_ok = iot_mqtt_is_connected();
        char extra[40] = {0};

        if (wifi_ok && iot_ok) {
            const int64_t now = esp_timer_get_time();
#if DEVICE_HAS_CAMERA
            if (!cam_boot_done) {
                cam_capture_publish();
                last_cam_us = now;
                cam_boot_done = true;
            } else if (now - last_cam_us > 300LL * 1000000LL) {
                cam_capture_publish();
                last_cam_us = now;
            }
#endif
            if (now - last_pub_us > 15LL * 1000000LL) {
                char json[1600];
                int n = build_telemetry_json(json, sizeof(json));
                if (n > 0 && n < (int)sizeof(json) && iot_mqtt_publish_telemetry(json, (size_t)n)) {
                    snprintf(extra, sizeof(extra), "pub ok");
                    last_pub_us = now;
                    status_rgb_flash_telemetry();
                    ESP_LOGI(TAG, "telemetry %d bytes", n);
                } else {
                    snprintf(extra, sizeof(extra), "pub fail");
                }
            } else if (!status_display_is_blank()) {
                snprintf(extra, sizeof(extra), "linked");
            }
        } else if (!wifi_ok) {
            snprintf(extra, sizeof(extra), "wifi...");
        } else {
            snprintf(extra, sizeof(extra), "iot...");
        }

        status_rgb_tick();
        status_rgb_show_link(wifi_ok, iot_ok);

        if (!status_display_is_blank() && extra[0]) {
            refresh_display(extra);
        }
        status_display_tick();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
#endif
}
