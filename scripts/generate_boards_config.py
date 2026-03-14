#!/usr/bin/env python3
"""
generate_boards_config.py — Generate docs/boards_config.js from project.json.

The generated file is loaded by docs/index.html as a synchronous <script> tag,
making the board list available to the web installer without an async fetch.

Run after editing project.json:
    python scripts/generate_boards_config.py
"""

import json
import pathlib

ROOT = pathlib.Path(__file__).parent.parent
PROJECT_JSON = ROOT / "project.json"
OUT_FILE = ROOT / "docs" / "boards_config.js"


def main() -> None:
    config = json.loads(PROJECT_JSON.read_text(encoding="utf-8"))
    boards = config["boards"]

    lines = [
        "// AUTO-GENERATED — do not edit by hand.",
        "// Re-generate with: python scripts/generate_boards_config.py",
        "window.BOARDS_CONFIG = [",
    ]

    for i, board in enumerate(boards):
        comma = "," if i < len(boards) - 1 else ""
        # Escape any single quotes in string values
        bid   = board["id"].replace('"', '\\"')
        name  = board["name"].replace('"', '\\"')
        icon  = board["icon"].replace('"', '\\"')
        meta  = board["meta"].replace('"', '\\"')
        lines.append(
            f'  {{ id: "{bid}", name: "{name}", icon: "{icon}", meta: "{meta}" }}{comma}'
        )

    lines.append("];")

    OUT_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_FILE.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
