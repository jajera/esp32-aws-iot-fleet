#include "iot_mqtt.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "iot";
static esp_mqtt_client_handle_t s_client;
static volatile bool s_connected;
static char s_thing[64];

static void on_mqtt(void *args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)args;
    (void)base;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "mqtt connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "mqtt disconnected");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "mqtt error");
        break;
    default:
        break;
    }
    (void)event_data;
}

bool iot_mqtt_init(const char *endpoint, const char *thing_name, const char *ca_pem,
                   const char *cert_pem, const char *key_pem)
{
    if (endpoint == NULL || endpoint[0] == '\0' || thing_name == NULL || thing_name[0] == '\0') {
        ESP_LOGE(TAG, "missing endpoint/thing");
        return false;
    }
    strncpy(s_thing, thing_name, sizeof(s_thing) - 1);

    char uri[160];
    snprintf(uri, sizeof(uri), "mqtts://%s:8883", endpoint);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = uri,
        .broker.verification.certificate = ca_pem,
        .credentials.client_id = s_thing,
        .credentials.authentication.certificate = cert_pem,
        .credentials.authentication.key = key_pem,
        .session.keepalive = 30,
        .network.reconnect_timeout_ms = 5000,
    };

    s_client = esp_mqtt_client_init(&cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "mqtt init failed");
        return false;
    }
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, on_mqtt, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));
    ESP_LOGI(TAG, "mqtt starting uri=%s client_id=%s", uri, s_thing);
    return true;
}

bool iot_mqtt_is_connected(void)
{
    return s_connected;
}

static bool publish_topic(const char *suffix, const char *json, size_t len)
{
    if (!s_connected || s_client == NULL || json == NULL || s_thing[0] == '\0') {
        return false;
    }
    char topic[96];
    snprintf(topic, sizeof(topic), "fleet/%s/%s", s_thing, suffix);
    int msg_id = esp_mqtt_client_publish(s_client, topic, json, (int)len, 1, 0);
    return msg_id >= 0;
}

bool iot_mqtt_publish_telemetry(const char *json, size_t len)
{
    return publish_topic("telemetry", json, len);
}

bool iot_mqtt_publish_event(const char *json, size_t len)
{
    return publish_topic("events", json, len);
}
