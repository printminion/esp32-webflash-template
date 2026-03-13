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
