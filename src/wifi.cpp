// ============================================================
// wifi.cpp — WiFi provisioning via captive portal (WiFiManager)
// ============================================================

#include <Arduino.h>
#include <WiFiManager.h>
#include "logger.h"

static WiFiManager wm;

void setupWifi() {
  // Uncomment to reset saved credentials during development:
  // wm.resetSettings();

  wm.setConfigPortalTimeout(180);  // portal closes after 3 min if unused
  wm.setConnectTimeout(30);

  // AP name shown to user during provisioning — includes firmware version for flash validation
  String apName = String("ESP32-") + String(FIRMWARE_VERSION)
                + String("-") + String((uint32_t)ESP.getEfuseMac(), HEX);

  LOG_STATUS("-- WiFi Setup ------------------------------------------");
  LOGF_STATUS("Connect to WiFi AP : %s", apName.c_str());
  LOG_STATUS("Then open          : http://192.168.4.1");
  LOGF_STATUS("Portal closes in   : %d min", 3);
  LOG_STATUS("--------------------------------------------------------");

  bool connected = wm.autoConnect(apName.c_str());

  if (!connected) {
    LOG_STATUS("WiFi: failed to connect — restarting in 5s");
    delay(5000);
    ESP.restart();
  }

  LOGF_STATUS("WiFi: connected, IP=%s", WiFi.localIP().toString().c_str());
}
