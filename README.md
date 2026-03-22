# ESP32 Webflash (Template)

A multi-board ESP32 firmware template with a browser-based web installer powered by [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## Supported Boards

> The canonical board list lives in [`project.json`](project.json). The table below is derived from it.

| Board               | Environment            |
| ------------------- | ---------------------- |
| Seeed XIAO ESP32-C3 | `seeed_xiao_esp32c3`   |
| Seeed XIAO ESP32-S3 | `seeed_xiao_esp32s3`   |
| Seeed XIAO ESP32-C6 | `seeed_xiao_esp32c6`   |
| Generic ESP32       | `generic_esp32`        |

Each board has a matching `-debug` environment with verbose serial logging enabled.

## Features

- **WiFi provisioning** — captive portal via [WiFiManager](https://github.com/tzapu/WiFiManager) on first boot
- **OTA updates** — browser-based firmware upload at `/update` via [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)
- **Board-specific config** — pin maps and feature flags isolated in `boards/<board>/board_config.h`
- **Debug / release builds** — `LOG()` macros compile out completely in release builds
- **Web installer** — flash directly from your browser at the [installer page](https://printminion.github.io/esp32-webflash-template/)
- **Firmware variants** — ship a no-WiFi build (smaller, no provisioning/OTA) alongside the full WiFi build; selectable in the web installer
- **Hardware BOM** — list project components with shop links and thumbnails in the web installer's "Required Hardware" card
- **Web installer project info** — description, YouTube demo, and HowTo links driven by `installer.*` fields in `project.json`

## Setting up your fork

After forking, complete these steps before pushing your first release tag.

### 1. Customize `project.json`

Update these fields to match your fork:

- `project.name` and `installer.title` — your project name
- `installer.baseUrl` — `https://<your-username>.github.io/<your-repo>`
- `installer.githubUrl` — your fork URL
- `installer.description` — one-sentence description shown under the page title (optional)
- `installer.youtubeUrl` / `installer.howToUrl` — optional YouTube demo and HowTo guide links
- Per-board: `sku`, `url`, `image` — optional SKU, shop link, and thumbnail shown in "Required Hardware"
- `components[]` — bill-of-materials: additional hardware components shown in "Required Hardware" alongside the board

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

```text
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

```text
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
>
> ```bash
> # Password-protected captive portal (recommended)
> PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_PASSWORD='\"yourpassword\"'" pio run -e <env>
>
> # Open AP — development/testing only
> PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e <env>
> ```
>
> Debug environments (no `-D NDEBUG`) allow an open AP by default — no flag required.

```bash
# Build all environments (requires WiFi AP flag — see note above)
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run

# Build the default variant for a board (no-wifi, no env suffix)
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32c3

# Build the WiFi variant (non-default — -wifi suffix)
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32c3-wifi

# Build debug variant (no flag required — open AP allowed by default)
pio run -e seeed_xiao_esp32c3-debug

# Upload to connected board
PLATFORMIO_BUILD_FLAGS="-D WIFI_AP_OPEN=1" pio run -e seeed_xiao_esp32c3 -t upload

# Monitor serial output
pio device monitor
```

## Firmware Variants

`project.json` → `firmwareVariants[]` defines the firmware builds the project ships. Each variant maps to a separate PlatformIO environment, CI build, and web installer button.

### How it works

- **First entry is the default** — no suffix on env names or binary filenames (e.g. `seeed_xiao_esp32c3`, `firmware-seeed_xiao_esp32c3-v1.0.0.bin`)
- **Subsequent entries** get a `-{id}` suffix (e.g. `seeed_xiao_esp32c3-wifi`, `firmware-seeed_xiao_esp32c3-wifi-v1.0.0.bin`)
- The web installer shows a **"Select firmware"** toggle when more than one variant is configured

### Built-in variants

| id | label | Effect |
| --- | --- | --- |
| `nowifi` | No WiFi | Adds `-D VARIANT_NO_WIFI`; disables WiFi provisioning, OTA, and auto-update |
| `wifi` | WiFi | No extra flags; full WiFi stack enabled |

`VARIANT_NO_WIFI` causes `board_config.h` to omit `FEATURE_WIFI_PROVISIONING`, `FEATURE_OTA`, and `FEATURE_VERSION_CHECK`, producing a minimal build with no network dependencies.

### Customising variants

Edit `firmwareVariants[]` in `project.json`, then regenerate derived files:

```bash
python scripts/generate_platformio.py    # updates platformio.ini
python scripts/generate_boards_config.py # updates docs/boards_config.js (web installer)
```

You can add, remove, or reorder variants freely. Whichever entry is **first** becomes the default — it gets no env suffix and appears as the initial selection in the web installer.

---

## Project Structure

```text
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
│   ├── boards_config.js  # auto-generated from project.json (loaded by index.html)
│   ├── assets/boards/    # locally cached board thumbnails
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

## Custom Partition Tables

The default PlatformIO partition table may be too small for firmware that includes large
libraries (display stacks, graphics engines, etc.) alongside WiFi. Use a custom CSV to
control OTA slot sizes and add a SPIFFS partition.

### How partition files are wired

`partitionsFile` in a board entry in `project.json` drives `board_build.partitions` in
the generated `platformio.ini`:

| `project.json` `partitionsFile`  | Effect in `platformio.ini`                             |
| -------------------------------- | ------------------------------------------------------ |
| `null`                           | No override — platform default                         |
| `"min_spiffs.csv"`               | `board_build.partitions = min_spiffs.csv`              |
| `"partitions_esp32s3_8mb.csv"`   | `board_build.partitions = partitions_esp32s3_8mb.csv`  |

The CSV file must live at the PlatformIO project root (same directory as `platformio.ini`).

### Adding a custom partition table for a board

1. Create a CSV file at the project root using the ESP-IDF partition table format. Verify
   that offsets are contiguous and the total does not exceed your board's flash size:

   ```csv
   # Name,   Type, SubType,  Offset,    Size,     Flags
   nvs,      data, nvs,      0x9000,    0x5000,
   otadata,  data, ota,      0xe000,    0x2000,
   app0,     app,  ota_0,    0x10000,   0x280000,
   app1,     app,  ota_1,    0x290000,  0x280000,
   spiffs,   data, spiffs,   0x510000,  0x2E0000,
   coredump, data, coredump, 0x7F0000,  0x10000,
   ```

2. Set `"partitionsFile": "your-table.csv"` on the board entry in `project.json`.

3. Regenerate `platformio.ini`:

   ```bash
   python scripts/generate_platformio.py
   ```

4. Commit the CSV and the updated `project.json` together.

> **Warning:** Changing the partition table on a device that already has firmware installed
> requires a full re-flash via USB. OTA cannot resize or move partitions.

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

### 0. Convert your Arduino sketch to a PlatformIO source file

> Skip this step if you are already using PlatformIO.

Arduino IDE `.ino` files are not valid C++ — the IDE silently adds `#include <Arduino.h>`
and forward-declares all functions. PlatformIO does not; you must do this manually:

a. Copy your `.ino` file into `src/` and rename it `main.cpp`. If you have multiple
   `.ino` tabs, copy each one into `src/` as a `.cpp` file.

b. Add `#include <Arduino.h>` as the very first line of `src/main.cpp` (and any other
   `.cpp` files that use Arduino types like `String`, `Serial`, etc.).

c. Add forward declarations (or move definitions above their first caller) for any
   functions used before they are defined. The Arduino IDE generated these silently;
   PlatformIO will emit `use before declaration` errors if they are missing.

d. Replace any submodule-relative includes with bare library includes:

   ```cpp
   // before
   #include "../libs/MyLib/MyLib.h"
   // after
   #include <MyLib.h>
   ```

e. Remove the git submodule directories (`lib/`, `libraries/`, or wherever they lived):

   ```bash
   git submodule deinit --all
   git rm -r <submodule-path>
   ```

f. Delete any `libraries.properties` or `library.json` files that belonged to the
   removed submodules.

Move your application logic into the `// TODO: add your application logic here` block in
`loop()`, or replace `src/main.cpp` entirely if your project has a different structure.

---

### 1. Add your libraries

Add entries to `lib_deps` in [`project.json`](project.json). Do **not** edit
`platformio.ini` directly (it is auto-generated and will be overwritten):

```json
"lib_deps": [
  "author/LibraryName @ ^1.2.3",
  "https://github.com/org/repo"
]
```

For libraries needed by only one board, use `extra_lib_deps` on that board's entry
instead of the top-level array:

```json
{
  "id": "seeed_xiao_esp32s3",
  "extra_lib_deps": [
    "author/BoardSpecificLib @ ^1.0.0"
  ]
}
```

After every `project.json` change, regenerate `platformio.ini`:

```bash
python scripts/generate_platformio.py
```

#### Display libraries — `User_Setup.h` pattern (TFT_eSPI)

TFT_eSPI reads pin numbers from a `User_Setup.h` file at compile time, not from runtime
arguments. The correct approach in PlatformIO is to place a `User_Setup.h` alongside the
board's `board_config.h`:

```text
boards/seeed_xiao_esp32s3/User_Setup.h   ← board-specific display config
```

Because `boards/seeed_xiao_esp32s3/` is already on the include path (via
`-I boards/seeed_xiao_esp32s3` in `build_flags`), TFT_eSPI will pick up this file
automatically and shadow the library's own default — no extra build flags needed.

Minimal `User_Setup.h` skeleton:

```cpp
#define USER_SETUP_LOADED

// Driver (uncomment the one matching your display)
// #define GC9A01_DRIVER
// #define ILI9341_DRIVER
// #define ST7789_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// SPI pin assignments — match your board_config.h values
#define TFT_CS    2
#define TFT_DC    4
#define TFT_RST  -1   // -1 if the RST pin is tied to 3.3 V
#define TFT_BL    7   // backlight — omit if not used

#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define SPI_FREQUENCY  40000000
```

---

### 2. Regenerate `platformio.ini`

```bash
python scripts/generate_platformio.py
```

Run this after every `project.json` change. The file is auto-generated and must not be
edited by hand.

---

### 3. Add your application code

Add code to `src/main.cpp` in the `// TODO: add your application logic here` section in
`loop()`. Additional source files go in `src/`.

---

### 4. Add custom pin / hardware config

Edit `boards/<your-board>/board_config.h`. This file is included at compile time for the
matching board only — add display pins, sensor addresses, or any board-specific
`#define`s here. Each `board_config.h` has a commented peripheral pin map section as a
starting point.

---

### 5. Disable unused features

If your project doesn't need WiFi, use `firmwareVariants` in `project.json` to ship a no-WiFi build (see [Firmware Variants](#firmware-variants)). The preferred approach is to configure a variant with `requiresWifi: false` and `"buildFlags": ["-D VARIANT_NO_WIFI"]` — this disables provisioning, OTA, and auto-update for that variant while keeping the WiFi build available.

For a quick local override without modifying `project.json`, you can comment out flags directly in `board_config.h`:

```cpp
// #define FEATURE_WIFI_PROVISIONING
// #define FEATURE_OTA
// #define FEATURE_VERSION_CHECK
```

If you keep WiFi, the first boot shows a captive portal before your app runs.

## Connect & Support

- 🐦 **Follow on X/Twitter:** [@printminion](https://x.com/printminion)
- 🖨️ **3D Enclosures:** [Custom 3D printed enclosures for DIY modules on Cults3D](https://cults3d.com/@printminion)
- ☕ **Buy Me a Coffee:** [buymeacoffee.com/printminion](https://buymeacoffee.com/printminion)

## License

MIT
