#!/usr/bin/env python3
"""
inject_version.py — Inject FIRMWARE_VERSION into platformio.ini.

Reads the version string from the INJECT_VERSION environment variable and
replaces the existing -D FIRMWARE_VERSION flag in platformio.ini.

Usage (called by CI workflows):
  INJECT_VERSION=v1.2.3 python scripts/inject_version.py
"""
import os
import re

version = os.environ["INJECT_VERSION"]
with open("platformio.ini", "r", encoding="utf-8") as f:
    content = f.read()
replacement = "-D FIRMWARE_VERSION='\"" + version + "\"'"
content, count = re.subn(
    r"-D FIRMWARE_VERSION='\"[^\"]*\"'",
    lambda _: replacement,
    content,
)
if count == 0:
    raise RuntimeError(
        "FIRMWARE_VERSION flag not found in platformio.ini — version injection failed"
    )
with open("platformio.ini", "w", encoding="utf-8") as f:
    f.write(content)
