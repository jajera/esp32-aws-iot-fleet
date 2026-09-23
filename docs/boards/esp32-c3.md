# esp32-c3 — ESP32-C3-DevKitM-1 class

| Field | Value |
|-------|--------|
| **Make / product** | Espressif ESP32-C3-DevKitM-1 (or compatible) |
| **Chip** | ESP32-C3 (QFN32), single-core RISC-V, emb. 4MB flash |
| **USB** | Native USB Serial/JTAG → `/dev/ttyACM*` (`303a:1001`) |
| **Status UI** | Addressable RGB (**GPIO2** primary, GPIO8 alt) + serial |
| **BOOT** | GPIO **9** (active-low) → `button` event |
| **PIO env** | `esp32-c3` |
| **Thing** | `esp32-c3-01` |
| **Thing Type** | `esp32-c3` |
| **MQTT topics** | `fleet/esp32-c3-01/…` |

## LEDs

| LED | Expected |
|-----|----------|
| **5 V / power** (hardwired) | On when powered |
| **Addressable RGB** (WS2812) | GPIO**2** on many C3 boards / clones; Espressif DevKitM-1 uses GPIO**8**. Boot probes both. |

## Telemetry

Full fleet payload (same keys on every model; unused sensors are `null` / `0`):

`device_id`, `ts`, `type`, `rssi`, `uptime_s`, `heap_free`, `heap_min_free`, `chip_temp_c`, `chip_model`, `chip_cores`, `chip_revision`, `chip_features`, `has_wifi`, `has_ble`, `has_bt`, `has_emb_flash`, `has_emb_psram`, `cpu_mhz`, `flash_bytes`, `psram_bytes`, `psram_free`, `mac`, `wifi_bssid`, `wifi_ssid`, `wifi_status`, `wifi_channel`, `wifi_ip`, `wifi_gateway`, `wifi_dns`, `reset_reason`, `app_version`, `model`, `clock_offset_ms`.

## Onboard

```bash
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2 FLEET_ALLOW_AWS=1
THING_NAME=esp32-c3-01 THING_TYPE=esp32-c3 \
  BOARD_ATTR=ESP32-C3-DevKitM-1 PATTERN=esp32-c3 \
  PIO_ENV=esp32-c3 UPLOAD_HINT=/dev/ttyACM0 \
  ./aws/provision-device.sh

pio run -e esp32-c3 -t upload    # /dev/ttyACM0
```

## Related

- C61: [`esp32-c61.md`](esp32-c61.md)
- S3: [`esp32-s3.md`](esp32-s3.md)
- Ideaspark OLED: [`ideaspark-oled.md`](ideaspark-oled.md)
