---
inclusion: always
---

# Tech

## Stack

- **PlatformIO** + **ESP-IDF** (`framework = espidf`) — not Arduino for the fleet path
- Platform: `espressif32`
- Partition table: `partitions.csv` (4MB OTA layout; do not overlap otadata and app0)
- App version / AWS placeholders via `platformio.ini` + local `secrets.ini`

## Model envs

| Env | Board | Notes |
|-----|-------|--------|
| `esp32-c61` (default) | `esp32-c61-devkitc1` | Primary; Thing `esp32-c61-01` |
| `ideaspark-oled` | `esp32dev` | Ideaspark ESP32 OLED-0.96 V3.0; Thing `ideaspark-oled-01` |
| `esp32-s3` | `esp32-s3-devkitc-1` | N16R8-class; Thing `esp32-s3-01` |
| `esp32-c3` | `esp32-c3-devkitm-1` | DevKitM-1 class; Thing `esp32-c3-01` |

Add an env when you add an AWS Thing Type / SKU. Put the name in
`.github/workflows/platformio-build.yml` matrix too.

## Commands

```bash
cp secrets.ini.example secrets.ini   # once; edit custom_aws_* + custom_wifi_*
pio run -e esp32-c61
pio run -e esp32-c61 -t upload
pio device monitor -e esp32-c61
./scripts/check_no_secrets.sh
```

First build auto-creates `secrets.ini` via `scripts/ensure_secrets.py` if missing.

## Secrets merge rule

`secrets.ini` may override **only** `custom_aws_iot_endpoint`,
`custom_aws_provisioning_template`, `custom_wifi_ssid`, and
`custom_wifi_password`. Do **not** redefine `build_flags` there —
that drops `APP_VERSION` and other shared flags.

## Components

When adding IDF components (AWS IoT Device SDK, wifi_prov, led_strip, …), use
`src/idf_component.yml` (or project-managed components) and document SPI/RMT
ownership per board. ESP32-C61 has **no RMT** — addressable LEDs need SPI + DMA.

**C61 talk-demo baseline** (verified feeder path to copy quirks from):
`iot-everywhere-aws-talk` → `demo/firmware/README.md` (ACM port, `huge_app` erase,
`led_strip` SPI+DMA, CDC). This fleet repo stays pure ESP-IDF; adapt, don’t paste Arduino.

## AWS defaults when documenting

| Convention | Value |
|------------|-------|
| Profile example | `sandbox` |
| Region example | `ap-southeast-2` |
| Mutation opt-in | `FLEET_ALLOW_AWS=1` |
| ARN samples | Account `123456789012` |
