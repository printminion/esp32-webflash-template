#pragma once

// ============================================================
// board_config.h — Seeed XIAO ESP32-C3
// ============================================================

// Onboard LED (active LOW on XIAO C3)
#define LED_PIN           10
#define LED_ACTIVE_LOW    true

// Button (built-in BOOT button — active LOW, INPUT_PULLUP; no external wiring needed)
// Change BUTTON_PIN to any GPIO if you wire an external button instead.
#define BUTTON_PIN        9
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
// Seeed XIAO ESP32-C3 GPIO aliases:
//   D0=2  D1=3  D2=4  D3=5  D4=6  D5=7
//   D6=21 D7=20 D8=8  D9=9  D10=10
//
// SPI bus (hardware SPI2)
// #define PIN_SPI_SCK     8   // D8
// #define PIN_SPI_MOSI   10   // D10
// #define PIN_SPI_MISO    9   // D9
//
// SPI device chip-selects (one per device)
// #define PIN_DISPLAY_CS  3   // D1
// #define PIN_SD_CS       4   // D2
//
// Display control lines
// #define PIN_DISPLAY_DC  5   // D3
// #define PIN_DISPLAY_RST -1  // -1 if tied to 3V3 (no reset needed)
// #define PIN_DISPLAY_BL 21   // D6 — backlight PWM
//
// I2C bus (sensors, touch controllers, etc.)
// #define PIN_I2C_SDA     6   // D4
// #define PIN_I2C_SCL     7   // D5
// #define PIN_I2C_INT    20   // D7 — interrupt line (optional)
// ============================================================
