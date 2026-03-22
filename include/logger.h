#pragma once

// ============================================================
// logger.h — Logging macros
//
// LOG_STATUS / LOGF_STATUS — always active (debug + release).
//   Use for user-facing status messages that must always appear.
//
// LOG / LOGF / LOG_RAW — debug only (DEBUG_BUILD defined).
//   Use for verbose diagnostics that are silent in release builds.
// ============================================================

#include <Arduino.h>

// Always-on: serial must be initialised first (LOG_BEGIN is always-on below)
#define LOG_STATUS(msg)       Serial.println(msg)
// Newline is printed separately so fmt need not be a string literal.
#define LOGF_STATUS(fmt, ...) do { Serial.printf(fmt, ##__VA_ARGS__); Serial.print('\n'); } while (0)

// Serial initialisation — always runs so LOG_STATUS works in release builds
#define LOG_BEGIN(baud) Serial.begin(baud)

#ifdef DEBUG_BUILD
  #define LOG(msg)       Serial.println(msg)
  #define LOGF(fmt, ...) do { Serial.printf(fmt, ##__VA_ARGS__); Serial.print('\n'); } while (0)
  #define LOG_RAW(msg)   Serial.print(msg)
#else
  #define LOG(msg)       do {} while (0)
  #define LOGF(fmt, ...) do {} while (0)
  #define LOG_RAW(msg)   do {} while (0)
#endif
