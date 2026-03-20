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

## Setting up your fork

After forking, complete these steps before pushing your first release tag.

### 1. Customize `project.json`

Update these fields to match your fork:

- `project.name` and `installer.title` — your project name
- `installer.baseUrl` — `https://<your-username>.github.io/<your-repo>`
- `installer.githubUrl` — your fork URL

### 2. Enable GitHub Pages

Repo → **Settings → Pages → Source: GitHub Actions**

The release workflow deploys the web installer there automatically.

### 3. Configure the WiFi AP policy

Required before pushing a release tag. Choose one option in
**Settings → Secrets and variables → Actions**:

- **Secret** `WIFI_AP_PASSWORD` (recommended) — add a password of at least 8 characters
  to ship a password-protected captive portal.
- **Variable** `WIFI_AP_ALLOW_OPEN_RELEASE` = `1` — opt in to an open AP
  (testing / demo only, not for production).

> Dev branch builds always succeed — they default to open AP if no secret is set.
> Release builds fail immediately if neither option is configured.

### 4. Push a version tag to trigger your first release

```sh
git tag v1.0.0
git push origin v1.0.0
```

The release workflow builds all boards, creates a GitHub Release with firmware
binaries attached, and deploys the web installer to GitHub Pages.

## Runtime Configuration

After flashing, the serial monitor may show these informational messages. Both features are **opt-in** — they do nothing until you configure them.

### OTA web server (`OTA_USERNAME` / `OTA_PASSWORD`)

```
OTA: web server not started — set OTA_USERNAME and OTA_PASSWORD in build_flags to enable.
```

**What it is:** A browser-based firmware upload endpoint at `http://<device-ip>/update`, powered by [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA). Useful for updating firmware over WiFi without USB.

**Why it's off by default:** The endpoint requires credentials to activate. Shipping a device with a reachable `/update` endpoint and no password would allow anyone on the LAN to replace the firmware.

**How to enable:**

**Via GitHub Actions (recommended for forks):** Add `OTA_USERNAME` and `OTA_PASSWORD` as repository secrets in **Settings → Secrets and variables → Actions**. The build workflow injects them automatically — no `platformio.ini` edits needed.

**Manually (local builds):**

```ini
# platformio.ini (or PLATFORMIO_BUILD_FLAGS env var)
build_flags =
  -D OTA_USERNAME='"admin"'
  -D OTA_PASSWORD='"yourpassword"'
```

> ⚠️ OTA traffic is plain HTTP (unencrypted on the LAN). Use a strong password and enable only on trusted networks.

---

### Auto-update check (`VERSION_CHECK_URL`)

```
VersionCheck: VERSION_CHECK_URL not configured — skipping.
```

**What it is:** On boot, the device fetches a small JSON file from your GitHub Pages and compares the remote version to the running firmware. If a newer release exists it downloads and applies it automatically via `esp_https_ota`, then reboots.

**Why it's off by default:** The feature requires a URL pointing to your specific deployment. Running it without configuration would either silently fail or, worse, check a URL it shouldn't.

**How to enable:**

**Via GitHub Actions (automatic):** Once `installer.baseUrl` is set in `project.json`, the build workflow computes and injects the correct per-board URL automatically — no secrets or manual edits needed.

**For local builds**, the URL is auto-populated per board by `scripts/generate_platformio.py`. After customising `project.json`, run:

```bash
python scripts/generate_platformio.py
```

This writes the correct `VERSION_CHECK_URL` for each board into `platformio.ini`. The URL points to `docs/version/<board-id>.json`, which the release workflow populates with the latest version and firmware download link.

To set it manually:

```ini
build_flags =
  -D VERSION_CHECK_URL='"https://<your-username>.github.io/<your-repo>/version/<board-id>.json"'
```

> Note: Auto-update only applies when running a release build (`vX.Y.Z` version tag). Dev builds (`dev-<sha>`) skip the update check intentionally.

---

## Building

Install [PlatformIO](https://platformio.org/) then:

> **WiFi provisioning AP policy** — release environments (`-D NDEBUG`) require an
> explicit AP security setting. `platformio.ini` is auto-generated and must not be
> edited by hand; pass the flag via `PLATFORMIO_BUILD_FLAGS` instead:
> ```bash
> # Password-protected captive portal (recommended)
> PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_PASSWORD='\"yourpassword\"'" pio run -e <env>
>
> # Open AP — development/testing only
> PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e <env>
> ```
> Debug environments (no `-D NDEBUG`) allow an open AP by default — no flag required.

```bash
# Build all environments (requires WiFi AP flag — see note above)
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run

# Build a specific release board
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32c3

# Build debug variant (no flag required — open AP allowed by default)
pio run -e seeed_xiao_esp32c3-debug

# Upload to connected board
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32c3 -t upload

# Monitor serial output
pio device monitor
```

## Project Structure

```
esp32-webflash-template/
├── src/                  # Shared firmware source
│   ├── main.cpp          # Entry point
│   ├── wifi.cpp          # WiFi provisioning
│   ├── ota.cpp           # OTA updates (ElegantOTA web endpoint)
│   └── version_check.cpp # Auto-update via esp_https_ota
├── include/
│   └── logger.h          # Debug logging macros
├── boards/               # Board-specific configs (one dir per board in project.json)
│   ├── seeed_xiao_esp32c3/board_config.h
│   ├── seeed_xiao_esp32s3/board_config.h
│   ├── seeed_xiao_esp32c6/board_config.h
│   └── generic_esp32/board_config.h
├── docs/                 # GitHub Pages web installer
│   ├── index.html
│   ├── boards_config.js  # auto-generated board list (loaded by index.html)
│   ├── manifest.json     # release channel firmware manifest
│   ├── version.json      # legacy per-firmware download index
│   └── version/          # per-board version files fetched by firmware OTA
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

## Migrating an Existing Project

To port an existing Arduino or PlatformIO project into this template:

1. **Add your libraries** — add entries to `lib_deps` in [`project.json`](project.json). Do **not** edit `platformio.ini` directly (it is auto-generated):
   ```json
   "lib_deps": [
     "bitbank2/AnimatedGIF @ ^2.1.1",
     "https://github.com/Seeed-Studio/Seeed_Arduino_RoundDisplay"
   ]
   ```
   For libraries needed by only one board, use `extra_lib_deps` on that board entry instead.

2. **Regenerate `platformio.ini`** after any `project.json` change:
   ```bash
   python scripts/generate_platformio.py
   ```

3. **Add your application code** to `src/main.cpp` in the `// TODO: add your application logic here` section in `loop()`. If you need additional source files, add them to `src/`.

4. **Add custom pin/hardware config** to `boards/<your-board>/board_config.h`. This file is included at compile time for the matching board only — add display pins, sensor addresses, or any board-specific `#define`s here.

5. **WiFi on first boot** — if your app doesn't need WiFi, disable the feature flags in `board_config.h` (`FEATURE_WIFI_PROVISIONING`, `FEATURE_OTA`, `FEATURE_VERSION_CHECK`). If you keep WiFi, the first boot will show a captive portal before your app runs.

## Connect & Support

- 🐦 **Follow on X/Twitter:** [@printminion](https://x.com/printminion)
- 🖨️ **3D Enclosures:** [Custom 3D printed enclosures for DIY modules on Cults3D](https://cults3d.com/@printminion)
- ☕ **Buy Me a Coffee:** [buymeacoffee.com/printminion](https://buymeacoffee.com/printminion)

## License

MIT
