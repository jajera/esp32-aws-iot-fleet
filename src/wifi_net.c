#include "wifi_net.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "wifi";
static EventGroupHandle_t s_wifi_events;
static const int WIFI_GOT_IP = BIT0;
static char s_ssid[33];

static void on_wifi(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_events, WIFI_GOT_IP);
        ESP_LOGW(TAG, "disconnected — retry");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_events, WIFI_GOT_IP);
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got ip " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

bool wifi_net_init(const char *ssid, const char *password)
{
    if (ssid == NULL || ssid[0] == '\0') {
        ESP_LOGE(TAG, "WIFI_SSID empty");
        return false;
    }
    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_events = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi, NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "connecting ssid=%s", ssid);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_events, WIFI_GOT_IP, pdFALSE, pdFALSE, pdMS_TO_TICKS(20000));
    return (bits & WIFI_GOT_IP) != 0;
}

bool wifi_net_is_connected(void)
{
    if (s_wifi_events == NULL) {
        return false;
    }
    return (xEventGroupGetBits(s_wifi_events) & WIFI_GOT_IP) != 0;
}

int8_t wifi_net_rssi(void)
{
    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
        return 0;
    }
    return ap.rssi;
}

void wifi_net_copy_ssid(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return;
    }
    strncpy(out, s_ssid, out_len - 1);
    out[out_len - 1] = '\0';
}

bool wifi_net_get_link(wifi_net_link_t *out)
{
    if (out == NULL) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->status = wifi_net_is_connected() ? 3 : 0;

    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        out->channel = (int8_t)ap.primary;
    }

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        return out->status == 3;
    }

    esp_netif_ip_info_t ip = {0};
    if (esp_netif_get_ip_info(netif, &ip) == ESP_OK) {
        snprintf(out->ip, sizeof(out->ip), IPSTR, IP2STR(&ip.ip));
        snprintf(out->gateway, sizeof(out->gateway), IPSTR, IP2STR(&ip.gw));
    }

    esp_netif_dns_info_t dns = {0};
    if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK &&
        dns.ip.type == ESP_IPADDR_TYPE_V4) {
        snprintf(out->dns, sizeof(out->dns), IPSTR, IP2STR(&dns.ip.u_addr.ip4));
    }
    return out->status == 3;
}
