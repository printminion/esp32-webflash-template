// ============================================================
// ota.cpp — Over-the-air updates via ElegantOTA
// Exposes /update endpoint on the device's web server
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include "logger.h"

// OTA endpoint credentials — must be defined explicitly via build_flags:
//   -D OTA_USERNAME='"youruser"' -D OTA_PASSWORD='"yourpass"'
// No defaults are provided in any build type so OTA is always opt-in.
// The endpoint is disabled (port 80 never opened) until both are set.

// Endpoint is active only when credentials are available.
#if defined(OTA_USERNAME) && defined(OTA_PASSWORD)
  #define OTA_ENDPOINT_ACTIVE 1
#else
  #define OTA_ENDPOINT_ACTIVE 0
#endif

#if OTA_ENDPOINT_ACTIVE
#include <WebServer.h>
#include <ElegantOTA.h>
static WebServer server(80);
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
  LOG_STATUS("OTA: WARNING — /update runs over plain HTTP; credentials and firmware are not encrypted on the LAN.");
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
