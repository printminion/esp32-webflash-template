// ============================================================
// wifi.cpp — WiFi provisioning via captive portal (WiFiManager)
// ============================================================
#include "board_config.h"
#ifdef FEATURE_WIFI_PROVISIONING

#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <esp_efuse.h>
#include "logger.h"

// Exactly one of WIFI_AP_PASSWORD or WIFI_AP_OPEN must be defined.
// WIFI_AP_PASSWORD sets a password for the provisioning AP (min 8 chars).
// WIFI_AP_OPEN=1 explicitly opts into an open (password-less) AP — not for production.
// Leaving both undefined is an error so developers must make an intentional choice.
#if defined(WIFI_AP_PASSWORD)
  // ESP32 SoftAP requires a minimum password length of 8 characters.
  static_assert(sizeof(WIFI_AP_PASSWORD) - 1 >= 8,
                "WIFI_AP_PASSWORD must be at least 8 characters (ESP32 SoftAP minimum).");
#elif defined(WIFI_AP_OPEN)
  // Literal build_flags entry (outer single quotes, inner double quotes):
  //   -D WIFI_AP_PASSWORD='"yourpassword"'
  #pragma message("WIFI_AP_OPEN set — provisioning AP has no password (not for production). " \
                  "Set -D WIFI_AP_PASSWORD='\"yourpassword\"' to secure the captive portal.")
#else
  #error "Set WIFI_AP_PASSWORD in build_flags, or define WIFI_AP_OPEN=1 to allow an open provisioning AP."
#endif

static WiFiManager wm;

static constexpr int kPortalTimeoutSec = 180;

void setupWifi() {
  // Uncomment to reset saved credentials during development:
  // wm.resetSettings();

  wm.setConfigPortalTimeout(kPortalTimeoutSec);
  wm.setConnectTimeout(30);

  // AP name shown to user during provisioning — includes firmware version for flash validation.
  // Use all 6 MAC bytes to guarantee uniqueness across devices.
  // esp_efuse_mac_get_default fills mac[0..5] in standard OUI-first (MSB-first) byte order.
  uint8_t macBytes[6];
  esp_efuse_mac_get_default(macBytes);
  char macSuffix[13];
  snprintf(macSuffix, sizeof(macSuffix), "%02x%02x%02x%02x%02x%02x",
           macBytes[0], macBytes[1], macBytes[2],
           macBytes[3], macBytes[4], macBytes[5]);
  // WiFi SSIDs are limited to 32 bytes. Cap the version portion so the full
  // 12-char MAC suffix (which guarantees uniqueness) is always preserved.
  // Layout: "ESP32-" (6) + version (≤13) + "-" (1) + mac (12) = ≤32
  constexpr int kMaxVersionLen = 32 - 6 - 1 - 12;  // 13
  String versionStr = String(FIRMWARE_VERSION);
  if (versionStr.length() > kMaxVersionLen) versionStr = versionStr.substring(0, kMaxVersionLen);
  String apName = String("ESP32-") + versionStr + String("-") + String(macSuffix);

  LOG_STATUS("-- WiFi Setup ------------------------------------------");
  LOGF_STATUS("Connect to WiFi AP : %s", apName.c_str());
  LOG_STATUS("Then open          : http://192.168.4.1");
  LOGF_STATUS("Portal closes in   : %d min", kPortalTimeoutSec / 60);
#ifdef WIFI_AP_OPEN
  LOG_STATUS("WARNING: provisioning AP has no password (WIFI_AP_OPEN set) — not for production.");
#endif
  LOG_STATUS("--------------------------------------------------------");

#ifdef WIFI_AP_PASSWORD
  bool connected = wm.autoConnect(apName.c_str(), WIFI_AP_PASSWORD);
#else
  bool connected = wm.autoConnect(apName.c_str());
#endif

  if (!connected) {
    LOG_STATUS("WiFi: failed to connect — restarting in 5s");
    delay(5000);
    ESP.restart();
  }

  LOGF_STATUS("WiFi: connected, IP=%s", WiFi.localIP().toString().c_str());
}

#endif // FEATURE_WIFI_PROVISIONING
