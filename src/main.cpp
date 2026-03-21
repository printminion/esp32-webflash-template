// ============================================================
// main.cpp — esp32-webflash-template
//
// Shared firmware entry point for all supported boards.
// Board-specific behaviour is driven by board_config.h and
// compile-time feature flags set in platformio.ini.
// ============================================================

#include <Arduino.h>

// Board config is resolved via the -I boards/<board> include path
// set per-environment in platformio.ini — no relative path needed.
#include "board_config.h"

#include "logger.h"

// ── Forward declarations ──────────────────────────────────
#ifdef FEATURE_WIFI_PROVISIONING
void setupWifi();
#endif
#ifdef FEATURE_OTA
void setupOTA();
void otaLoop();
#endif
#ifdef FEATURE_VERSION_CHECK
void checkAndApplyUpdate();
#endif
void blinkLed(int times, int delayMs = 200);

// ─────────────────────────────────────────────────────────
void setup() {
  LOG_BEGIN(SERIAL_BAUD);
  LOGF_STATUS("Project  : %s", PROJECT_NAME);
  LOGF_STATUS("Board    : %s", BOARD_NAME);
  LOGF_STATUS("Version  : %s", FIRMWARE_VERSION);
#ifdef FEATURE_WIFI_PROVISIONING
  LOG_STATUS("WiFi     : enabled");
#else
  LOG_STATUS("WiFi     : disabled");
#endif
#ifdef FEATURE_OTA
  LOG_STATUS("OTA      : enabled");
#else
  LOG_STATUS("OTA      : disabled");
#endif
#ifdef FEATURE_VERSION_CHECK
  LOG_STATUS("VerCheck : enabled");
#else
  LOG_STATUS("VerCheck : disabled");
#endif

  pinMode(LED_PIN, OUTPUT);
  blinkLed(3);  // startup indication

#ifdef FEATURE_WIFI_PROVISIONING
  setupWifi();
#endif

#ifdef FEATURE_VERSION_CHECK
  checkAndApplyUpdate();  // check for OTA update before starting services
#endif

#ifdef FEATURE_OTA
  setupOTA();
#endif

  LOG_STATUS("Setup complete");
}

void loop() {
#ifdef FEATURE_OTA
  // ElegantOTA / ArduinoOTA handler — must be called in loop
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
