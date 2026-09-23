---
inclusion: always
---

# Product

`esp32-aws-iot-fleet` is the **central ESP32 firmware repo** for jajera.

One PlatformIO tree, one env per board/model, shared ESP-IDF firmware.
**Today:** pre-provisioned Things + Wi‑Fi + MQTT telemetry on `fleet/<thing>/…`.
**Next:** Fleet Provisioning by Claim (CSR), then OTA.

## In scope

- Shared ESP-IDF firmware under `src/` / `include/`
- PlatformIO envs per hardware SKU / Thing Type
- `aws/provision-device.sh` for per-device certs
- Secrets hygiene for a **public** repo
- AWS claim examples under `aws/examples/` (placeholders)
- CI: PlatformIO matrix + actionsforge hygiene

## Out of scope (for now)

- Talk slides / Amplify dashboard (see `iot-everywhere-aws-talk`)
- Full hands-on lab prose (see `aws-iot-walkthrough`)
- Committing claim certs, device keys, Wi-Fi passwords, or live account ARNs
- Non-Espressif boards (e.g. Pico W) — same MQTT contract, separate firmware

## Related

| Repo | Role |
|------|------|
| This repo | Central build + device firmware |
| `iot-everywhere-aws-talk` | Stage talk + thin demo / dashboard |
| `aws-iot-walkthrough` | Full lab docs |

Keep demo feeders thin; prefer shared logic here over copying bootstrap into talk repos.
