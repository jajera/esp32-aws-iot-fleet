#pragma once

// Compile-time identity. Override AWS_*/WIFI_* via secrets.ini (gitignored).

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

#ifndef DEVICE_MODEL
#error "DEVICE_MODEL must be set by the PlatformIO env (e.g. ideaspark-oled)"
#endif

#ifndef DEVICE_THING_TYPE
#define DEVICE_THING_TYPE DEVICE_MODEL
#endif

#ifndef AWS_IOT_ENDPOINT
#define AWS_IOT_ENDPOINT ""
#endif

#ifndef AWS_PROVISIONING_TEMPLATE
#define AWS_PROVISIONING_TEMPLATE ""
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif
