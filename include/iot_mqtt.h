#pragma once

#include <stdbool.h>
#include <stddef.h>

bool iot_mqtt_init(const char *endpoint, const char *thing_name, const char *ca_pem,
                   const char *cert_pem, const char *key_pem);
bool iot_mqtt_is_connected(void);
bool iot_mqtt_publish_telemetry(const char *json, size_t len);
bool iot_mqtt_publish_event(const char *json, size_t len);
