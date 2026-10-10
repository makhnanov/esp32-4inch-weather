#!/usr/bin/env python3
"""Generate secrets.h from .env so credentials live in exactly one place.

.env is the source of truth and stays out of version control; secrets.h is a
build artifact. Re-run this (or just `make flash`) after editing .env.
"""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
ENV = ROOT / ".env"
# The sketch lives in its own directory so that arduino-cli finds a .ino
# matching the folder name no matter what the repo or clone is called.
SKETCH_DIR = ROOT / "esp32-weather"
OUT = SKETCH_DIR / "secrets.h"

# .env key -> C macro. Several spellings are accepted for the same macro so a
# hand-written .env does not have to guess our naming.
KEYMAP = {
    "wifi": "WIFI_SSID",
    "wifi_ssid": "WIFI_SSID",
    "ssid": "WIFI_SSID",
    "password": "WIFI_PASS",
    "wifi_password": "WIFI_PASS",
    "pass": "WIFI_PASS",
}


def c_string(value):
    """Quote a value as a C string literal."""
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return '"%s"' % escaped


def main():
    if not ENV.is_file():
        sys.exit("error: %s not found" % ENV)

    found = {}
    for raw in ENV.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, value = line.partition("=")
        macro = KEYMAP.get(key.strip().lower())
        if not macro:
            continue
        # Strip surrounding quotes if the user added them; keep inner spaces,
        # which are legal in both an SSID and a passphrase.
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        found[macro] = value

    missing = [m for m in ("WIFI_SSID", "WIFI_PASS") if m not in found]
    if missing:
        sys.exit("error: %s has no value for %s" % (ENV.name, ", ".join(missing)))

    OUT.write_text(
        "// Generated from .env by tools/gen_secrets.py -- do not edit, do not commit.\n"
        "#pragma once\n\n"
        "#define WIFI_SSID %s\n"
        "#define WIFI_PASS %s\n" % (c_string(found["WIFI_SSID"]), c_string(found["WIFI_PASS"])),
        encoding="utf-8",
    )
    # Print the SSID but never the passphrase.
    print("secrets.h written (ssid=%r, password=%d chars)"
          % (found["WIFI_SSID"], len(found["WIFI_PASS"])))


if __name__ == "__main__":
    main()
