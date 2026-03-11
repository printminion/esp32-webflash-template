# esp32-webflash-template

A multi-board ESP32 firmware template with a browser-based web installer powered by [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## Supported Boards

| Board | Environment |
|---|---|
| Seeed XIAO ESP32-C3 | `seeed_xiao_esp32c3` |
| Seeed XIAO ESP32-S3 | `seeed_xiao_esp32s3` |
| Generic ESP32 (DevKitC) | `generic_esp32` |

Each board has a matching `-debug` environment with verbose serial logging enabled.

## Features

- **WiFi provisioning** — captive portal via [WiFiManager](https://github.com/tzapu/WiFiManager) on first boot
- **OTA updates** — browser-based firmware upload at `/update` via [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)
- **Board-specific config** — pin maps and feature flags isolated in `boards/<board>/board_config.h`
- **Debug / release builds** — `LOG()` macros compile out completely in release builds
- **Web installer** — flash directly from your browser at the [installer page](https://printminion.github.io/esp32-webflash-template/)

## Building

Install [PlatformIO](https://platformio.org/) then:

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
├── boards/               # Board-specific configs
│   ├── seeed_xiao_esp32c3/board_config.h
│   ├── seeed_xiao_esp32s3/board_config.h
│   └── generic_esp32/board_config.h
├── docs/                 # GitHub Pages web installer
│   ├── index.html
│   └── manifest.json
├── .github/workflows/    # CI/CD
│   ├── release.yml       # Build + publish on git tags
│   └── dev.yml           # Build on dev branch push
└── platformio.ini        # All board environments
```

## CI/CD

- **Release build** — push a tag like `v1.0.0` → GitHub Actions builds all environments, creates a GitHub Release with `.bin` assets, and deploys the web installer to GitHub Pages.
- **Dev build** — push to the `dev` branch → builds all environments and deploys to the `dev/` channel on GitHub Pages.

## Adding a New Board

1. Create `boards/<your_board>/board_config.h` with pin definitions and feature flags
2. Add `[env:your_board]` and `[env:your_board-debug]` sections to `platformio.ini`
3. Add the board `#ifdef` block in `src/main.cpp`
4. Add the board entry to `docs/manifest.json`

## License

MIT
