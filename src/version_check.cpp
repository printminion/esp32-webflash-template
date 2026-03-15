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
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #include <esp_crt_bundle.h>
#endif
#include "board_config.h"
#include "logger.h"

// ── Configuration ─────────────────────────────────────────
#ifndef VERSION_CHECK_URL
  #define VERSION_CHECK_URL \
    "https://printminion.github.io/esp32-webflash-template/version.json"
#endif

// TLS security for version check and OTA download.
// Default: secure (validates TLS certificates via the ESP-IDF CA bundle).
// Override to 1 in platformio.ini build_flags for dev hardware without a
// trusted CA bundle — never enable in production (enables MITM/RCE).
#ifndef VERSION_CHECK_INSECURE
  #define VERSION_CHECK_INSECURE 0
#endif

// ── Helpers ───────────────────────────────────────────────

// Parse a semver string "vMAJOR.MINOR.PATCH" or "MAJOR.MINOR.PATCH"
// into a single comparable uint32.  Sets *valid=false for non-release strings
// (e.g. "dev-abc") so the caller can distinguish them from a real v0.0.0.
static uint32_t parseSemver(const char* s, bool* valid) {
  *valid = false;
  if (!s || !*s) return 0;
  if (*s == 'v') s++;               // strip leading 'v'
  unsigned maj = 0, min = 0, pat = 0;
  if (sscanf(s, "%u.%u.%u", &maj, &min, &pat) != 3) return 0;
  *valid = true;
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

  // Debug builds append " (debug)" to BOARD_NAME, but version.json uses the
  // release name as the key. Strip the suffix so lookups succeed in both modes.
  String boardKey = BOARD_NAME;
  if (boardKey.endsWith(" (debug)")) boardKey.remove(boardKey.length() - 8);
  const char* firmwareUrl = doc["boards"][boardKey.c_str()];

  if (!remoteVersion || !firmwareUrl || firmwareUrl[0] == '\0') {
    LOGF("VersionCheck: missing fields in version.json (board key: %s)", BOARD_NAME);
    return;
  }

  LOGF_STATUS("VersionCheck: remote=%s local=%s", remoteVersion, FIRMWARE_VERSION);

  // ── 3. Compare versions ───────────────────────────────
  bool remoteValid, localValid;
  uint32_t remote = parseSemver(remoteVersion, &remoteValid);
  uint32_t local  = parseSemver(FIRMWARE_VERSION, &localValid);

  if (!remoteValid) {
    LOGF("VersionCheck: remote version is not a release tag — skipping");
    return;
  }
  if (!localValid) {
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
  cfg.url            = firmwareUrl;
  cfg.transport_type = HTTP_TRANSPORT_OVER_SSL;
  cfg.buffer_size    = 2048;  // GitHub CDN headers exceed the 512-byte default
#if VERSION_CHECK_INSECURE
  cfg.skip_cert_common_name_check = true;
#else
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
  #endif
#endif

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
