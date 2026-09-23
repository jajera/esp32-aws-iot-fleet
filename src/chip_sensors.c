#include "chip_sensors.h"

#include "esp_log.h"
#include "sdkconfig.h"

#if defined(DEVICE_HAS_CHIP_TEMP) && DEVICE_HAS_CHIP_TEMP
#include "driver/temperature_sensor.h"
#endif

static const char *TAG = "sensors";

#if defined(DEVICE_HAS_CHIP_TEMP) && DEVICE_HAS_CHIP_TEMP
static temperature_sensor_handle_t s_temp;
static bool s_temp_ok;
#endif

void chip_sensors_init(void)
{
#if defined(DEVICE_HAS_CHIP_TEMP) && DEVICE_HAS_CHIP_TEMP
    temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    if (temperature_sensor_install(&cfg, &s_temp) != ESP_OK) {
        ESP_LOGW(TAG, "temp sensor install failed");
        return;
    }
    if (temperature_sensor_enable(s_temp) != ESP_OK) {
        ESP_LOGW(TAG, "temp sensor enable failed");
        return;
    }
    s_temp_ok = true;
    ESP_LOGI(TAG, "chip temperature sensor ready");
#else
    ESP_LOGI(TAG, "chip temperature not available on this model");
#endif
}

bool chip_sensors_read_temp_c(float *out_c)
{
#if defined(DEVICE_HAS_CHIP_TEMP) && DEVICE_HAS_CHIP_TEMP
    if (!s_temp_ok || out_c == NULL) {
        return false;
    }
    return temperature_sensor_get_celsius(s_temp, out_c) == ESP_OK;
#else
    (void)out_c;
    return false;
#endif
}
