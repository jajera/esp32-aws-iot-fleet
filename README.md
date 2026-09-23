# esp32-aws-iot-fleet

Central ESP32 firmware for jajera: one PlatformIO + ESP-IDF tree, one env per board, MQTT telemetry on `fleet/<thing>/…`.

**Works today (pre-provisioned):** Wi‑Fi STA, AWS IoT MQTT, connectivity telemetry, BOOT button events, OLED (Ideaspark) / RGB (S3, C3).

**Next (claim path):** SoftAP/BLE Wi‑Fi provisioning and Fleet Provisioning by Claim — still TODO in `bootstrap.c`.

Talk/demo sketches stay thin; **this** repo owns the board matrix, shared firmware, CI, and AWS device onboarding scripts.

## Layout

```text
platformio.ini          # public build config (no secrets)
secrets.ini.example     # copy → secrets.ini (Wi‑Fi + IoT endpoint)
secrets/                # local PEMs only (gitignored except README)
src/ include/           # shared firmware
aws/provision-device.sh # Thing + cert → secrets/devices/<thing>/
aws/examples/           # claim policy / template stubs (future)
docs/boards/            # per-model notes
scripts/                # ensure_secrets + secret scan
partitions.csv          # 4MB OTA-capable partition table
AGENTS.md               # agent entrypoint
.kiro/ .cursor/         # steering, hooks, MCP
```

### Model envs (today)

| Env | Board | Role |
|-----|-------|------|
| `esp32-c61` (default) | ESP32-C61-DevKitC-1 | **Primary** — Thing `esp32-c61-01` |
| `ideaspark-oled` | Ideaspark ESP32 OLED-0.96 V3.0 | OLED status; Thing `ideaspark-oled-01` |
| `esp32-s3` | ESP32-S3 N16R8-class | RGB status; Thing `esp32-s3-01` |
| `esp32-c3` | ESP32-C3-DevKitM-1 class | RGB status; Thing `esp32-c3-01` |

Board notes: [`docs/boards/esp32-c61.md`](docs/boards/esp32-c61.md), [`docs/boards/ideaspark-oled.md`](docs/boards/ideaspark-oled.md), [`docs/boards/esp32-s3.md`](docs/boards/esp32-s3.md), [`docs/boards/esp32-c3.md`](docs/boards/esp32-c3.md).

## Secrets (public repo)

Nothing sensitive belongs in git.

| Commit | Keep local |
|--------|------------|
| `secrets.ini.example` | `secrets.ini` (`custom_wifi_*`, `custom_aws_*`) |
| `secrets/README.md` | `secrets/devices/*/device_certs.h`, PEMs, keys |
| `aws/examples/*` placeholders | Live claim certs if you add them |

Before push:

```bash
./scripts/check_no_secrets.sh
```

Do **not** redefine `build_flags` in `secrets.ini`.

## Quick start — esp32-c61 (primary)

```bash
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2 FLEET_ALLOW_AWS=1
THING_NAME=esp32-c61-01 THING_TYPE=esp32-c61 \
  BOARD_ATTR=ESP32-C61-DevKitC-1 PIO_ENV=esp32-c61 UPLOAD_HINT=/dev/ttyACM0 \
  ./aws/provision-device.sh
# edit secrets.ini: Wi-Fi + endpoint
pio run -e esp32-c61 -t upload     # /dev/ttyACM0
```

Details: [`docs/boards/esp32-c61.md`](docs/boards/esp32-c61.md).

## Quick start — ideaspark-oled

```bash
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2 FLEET_ALLOW_AWS=1
THING_NAME=ideaspark-oled-01 THING_TYPE=ideaspark-oled \
  BOARD_ATTR=Ideaspark_ESP32_OLED-0.96_V3.0 PIO_ENV=ideaspark-oled UPLOAD_HINT=/dev/ttyUSB0 \
  ./aws/provision-device.sh
pio run -e ideaspark-oled -t upload     # /dev/ttyUSB0
```

Details: [`docs/boards/ideaspark-oled.md`](docs/boards/ideaspark-oled.md).

## Quick start — esp32-s3 / esp32-c3

```bash
# S3 (ACM — CH343 or native USB)
THING_NAME=esp32-s3-01 THING_TYPE=esp32-s3 BOARD_ATTR=ESP32-S3-N16R8 \
  PIO_ENV=esp32-s3 UPLOAD_HINT=/dev/ttyACM0 ./aws/provision-device.sh
pio run -e esp32-s3 -t upload

# C3 (native USB Serial/JTAG)
THING_NAME=esp32-c3-01 THING_TYPE=esp32-c3 BOARD_ATTR=ESP32-C3-DevKitM-1 \
  PIO_ENV=esp32-c3 UPLOAD_HINT=/dev/ttyACM0 ./aws/provision-device.sh
pio run -e esp32-c3 -t upload
```

Details: [`esp32-s3.md`](docs/boards/esp32-s3.md), [`esp32-c3.md`](docs/boards/esp32-c3.md).

## Telemetry contract

Every model publishes JSON to `fleet/<thing>/telemetry` (~15s) with the same keys (`chip_temp_c` / `psram_*` may be null/`0`). BOOT → `fleet/<thing>/events` (`type=button`).

## Claim provisioning (roadmap)

1. Factory flash + batch claim credentials.
2. SoftAP/BLE Wi‑Fi provisioning.
3. Fleet Provisioning by Claim (CSR).
4. Persist device cert; MQTT + OTA as today.

## Adding a model

1. Add `[env:my-model]` in `platformio.ini` with `DEVICE_MODEL` / `DEVICE_THING_TYPE` / `THING_NAME`.
2. Create matching Thing Type in AWS IoT; allowlist in `aws/examples/preprovision_hook.py`.
3. Add `docs/boards/<env>.md` and CI matrix entry in `.github/workflows/platformio-build.yml`.
4. Keep drivers behind `DEVICE_*` flags; leave Wi‑Fi/MQTT shared.

## CI

actionsforge reusables: markdown lint, YAML lint, commitmsg-conform, Dependabot auto-merge. Local `platformio-build.yml` builds all four envs + `scripts/check_no_secrets.sh`.

## Agent tooling

- **Kiro:** `.kiro/steering/`, `.kiro/hooks/` (`FLEET_ALLOW_AWS=1` to mutate AWS), MCP.
- **Cursor:** `.cursor/rules/` + `.cursor/mcp.json`.
- Start at [AGENTS.md](AGENTS.md).

## License

See [LICENSE](LICENSE).
