/*
 * MTMPlatformDetect.h
 * ===================
 * Compile-time platform detection for MicroTask Monitor.
 *
 * This file is the ONLY place in the library that probes the toolchain for
 * `__AVR_*`, `ESP32`, `__STM32F4__`, etc. Every other source file uses the
 * MTM_PLATFORM_* macros defined here so platform backends can be swapped
 * without leaking toolchain-specific macros into the public API.
 *
 * Supported platforms:
 *
 *   MTM_PLATFORM_AVR       Arduino Uno / Nano / Mega / Leonardo
 *   MTM_PLATFORM_ESP32     ESP32, ESP32-S2, S3, C3, C6 (IDF/Arduino core)
 *   MTM_PLATFORM_ESP8266   ESP8266
 *   MTM_PLATFORM_STM32     STM32duino core (F0/F1/F4/G0/H7)
 *   MTM_PLATFORM_RP2040    Raspberry Pi Pico / RP2040 (earlephilhower)
 *   MTM_PLATFORM_SAMD      Arduino SAMD core
 *   MTM_PLATFORM_HOST      host-side unit tests / desktop builds
 *
 * When none of the embedded toolchains are detected we fall back to
 * MTM_PLATFORM_HOST so the library still compiles for tests and for users
 * who want to dry-run sketches in a simulator.
 */
#ifndef MTM_PLATFORM_DETECT_H
#define MTM_PLATFORM_DETECT_H

/* ------------------------------------------------------------------ */
/* 1. Detect architecture                                              */
/* ------------------------------------------------------------------ */

#if defined(ARDUINO_ARCH_AVR) || defined(__AVR__)
    #define MTM_PLATFORM_AVR 1
    #define MTM_PLATFORM_NAME "AVR"
#elif defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #define MTM_PLATFORM_ESP32 1
    /* Sub-variant detection. The ESP32 Arduino core defines one of:
       ARDUINO_ESP32C3_DEV / ARDUINO_ESP32C6_DEV / ARDUINO_ESP32S2_DEV /
       ARDUINO_ESP32S3_DEV / CONFIG_IDF_TARGET_ESP32* — we just record the
       sub-family without changing the platform macro. */
    #if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(ARDUINO_ESP32S2_DEV)
        #define MTM_PLATFORM_NAME "ESP32-S2"
    #elif defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV)
        #define MTM_PLATFORM_NAME "ESP32-S3"
    #elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
        #define MTM_PLATFORM_NAME "ESP32-C3"
    #elif defined(CONFIG_IDF_TARGET_ESP32C6) || defined(ARDUINO_ESP32C6_DEV)
        #define MTM_PLATFORM_NAME "ESP32-C6"
    #else
        #define MTM_PLATFORM_NAME "ESP32"
    #endif
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    #define MTM_PLATFORM_ESP8266 1
    #define MTM_PLATFORM_NAME "ESP8266"
#elif defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4) || \
      defined(STM32F0) || defined(STM32G0) || defined(STM32H7) || \
      defined(ARDUINO_ARCH_STM32)
    #define MTM_PLATFORM_STM32 1
    #if defined(STM32F0)
        #define MTM_PLATFORM_NAME "STM32F0"
    #elif defined(STM32F1)
        #define MTM_PLATFORM_NAME "STM32F1"
    #elif defined(STM32F4)
        #define MTM_PLATFORM_NAME "STM32F4"
    #elif defined(STM32G0)
        #define MTM_PLATFORM_NAME "STM32G0"
    #elif defined(STM32H7)
        #define MTM_PLATFORM_NAME "STM32H7"
    #else
        #define MTM_PLATFORM_NAME "STM32"
    #endif
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2040) || defined(__RP2040__)
    #define MTM_PLATFORM_RP2040 1
    #define MTM_PLATFORM_NAME "RP2040"
#elif defined(ARDUINO_ARCH_SAMD) || defined(__SAMD21__) || defined(__SAMD51__)
    #define MTM_PLATFORM_SAMD 1
    #define MTM_PLATFORM_NAME "SAMD"
#else
    /* Fallback for host builds, unit tests and unsupported cores. */
    #define MTM_PLATFORM_HOST 1
    #define MTM_PLATFORM_NAME "HOST"
#endif

/* ------------------------------------------------------------------ */
/* 2. Convenience flags                                                */
/* ------------------------------------------------------------------ */

#if defined(MTM_PLATFORM_AVR)
    #define MTM_HAS_FREE_RTOS 0
    #define MTM_HAS_INTERNAL_TEMP 1
    #define MTM_HAS_MULTICORE 0
#elif defined(MTM_PLATFORM_ESP32)
    #define MTM_HAS_FREE_RTOS 1
    #define MTM_HAS_INTERNAL_TEMP 1
    #define MTM_HAS_MULTICORE 1
#elif defined(MTM_PLATFORM_ESP8266)
    #define MTM_HAS_FREE_RTOS 0
    #define MTM_HAS_INTERNAL_TEMP 0
    #define MTM_HAS_MULTICORE 0
#elif defined(MTM_PLATFORM_STM32)
    #define MTM_HAS_FREE_RTOS 0
    #define MTM_HAS_INTERNAL_TEMP 1
    #define MTM_HAS_MULTICORE 0
#elif defined(MTM_PLATFORM_RP2040)
    #define MTM_HAS_FREE_RTOS 0
    #define MTM_HAS_MULTICORE 1
    #define MTM_HAS_INTERNAL_TEMP 1
#elif defined(MTM_PLATFORM_SAMD)
    #define MTM_HAS_FREE_RTOS 0
    #define MTM_HAS_INTERNAL_TEMP 0
    #define MTM_HAS_MULTICORE 0
#else
    #define MTM_HAS_FREE_RTOS 0
    #define MTM_HAS_INTERNAL_TEMP 0
    #define MTM_HAS_MULTICORE 0
#endif

#endif // MTM_PLATFORM_DETECT_H
