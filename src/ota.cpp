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

// OTA endpoint credentials. Override via build_flags in platformio.ini:
//   -D OTA_USERNAME='"youruser"' -D OTA_PASSWORD='"yourpass"'
// Defaults ("esp32"/"esp32") are provided so the template builds out of the box.
// Change these in project.json (and re-run generate_platformio.py) before deploying.
#ifndef OTA_USERNAME
  #define OTA_USERNAME "esp32"
#endif
#ifndef OTA_PASSWORD
  #define OTA_PASSWORD "esp32"
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
