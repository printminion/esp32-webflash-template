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
if ! command -v pio &>/dev/null; then
  echo "Error: 'pio' not found. Install PlatformIO: pip install platformio"
  exit 1
fi
if ! command -v esptool.py &>/dev/null; then
  echo "esptool.py not found — installing..."
  pip install esptool
fi

# ── Build ─────────────────────────────────────────────────────────────────────
echo ""
echo "==> Building ${ENV}..."
pio run -e "${ENV}"

# ── Merge ─────────────────────────────────────────────────────────────────────
mkdir -p "${OUT_DIR}"
echo ""
echo "==> Merging binary for ${CHIP_NAME} (bootloader @ ${BL_OFFSET})..."
esptool.py --chip "${CHIP_NAME}" merge_bin \
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
  esptool.py --chip "${CHIP_NAME}" --port "${FLASH_PORT}" --baud 921600 \
    write_flash 0x0 "${OUTPUT}"
  echo ""
  echo "==> Done. Monitor with: pio device monitor -p ${FLASH_PORT} -b 115200"
else
  echo ""
  echo "To flash manually:"
  echo "  esptool.py --chip ${CHIP_NAME} --port <PORT> --baud 921600 write_flash 0x0 ${OUTPUT}"
fi
