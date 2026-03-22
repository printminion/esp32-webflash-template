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


def js(s: str) -> str:
    """Return s as a JS string literal, preserving Unicode (emoji etc.)."""
    return json.dumps(s, ensure_ascii=False)


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
        # Use json.dumps for correct JS string literal escaping (handles
        # backslashes, newlines, Unicode, etc. — not just double quotes).
        bid        = js(board["id"])
        name       = js(board["name"])
        icon       = js(board["icon"])
        meta       = js(board["meta"])
        chipFamily = js(board["chipFamily"])
        lines.append(
            f'  {{ id: {bid}, name: {name}, icon: {icon}, meta: {meta}, chipFamily: {chipFamily} }}{comma}'
        )

    lines.append("];")

    variants = config.get("firmwareVariants", [])
    if variants:
        lines.append("")
        lines.append("window.VARIANTS_CONFIG = [")
        for i, v in enumerate(variants):
            comma = "," if i < len(variants) - 1 else ""
            vid   = js(v["id"])
            label = js(v["label"])
            desc  = js(v.get("description", ""))
            lines.append(
                f'  {{ id: {vid}, label: {label}, description: {desc} }}{comma}'
            )
        lines.append("];")

    branding = config.get("branding", [])
    if branding:
        lines.append("")
        lines.append("window.BRANDING_CONFIG = [")
        for i, link in enumerate(branding):
            comma = "," if i < len(branding) - 1 else ""
            icon  = js(link["icon"])
            label = js(link["label"])
            url   = js(link["url"])
            lines.append(
                f'  {{ icon: {icon}, label: {label}, url: {url} }}{comma}'
            )
        lines.append("];")

    installer = config.get("installer")
    if installer:
        proj = config["project"]

        lines += [
            "",
            "window.PROJECT_CONFIG = {",
            f'  title:              {js(installer["title"])},',
            f'  h1:                 {js(installer["h1"])},',
            f'  subtitle:           {js(installer["subtitle"])},',
            f'  baseUrl:            {js(installer["baseUrl"])},',
            f'  githubUrl:          {js(installer["githubUrl"])},',
            f'  badgeText:          {js(installer.get("badgeText", "Web Installer"))},',
            f'  projectName:        {js(proj["name"])},',
            f'  projectDescription: {js(proj["description"])}',
            "};",
        ]

    OUT_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_FILE.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
