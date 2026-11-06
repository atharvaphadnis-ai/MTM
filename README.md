# MicroTask Monitor (MTM)

> Universal embedded system monitoring library for Arduino-compatible
> microcontrollers — inspired by Windows Task Manager.

MicroTask Monitor (MTM) is a portable, production-quality Arduino
library that lets you monitor the **real** resource utilization of your
microcontroller — RAM, heap, CPU, storage, temperature, power, RTOS
tasks, network, uptime and execution profiling — through **one unified
API** that compiles unchanged across AVR, ESP32, ESP8266, STM32, RP2040
and SAMD.

```cpp
#include <MicroTaskMonitor.h>

MicroTaskMonitor monitor;

void setup() {
    Serial.begin(115200);
    monitor.begin();
    monitor.setSamplingInterval(1000);
    monitor.enable(RAM_MONITOR);
    monitor.enable(CPU_MONITOR);
    monitor.enable(TEMPERATURE_MONITOR);
}

void loop() {
    monitor.update();
    if (monitor.available()) {
        monitor.printReport(Serial);
    }
}
```

---

## Table of contents

1. [Project vision](#project-vision)
2. [Features](#features)
3. [Supported hardware](#supported-hardware)
4. [Installation](#installation)
   - [Arduino Library Manager](#installation-arduino-library-manager)
   - [PlatformIO](#installation-platformio)
   - [Manual install](#installation-manual)
5. [Getting started](#getting-started)
6. [API documentation](#api-documentation)
   - [Engine lifecycle](#engine-lifecycle)
   - [Capability queries](#capability-queries)
   - [Individual metric access](#individual-metric-access)
   - [System snapshot](#system-snapshot)
   - [Telemetry output](#telemetry-output)
   - [Alerts and thresholds](#alerts-and-thresholds)
   - [Transports](#transports)
   - [Performance profiler](#performance-profiler)
7. [Monitoring modules](#monitoring-modules)
   - [RAM Monitor](#module-ram)
   - [CPU Monitor](#module-cpu)
   - [Storage Monitor](#module-storage)
   - [GPU / Accelerator Monitor](#module-gpu)
   - [Temperature Monitor](#module-temperature)
   - [Power Monitor](#module-power)
   - [RTOS Task Monitor](#module-rtos)
   - [Network Monitor](#module-network)
   - [Runtime Monitor](#module-runtime)
   - [Performance Profiler](#module-profiler)
8. [Telemetry protocol](#telemetry-protocol)
9. [Examples](#examples)
10. [Performance overhead](#performance-overhead)
11. [Limitations](#limitations)
12. [Testing and validation](#testing-and-validation)
13. [Contributing](#contributing)
14. [License](#license)

---

<a name="project-vision"></a>
## 1. Project vision

MicroTask Monitor is the embedded equivalent of Windows Task Manager.
It does not draw charts; it provides **honest, measured, low-overhead
telemetry** that can be consumed by a serial monitor, a custom desktop
dashboard, an MQTT broker, or any other downstream consumer.

Three priorities drive every design decision:

1. **Portability** — the same sketch compiles unchanged across every
   supported board. Unsupported metrics report `NOT_SUPPORTED`; the
   library never invents values.
2. **Reliability** — no blocking monitoring operations, no mandatory
   background threads, no mandatory RTOS, no dynamic memory in the hot
   path. The library cannot corrupt the application it monitors.
3. **Extensibility** — new platforms and metrics slot in via small,
   focused backend files. The public API never changes for a new
   backend.

<a name="features"></a>
## 2. Features

- **RAM**: total, used, free, minimum-ever free, peak used,
  fragmentation (where available), largest free block, PSRAM on ESP32,
  stack high-water mark on FreeRTOS cores.
- **CPU**: frequency, utilisation (MEASURED on FreeRTOS-enabled cores
  with multicore support; ESTIMATE on bare-metal AVR/ESP8266),
  per-core monitoring, load averages over 1 / 5 / 60 s windows.
- **Storage**: flash capacity, LittleFS / SPIFFS / SD via explicit
  bind, EEPROM size on AVR / ESP8266 / SAMD, flash wear statistics where
  the underlying driver exposes them.
- **GPU / Accelerator**: capability-based — most MCUs return
  `NOT_SUPPORTED`. Forward-compatible with future chips that expose
  real GPU / DMA counters.
- **Temperature**: internal sensor on AVR / ESP32 (classic, S2, S3) /
  STM32 / RP2040 / SAMD51. External sensor hook on every platform.
- **Power**: voltage, current, power and battery percent via user-
  supplied sensors. No fake readings.
- **RTOS**: full FreeRTOS task statistics on ESP32 / ESP32-S2/S3/C3/C6
  — task count, names, states, priorities, stack high-water marks,
  per-task runtime. No RTOS dependency on bare-metal cores.
- **Network**: Wi-Fi link state, RSSI, IP, bytes TX/RX, throughput on
  ESP32 / ESP8266 / Pico W / SAMD-WINC.
- **Runtime**: uptime, reset reason (per-chip), reboot counter (when
  persistent storage is available), watchdog reset flag.
- **Performance Profiler**: scoped function/loop timing with
  min/max/avg/count and a per-scope circular history.
- **Telemetry**: human-readable serial report, stable JSON, CSV row
  per sample, callback-based streaming.
- **Alerts**: warning + critical thresholds with hysteresis on every
  metric. User callbacks invoked from `update()` — never from an ISR.
- **History**: fixed-size circular buffers per metric, no heap.
- **Transport**: USB Serial, UART, Wi-Fi TCP (ESP32 / ESP8266 / Pico W
  / SAMD-WINC), ESP32 BLE, custom user-defined transports.

<a name="supported-hardware"></a>
## 3. Supported hardware

The library is **hardware-portable**: it auto-detects the target
architecture at compile time and selects the appropriate backends.
The same sketch compiles on every board below without `#ifdef`
changes in user code.

| Platform | Boards tested against the public API | RAM | CPU | Storage | Temp | RTOS | Net |
|----------|---------------------------------------|:---:|:---:|:-------:|:----:|:----:|:---:|
| **AVR**  | Uno, Nano, Mega 2560, Leonardo        | M   | E   | M       | M\* | –    | –   |
| **ESP32** (classic) | ESP32-DevKit, NodeMCU-32S    | M   | M   | M       | M   | M    | M\*\* |
| **ESP32-S2** | ESP32-S2-Saola, UnexpectedMaker TinyS2 | M | M | M | M | M | – |
| **ESP32-S3** | ESP32-S3-DevKitC                | M   | M   | M       | M   | M    | M\*\* |
| **ESP32-C3** | ESP32-C3-DevKitM                | M   | M   | M       | –   | M    | M\*\* |
| **ESP32-C6** | ESP32-C6-DevKitC                | M   | M   | M       | –   | M    | M\*\* |
| **ESP8266** | NodeMCU v2, Wemos D1 mini         | M   | E   | M       | –   | –    | M\*\* |
| **STM32 F0** | Nucleo-F091RC, BluePill F0       | M   | E   | M       | M   | –    | –   |
| **STM32 F1** | BluePill F103C8                  | M   | E   | M       | M   | –    | –   |
| **STM32 F4** | BlackPill F411, Nucleo-F446RE    | M   | M   | M       | M   | –    | –   |
| **STM32 G0** | Nucleo-G070RB                    | M   | E   | M       | M   | –    | –   |
| **STM32 H7** | Nucleo-H743ZI                    | M   | M   | M       | M   | –    | –   |
| **RP2040**  | Raspberry Pi Pico, Pico W         | M   | E   | M       | M   | –    | M\*\* (Pico W only) |
| **SAMD21** | Arduino Zero, MKR1000             | M   | E   | M       | –   | –    | M\*\* (WiFiNINA) |
| **SAMD51** | Adafruit Grand Central M4         | M   | E   | M       | M   | –    | –   |

Legend: **M** = MEASURED · **E** = ESTIMATE · **–** = NOT_SUPPORTED.
\* AVR temperature reading is approximate (±10 °C) due to per-chip
calibration. \*\* Network metric requires WiFi to be connected; on
disconnected boards it reports `connected=false` but stays MEASURED.

> **Hardware-validation status.** The library has been **compiled**
> against every platform above using PlatformIO. **Live hardware
> tests** have been run on Arduino Uno (ATmega328P), ESP32-DevKit v1,
> ESP8266 NodeMCU v2, Raspberry Pi Pico (RP2040) and STM32 BluePill F1.
> Other boards compile cleanly but have not been measured against real
> hardware — please report issues if you find a regression.

<a name="installation"></a>
## 4. Installation

<a name="installation-arduino-library-manager"></a>
### 4.1 Arduino Library Manager

1. Open the Arduino IDE.
2. Open the Library Manager: `Tools ▸ Manage Libraries…`
3. Search for `MicroTaskMonitor`.
4. Click **Install**.

> If you cloned this repository, copy it (or symlink it) into your
> Arduino `libraries/` folder. The folder must be named exactly
> `MicroTaskMonitor`.

<a name="installation-platformio"></a>
### 4.2 PlatformIO

```ini
; platformio.ini
[env:esp32]
platform   = espressif32
board      = esp32dev
framework  = arduino
lib_deps   =
    MicroTaskMonitor
```

Or, if you keep the library locally:

```ini
[env:esp32]
platform   = espressif32
board      = esp32dev
framework  = arduino
lib_extra_dirs = /path/to/MicroTaskMonitor/..
```

The library has **no mandatory dependencies**. Optional transports
(Wi-Fi, BLE) only link when the corresponding headers are present in
the sketch.

<a name="installation-manual"></a>
### 4.3 Manual install

Clone the repository and copy the `MicroTaskMonitor` folder into:

- `~/Documents/Arduino/libraries/` (macOS / Linux)
- `Documents\Arduino\libraries\` (Windows)

Restart the Arduino IDE. The examples will appear under
`File ▸ Examples ▸ MicroTaskMonitor`.

<a name="getting-started"></a>
## 5. Getting started

The fastest way to see the library in action is to flash the
`BasicMonitoring` example:

```cpp
#include <MicroTaskMonitor.h>

MicroTaskMonitor monitor;

void setup() {
    Serial.begin(115200);
    monitor.begin();
    monitor.setSamplingInterval(1000);
    monitor.enable(RAM_MONITOR);
    monitor.enable(CPU_MONITOR);
    monitor.enable(STORAGE_MONITOR);
    monitor.enable(TEMPERATURE_MONITOR);
}

void loop() {
    monitor.update();
    if (monitor.available()) {
        monitor.printReport(Serial);
    }
}
```

Open the Serial Monitor at 115200 baud. You will see something like:

```
==== MicroTask Monitor v1.0.0 ====
Board:        ESP32
Uptime (ms):  1247

-- RAM --
  Status:        MEASURED
  Total:        319232 bytes
  Used:          14432 bytes
  Free:         304800 bytes
  Min ever free:294112 bytes
  Peak used:    25120 bytes
  Fragmentation:5 %
  Largest free: 291840 bytes
  Alloc fails:  0
  PSRAM:        NOT_SUPPORTED
  Stack:        MEASURED
    Size:       8192
    HWM:        5300

-- CPU --
  Freq:          MEASURED
    240000000 Hz
  Usage:         MEASURED
    7 %
  Load avg (1/5/60s): 7 / 7 / 7 %

-- Storage --
  Flash:    1332016 / 4194304 bytes
  EEPROM:   4096 bytes

-- Temperature --
  Status:   MEASURED
  Now:      53.0 C
  Min:      50.0 C
  Max:      53.0 C

-- Runtime --
  Reset reason: 1
  Reboot count: 3
  WDT reset:    no
```

<a name="api-documentation"></a>
## 6. API documentation

<a name="engine-lifecycle"></a>
### 6.1 Engine lifecycle

```cpp
MicroTaskMonitor monitor;          // declare once, globally
monitor.begin();                   // detect platform, allocate backends
monitor.update();                  // call from loop() — samples per interval
bool ready = monitor.available();  // true after the first sample
```

The engine samples at most once every `setSamplingInterval()`
milliseconds (default 1000 ms). `update()` is non-blocking — it returns
immediately if the interval has not elapsed.

<a name="capability-queries"></a>
### 6.2 Capability queries

```cpp
uint32_t caps = monitor.capabilities();
bool cpuAvailable = monitor.isSupported(CPU_MONITOR);
bool psramAvailable = monitor.isSupported(PSRAM_MONITOR);
```

`isSupported(cap)` returns whether the platform CAN supply the metric —
independent of whether the user has enabled it. Use it to gate UI code
or to skip features that would only produce `NOT_SUPPORTED` snapshots.

<a name="individual-metric-access"></a>
### 6.3 Individual metric access

```cpp
auto ram = monitor.memory();
Serial.println(ram.total);
Serial.println(ram.used);
Serial.println(ram.free);
Serial.println(ram.peak);
Serial.println(ram.fragmentation);
Serial.println(ram.status == MTM_METRIC_MEASURED ? "MEASURED" : "OTHER");

auto cpu = monitor.cpu();
Serial.println(cpu.usage);
Serial.println(cpu.frequency);

auto storage = monitor.storage();
Serial.println(storage.total);
Serial.println(storage.free);

auto temperature = monitor.temperature();
Serial.println(temperature.value);  // millidegrees Celsius
```

Each snapshot struct carries a `status` field of type
`MTMMetricStatus`. **Always check `status` before interpreting the
numeric value** — a `NOT_SUPPORTED` metric has zero in every numeric
field.

<a name="system-snapshot"></a>
### 6.4 System snapshot

```cpp
auto system = monitor.snapshot();   // full MTMSystemSnapshot
```

`MTMSystemSnapshot` is a flat POD struct with every per-module
snapshot embedded. Use it when you want one consistent snapshot taken
at the same instant — useful for telemetry and for the alerts engine.

<a name="telemetry-output"></a>
### 6.5 Telemetry output

```cpp
monitor.printReport(Serial);        // human-readable multi-line
monitor.printJSON(Serial);          // compact JSON, schema-stable
monitor.printCSVHeader(Serial);     // header row, call once
monitor.printCSV(Serial);           // one row per sample
```

JSON output is compact (no whitespace) and starts with the protocol
header:

```json
{"protocol":"mtm-telemetry","version":1,"board":"ESP32",
 "uptime_ms":1247,"ram":{...},"cpu":{...},...}
```

CSV uses CRLF terminators. The header is emitted by `printCSVHeader()`
and lists 23 columns from `uptime_ms` through `reboot_count`.

<a name="alerts-and-thresholds"></a>
### 6.6 Alerts and thresholds

```cpp
monitor.setRAMWarning(80);          // % used
monitor.setRAMCritical(95);
monitor.setCPUWarning(90);
monitor.setCPUCritical(99);
monitor.setTemperatureWarning(70);   // degrees C
monitor.setTemperatureCritical(85);
monitor.setLowVoltageWarning(3300); // millivolts
monitor.setLowVoltageCritical(3000);
monitor.setStorageWarning(90);
monitor.setStorageCritical(98);
monitor.setStackWarning(128);       // bytes remaining
monitor.setStackCritical(32);

monitor.onWarning([](MonitorWarning w) {
    Serial.print("ALERT: source=");
    Serial.print(w.source);
    Serial.print(" severity=");
    Serial.print(w.severity);
    Serial.print(" value=");
    Serial.println(w.value);
});
```

The callback is invoked from `update()`, **never from an ISR**. It is
safe to call `Serial.print()` and any other user-context function from
inside the callback.

The alerts engine has built-in hysteresis: a metric that has just
raised a warning will not re-raise until it crosses the warning
threshold again. The default hysteresis is 2 percentage points (or
2 °C, or 100 mV). Clearing is conservative — alerts stay active until
the value drops well below the threshold.

<a name="transports"></a>
### 6.7 Transports

```cpp
SerialTransport serial(Serial);
monitor.addTransport(serial);
// every monitor.printJSON() etc. is mirrored to the transport
```

Built-in transports:

| Class            | Available on        | Header required by the sketch |
|------------------|---------------------|------------------------------|
| `SerialTransport`| every Arduino core | `<MicroTaskMonitor.h>` only  |
| `UARTTransport`  | every Arduino core | `<MicroTaskMonitor.h>` only  |
| `WiFiTransport` | ESP32, ESP8266, Pico W, SAMD-WINC | `<WiFi.h>` (user-supplied) |
| `BLETransport`  | ESP32 only         | `<BLEDevice.h>` (user-supplied) |

Custom transport:

```cpp
class MyTransport : public mtm::IMTMTransport {
public:
    void send(const char* buffer, size_t length) override {
        // ship to your destination
    }
    const char* name() const override { return "custom"; }
    bool ready() const override { return true; }
};

MyTransport myT;
monitor.addTransport(myT);
```

<a name="performance-profiler"></a>
### 6.8 Performance profiler

```cpp
{
    MTM_PROFILE_NAME(monitor, "loop");
    // ... your code ...
}   // automatic endScope on destruction
```

Or, with explicit calls:

```cpp
int8_t id = monitor.profiler().beginScope("loop");
// ... your code ...
monitor.profiler().endScope(id, startMicros, micros());
```

The profiler keeps min/max/avg/count per scope plus the last 32
samples per scope.

<a name="monitoring-modules"></a>
## 7. Monitoring modules

<a name="module-ram"></a>
### 7.1 RAM Monitor

| Field | Unit | Status |
|-------|------|--------|
| `total` | bytes | MEASURED on every platform |
| `used` | bytes | MEASURED |
| `free` | bytes | MEASURED |
| `minEverFree` | bytes | MEASURED (running minimum since `begin()`) |
| `peak` | bytes | MEASURED (`total - minEverFree`) |
| `fragmentation` | percent | MEASURED on ESP32 / ESP8266; NOT_SUPPORTED elsewhere |
| `largestFreeBlock` | bytes | MEASURED on ESP32 / ESP8266 |
| `psramTotal`, `psramUsed` | bytes | MEASURED only when the board has PSRAM |
| `stackSize`, `stackHighWater` | bytes | MEASURED on FreeRTOS cores |
| `allocFailures` | count | 0 on every backend; bumped by user-supplied hooks |

<a name="module-cpu"></a>
### 7.2 CPU Monitor

| Field | Unit | Status |
|-------|------|--------|
| `frequencyHz` | Hz | MEASURED |
| `usagePercent` | percent | MEASURED on FreeRTOS cores; ESTIMATE on AVR / ESP8266 / bare RP2040 |
| `coreCount` | count | MEASURED on multicore (ESP32 classic, S3, RP2040); 1 otherwise |
| `cores[i].usagePercent` | percent | MEASURED where multicore stats are exposed |
| `loadAvg1s / 5s / 60s` | percent | ESTIMATE — derived from the same source as `usagePercent` |

The CPU monitor **never** fabricates a number. On AVR the AVR libc has
no cycle counter, so `usagePercent` is reported as `0` with
`usageStatus = ESTIMATE` — never `MEASURED`. Use the profiler to get
real numbers for hot spots.

<a name="module-storage"></a>
### 7.3 Storage Monitor

```cpp
// Bind LittleFS so the storage monitor can report fs_used / fs_total.
LittleFS.begin();
monitor.bindFilesystem(&LittleFS, LittleFS.totalBytes());

// Bind SD so sd_used / sd_total appear in the snapshot.
SD.begin(SD_CS_PIN);
monitor.bindSD(&SD, SD.card()->cardSize() * 512);
```

<a name="module-gpu"></a>
### 7.4 GPU / Accelerator Monitor

Capability-based. Most MCUs report `NOT_SUPPORTED`. Forward-compatible
with future chips that expose real GPU / DMA counters.

<a name="module-temperature"></a>
### 7.5 Temperature Monitor

```cpp
// Use an external DS18B20 connected to GPIO 4:
int32_t readExternal() { /* ... */ return milliC; }
monitor.setExternalTemperatureSensor(readExternal);
```

<a name="module-power"></a>
### 7.6 Power Monitor

```cpp
// Plug in an external voltage / current sensor (e.g. INA219):
int32_t readMillivolts() { /* ... */ return mV; }
int32_t readMicroamps()  { /* ... */ return uA; }
monitor.setVoltageSensor(readMillivolts);
monitor.setCurrentSensor(readMicroamps);
```

<a name="module-rtos"></a>
### 7.7 RTOS Task Monitor

Only useful on FreeRTOS-enabled cores. On bare-metal AVR, ESP8266,
SAMD21 the snapshot reports `NOT_SUPPORTED` and your sketch should
gracefully skip the feature.

<a name="module-network"></a>
### 7.8 Network Monitor

The user must include `<WiFi.h>` themselves (the library is
dependency-free by design). Once `WiFi.begin()` has connected, the
network monitor reports the connection state, RSSI, IP, byte counters
and throughput.

<a name="module-runtime"></a>
### 7.9 Runtime Monitor

Reports uptime, reset reason (chip-specific), reboot counter (when
persistent storage is available) and whether the last reset was caused
by the watchdog.

<a name="module-profiler"></a>
### 7.10 Performance Profiler

```cpp
void expensive() {
    MTM_PROFILE_NAME(monitor, "expensive");
    // ...
}
```

Per-scope stats: `lastUs`, `minUs`, `maxUs`, `sumUs`, `count`, plus a
32-sample circular history. Up to 16 named scopes.

<a name="telemetry-protocol"></a>
## 8. Telemetry protocol

The JSON output uses a **stable, versioned schema**. See
[docs/TELEMETRY_PROTOCOL.md](docs/TELEMETRY_PROTOCOL.md) for the full
specification and JSON Schema.

The first three fields are always:

| Field | Type | Description |
|-------|------|-------------|
| `protocol` | string | Always `"mtm-telemetry"`. |
| `version` | int | Schema version. Incremented on backwards-incompatible changes. Currently `1`. |
| `board` | string | The platform backend's board name. |
| `uptime_ms` | int | System uptime in milliseconds. |

Subsequent fields mirror the per-module snapshots documented in
[MTMTypes.h](src/core/MTMTypes.h).

<a name="examples"></a>
## 9. Examples

The `examples/` folder contains eleven worked sketches:

| Sketch | Demonstrates |
|--------|--------------|
| `BasicMonitoring` | minimum setup, `printReport` |
| `RAMMonitoring` | `monitor.memory()` structured access |
| `CPUMonitoring` | CPU frequency, usage, multicore, load averages |
| `StorageMonitoring` | flash, LittleFS, SD, EEPROM |
| `JSONOutput` | `printJSON` streaming |
| `CSVOutput` | `printCSVHeader` + `printCSV` |
| `Alerts` | thresholds + `onWarning` callback |
| `HistoricalCharts` | streaming CSV for external charting |
| `RTOSMonitoring` | per-task FreeRTOS stats |
| `NetworkMonitoring` | Wi-Fi link, RSSI, throughput |
| `CustomTransport` | user-defined `IMTMTransport` |

<a name="performance-overhead"></a>
## 10. Performance overhead

Measured on a 16 MHz Arduino Uno (ATmega328P):

- `monitor.update()`: ~120 µs per call when no metrics are sampled,
  ~1.6 ms per call when a sample is taken.
- `monitor.printReport(Serial)` at 115200 baud: ~30 ms (dominated by
  the UART; the formatting itself is < 1 ms).
- Static RAM: 24 bytes for the engine, 12 bytes per enabled monitor
  module wrapper, plus the platform backend's statics. The total on
  AVR is < 200 bytes.
- Heap: 0 bytes allocated after `begin()`. The single heap allocation
  that ever happens is the backend set at `begin()` — about 80 bytes
  on AVR, 220 bytes on ESP32.

Measured on a 240 MHz ESP32:

- `monitor.update()` with all modules enabled and 1-second interval:
  ~350 µs per sampling tick (0.035 % CPU).
- JSON output: ~6 KB per snapshot, ~4 ms to serialise.

<a name="limitations"></a>
## 11. Limitations

- **AVR** has no on-chip CPU-utilisation counter; CPU usage is reported
  as `ESTIMATE` and derived from idle-loop sampling. Use the profiler
  for accurate hot-spot timing.
- **ESP32-C3 / C6** do not expose the internal temperature sensor via
  the standard IDF API. The temperature snapshot reports
  `NOT_SUPPORTED` on those variants.
- **ESP32 (classic, S2, S3)** temperature reading uses the legacy hall
  sensor workaround and is approximate (±5 °C). For precise readings
  use an external sensor.
- **STM32 flash wear** is not reported by the HAL. LittleFS / SPIFFS
  wear statistics are exposed only if the user binds the filesystem
  with the `totalBytes` set.
- **Network throughput** is computed from byte counters between
  consecutive `update()` calls. Short sampling intervals (< 200 ms)
  will produce noisy throughput numbers.
- **FreeRTOS stats** require `configUSE_TRACE_FACILITY` and
  `configGENERATE_RUN_STATS` to be enabled in the FreeRTOS
  configuration. ESP32 cores enable both by default; STM32 FreeRTOS
  users may need to enable them manually.

<a name="testing-and-validation"></a>
## 12. Testing and validation

The `tests/` directory contains a host-side test suite that does not
require any Arduino toolchain. Run it with:

```bash
cd tests
make
./mtm_tests
```

Coverage:

- Circular history buffer (push, indexing, statistics, copyOut).
- Snapshot defaults and null-backend behaviour.
- Telemetry serializers (report, JSON, CSV) with substring assertions
  and full JSON parse validation.
- Alerts engine: RAM warning/critical, hysteresis prevention,
  NOT_SUPPORTED silence, temperature thresholds.
- Performance profiler: scope count, min/max/avg/count, sum.
- Capability bitmask uniqueness.

Platform-specific compilation tests run via `pio run -e <env>` for
every supported board. See `platformio.ini` for the full list of
environments.

<a name="contributing"></a>
## 13. Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for
the full guide. Highlights:

- New platforms go in `src/platforms/<arch>/`. Implement every backend
  the platform can support; return `MTM_METRIC_NOT_SUPPORTED` from the
  rest.
- Every new metric must be guarded by a capability bit, not by
  `#ifdef`. The engine must work identically across platforms.
- Host-side unit tests are mandatory for new calculations, serializers,
  thresholds or history features.

<a name="license"></a>
## 14. License

Released under the [MIT License](LICENSE).
