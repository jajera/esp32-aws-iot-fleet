# esp32-aws-iot-fleet — Agent notes

Central ESP32 multi-model AWS IoT fleet firmware (PlatformIO + ESP-IDF).

## Read first

| Location | What |
|----------|------|
| `.kiro/steering/product.md` | Scope vs talk/lab repos |
| `.kiro/steering/tech.md` | Envs, commands, secrets merge |
| `.kiro/steering/lab-safety.md` | AWS / flash / secrets guards |
| `.kiro/steering/firmware.md` | ESP-IDF conventions |
| `.cursor/rules/` | Same guidance for Cursor |

## Layout

```text
src/ include/           shared firmware (Wi‑Fi, MQTT, telemetry, OLED/RGB)
platformio.ini          esp32-c61 (default), ideaspark-oled, esp32-s3, esp32-c3
aws/provision-device.sh pre-provision Thing + device_certs.h
aws/examples/           claim policy / template stubs (future)
docs/boards/            per-model notes
secrets/ certs/         README only — PEMs gitignored
scripts/                ensure_secrets.py, check_no_secrets.sh
```

## Validate

```bash
pio run -e esp32-c61
pio run -e ideaspark-oled
pio run -e esp32-s3
pio run -e esp32-c3
./scripts/check_no_secrets.sh
```

## AWS mutations

Blocked by default (Kiro hook). Operator opt-in:

```bash
export FLEET_ALLOW_AWS=1
```
