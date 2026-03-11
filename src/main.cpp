// ============================================================
// main.cpp — esp32-webflash-template
//
// Shared firmware entry point for all supported boards.
// Board-specific behaviour is driven by board_config.h and
// compile-time feature flags set in platformio.ini.
// ============================================================

#include <Arduino.h>

// Resolve board_config.h from the active board directory.
// PlatformIO adds boards/<board>/ to the include path via
// the board_build.include_dirs option (see platformio.ini).
#if defined(BOARD_SEEED_XIAO_ESP32C3)
  #include "../boards/seeed_xiao_esp32c3/board_config.h"
#elif defined(BOARD_SEEED_XIAO_ESP32S3)
  #include "../boards/seeed_xiao_esp32s3/board_config.h"
#elif defined(BOARD_GENERIC_ESP32)
  #include "../boards/generic_esp32/board_config.h"
#else
  #error "Unknown board — please add a board_config.h entry in main.cpp"
#endif

#include "logger.h"

// ── Forward declarations ──────────────────────────────────
void setupWifi();
void setupOTA();
void blinkLed(int times, int delayMs = 200);

// ─────────────────────────────────────────────────────────
void setup() {
  LOG_BEGIN(SERIAL_BAUD);
  LOGF("Board    : %s", BOARD_NAME);
  LOGF("Version  : %s", FIRMWARE_VERSION);

  pinMode(LED_PIN, OUTPUT);
  blinkLed(3);  // startup indication

#ifdef FEATURE_WIFI_PROVISIONING
  setupWifi();
#endif

#ifdef FEATURE_OTA
  setupOTA();
#endif

  LOG("Setup complete");
}

void loop() {
#ifdef FEATURE_OTA
  // ElegantOTA / ArduinoOTA handler — must be called in loop
  extern void otaLoop();
  otaLoop();
#endif

  // TODO: add your application logic here
  delay(10);
}

// ─────────────────────────────────────────────────────────
void blinkLed(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LED_ACTIVE_LOW ? LOW : HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);
    delay(delayMs);
  }
}
