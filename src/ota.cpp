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

// OTA endpoint credentials.
// In DEBUG_BUILD, "esp32"/"esp32" defaults are provided for convenience.
// In release builds, define OTA_USERNAME and OTA_PASSWORD via build_flags to
// enable the /update endpoint. Without explicit credentials the endpoint is
// disabled entirely, preventing an open update surface on the LAN.
//   -D OTA_USERNAME='"youruser"' -D OTA_PASSWORD='"yourpass"'
#ifdef DEBUG_BUILD
  #ifndef OTA_USERNAME
    #define OTA_USERNAME "esp32"
  #endif
  #ifndef OTA_PASSWORD
    #define OTA_PASSWORD "esp32"
  #endif
#endif

// Endpoint is active only when credentials are available.
#if defined(OTA_USERNAME) && defined(OTA_PASSWORD)
  #define OTA_ENDPOINT_ACTIVE 1
#else
  #define OTA_ENDPOINT_ACTIVE 0
#endif

void setupOTA() {
#if OTA_ENDPOINT_ACTIVE
  server.on("/", []() {
    server.send(200, "text/plain",
      "Firmware: " FIRMWARE_VERSION "\nBoard: " BOARD_NAME "\n\nVisit /update for OTA.");
  });
  ElegantOTA.setAuth(OTA_USERNAME, OTA_PASSWORD);
  ElegantOTA.begin(&server);
  server.begin();
  LOGF_STATUS("OTA: /update ready at http://%s/update", WiFi.localIP().toString().c_str());
#else
  LOG_STATUS("OTA: web server not started — set OTA_USERNAME and OTA_PASSWORD in build_flags to enable.");
#endif
}

void otaLoop() {
#if OTA_ENDPOINT_ACTIVE
  server.handleClient();
  ElegantOTA.loop();
#endif
}
