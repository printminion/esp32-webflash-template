#pragma once

// ============================================================
// board_config.h — Generic ESP32 (e.g. ESP32-DevKitC)
// ============================================================

// Onboard LED
#define LED_PIN           2
#define LED_ACTIVE_LOW    false

// UART
#define SERIAL_BAUD       115200

// Feature flags
#define FEATURE_WIFI_PROVISIONING
#define FEATURE_OTA
#define FEATURE_VERSION_CHECK
