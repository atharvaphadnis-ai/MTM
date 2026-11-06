# Changelog

All notable changes to **MicroTask Monitor (MTM)** are documented here.

The format is loosely based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and the project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] — 2026-11-06

### Added
- Initial public release of the MicroTask Monitor library.
- Unified `MicroTaskMonitor` API with `begin()`, `update()`, `available()`,
  `printReport()`, `printJSON()`, `printCSV()` and `snapshot()`.
- RAM monitor: total/used/free SRAM, peak utilisation, minimum-ever free
  memory, heap fragmentation estimation, largest free block, PSRAM on ESP32,
  stack high-water mark on supported cores.
- CPU monitor: frequency, idle/active time, utilisation percentage, load
  averages over configurable windows, multicore monitoring on ESP32 / RP2040.
- Storage monitor: flash capacity, application size on AVR, LittleFS /
  SPIFFS / SD / EEPROM statistics, wear statistics where the underlying
  driver exposes them.
- Temperature monitor: internal sensor on AVR, ESP32, STM32, RP2040;
  external sensor hook; rolling history; configurable warnings.
- Power monitor: voltage and (where available) current readings, energy
  estimates, low-voltage alerts.
- RTOS monitor: task count, names, states, priorities, stack high-water
  marks and per-task runtime on FreeRTOS-enabled cores (ESP32, ESP32-S2/S3/C3/C6,
  RP2040 with FreeRTOS, STM32 with FreeRTOS).
- Network monitor: Wi-Fi state, RSSI, IP, bytes TX/RX, throughput, link
  uptime on ESP32 / ESP8266 / RP2040-Pico-W / SAMD-WINC.
- Runtime monitor: uptime, reset reason, restart counter (optional,
  persisted in EEPROM / NVS / flash), watchdog status.
- Performance profiler: scoped function/loop timing with min/max/avg/count
  and configurable history.
- Historical data engine with fixed-size circular buffers per metric.
- Alerts engine: warning + critical thresholds, user callbacks invoked from
  user context (never from an ISR).
- Telemetry protocol v1: stable JSON + CSV + human-readable serial reports.
- Transport layer: `SerialTransport`, `UARTTransport`, `WiFiTransport`,
  `BLETransport` (ESP32 only), user-defined custom transports.
- Hardware abstraction layer with compile-time platform detection for AVR,
  ESP32, ESP8266, STM32, RP2040 and SAMD.
- Capability-detection API: `capabilities()`, `isSupported(metric)`.
- Host-side unit tests for core calculations, history buffers, telemetry
  serialization, sampling, thresholds and unsupported-capability handling.
- Eleven worked examples covering every public feature.

### Known limitations
- AVR (Uno/Nano/Mega/Leonardo) has no on-chip CPU-utilisation counter;
  CPU usage is reported as `ESTIMATE` and derived from idle-loop sampling.
- STM32 flash wear statistics require a wear-levelling filesystem; raw
  flash wear is not reported by the HAL.
- ESP32-C3 / C6 do not expose internal temperature via the standard API;
  the temperature sensor reads `NOT_SUPPORTED` on those variants.
- BLE transport is ESP32-only and disabled at compile time elsewhere.
