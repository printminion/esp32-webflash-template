#pragma once

// ============================================================
// logger.h — Debug logging macros
// Compiles to nothing in release builds (NDEBUG defined)
// ============================================================

#include <Arduino.h>

#ifdef DEBUG_BUILD
  #define LOG_BEGIN(baud)   Serial.begin(baud)
  #define LOG(msg)          Serial.println(msg)
  #define LOGF(fmt, ...)    Serial.printf(fmt "\n", ##__VA_ARGS__)
  #define LOG_RAW(msg)      Serial.print(msg)
#else
  #define LOG_BEGIN(baud)
  #define LOG(msg)
  #define LOGF(fmt, ...)
  #define LOG_RAW(msg)
#endif
