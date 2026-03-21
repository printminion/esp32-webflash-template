#pragma once

// ============================================================
// board_config.h — Seeed XIAO ESP32-S3
// ============================================================

// Onboard LED (active LOW on XIAO S3)
#define LED_PIN           21
#define LED_ACTIVE_LOW    true

// UART
#define SERIAL_BAUD       115200

// Feature flags
#define FEATURE_WIFI_PROVISIONING
#define FEATURE_OTA
#define FEATURE_VERSION_CHECK

// ============================================================
// Peripheral pin map — fill in for your hardware
//
// Seeed XIAO ESP32-S3 GPIO aliases:
//   D0=1  D1=2  D2=3  D3=4  D4=5  D5=6
//   D6=7  D7=8  D8=9  D9=10
//
// SPI bus (shared by display, SD card, etc.)
// #define PIN_SPI_SCK     9   // D8
// #define PIN_SPI_MOSI   10   // D9
// #define PIN_SPI_MISO    8   // D7
//
// SPI device chip-selects (one per device)
// #define PIN_DISPLAY_CS  2   // D1
// #define PIN_SD_CS       3   // D2
//
// Display control lines
// #define PIN_DISPLAY_DC  4   // D3
// #define PIN_DISPLAY_RST -1  // -1 if tied to 3V3 (no reset needed)
// #define PIN_DISPLAY_BL  7   // D6 — backlight PWM
//
// I2C bus (sensors, touch controllers, etc.)
// #define PIN_I2C_SDA     5   // D4
// #define PIN_I2C_SCL     6   // D5
// #define PIN_I2C_INT     8   // D7 — interrupt line (optional)
// ============================================================
