---
inclusion: always
---

# Structure

```plaintext
.github/workflows/     # platformio-build + actionsforge hygiene
.cursor/rules/         # Cursor agent rules
.cursor/mcp.json       # Espressif + AWS docs MCP
.kiro/steering/        # always-on product/tech/safety
.kiro/hooks/           # AWS mutation guard
.kiro/settings/mcp.json
.kiro/specs/           # optional design notes
aws/provision-device.sh # pre-provision Thing + device_certs.h
aws/examples/          # claim policy, template stubs (future)
docs/boards/           # per-model notes
certs/                 # README only — PEMs gitignored
secrets/               # README only — claim/device PEMs gitignored
scripts/               # ensure_secrets.py, check_no_secrets.sh
include/               # public headers
src/                   # shared firmware (Wi‑Fi, MQTT, OLED/RGB, sensors)
platformio.ini         # envs + public flags
secrets.ini.example    # → secrets.ini (gitignored)
partitions.csv
sdkconfig.defaults
sdkconfig.defaults.*   # per-target (esp32c3, esp32s3, esp32c61, …)
README.md
AGENTS.md
```

## Rules

- One codebase; model differences are board + `DEVICE_MODEL` / `DEVICE_THING_TYPE`.
- Keep bootstrap shared; put SKU drivers behind `DEVICE_MODEL` checks.
- Never commit `secrets.ini`, PEMs, keys, or generated `sdkconfig.<env>`.
- Prefer fixing the fleet hub over duplicating firmware into talk/demo repos.
