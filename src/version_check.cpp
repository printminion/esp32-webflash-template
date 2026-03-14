// ============================================================
// version_check.cpp — Automatic OTA version check
//
// On demand, fetches version.json from GitHub Pages and compares
// the remote version to the running firmware. If a newer version
// is available it downloads and applies the firmware via
// esp_https_ota, then reboots.
//
// Called once from setup() after WiFi is connected.
// ============================================================

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_idf_version.h>
#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <esp_crt_bundle.h>
#include "board_config.h"
#include "logger.h"

// ── Configuration ─────────────────────────────────────────
#ifndef VERSION_CHECK_URL
  #define VERSION_CHECK_URL \
    "https://printminion.github.io/esp32-webflash-template/version.json"
#endif

// Accept any TLS cert from GitHub Pages (avoids bundling a root CA).
// For production, replace with the ISRG Root X1 PEM.
#define VERSION_CHECK_INSECURE 1

// ── Helpers ───────────────────────────────────────────────

// Parse a semver string "vMAJOR.MINOR.PATCH" or "MAJOR.MINOR.PATCH"
// into a single comparable uint32.  Non-release strings (e.g. "dev-abc")
// return 0 so they never trigger an update.
static uint32_t parseSemver(const char* s) {
  if (!s) return 0;
  if (*s == 'v') s++;               // strip leading 'v'
  unsigned maj = 0, min = 0, pat = 0;
  if (sscanf(s, "%u.%u.%u", &maj, &min, &pat) != 3) return 0;
  return (maj << 20) | (min << 10) | pat;
}

// ── Public API ────────────────────────────────────────────

void checkAndApplyUpdate() {
  LOG_STATUS("Checking for updates...");
  LOGF("VersionCheck: running firmware %s on %s", FIRMWARE_VERSION, BOARD_NAME);

  // ── 1. Fetch version.json ──────────────────────────────
  WiFiClientSecure client;
#if VERSION_CHECK_INSECURE
  client.setInsecure();
#endif

  HTTPClient http;
  http.begin(client, VERSION_CHECK_URL);
  http.setTimeout(8000);
  int code = http.GET();

  if (code != 200) {
    LOGF_STATUS("VersionCheck: HTTP %d — skipping", code);
    http.end();
    return;
  }

  String body = http.getString();
  http.end();

  // ── 2. Parse JSON ──────────────────────────────────────
  // Expected shape:
  // {
  //   "version": "v1.2.3",
  //   "boards": {
  //     "Seeed XIAO ESP32-C3": "https://.../firmware-seeed_xiao_esp32c3-v1.2.3.bin",
  //     ...
  //   }
  // }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    LOGF_STATUS("VersionCheck: JSON parse error: %s", err.c_str());
    return;
  }

  const char* remoteVersion = doc["version"];
  const char* firmwareUrl   = doc["boards"][BOARD_NAME];

  if (!remoteVersion || !firmwareUrl) {
    LOGF("VersionCheck: missing fields in version.json (board key: %s)", BOARD_NAME);
    return;
  }

  LOGF_STATUS("VersionCheck: remote=%s local=%s", remoteVersion, FIRMWARE_VERSION);

  // ── 3. Compare versions ───────────────────────────────
  uint32_t remote = parseSemver(remoteVersion);
  uint32_t local  = parseSemver(FIRMWARE_VERSION);

  if (remote == 0) {
    LOGF("VersionCheck: remote version is not a release tag — skipping");
    return;
  }
  if (local == 0) {
    LOG("VersionCheck: local version is a dev build — skipping auto-update");
    return;
  }
  if (remote <= local) {
    LOG_STATUS("Firmware is up to date.");
    return;
  }

  // ── 4. Apply OTA update ───────────────────────────────
  LOGF_STATUS("Downloading update %s...", remoteVersion);

  esp_http_client_config_t cfg = {};
  cfg.url                       = firmwareUrl;
  cfg.transport_type            = HTTP_TRANSPORT_OVER_SSL;
  cfg.skip_cert_common_name_check = true;
  cfg.crt_bundle_attach         = esp_crt_bundle_attach;

  esp_https_ota_config_t ota_cfg = {};
  ota_cfg.http_config = &cfg;

  esp_https_ota_handle_t handle = NULL;
  esp_err_t ret = esp_https_ota_begin(&ota_cfg, &handle);
  if (ret != ESP_OK) {
    LOGF_STATUS("VersionCheck: OTA begin failed (0x%x) — continuing with current firmware", ret);
    return;
  }

  int lastPct = -1;
  while ((ret = esp_https_ota_perform(handle)) == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
    int total = esp_https_ota_get_image_size(handle);
    int done  = esp_https_ota_get_image_len_read(handle);
    if (total > 0) {
      int pct = done * 100 / total;
      if (pct != lastPct) {
        LOGF_STATUS("OTA: %d%%", pct);
        lastPct = pct;
      }
    }
  }

  bool complete = esp_https_ota_is_complete_data_received(handle);
  esp_err_t finish_ret = esp_https_ota_finish(handle);

  if (complete && finish_ret == ESP_OK) {
    LOG_STATUS("OTA complete — rebooting");
    delay(500);
    esp_restart();
  } else {
    LOGF_STATUS("VersionCheck: OTA failed (0x%x) — continuing with current firmware", finish_ret);
  }
}
