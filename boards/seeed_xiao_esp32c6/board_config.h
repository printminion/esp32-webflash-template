#pragma once

// ============================================================
// board_config.h — Seeed XIAO ESP32-C6
// ============================================================

// Onboard LED (active LOW on XIAO C6)
#define LED_PIN           15
#define LED_ACTIVE_LOW    true

// UART
#define SERIAL_BAUD       115200

// Feature flags
#define FEATURE_WIFI_PROVISIONING
#define FEATURE_OTA
#define FEATURE_VERSION_CHECK
