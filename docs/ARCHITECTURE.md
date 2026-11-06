# Architecture

This document describes the internal architecture of MicroTask
Monitor. It is intended for contributors who want to add a new
platform backend, a new metric, or a new transport. Users who only
need to consume the public API can skip this file.

## 1. Layered overview

```
┌────────────────────────────────────────────────────────────┐
│  Public API (MicroTaskMonitor.h)                            │
│  - MicroTaskMonitor class                                   │
│  - MonitorWarning callback type                             │
│  - MTMCapability enum aliases (RAM_MONITOR, CPU_MONITOR, …) │
└──────────────────────────┬─────────────────────────────────┘
                           │ owns
┌──────────────────────────▼─────────────────────────────────┐
│  Engine (MicroTaskMonitor.cpp)                              │
│  - sampling interval bookkeeping                            │
│  - per-module update() orchestration                         │
│  - snapshot aggregation                                      │
│  - alerts engine invocation (from update(), never from ISR) │
│  - transport fan-out                                         │
└──────────────────────────┬─────────────────────────────────┘
                           │ owns
┌──────────────────────────▼─────────────────────────────────┐
│  Module wrappers (src/metrics/MTMMonitorBase.h)             │
│  - RAMMonitor, CPUMonitor, StorageMonitor, TemperatureMonitor,│
│    PowerMonitor, RTOSMonitor, NetworkMonitor, RuntimeMonitor  │
│  - history bookkeeping                                       │
│  - running minima                                            │
└──────────────────────────┬─────────────────────────────────┘
                           │ owns (one of each)
┌──────────────────────────▼─────────────────────────────────┐
│  HAL interfaces (src/hal/MTMHalInterfaces.h)                │
│  - IRAMBackend, ICPUBackend, IStorageBackend, IGPUBackend,   │
│    ITemperatureBackend, IPowerBackend, IRTOSBackend,          │
│    INetworkBackend, IRuntimeBackend                          │
└──────────────────────────┬─────────────────────────────────┘
                           │ implemented by
┌──────────────────────────▼─────────────────────────────────┐
│  Platform backends                                           │
│  src/platforms/<avr|esp32|esp8266|stm32|rp2040|samd>/        │
│  - one folder per architecture                              │
│  - one *Backends.cpp + *Backends.h per platform             │
└──────────────────────────┬─────────────────────────────────┘
                           │ selected by
┌──────────────────────────▼─────────────────────────────────┐
│  Platform factory (src/hal/MTMPlatformFactory.*)            │
│  - compile-time selection via MTMPlatformDetect.h           │
│  - returns a MTMBackendSet owning all 9 backends             │
│  - fills missing backends with the Null implementations      │
└────────────────────────────────────────────────────────────┘
```

Supporting subsystems that the engine calls into:

```
┌─────────────────────┐   ┌─────────────────────┐   ┌─────────────────────┐
│  Telemetry          │   │  Alerts              │   │  History             │
│  src/telemetry/     │   │  src/alerts/         │   │  src/history/        │
│  - report, JSON, CSV│   │  - thresholds        │   │  - circular buffer   │
│  - writes to Print& │   │  - hysteresis        │   │  - templated        │
└─────────────────────┘   └─────────────────────┘   └─────────────────────┘

┌─────────────────────┐   ┌─────────────────────┐   ┌─────────────────────┐
│  Profiler           │   │  Transports          │   │  Platform detection │
│  src/profiling/     │   │  src/transports/     │   │  src/hal/MTMPlatform │
│  - scoped timing    │   │  - Serial, UART      │   │  Detect.h           │
│  - min/max/avg/count│   │  - WiFi, BLE         │   │                     │
│                     │   │  - user-defined      │   │                     │
└─────────────────────┘   └─────────────────────┘   └─────────────────────┘
```

## 2. Compile-time platform selection

`src/hal/MTMPlatformDetect.h` is the **only** file in the library
that probes the toolchain for `__AVR__`, `ESP32`, `__STM32F4__`,
`__RP2040__`, `__SAMD21__`, etc. Every other source file uses the
`MTM_PLATFORM_*` macros defined here. This keeps platform-specific
toolchain macros out of the public API and out of the engine code.

The detection produces a single `MTM_PLATFORM_*` macro:

- `MTM_PLATFORM_AVR`     — Arduino Uno / Nano / Mega / Leonardo
- `MTM_PLATFORM_ESP32`   — ESP32 (classic, S2, S3, C3, C6)
- `MTM_PLATFORM_ESP8266` — ESP8266
- `MTM_PLATFORM_STM32`   — STM32 F0 / F1 / F4 / G0 / H7
- `MTM_PLATFORM_RP2040`  — Raspberry Pi Pico / Pico W
- `MTM_PLATFORM_SAMD`    — Arduino SAMD21 / SAMD51
- `MTM_PLATFORM_HOST`    — host build / unit tests / unknown core

The `MTM_PLATFORM_HOST` fallback exists so the library still compiles
on desktops and CI environments where none of the embedded toolchains
are present.

## 3. The platform factory

