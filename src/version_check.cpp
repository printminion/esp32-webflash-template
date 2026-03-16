// ============================================================
// version_check.cpp — Automatic OTA version check (FEATURE_VERSION_CHECK)
//
// On demand, fetches version.json from GitHub Pages and compares
// the remote version to the running firmware. If a newer version
// is available it downloads and applies the firmware via
// esp_https_ota, then reboots.
//
// Called once from setup() after WiFi is connected.
// ============================================================
#include "board_config.h"
#ifdef FEATURE_VERSION_CHECK

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_idf_version.h>
#include <esp_https_ota.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #include <esp_crt_bundle.h>
#endif
#include "logger.h"

// ── Configuration ─────────────────────────────────────────
// VERSION_CHECK_URL must be set explicitly via build_flags or generate_platformio.py.
// When unset the feature is disabled at runtime (no network call, no supply-chain risk).
// Set in build_flags: -D VERSION_CHECK_URL='"https://your-domain/version.json"'
#ifndef VERSION_CHECK_URL
  #pragma message("VERSION_CHECK_URL not set — version checking disabled at runtime. " \
                  "Set in build_flags or re-run scripts/generate_platformio.py.")
  #define VERSION_CHECK_URL ""
#endif

// TLS security for version check and OTA download.
// VERSION_CHECK_INSECURE=0 (default): cert validation enabled.
//   version.json fetch: uses built-in mbedTLS CA bundle on IDF 5.0+;
//     on older IDF, skips the check entirely (fails closed) — set
//     VERSION_CHECK_INSECURE=1 to override.
//   OTA download: uses built-in CA bundle on IDF 5.0+.
// VERSION_CHECK_INSECURE=1 (dev only — never use in production):
//   version.json fetch: WiFiClientSecure.setInsecure() — skips all TLS
//     verification (cert chain + hostname).
//   OTA download: sets skip_cert_common_name_check only (hostname/CN check
//     disabled; no CA bundle is attached so chain validation also fails open
//     on most configurations). Equivalent to disabling TLS for the download.
#ifndef VERSION_CHECK_INSECURE
  #define VERSION_CHECK_INSECURE 0
#endif
#if VERSION_CHECK_INSECURE && defined(NDEBUG)
  #error "VERSION_CHECK_INSECURE=1 must not be used in release builds (enables MITM/RCE). " \
         "Remove it from build_flags or restrict it to debug environments."
#endif

// On IDF < 5.0 the mbedTLS CA bundle is not accessible via the Arduino API,
// so secure version checks are skipped at runtime (fail-closed). Warn at
// compile time so developers know this build will never auto-update unless
// VERSION_CHECK_INSECURE=1 is explicitly set.
#if !VERSION_CHECK_INSECURE && ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
  #pragma message("FEATURE_VERSION_CHECK: IDF < 5.0 detected without VERSION_CHECK_INSECURE=1 — " \
                  "version checks will be skipped at runtime (no accessible CA bundle). " \
                  "Set VERSION_CHECK_INSECURE=1 in build_flags or upgrade to IDF 5.")
#endif

// ── Helpers ───────────────────────────────────────────────

// Parse a semver string "vMAJOR.MINOR.PATCH" or "MAJOR.MINOR.PATCH"
// into a single comparable uint32.  Sets *valid=false for non-release strings
// (e.g. "dev-abc") so the caller can distinguish them from a real v0.0.0.
static uint32_t parseSemver(const char* s, bool* valid) {
  *valid = false;
  if (!s || !*s) return 0;
  if (*s == 'v') s++;               // strip leading 'v'
  // Use unsigned long to avoid UB when the version string component exceeds UINT_MAX.
  // Range checks below reject values that overflow the packed uint32 bitfield.
  unsigned long maj = 0, min = 0, pat = 0;
  int consumed = 0;
  if (sscanf(s, "%lu.%lu.%lu%n", &maj, &min, &pat, &consumed) != 3) return 0;
  if (s[consumed] != '\0') return 0;  // reject suffixes: -rc1, +meta, etc.
  // Reject out-of-range components to prevent bitfield overflow:
  // major: 12-bit (0–4095), minor/patch: 10-bit each (0–1023).
  if (maj > 4095 || min > 1023 || pat > 1023) return 0;
  *valid = true;
  return (maj << 20) | (min << 10) | pat;
}

// ── Public API ────────────────────────────────────────────

