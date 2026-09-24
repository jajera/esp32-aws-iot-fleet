# Local secrets (gitignored)

Never commit files here except this README.

## Wi‑Fi / endpoint

Use repo-root `secrets.ini` (from `secrets.ini.example`) for:

- `custom_wifi_ssid` / `custom_wifi_password`
- `custom_aws_iot_endpoint`

## Per-device certs (pre-provisioned pattern)

```text
secrets/devices/<thing-name>/device_certs.h   # from aws/provision-device.sh
```

Example: `secrets/devices/esp32-c61-01/device_certs.h` (also `ideaspark-oled-01`, `ideaspark-oled-02`, `esp32-s3-01`, `esp32-c3-01`, `esp32-cam-01`)

## Fleet claim batch (future)

```text
secrets/claim/claim-cert.pem
secrets/claim/claim-key.pem
secrets/AmazonRootCA1.pem
```
