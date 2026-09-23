# esp32-s3 — ESP32-S3 (N16R8-class)

| Field | Value |
|-------|--------|
| **Make / product** | ESP32-S3 DevKit-class (16MB flash / 8MB PSRAM) |
| **Chip** | ESP32-S3 |
| **USB** | CH343 USB-UART → `/dev/ttyACM0` (`1a86:55d3`) |
| **Status UI** | Addressable RGB (GPIO48 v1.0 / GPIO38 v1.1) + serial |
| **BOOT** | GPIO **0** (active-low) → `button` event |
| **PIO env** | `esp32-s3` |
| **Thing** | `esp32-s3-01` |
| **Thing Type** | `esp32-s3` |
| **MQTT topics** | `fleet/esp32-s3-01/…` |

## LEDs

| LED | Expected |
|-----|----------|
| **3.3 V power** (hardwired red/amber near USB) | On whenever the board is powered |
| **Addressable RGB** (WS2812 near the pin header) | Driven by firmware — dark until `status_rgb` runs |

Boot probes GPIO48 then GPIO38 (white), then R→G→B on the preferred pin. Steady **green** = MQTT up; **amber** = Wi‑Fi only; **blue flash** = telemetry sent. Override pin: `-DSTATUS_RGB_GPIO=38`.

## Telemetry

Every 15s to `fleet/esp32-s3-01/telemetry` (includes `chip_temp_c` via IDF temp sensor + PSRAM):

`device_id`, `ts`, `type`, `rssi`, `uptime_s`, `heap_free`, `chip_temp_c`, `chip_model`, `chip_cores`, `chip_revision`, `cpu_mhz`, `flash_bytes`, `psram_bytes`, `psram_free`, `wifi_ssid`, `wifi_status`, `wifi_channel`, `wifi_ip`, `wifi_gateway`, `wifi_dns`, `model`, `clock_offset_ms`.

BOOT publishes `fleet/esp32-s3-01/events` (`type=button`, `event=press`).

## Onboard

```bash
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2 FLEET_ALLOW_AWS=1
THING_NAME=esp32-s3-01 THING_TYPE=esp32-s3 \
  BOARD_ATTR=ESP32-S3-N16R8 PATTERN=esp32-s3 \
  PIO_ENV=esp32-s3 UPLOAD_HINT=/dev/ttyACM0 \
  ./aws/provision-device.sh

# secrets.ini: Wi-Fi + endpoint
pio run -e esp32-s3 -t upload    # /dev/ttyACM0
```

Confirm port:

```bash
ls -l /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
lsusb | rg -i 'espressif|303a|1a86|ch340|55d3'
```

- Espressif native USB (`303a:`) → often C61 → `esp32-c61`
- CH340 `/dev/ttyUSB*` → Ideaspark → `ideaspark-oled`
- CH343 `/dev/ttyACM*` (`1a86:55d3`) → this env

## Related

- C61: [`esp32-c61.md`](esp32-c61.md)
- Ideaspark OLED: [`ideaspark-oled.md`](ideaspark-oled.md)
- ESP32-C3: [`esp32-c3.md`](esp32-c3.md)