`src/hal/MTMPlatformFactory.cpp` is the second file (after
`MTMPlatformDetect.h`) that does any `#ifdef` on the platform. It
contains one `#if defined(MTM_PLATFORM_*)` block per platform that
includes the matching `*Backends.h` and constructs the `MTMBackendSet`
by calling the platform's `g_mtmXXXMake<Module>()` factory functions.

The factory uses **Null backends** (`MTMNullBackends.h`) to fill in
metrics a particular platform cannot supply. This means the engine
never has to deal with null pointers — every backend slot in the
`MTMBackendSet` always points to a valid object, even if that object
only ever returns `MTM_METRIC_NOT_SUPPORTED`.

This is the **only** place in the library that uses heap allocation
for backends. The allocation happens once, at `begin()`. After
`begin()`, the library allocates nothing.

## 4. The engine's `update()` cycle

```
update()
  ├── check if sampling interval elapsed (millis() - lastSampleMs < intervalMs)
  │     ├── yes → continue
  │     └── no  → return
  ├── lastSampleMs = millis()
  ├── for each enabled module:
  │     └── module.update(nowMs)
  │           └── backend->update(nowMs)
  ├── firstSampleTaken = true
  ├── snapshot = aggregateSnapshot()
  ├── alerts.evaluate(snapshot, nowMs)
  │     └── if thresholds crossed → invoke user callback
  └── (transports are user-driven: the engine does NOT auto-push)
```

The user controls when telemetry is emitted (typically
`monitor.printJSON(Serial)` from the sketch). The engine itself
never pushes to transports automatically, keeping the system
deterministic.

## 5. Capability bitmask

Each platform reports a 32-bit capability bitmask in
`g_mtmXXXCapabilities()`. The bits are the `MTM_CAP_*` constants in
`core/MTMTypes.h`. The bitmask is built at the same time as the
backends so the engine can answer `isSupported()` immediately after
`begin()`.

Capability bits are tested with `caps & MTM_CAP_CPU_MONITOR`. The
convenience aliases at the bottom of `MTMCapability` (e.g.
`RAM_MONITOR == MTM_CAP_RAM_MONITOR`) match the names used in the
README and examples.

## 6. Adding a new platform

1. Create `src/platforms/<arch>/` with two files:
   - `MTM<Arch>Backends.h` — declares the factory functions.
   - `MTM<Arch>Backends.cpp` — implements the backends and the
     factory functions.

2. Add the architecture detection to `MTMPlatformDetect.h`.

3. Add the new platform's `#elif` block to
   `MTMPlatformFactory.cpp`.

4. For each backend that the platform can implement, write a small
   class derived from the matching interface. For each backend that
   the platform cannot implement, return 0 from the factory
   function — the engine will substitute a Null backend.

5. Add a row to the compatibility table in `README.md` and to the
   table in `docs/ARCHITECTURE.md`.

6. Add at least one `tests/mocks/` mock if the platform introduces
   new metric semantics that the existing tests do not cover.

7. Add a `pio run -e <env>` env to `platformio.ini`.

8. Run the host-side unit tests; they should still pass without
   modification.

## 7. Adding a new metric

1. Add the snapshot struct (`MTM<Name>Snapshot`) to `core/MTMTypes.h`.
   Include a `MTMMetricStatus status` field.

2. Add the capability bit (`MTM_CAP_<NAME>_MONITOR`).

3. Add the interface (`I<Name>Backend`) to
   `hal/MTMHalInterfaces.h`.

4. Add the Null implementation to `MTMNullBackends.h`.

5. Add the wrapper class (`<Name>Monitor`) to
   `metrics/MTMMonitorBase.h`.

6. Add the field to `MTMSystemSnapshot` and to the snapshot
   aggregation in `MicroTaskMonitor.cpp`.

7. Add a public accessor `MicroTaskMonitor::<name>()` and a
   capability dispatch in `enable()` / `disable()` / `isEnabled()`.

8. Add the platform-specific implementations to every relevant
   `src/platforms/<arch>/*Backends.cpp` and the capability bitmask
   to `g_mtmXXXCapabilities()`.

9. Extend the serializers (`MTMSerializers.cpp`) and the CSV header.

10. Add at least one test in `MTMTests.cpp`.

## 8. Memory and concurrency contract

- **No heap allocation in the hot path.** The single allocation that
  happens at `begin()` is the backend set; everything else is static
  or stack.

- **No `String` in the public API or in the backends.** All strings
  are `char*` with explicit length, or `Print&` for output.

- **No blocking operations.** `update()` returns in O(1) on every
  platform. Serializers run in O(snapshot size).

- **No ISRs.** The engine never registers an interrupt handler.
  The alerts callback is invoked from `update()`, which runs in user
  context.

- **No RTOS dependency.** Bare-metal AVR / ESP8266 / STM32 / SAMD
  cores build and run without any FreeRTOS linkage.

- **No background threads.** The engine is purely driven by the
  sketch's `loop()` calls.

## 9. Build matrix

The library is expected to compile and pass the host-side unit tests
on every supported platform. Use `pio run -e <env>` to compile for a
specific board. The host test env (`pio run -e host`) compiles and
runs the unit tests on the development machine.
