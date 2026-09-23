---
inclusion: fileMatch
fileMatchPattern: ["src/**", "include/**", "platformio.ini", "partitions.csv", "sdkconfig.defaults", "sdkconfig.defaults.*"]
---

# Firmware conventions

## Language / framework

- Pure **ESP-IDF** C for fleet code (`framework = espidf`).
- Headers in `include/`; sources in `src/`; register via `src/CMakeLists.txt`.
- Log with `ESP_LOGx` and a short `TAG` per module.

## Identity

Compile-time macros from the PlatformIO env:

- `DEVICE_MODEL` / `DEVICE_THING_TYPE` — required per env
- `APP_VERSION` — shared
- `AWS_IOT_ENDPOINT` / `AWS_PROVISIONING_TEMPLATE` — from `custom_aws_*`

Do not hardcode Thing names or endpoints in source.

## Bootstrap

State machine in `bootstrap.c` / `bootstrap.h`:

`WIFI_NEEDED` → `PROVISION_AWS` → `READY` (or `ERROR`)

Advance states only when the corresponding feature is implemented. Stub
`bootstrap_poll` may log and stay put; do not fake READY.

## Partitions

Edit `partitions.csv` carefully: **otadata must not overlap app0** (app0 starts
at `0x10000`). Keep OTA dual-slot layout unless intentionally changing flash
strategy. Set flash size per env when the board is not 4MB.

## Adding IDF components

1. Declare in `src/idf_component.yml` (or managed components).
2. Note peripheral ownership (SPI2, RMT, UART) in README / steering.
3. Rebuild all matrix envs that need the component.
