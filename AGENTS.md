# AGENTS.md

This file provides guidance to AI agents (Claude Code, Copilot, Cursor, etc.) when working with code in this repository.

## Git Commits

- Use [Conventional Commits](https://www.conventionalcommits.org/): `type(scope): description`
  - Types: `feat`, `fix`, `chore`, `docs`, `refactor`, `ci`, `test`
  - Example: `fix(ota): handle esp_https_ota API difference between ESP-IDF versions`
- Each commit must be atomic — one logical change per commit.
- Do **not** add `Co-Authored-By` trailers.

## Build Commands

```bash
# Build all environments (release envs require a WiFi AP flag — see wifi.cpp)
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run

# Build a single release environment
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32c3
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32s3
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32c6
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e generic_esp32

# Build debug variant (no WiFi AP flag required — open AP allowed in debug builds)
pio run -e seeed_xiao_esp32c3-debug

# Flash to connected device
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32c3 -t upload

# Monitor serial output
pio device monitor -b 115200
```

There are no automated tests — validation is done via CI builds on GitHub Actions.

## Architecture

### Board-specific configuration via include paths
Each PlatformIO environment adds `-I boards/<board>` to its build flags. This means `#include "board_config.h"` in any source file resolves to the correct board's header at compile time — no runtime conditionals needed. Adding a new board requires: a new entry in `project.json` (source of truth), a new `boards/<board>/board_config.h`, and regenerating `platformio.ini` via `python scripts/generate_platformio.py`. Both CI workflows build their matrix dynamically from `project.json` — no workflow file edits needed.

### Feature flags
`board_config.h` defines `FEATURE_WIFI_PROVISIONING`, `FEATURE_OTA`, and `FEATURE_VERSION_CHECK`. These gate entire subsystems in `main.cpp`. All currently-supported boards enable all three.

### Logging system
`include/logger.h` provides `LOG()`, `LOGF()`, `LOG_RAW()` macros. When `DEBUG_BUILD` is not defined they compile to nothing; when `DEBUG_BUILD` is defined they emit via Serial. Never use `Serial.print` directly.

### OTA channels (release vs dev)
- **Release channel**: triggered by `v*` tags → builds inject version from tag, firmware binaries attached to GitHub Release, `docs/manifest.json`, `docs/version.json`, and `docs/version/` updated on `main` branch
- **Dev channel**: triggered by push to `dev` → firmware deployed to `docs/dev/`, version string is `dev-<SHORT_SHA>`

The web installer at `docs/index.html` switches between channels dynamically. `docs/version/{board-id}.json` (one file per board) is fetched by the firmware's `version_check.cpp` at runtime for auto-update; `docs/version.json` is a legacy boards-object kept for backward compatibility.

### ESP32-C6 platform difference
`seeed_xiao_esp32c6` uses the [pioarduino fork](https://github.com/pioarduino/platform-espressif32) instead of the official `espressif32` platform because the official platform lacks Arduino framework support for C6. This also requires `uv` to be pre-installed in CI (`pip install uv`). All other boards use `platform = espressif32`.

### esp_https_ota API compatibility
`src/version_check.cpp` uses two `ESP_IDF_VERSION` guards:
- `>= ESP_IDF_VERSION_VAL(4, 1, 0)` — selects between the streaming `esp_https_ota_begin/perform/finish` API (4.1+) and the legacy single-shot `esp_https_ota()` API.
- `>= ESP_IDF_VERSION_VAL(5, 0, 0)` — attaches the ESP-IDF CA bundle (`esp_crt_bundle_attach`) for TLS certificate validation without a hard-coded PEM.

## CI/CD

### Release workflow (`release.yml`)
The `release` job checks out the `main` branch (not the tag) so it can commit the updated manifests back to `main` without rebase conflicts. Do **not** add `git pull --rebase` to the manifest commit step — it was removed intentionally.

### Dev workflow (`dev.yml`)
Deploys to `docs/dev/` on GitHub Pages. Uses `keep_files: true` so the release channel (`docs/manifest.json`) is not overwritten.
