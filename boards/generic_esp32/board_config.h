#pragma once

// ============================================================
// board_config.h — Generic ESP32 (e.g. ESP32-DevKitC)
// ============================================================

// Onboard LED
#define LED_PIN           2
#define LED_ACTIVE_LOW    false

// Button (built-in BOOT button — active LOW, INPUT_PULLUP; no external wiring needed)
// Change BUTTON_PIN to any GPIO if you wire an external button instead.
#define BUTTON_PIN        0
#define BUTTON_ACTIVE_LOW true

// UART
#define SERIAL_BAUD       115200

// Feature flags
#ifndef VARIANT_NO_WIFI
#define FEATURE_WIFI_PROVISIONING
#define FEATURE_OTA
#define FEATURE_VERSION_CHECK
#endif

// ============================================================
// Peripheral pin map — fill in for your hardware
//
// Generic ESP32 (DevKitC) common GPIO assignments:
//   VSPI: SCK=18  MISO=19  MOSI=23  CS=5
//   HSPI: SCK=14  MISO=12  MOSI=13  CS=15
//   I2C:  SDA=21  SCL=22
//
// SPI bus (VSPI — recommended for most peripherals)
// #define PIN_SPI_SCK    18
// #define PIN_SPI_MOSI   23
// #define PIN_SPI_MISO   19
//
// SPI device chip-selects (one per device)
// #define PIN_DISPLAY_CS  5
// #define PIN_SD_CS       15
//
// Display control lines
// #define PIN_DISPLAY_DC  2
// #define PIN_DISPLAY_RST -1  // -1 if tied to 3V3 (no reset needed)
// #define PIN_DISPLAY_BL  4   // backlight PWM
//
// I2C bus (sensors, touch controllers, etc.)
// #define PIN_I2C_SDA    21
// #define PIN_I2C_SCL    22
// #define PIN_I2C_INT    34   // interrupt line (input-only on DevKitC)
// ============================================================
