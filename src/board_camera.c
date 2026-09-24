#include "board_camera.h"

#if DEVICE_HAS_CAMERA

#include "driver/gpio.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

/* AI-Thinker ESP32-CAM pin map (module on CAM-MB). */
#ifndef CAM_PIN_PWDN
#define CAM_PIN_PWDN 32
#endif
#ifndef CAM_PIN_RESET
#define CAM_PIN_RESET -1
#endif
#ifndef CAM_PIN_XCLK
#define CAM_PIN_XCLK 0
#endif
#ifndef CAM_PIN_SIOD
#define CAM_PIN_SIOD 26
#endif
#ifndef CAM_PIN_SIOC
#define CAM_PIN_SIOC 27
#endif
#ifndef CAM_PIN_D7
#define CAM_PIN_D7 35
#endif
#ifndef CAM_PIN_D6
#define CAM_PIN_D6 34
#endif
#ifndef CAM_PIN_D5
#define CAM_PIN_D5 39
#endif
#ifndef CAM_PIN_D4
#define CAM_PIN_D4 36
#endif
#ifndef CAM_PIN_D3
#define CAM_PIN_D3 21
#endif
#ifndef CAM_PIN_D2
#define CAM_PIN_D2 19
#endif
#ifndef CAM_PIN_D1
#define CAM_PIN_D1 18
#endif
#ifndef CAM_PIN_D0
#define CAM_PIN_D0 5
#endif
#ifndef CAM_PIN_VSYNC
#define CAM_PIN_VSYNC 25
#endif
#ifndef CAM_PIN_HREF
#define CAM_PIN_HREF 23
#endif
#ifndef CAM_PIN_PCLK
#define CAM_PIN_PCLK 22
#endif
#ifndef CAM_FLASH_GPIO
#define CAM_FLASH_GPIO 4
#endif

static const char *TAG = "cam";
static board_camera_status_t s_status;
static camera_fb_t *s_fb;

static void flash_pulse(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << CAM_FLASH_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(CAM_FLASH_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(40));
    gpio_set_level(CAM_FLASH_GPIO, 0);
}

bool board_camera_init(void)
{
    memset(&s_status, 0, sizeof(s_status));
    strncpy(s_status.sensor, "unknown", sizeof(s_status.sensor) - 1);

    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        /* No PSRAM on many CAM-MB modules — keep frame in DRAM. */
        .frame_size = FRAMESIZE_QQVGA,
        .jpeg_quality = 12,
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_DRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init failed: %s", esp_err_to_name(err));
        s_status.ok = false;
        return false;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
        switch (sensor->id.PID) {
        case OV2640_PID:
            strncpy(s_status.sensor, "OV2640", sizeof(s_status.sensor) - 1);
            break;
        case OV3660_PID:
            strncpy(s_status.sensor, "OV3660", sizeof(s_status.sensor) - 1);
            break;
        case OV5640_PID:
            strncpy(s_status.sensor, "OV5640", sizeof(s_status.sensor) - 1);
            break;
        default:
            snprintf(s_status.sensor, sizeof(s_status.sensor), "pid=0x%04x", sensor->id.PID);
            break;
        }
        sensor->set_brightness(sensor, 0);
        sensor->set_contrast(sensor, 0);
        sensor->set_saturation(sensor, 0);
        sensor->set_whitebal(sensor, 1);
        sensor->set_gain_ctrl(sensor, 1);
        sensor->set_exposure_ctrl(sensor, 1);
    }

    s_status.ok = true;
    ESP_LOGI(TAG, "camera ready sensor=%s qqvga jpeg", s_status.sensor);
    return true;
}

bool board_camera_capture_jpeg(uint8_t **out_buf, size_t *out_len)
{
    if (!s_status.ok || !out_buf || !out_len) {
        return false;
    }
    if (s_fb) {
        esp_camera_fb_return(s_fb);
        s_fb = NULL;
    }

    flash_pulse();
    s_fb = esp_camera_fb_get();
    if (!s_fb || !s_fb->buf || s_fb->len == 0) {
        ESP_LOGE(TAG, "capture failed");
        s_status.fail_count++;
        if (s_fb) {
            esp_camera_fb_return(s_fb);
            s_fb = NULL;
        }
        return false;
    }

    *out_buf = s_fb->buf;
    *out_len = s_fb->len;
    s_status.last_frame_bytes = (uint32_t)s_fb->len;
    s_status.capture_count++;
    ESP_LOGI(TAG, "captured %u jpeg bytes", (unsigned)s_fb->len);
    return true;
}

void board_camera_release_frame(void)
{
    if (s_fb) {
        esp_camera_fb_return(s_fb);
        s_fb = NULL;
    }
}

void board_camera_get_status(board_camera_status_t *out)
{
    if (out) {
        *out = s_status;
    }
}

#endif
