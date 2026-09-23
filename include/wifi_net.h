#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char ip[16];
    char gateway[16];
    char dns[16];
    int8_t channel;
    int8_t status; // WL-style: 3=connected, 0=idle/other
} wifi_net_link_t;

bool wifi_net_init(const char *ssid, const char *password);
bool wifi_net_is_connected(void);
int8_t wifi_net_rssi(void);
void wifi_net_copy_ssid(char *out, size_t out_len);
bool wifi_net_get_link(wifi_net_link_t *out);
