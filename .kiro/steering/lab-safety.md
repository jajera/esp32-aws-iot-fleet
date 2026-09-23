---
inclusion: always
---

# Lab and fleet safety

This repository is a **public firmware hub**. Operators own AWS accounts, claim
certs, and device flashing. The agent does not provision AWS or flash boards
unprompted.

## Never run AWS mutations unprompted

Do not create/update/delete IoT Things, certs, policies, templates, or other
resources unless the operator explicitly asked and opted in:

```bash
export FLEET_ALLOW_AWS=1
```

The `guard-aws-mutations` hook blocks accidental mutating AWS CLI / IaC from
shell tools. Read-only (`describe` / `list` / `get` / `s3 ls`) is allowed.

## Secrets

| Never commit | Safe to commit |
|--------------|----------------|
| `secrets.ini` | `secrets.ini.example` |
| `secrets/**/*.pem`, `*.key` | `secrets/README.md` |
| `certs/**` payloads | `certs/README.md` |
| Live account ARNs in examples (prefer placeholders) | `REGION` / `ACCOUNT_ID` placeholders |

Before push: `./scripts/check_no_secrets.sh`

## Firmware / flash

- Do not run `pio run -t upload` / `esptool.py write_flash` unless the operator
  asked and a board is known connected.
- Prefer `pio run -e <env>` (build only) to validate changes.
- C61 RGB / RMT notes: see `.kiro/steering/tech.md` and talk-demo learnings —
  SPI + DMA for WS2812 when there is no RMT.

## Cost

Fleet templates, claim certs, and always-on IoT rules can be billable. Point at
current AWS pricing pages; do not hardcode rates.
