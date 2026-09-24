# esp32-cam — AI-Thinker ESP32-CAM on CAM-MB

| Field | Value |
|-------|--------|
| **Make / product** | AI-Thinker ESP32-CAM + ESP32-CAM-MB (CH340) |
| **Chip** | ESP32-D0WD (classic), typically 4MB flash |
| **USB** | CH340 on MB → `/dev/ttyUSB0` |
| **Camera** | OV2640 (DVP) — AI-Thinker pin map |
| **Storage** | microSD 1-bit SDMMC (CLK **14**, CMD **15**, D0 **2**) |
| **Flash LED** | GPIO **4** (brief pulse on capture) |
| **BOOT** | GPIO0 is camera XCLK — no app button ISR |
| **PIO env** | `esp32-cam` |
| **Thing** | `esp32-cam-01` |
| **Thing Type** | `esp32-cam` |
| **MQTT topics** | `fleet/esp32-cam-01/…` |

## Camera + SD

On boot the firmware:

1. Mounts microSD at `/sdcard` (SPI, then SDMMC 1-bit fallback) — never formats the card
2. Initializes the OV2640 (QQVGA JPEG in DRAM — works without PSRAM)
3. Captures one JPEG to `/sdcard/fleet/esp32-cam-01_<epoch>.jpg`
4. Re-captures every **5 minutes** while online

Telemetry includes `camera_ok`, `camera_sensor`, `camera_frame_bytes`, `sd_ok`, `sd_total_mb`, `sd_free_mb`, write/fail counters.

**SD card:** **FAT32**, seated in the CAM module slot. Snapshots also publish to `fleet/<thing>/camera` → S3 `cameras/<thing>/latest.jpg` for the dashboard widget (independent of SD).

## Onboard

```bash
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2 FLEET_ALLOW_AWS=1
THING_NAME=esp32-cam-01 THING_TYPE=esp32-cam \
  BOARD_ATTR=AI-Thinker_ESP32-CAM-MB PIO_ENV=esp32-cam UPLOAD_HINT=/dev/ttyUSB0 \
  ./aws/provision-device.sh
pio run -e esp32-cam -t upload
pio device monitor -e esp32-cam
```

Hold **IO0** on the MB if auto-reset fails during flash. Leave `ideaspark-oled-01` alone (different MAC).

## Related

- Ideaspark OLED (same CH340 port class): [`ideaspark-oled.md`](ideaspark-oled.md)
- C61 / S3 / C3: sibling board notes in this folder
