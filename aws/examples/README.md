# AWS IoT examples (placeholders only)

Replace `REGION`, `ACCOUNT_ID`, and `TEMPLATE_NAME` before use.

| File | Purpose |
|------|---------|
| `claim-policy.json` | IoT policy attached to the shared claim certificate |
| `provisioning-template.json` | Fleet provisioning template body (Thing Type = Model) |
| `preprovision_hook.py` | Example Lambda to allowlist Model + SerialNumber |

Create Thing Types in AWS IoT that match your PlatformIO `DEVICE_THING_TYPE` values (`ideaspark-oled`, `esp32-s3`, `esp32-c61`, `esp32-c3`, ...).

Rotate claim certificates per manufacturing batch. Do not commit claim PEMs; store them under `secrets/claim/`.
