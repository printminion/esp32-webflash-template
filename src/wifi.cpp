// ============================================================
// wifi.cpp — WiFi provisioning via captive portal (WiFiManager)
// ============================================================

#include <Arduino.h>
#include <WiFiManager.h>
#include "logger.h"

static WiFiManager wm;

void setupWifi() {
  LOG("Starting WiFi provisioning...");

  // Uncomment to reset saved credentials during development:
  // wm.resetSettings();

  wm.setConfigPortalTimeout(180);  // portal closes after 3 min if unused
  wm.setConnectTimeout(30);

  // AP name shown to user during provisioning — includes firmware version for flash validation
  String apName = String("ESP32-") + String(FIRMWARE_VERSION)
                + String("-") + String((uint32_t)ESP.getEfuseMac(), HEX);

  bool connected = wm.autoConnect(apName.c_str());

  if (!connected) {
    LOGF("WiFi: failed to connect — restarting in 5s");
    delay(5000);
    ESP.restart();
  }

  LOGF("WiFi: connected, IP=%s", WiFi.localIP().toString().c_str());
}
