// ============================================================
// ota.cpp — Over-the-air updates via ElegantOTA
// Exposes /update endpoint on the device's web server
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include "logger.h"

static WebServer server(80);

// OTA endpoint credentials must be set via build_flags in platformio.ini:
//   -D OTA_USERNAME='"youruser"' -D OTA_PASSWORD='"yourpass"'
// In debug builds a fallback is provided for convenience; release builds
// require explicit credentials to prevent accidental open /update endpoints.
#ifdef DEBUG_BUILD
  #ifndef OTA_USERNAME
    #define OTA_USERNAME "esp32"
  #endif
  #ifndef OTA_PASSWORD
    #define OTA_PASSWORD "esp32"
  #endif
#else
  #if !defined(OTA_USERNAME) || !defined(OTA_PASSWORD)
    #error "OTA_USERNAME and OTA_PASSWORD must be defined in build_flags for release builds."
  #endif
#endif

void setupOTA() {
  server.on("/", []() {
    server.send(200, "text/plain",
      "Firmware: " FIRMWARE_VERSION "\nBoard: " BOARD_NAME "\n\nVisit /update for OTA.");
  });

  ElegantOTA.setAuth(OTA_USERNAME, OTA_PASSWORD);
  ElegantOTA.begin(&server);
  server.begin();

  LOGF_STATUS("OTA: ready at http://%s/update", WiFi.localIP().toString().c_str());
}

void otaLoop() {
  server.handleClient();
  ElegantOTA.loop();
}
