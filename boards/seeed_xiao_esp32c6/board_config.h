#pragma once

// ============================================================
// board_config.h — Seeed XIAO ESP32-C6
// ============================================================

// Onboard LED (active LOW on XIAO C6)
#define LED_PIN           15
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
// Seeed XIAO ESP32-C6 GPIO aliases:
//   D0=0  D1=1  D2=2  D3=21 D4=22 D5=23
//   D6=16 D7=17 D8=18 D9=19 D10=20
//
// SPI bus (hardware SPI2)
// #define PIN_SPI_SCK    18   // D8
// #define PIN_SPI_MOSI   20   // D10
// #define PIN_SPI_MISO   19   // D9
//
// SPI device chip-selects (one per device)
// #define PIN_DISPLAY_CS  1   // D1
// #define PIN_SD_CS       2   // D2
//
// Display control lines
// #define PIN_DISPLAY_DC 21   // D3
// #define PIN_DISPLAY_RST -1  // -1 if tied to 3V3 (no reset needed)
// #define PIN_DISPLAY_BL 16   // D6 — backlight PWM
//
// I2C bus (sensors, touch controllers, etc.)
// #define PIN_I2C_SDA    22   // D4
// #define PIN_I2C_SCL    23   // D5
// #define PIN_I2C_INT    17   // D7 — interrupt line (optional)
// ============================================================
