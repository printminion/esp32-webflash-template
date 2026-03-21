#!/usr/bin/env python3
"""
generate_platformio.py — Generate platformio.ini from project.json.

Run after editing project.json to update platformio.ini:
    python scripts/generate_platformio.py

Use --check to verify platformio.ini is in sync (useful as a CI lint step):
    python scripts/generate_platformio.py --check
"""

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).parent.parent
PROJECT_JSON = ROOT / "project.json"
PLATFORMIO_INI = ROOT / "platformio.ini"

# ── Shared / fixed sections ───────────────────────────────────────────────────
# These don't vary by board; only the [env:*] sections are generated.

HEADER = """\
; ============================================================
; esp32-webflash-template — PlatformIO configuration
; AUTO-GENERATED from project.json — do not edit by hand.
; Re-generate with: python scripts/generate_platformio.py
; ============================================================
"""

PLATFORMIO_SECTION = """\
[platformio]
default_envs =
"""

ENV_SHARED_BASE = """\
; ------------------------------------------------------------
; Shared settings inherited by all environments
; ------------------------------------------------------------
[env]
platform = espressif32
framework = arduino
monitor_speed = 115200
upload_speed = 921600

lib_deps =
    tzapu/WiFiManager @ ^2.0.17
    ayushsharma82/ElegantOTA @ ^3.1.0
    bblanchon/ArduinoJson @ ^7.3.1
"""

ENV_SHARED_PROJECT_LIBS_HEADER = """\
    ; --- project lib_deps (from project.json) ---
"""

ENV_SHARED_TAIL = """\
build_flags =
    -D FIRMWARE_VERSION='"dev"'
    -D PROJECT_NAME='"{project_name}"'
    -D ARDUINO_LOOP_STACK_SIZE=8192
"""


def render_env(board: dict, debug: bool, installer_base: str = "") -> str:
    bid           = board["id"]
    name          = board["name"]
    flag          = board["buildFlag"]
    pio_board     = board["platformioBoard"]
    platform      = board["platform"]
    partitions    = board.get("partitionsFile")
    extra_lib_deps = board.get("extra_lib_deps", [])

    suffix     = "-debug" if debug else ""
    board_name = f'{name} (debug)' if debug else name

    lines = [
        f"; {'─' * 60}",
        f"; {name}  —  {'debug' if debug else 'release'}",
        f"; {'─' * 60}",
        f"[env:{bid}{suffix}]",
        f"board = {pio_board}",
    ]

    # Override platform only when it differs from the shared [env] default
    if platform != "espressif32":
        lines.append(f"platform = {platform}")

    if partitions:
        lines.append(f"board_build.partitions = {partitions}")

    if extra_lib_deps:
        lines.append("lib_deps =")
        lines.append("    ${env.lib_deps}")
        lines.append("    ; --- board-specific extra_lib_deps (from project.json) ---")
        for dep in extra_lib_deps:
            lines.append(f"    {dep}")

    lines.append("build_flags =")
    lines.append("    ${env.build_flags}")
    lines.append(f"    -D {flag}")
    lines.append(f"    -D BOARD_NAME='\"{board_name}\"'")

    if debug:
        lines.append("    -D DEBUG_BUILD")
        lines.append("    -D CORE_DEBUG_LEVEL=4")
        # WIFI_AP_OPEN is intentionally omitted: wifi.cpp already allows an open AP in
        # debug builds (no NDEBUG) without any AP flag, and omitting it here lets users
        # set WIFI_AP_PASSWORD globally without hitting the "both defined" build error.
    else:
        lines.append("    -D NDEBUG")
        # Emit explanatory comments so users understand why the release env
        # doesn't compile without an explicit WiFi AP policy.
        lines.append("    ; wifi.cpp requires WIFI_AP_PASSWORD or WIFI_AP_OPEN=1 for release builds.")
        lines.append("    ; Add one of the following to build_flags:")
        lines.append("    ;   -D WIFI_AP_PASSWORD='\"yourpassword\"'  ; password-protected (recommended)")
        lines.append("    ;   -D WIFI_AP_OPEN=1                      ; open AP — not for production")
        # Per-board Auto-OTA opt-in hint with the pre-computed URL.
        if installer_base:
            url = f"{installer_base}/version/{bid}.json"
            lines.append("    ; Auto-OTA opt-in: uncomment the line below to enable version checking.")
            lines.append(f"    ;   -D VERSION_CHECK_URL='\"{url}\"'")

    lines.append(f"    -I boards/{bid}")

    if debug:
        lines.append("build_type = debug")

    return "\n".join(lines)


def generate(config: dict) -> str:
    project_name   = config["project"]["name"]
    installer_base = config.get("installer", {}).get("baseUrl", "").rstrip("/")
    boards         = config["boards"]
    project_lib_deps = config.get("lib_deps", [])

    parts = [HEADER.rstrip()]

    # [platformio] with default_envs list
    default_envs = "\n".join(f"    {b['id']}" for b in boards)
    parts.append(PLATFORMIO_SECTION.rstrip() + "\n" + default_envs)

    # Shared [env] — base deps + optional project-level lib_deps
    shared = ENV_SHARED_BASE
    if project_lib_deps:
        shared += ENV_SHARED_PROJECT_LIBS_HEADER
        for dep in project_lib_deps:
            shared += f"    {dep}\n"
    shared += "\n" + ENV_SHARED_TAIL.format(project_name=project_name)
    parts.append(shared.rstrip())

    # Per-board sections
    for board in boards:
        parts.append(render_env(board, debug=False, installer_base=installer_base))
        parts.append(render_env(board, debug=True))

    return "\n\n".join(parts) + "\n"


def main() -> None:
    check_mode = "--check" in sys.argv

    config  = json.loads(PROJECT_JSON.read_text(encoding="utf-8"))
    content = generate(config)

    if check_mode:
        if not PLATFORMIO_INI.exists():
            print("ERROR: platformio.ini does not exist. Run without --check to generate it.")
            sys.exit(1)
        current = PLATFORMIO_INI.read_text(encoding="utf-8")
        if current == content:
            print("OK: platformio.ini is in sync with project.json")
        else:
            print("ERROR: platformio.ini is out of sync with project.json.")
            print("Run: python scripts/generate_platformio.py")
            sys.exit(1)
    else:
        PLATFORMIO_INI.write_text(content, encoding="utf-8")
        print(f"Wrote {PLATFORMIO_INI.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
