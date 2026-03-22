// ============================================================
// ota.cpp — Over-the-air updates via ElegantOTA
// Exposes /update endpoint on the device's web server
// ============================================================
#include "board_config.h"
#ifdef FEATURE_OTA

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
  // Guard against empty-string credentials (e.g. -D OTA_USERNAME='""'), which would
  // activate the endpoint with no effective authentication.
  static_assert(sizeof(OTA_USERNAME) > 1, "OTA_USERNAME must not be empty.");
  static_assert(sizeof(OTA_PASSWORD) > 1, "OTA_PASSWORD must not be empty.");
#else
  #define OTA_ENDPOINT_ACTIVE 0
#endif

// OTA activation window — server stops accepting connections after this many minutes.
// Limits the brute-force window on the plain-HTTP endpoint.
// Override via build_flags: -D OTA_WINDOW_MINUTES=30
// Set to 0 to disable the timeout (always-on, previous behaviour).
#ifndef OTA_WINDOW_MINUTES
  #define OTA_WINDOW_MINUTES 10
#endif

// Warn at compile time when HTTP OTA is enabled in a release build: credentials
// and firmware are sent unencrypted over the LAN.
#if OTA_ENDPOINT_ACTIVE && defined(NDEBUG)
  #pragma message("OTA: /update endpoint is active in a release build and serves over plain HTTP. " \
                  "Credentials and firmware are not encrypted on the LAN.")
#endif

#if OTA_ENDPOINT_ACTIVE
#include <WebServer.h>
#include <ElegantOTA.h>
static WebServer server(80);
static unsigned long otaStartMs = 0;
static bool otaActive = false;
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
  otaStartMs = millis();
  otaActive  = true;
  LOG_STATUS("OTA: WARNING — /update runs over plain HTTP; credentials and firmware are not encrypted on the LAN.");
  LOGF_STATUS("OTA: /update ready at http://%s/update", WiFi.localIP().toString().c_str());
#if OTA_WINDOW_MINUTES > 0
  LOGF_STATUS("OTA: endpoint will close after %d min — reboot to reopen.", OTA_WINDOW_MINUTES);
#endif
#else
  LOG_STATUS("OTA: web server not started — set OTA_USERNAME and OTA_PASSWORD in build_flags to enable.");
#endif
}

void otaLoop() {
#if OTA_ENDPOINT_ACTIVE
  if (!otaActive) return;

#if OTA_WINDOW_MINUTES > 0
  unsigned long elapsedMin = (millis() - otaStartMs) / 60000UL;
  if (elapsedMin >= OTA_WINDOW_MINUTES) {
    server.stop();
    otaActive = false;
    LOGF_STATUS("OTA: window closed after %lu min — /update is no longer available. Reboot to reopen.",
                elapsedMin);
    return;
  }
#endif

  server.handleClient();
  ElegantOTA.loop();
#endif
}

#endif // FEATURE_OTA
