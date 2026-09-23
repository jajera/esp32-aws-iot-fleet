# Ensures secrets.ini exists locally without committing it.
# PlatformIO pre-script: copies secrets.ini.example when secrets.ini is missing.

Import("env")  # noqa: F821 — provided by PlatformIO

from pathlib import Path

example = Path("secrets.ini.example")
secrets = Path("secrets.ini")

if not secrets.exists():
    if not example.exists():
        raise SystemExit("secrets.ini.example is missing; cannot create secrets.ini")
    secrets.write_text(example.read_text(encoding="utf-8"), encoding="utf-8")
    print("Created secrets.ini from secrets.ini.example — edit local values; do not commit")
