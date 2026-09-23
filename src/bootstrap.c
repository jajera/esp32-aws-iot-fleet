#include "bootstrap.h"

#include "app_config.h"
#include "esp_log.h"

static const char *TAG = "bootstrap";
static bootstrap_state_t s_state = BOOTSTRAP_STATE_WIFI_NEEDED;

void bootstrap_init(void)
{
    ESP_LOGI(TAG, "model=%s thing_type=%s app=%s", DEVICE_MODEL, DEVICE_THING_TYPE,
             APP_VERSION);
    ESP_LOGI(TAG, "aws endpoint=%s template=%s", AWS_IOT_ENDPOINT, AWS_PROVISIONING_TEMPLATE);

    // TODO: load Wi-Fi + device cert presence from NVS.
    // TODO: if device cert exists -> READY; else if Wi-Fi configured -> PROVISION_AWS; else WIFI_NEEDED.
    s_state = BOOTSTRAP_STATE_WIFI_NEEDED;
}

bootstrap_state_t bootstrap_state(void)
{
    return s_state;
}

bool bootstrap_is_ready(void)
{
    return s_state == BOOTSTRAP_STATE_READY;
}

void bootstrap_poll(void)
{
    switch (s_state) {
    case BOOTSTRAP_STATE_WIFI_NEEDED:
        // TODO: SoftAP or BLE Wi-Fi provisioning (wifi_provisioning).
        ESP_LOGD(TAG, "waiting for Wi-Fi credentials");
        break;
    case BOOTSTRAP_STATE_PROVISION_AWS:
        // TODO: Fleet Provisioning by Claim + CSR using claim cert from secure storage.
        // Publish CreateCertificateFromCsr / RegisterThing with Model + SerialNumber.
        ESP_LOGD(TAG, "AWS fleet provisioning pending");
        break;
    case BOOTSTRAP_STATE_READY:
        break;
    case BOOTSTRAP_STATE_ERROR:
        ESP_LOGE(TAG, "bootstrap error — see prior logs");
        break;
    }
}