void checkAndApplyUpdate() {
  if (strlen(VERSION_CHECK_URL) == 0) {
    LOG_STATUS("VersionCheck: VERSION_CHECK_URL not configured — skipping.");
    return;
  }
  LOG_STATUS("Checking for updates...");
  LOGF("VersionCheck: running firmware %s on %s", FIRMWARE_VERSION, BOARD_NAME);

  // ── 1. Fetch version.json ──────────────────────────────
  WiFiClientSecure client;
#if VERSION_CHECK_INSECURE
  client.setInsecure();
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  // IDF 5.0+ (Arduino-ESP32 3.x): attach the built-in mbedTLS CA bundle.
  // setCACertBundle(ptr, size) requires both start and end linker symbols.
  // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE is enabled by default in Arduino builds.
  extern const uint8_t x509_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
  extern const uint8_t x509_crt_bundle_end[]   asm("_binary_x509_crt_bundle_end");
  client.setCACertBundle(x509_crt_bundle_start,
                         (size_t)(x509_crt_bundle_end - x509_crt_bundle_start));
#else
  // IDF < 5.0: no accessible CA bundle via Arduino API. Fail closed rather than
  // silently skipping TLS verification. Set VERSION_CHECK_INSECURE=1 in
  // build_flags to opt into unverified checks on this platform.
  LOG_STATUS("VersionCheck: skipping — TLS CA bundle unavailable on IDF < 5.0.");
  LOG_STATUS("Set VERSION_CHECK_INSECURE=1 in build_flags to override.");
  return;
#endif

  HTTPClient http;
  if (!http.begin(client, VERSION_CHECK_URL)) {
    LOG_STATUS("VersionCheck: HTTP client init failed — skipping");
    return;
  }
  http.setTimeout(8000);
  int code = http.GET();

  if (code != 200) {
    LOGF_STATUS("VersionCheck: HTTP %d — skipping", code);
    http.end();
    return;
  }

  int contentLength = http.getSize();
  if (contentLength > 4096) {
    LOGF_STATUS("VersionCheck: response too large (%d bytes) — skipping", contentLength);
    http.end();
    return;
  }

  // Static buffer: avoids consuming ~4KB of the 8KB loop task stack.
  static char jsonBuf[4097];
  int bytesRead = 0;

  if (contentLength > 0) {
    // Known Content-Length: read exactly N bytes (returns promptly after last byte).
    bytesRead = http.getStream().readBytes(jsonBuf, contentLength);
    jsonBuf[bytesRead] = '\0';
    http.end();
    if (bytesRead != contentLength) {
      LOGF_STATUS("VersionCheck: partial read (%d/%d bytes) — skipping", bytesRead, contentLength);
      return;
    }
  } else {
    // Chunked/unknown size: read into the capped buffer incrementally using
    // available() so we only read bytes that are already in the TCP buffer.
    // This avoids both getString()'s unbounded heap allocation and readBytes()'
    // blocking wait for stream timeout when the response is smaller than the cap.
    Stream& stream = http.getStream();
    unsigned long start = millis();
    while (bytesRead < (int)(sizeof(jsonBuf) - 1)) {
      if ((millis() - start) >= 8000) break;  // wrap-safe 8s timeout
      int avail = stream.available();
      if (avail > 0) {
        int chunk = min(avail, (int)(sizeof(jsonBuf) - 1) - bytesRead);
        bytesRead += stream.readBytes(jsonBuf + bytesRead, chunk);
      } else if (!http.connected()) {
        break;  // server closed connection — all data received
      } else {
        vTaskDelay(1);  // yield so the TCP/IP task can deliver the next chunk
      }
    }
    jsonBuf[bytesRead] = '\0';
    http.end();
    if (bytesRead == (int)(sizeof(jsonBuf) - 1)) {
      LOG_STATUS("VersionCheck: response too large — skipping");
      return;
    }
  }

  // ── 2. Parse JSON ──────────────────────────────────────
  // Expected shape:
  // {
  //   "version": "v1.2.3",
  //   "boards": {
  //     "Seeed XIAO ESP32-C3": "https://.../firmware-seeed_xiao_esp32c3-v1.2.3.bin",
  //     ...
  //   }
  // }

  // Debug builds append " (debug)" to BOARD_NAME, but version.json uses the
  // release name as the key. Strip the suffix so lookups succeed in both modes.
  String boardKey = BOARD_NAME;
  if (boardKey.endsWith(" (debug)")) boardKey.remove(boardKey.length() - 8);

  // Use a filter so ArduinoJson only allocates nodes for the two fields we
  // actually read. Both JsonDocument objects are still dynamically allocated
  // (ArduinoJson v7 has no fixed-capacity API), but the filter limits allocation
  // to the "version" string and a single firmware URL — bounded in practice.
  JsonDocument filter;
  filter["version"] = true;
  filter["boards"][boardKey.c_str()] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonBuf,
                                             DeserializationOption::Filter(filter));
  if (err) {
    LOGF_STATUS("VersionCheck: JSON parse error: %s", err.c_str());
    return;
  }

  const char* remoteVersion = doc["version"];
  const char* firmwareUrl   = doc["boards"][boardKey.c_str()];

  if (!remoteVersion) {
    LOG_STATUS("VersionCheck: missing version field in version.json — skipping");
    return;
  }
  // Sanity-check field lengths before use (guards against malformed/malicious payloads).
  if (strlen(remoteVersion) > 32) {
    LOG_STATUS("VersionCheck: remote version string too long — skipping");
    return;
  }
  if (!firmwareUrl || firmwareUrl[0] == '\0') {
    // No URL for this board is a normal state (e.g. placeholder version.json
    // before a release populates it). Log at debug level only.
    LOGF("VersionCheck: no firmware URL for board '%s' — skipping", boardKey.c_str());
    return;
  }
  if (strlen(firmwareUrl) > 512) {
    LOG_STATUS("VersionCheck: firmware URL too long — skipping");
    return;
  }

  LOGF_STATUS("VersionCheck: remote=%s local=%s", remoteVersion, FIRMWARE_VERSION);

  // ── 3. Compare versions ───────────────────────────────
  bool remoteValid, localValid;
  uint32_t remote = parseSemver(remoteVersion, &remoteValid);
  uint32_t local  = parseSemver(FIRMWARE_VERSION, &localValid);

  if (!remoteValid) {
    LOGF_STATUS("VersionCheck: remote version '%s' is not a release tag — skipping", remoteVersion);
    return;
  }
  if (!localValid) {
    LOG_STATUS("VersionCheck: local version is a dev build — skipping auto-update");
    return;
  }
  if (remote <= local) {
    LOG_STATUS("Firmware is up to date.");
    return;
  }

  // ── 4. Apply OTA update ───────────────────────────────
  LOGF_STATUS("Downloading update %s...", remoteVersion);

  // Security note: integrity of the downloaded firmware depends on HTTPS TLS
  // validation of the host serving version.json and the firmware binary.
  // A future hardening step would add per-board SHA-256 hashes to version.json
  // so the downloaded image can be verified before rebooting.

  // Reject non-https URLs — firmwareUrl comes from JSON and must use TLS.
  if (strncmp(firmwareUrl, "https://", 8) != 0) {
    LOGF_STATUS("VersionCheck: firmware URL must start with https:// — aborting (%s)", firmwareUrl);
    return;
  }

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

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0)
  // Streaming API (ESP-IDF 4.1+): progress reporting, graceful abort on failure.
  esp_https_ota_config_t ota_cfg = {};
  ota_cfg.http_config = &cfg;

  esp_https_ota_handle_t handle = NULL;
  esp_err_t ret = esp_https_ota_begin(&ota_cfg, &handle);
  if (ret != ESP_OK) {
    LOGF_STATUS("VersionCheck: OTA begin failed (0x%x) — continuing with current firmware", (unsigned)ret);
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
    // Yield to other tasks to prevent watchdog timeouts during long downloads.
    vTaskDelay(1);
  }

  bool complete = esp_https_ota_is_complete_data_received(handle);
  esp_err_t finish_ret = esp_https_ota_finish(handle);

  if (complete && finish_ret == ESP_OK) {
    LOG_STATUS("OTA complete — rebooting");
    delay(500);
    ESP.restart();
  } else {
    LOGF_STATUS("VersionCheck: OTA failed (0x%x) — continuing with current firmware", (unsigned)finish_ret);
  }
#else
  // Legacy single-shot API (ESP-IDF < 4.1): no streaming progress.
  esp_err_t ret = esp_https_ota(&cfg);
  if (ret == ESP_OK) {
    LOG_STATUS("OTA complete — rebooting");
    delay(500);
    ESP.restart();
  } else {
    LOGF_STATUS("VersionCheck: OTA failed (0x%x) — continuing with current firmware", (unsigned)ret);
  }
#endif
}

#endif // FEATURE_VERSION_CHECK
