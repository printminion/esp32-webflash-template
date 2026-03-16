#!/usr/bin/env bash
# build_merged.sh — Build a merged firmware binary (bootloader + partitions + app)
# that matches exactly what the CI produces for the web installer.
#
# Usage:
#   ./scripts/build_merged.sh <environment> [--flash <port>]
#
# Examples:
#   ./scripts/build_merged.sh seeed_xiao_esp32c6
#   ./scripts/build_merged.sh seeed_xiao_esp32c6 --flash COM3
#   ./scripts/build_merged.sh generic_esp32 --flash /dev/ttyUSB0
#
# Boards are read from project.json at the repository root.

set -euo pipefail

# ── Locate project.json ───────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_JSON="${SCRIPT_DIR}/../project.json"

if [[ ! -f "$PROJECT_JSON" ]]; then
  echo "Error: project.json not found at ${PROJECT_JSON}"
  exit 1
fi

if ! command -v jq &>/dev/null; then
  echo "Error: jq is required. Install it: https://stedolan.github.io/jq/download/"
  exit 1
fi

# ── Argument parsing ──────────────────────────────────────────────────────────
ENV="${1:-}"
FLASH_PORT=""

if [[ -z "$ENV" ]]; then
  VALID=$(jq -r '[.boards[].id] | join(" | ")' "$PROJECT_JSON")
  echo "Usage: $0 <environment> [--flash <port>]"
  echo "Environments: ${VALID}"
  exit 1
fi

CHIP_NAME=$(jq -r --arg id "$ENV" '.boards[] | select(.id == $id) | .chip' "$PROJECT_JSON")
BL_OFFSET=$(jq -r --arg id "$ENV" '.boards[] | select(.id == $id) | .bootloaderOffset' "$PROJECT_JSON")

if [[ -z "$CHIP_NAME" || "$CHIP_NAME" == "null" ]]; then
  VALID=$(jq -r '[.boards[].id] | join(" | ")' "$PROJECT_JSON")
  echo "Unknown environment: $ENV"
  echo "Valid environments: ${VALID}"
  exit 1
fi

shift
while [[ $# -gt 0 ]]; do
  case "$1" in
    --flash)
      if [[ -z "${2:-}" ]]; then
        echo "Error: --flash requires a port argument (e.g. --flash COM3 or --flash /dev/ttyUSB0)"
        exit 1
      fi
      FLASH_PORT="$2"; shift 2 ;;
    *) echo "Unknown argument: $1"; exit 1 ;;
  esac
done
BUILD_DIR=".pio/build/${ENV}"
OUT_DIR="firmware"
VERSION="dev-local"
OUTPUT="${OUT_DIR}/firmware-${ENV}-${VERSION}.bin"

# ── Dependency check ──────────────────────────────────────────────────────────
# Locate pio — PlatformIO installs into a venv that may not be on PATH in Git Bash
PIO=""
for candidate in \
    pio \
    platformio \
    "${USERPROFILE:-}/.platformio/penv/Scripts/platformio" \
    "${USERPROFILE:-}/.platformio/penv/Scripts/platformio.exe" \
    "$HOME/.platformio/penv/Scripts/platformio" \
    "$HOME/.platformio/penv/Scripts/platformio.exe" \
    "$HOME/.platformio/penv/bin/platformio"; do
  if [[ -x "$candidate" ]] || command -v "$candidate" &>/dev/null 2>&1; then
    PIO="$candidate"
    break
  fi
done
if [[ -z "$PIO" ]]; then
  echo "Error: PlatformIO not found. Install it: pip install platformio"
  exit 1
fi

# Locate esptool — prefer running as a Python module to avoid missing-deps issues
# with the standalone script in PlatformIO's bundled package.
ESPTOOL_CMD=""

# Try PlatformIO's own Python first (has all deps for the bundled esptool)
for pio_python in \
    "${USERPROFILE:-}/.platformio/penv/Scripts/python.exe" \
    "$HOME/.platformio/penv/Scripts/python.exe" \
    "$HOME/.platformio/penv/bin/python"; do
  if [[ -f "$pio_python" ]] && "$pio_python" -m esptool version &>/dev/null 2>&1; then
    ESPTOOL_CMD="$pio_python -m esptool"
    break
  fi
done

# Fall back to system Python
if [[ -z "$ESPTOOL_CMD" ]]; then
  if python -m esptool version &>/dev/null 2>&1; then
    ESPTOOL_CMD="python -m esptool"
  else
    echo "esptool not found in PlatformIO or system Python — installing..."
    python -m pip install esptool
    ESPTOOL_CMD="python -m esptool"
  fi
fi

# ── Build ─────────────────────────────────────────────────────────────────────
# Inject FIRMWARE_VERSION so the compiled binary reports the same version string
# as the output filename (mirrors what CI does via sed on platformio.ini).
# Back up platformio.ini to a temp file and restore it on exit — using git checkout
# would silently discard any uncommitted local edits the developer may have.
PLATFORMIO_BACKUP=$(mktemp)
cp platformio.ini "$PLATFORMIO_BACKUP"
trap 'cp "$PLATFORMIO_BACKUP" platformio.ini; rm -f "$PLATFORMIO_BACKUP"' EXIT

echo ""
echo "==> Injecting firmware version: ${VERSION}"
# sed -i behaves differently on GNU (Linux/CI) vs BSD (macOS): GNU omits the
# backup extension, BSD requires an explicit empty string argument.
# Use a variable + || true so pipefail does not abort the script when
# `sed --version` exits non-zero on BSD.
_is_gnu_sed=0
sed --version 2>&1 | grep -q GNU && _is_gnu_sed=1 || true
if [[ "$_is_gnu_sed" -eq 1 ]]; then
  sed -i "s/-D FIRMWARE_VERSION='\"dev\"'/-D FIRMWARE_VERSION='\"${VERSION}\"'/" platformio.ini
else
  sed -i '' "s/-D FIRMWARE_VERSION='\"dev\"'/-D FIRMWARE_VERSION='\"${VERSION}\"'/" platformio.ini
fi

echo "==> Building ${ENV}..."
"$PIO" run -e "${ENV}"

# ── Merge ─────────────────────────────────────────────────────────────────────
mkdir -p "${OUT_DIR}"
echo ""
echo "==> Merging binary for ${CHIP_NAME} (bootloader @ ${BL_OFFSET})..."
$ESPTOOL_CMD --chip "${CHIP_NAME}" merge_bin \
  -o "${OUTPUT}" \
  "${BL_OFFSET}" "${BUILD_DIR}/bootloader.bin" \
  0x8000              "${BUILD_DIR}/partitions.bin" \
  0x10000             "${BUILD_DIR}/firmware.bin"

SIZE=$(du -h "${OUTPUT}" | cut -f1)
echo ""
echo "==> Merged binary: ${OUTPUT} (${SIZE})"

# ── Flash (optional) ──────────────────────────────────────────────────────────
if [[ -n "$FLASH_PORT" ]]; then
  echo ""
  echo "==> Flashing to ${FLASH_PORT}..."
  $ESPTOOL_CMD --chip "${CHIP_NAME}" --port "${FLASH_PORT}" --baud 921600 \
    write_flash 0x0 "${OUTPUT}"
  echo ""
  echo "==> Done. Monitor with: pio device monitor -p ${FLASH_PORT} -b 115200"
else
  echo ""
  echo "To flash manually:"
  echo "  $ESPTOOL_CMD --chip ${CHIP_NAME} --port <PORT> --baud 921600 write_flash 0x0 ${OUTPUT}"
fi
