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
# Environments: seeed_xiao_esp32c3 | seeed_xiao_esp32s3 | seeed_xiao_esp32c6 | generic_esp32

set -euo pipefail

# ── Chip / bootloader-offset map (mirrors CI matrix) ─────────────────────────
declare -A CHIP=(
  [seeed_xiao_esp32c3]="esp32c3"
  [seeed_xiao_esp32s3]="esp32s3"
  [seeed_xiao_esp32c6]="esp32c6"
  [generic_esp32]="esp32"
)
declare -A BOOTLOADER_OFFSET=(
  [seeed_xiao_esp32c3]="0x0"
  [seeed_xiao_esp32s3]="0x0"
  [seeed_xiao_esp32c6]="0x0"
  [generic_esp32]="0x1000"   # Xtensa ESP32 reserves 0x0–0xFFF for secure boot data
)

# ── Argument parsing ──────────────────────────────────────────────────────────
ENV="${1:-}"
FLASH_PORT=""

if [[ -z "$ENV" ]]; then
  echo "Usage: $0 <environment> [--flash <port>]"
  echo "Environments: ${!CHIP[*]}"
  exit 1
fi

if [[ -z "${CHIP[$ENV]+_}" ]]; then
  echo "Unknown environment: $ENV"
  echo "Valid environments: ${!CHIP[*]}"
  exit 1
fi

shift
while [[ $# -gt 0 ]]; do
  case "$1" in
    --flash) FLASH_PORT="${2:-}"; shift 2 ;;
    *) echo "Unknown argument: $1"; exit 1 ;;
  esac
done

CHIP_NAME="${CHIP[$ENV]}"
BL_OFFSET="${BOOTLOADER_OFFSET[$ENV]}"
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
    "$USERPROFILE/.platformio/penv/Scripts/platformio" \
    "$USERPROFILE/.platformio/penv/Scripts/platformio.exe" \
    "$HOME/.platformio/penv/Scripts/platformio" \
    "$HOME/.platformio/penv/Scripts/platformio.exe" \
    "$HOME/.platformio/penv/bin/platformio"; do
  if [[ -f "$candidate" ]] || command -v "$candidate" &>/dev/null 2>&1; then
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
    "$USERPROFILE/.platformio/penv/Scripts/python.exe" \
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
echo ""
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
