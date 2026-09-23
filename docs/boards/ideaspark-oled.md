# ideaspark-oled — Ideaspark ESP32 OLED-0.96 V3.0

| Field | Value |
|-------|--------|
| **Make / product** | Ideaspark ESP32 OLED-0.96 V3.0 |
| **Chip** | ESP32-WROOM / D0WD (classic) |
| **USB** | CH340 → `/dev/ttyUSB0` |
| **Display** | Onboard 0.96″ SSD1306 128×64 (SDA **21**, SCL **22**, addr **0x3C**) |
| **PIO env** | `ideaspark-oled` |
| **Thing** | `ideaspark-oled-01` |
| **Thing Type** | `ideaspark-oled` |
| **MQTT topics** | `fleet/ideaspark-oled-01/…` |

**Topics:** `fleet/ideaspark-oled-01/…` (not Amplify ingest).

## LCD UI

```text
ideaspark-oled-01
WIFI IOT
RSSI -45  [##########]
IOT  CONNECTED
linked / pub ok
```

Display blanks after **30s** idle (`esp_lcd_panel_disp_on_off`). Press **BOOT** (GPIO0, active-low) to wake. BOOT also publishes `fleet/<thing>/events` (`type=button`).

## Telemetry (onboard only — no external sensors)

Publishes every 15s to `fleet/ideaspark-oled-01/telemetry`, same shape as the talk contract (minus `chip_temp_c` — classic ESP32 has no IDF temp sensor):

`device_id`, `ts`, `type`, `rssi`, `uptime_s`, `heap_free`, `chip_model`, `cpu_mhz`, `flash_bytes`, `wifi_ssid`, `wifi_status`, `wifi_channel`, `wifi_ip`, `wifi_gateway`, `wifi_dns`, `model`, `clock_offset_ms`.

## Onboard

```bash
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2
./aws/provision-device.sh   # defaults: ideaspark-oled-01 / ideaspark-oled
# secrets.ini: Wi-Fi + endpoint
pio run -e ideaspark-oled -t upload
pio device monitor -e ideaspark-oled
```

## Talk vs this board

| | ESP32-C61 (fleet) | This board |
|--|-------------------|------------|
| Hardware | ESP32-C61-DevKitC-1 | Ideaspark ESP32 OLED-0.96 V3.0 |
| Thing | `esp32-c61-01` | `ideaspark-oled-01` |
| Topic prefix | `fleet/` | `fleet/` |
| Status UI | Serial / RGB (later) | Onboard OLED |

C61: [`esp32-c61.md`](esp32-c61.md). S3: [`esp32-s3.md`](esp32-s3.md). C3: [`esp32-c3.md`](esp32-c3.md).
