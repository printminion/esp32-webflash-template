# ESP32 Webflash (Template)

A multi-board ESP32 firmware template with a browser-based web installer powered by [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## Supported Boards

> The canonical board list lives in [`project.json`](project.json). The table below is derived from it.

| Board | Environment |
|---|---|
| Seeed XIAO ESP32-C3 | `seeed_xiao_esp32c3` |
| Seeed XIAO ESP32-S3 | `seeed_xiao_esp32s3` |
| Seeed XIAO ESP32-C6 | `seeed_xiao_esp32c6` |
| Generic ESP32 | `generic_esp32` |

Each board has a matching `-debug` environment with verbose serial logging enabled.

## Features

- **WiFi provisioning** — captive portal via [WiFiManager](https://github.com/tzapu/WiFiManager) on first boot
- **OTA updates** — browser-based firmware upload at `/update` via [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)
- **Board-specific config** — pin maps and feature flags isolated in `boards/<board>/board_config.h`
- **Debug / release builds** — `LOG()` macros compile out completely in release builds
- **Web installer** — flash directly from your browser at the [installer page](https://printminion.github.io/esp32-webflash-template/)

## Building

Install [PlatformIO](https://platformio.org/) then:

> **WiFi provisioning AP policy** — release environments (`-D NDEBUG`) require an
> explicit AP security setting in `build_flags`. Add one of these to the target env
> in `platformio.ini` before building:
> ```ini
> -D WIFI_AP_PASSWORD='"yourpassword"'   ; password-protected captive portal (recommended)
> -D WIFI_AP_OPEN=1                      ; open AP — development/testing only
> ```
> Debug environments (`-debug` suffix) default to `WIFI_AP_OPEN=1`.

```bash
# Build all release environments
pio run

# Build a specific board
pio run -e seeed_xiao_esp32c3

# Build debug variant
pio run -e seeed_xiao_esp32c3-debug

# Upload to connected board
pio run -e seeed_xiao_esp32c3 -t upload

# Monitor serial output
pio device monitor
```

## Project Structure

```
esp32-webflash-template/
├── src/                  # Shared firmware source
│   ├── main.cpp          # Entry point
│   ├── wifi.cpp          # WiFi provisioning
│   └── ota.cpp           # OTA updates
├── include/
│   └── logger.h          # Debug logging macros
├── boards/               # Board-specific configs (one dir per board in project.json)
│   ├── seeed_xiao_esp32c3/board_config.h
│   ├── seeed_xiao_esp32s3/board_config.h
│   ├── seeed_xiao_esp32c6/board_config.h
│   └── generic_esp32/board_config.h
├── docs/                 # GitHub Pages web installer
│   ├── index.html
│   └── manifest.json
├── .github/workflows/    # CI/CD
│   ├── release.yml       # Build + publish on git tags
│   └── dev.yml           # Build on dev branch push
├── project.json          # Central config: project name + all board definitions
├── platformio.ini        # Auto-generated from project.json
└── schemas/
    └── project.schema.json  # JSON Schema for project.json validation
```

## CI/CD

- **Release build** — push a tag like `v1.0.0` → GitHub Actions builds all environments, creates a GitHub Release with `.bin` assets, and deploys the web installer to GitHub Pages.
- **Dev build** — push to the `dev` branch → builds all environments and deploys to the `dev/` channel on GitHub Pages.

## Adding a New Board

1. Add a new entry to the `boards` array in [`project.json`](project.json) — this is the single source of truth
2. Create `boards/<your_board>/board_config.h` with pin definitions and feature flags
3. Regenerate derived files from the new config:
   ```bash
   python scripts/generate_platformio.py   # updates platformio.ini
   python scripts/generate_boards_config.py  # updates docs/boards_config.js
   ```
4. Commit all changed files together

CI/CD picks up the new board automatically via the dynamic matrix in the workflows.

## Connect & Support

- 🐦 **Follow on X/Twitter:** [@printminion](https://x.com/printminion)
- 🖨️ **3D Enclosures:** [Custom 3D printed enclosures for DIY modules on Cults3D](https://cults3d.com/@printminion)
- ☕ **Buy Me a Coffee:** [buymeacoffee.com/printminion](https://buymeacoffee.com/printminion)

## License

MIT
