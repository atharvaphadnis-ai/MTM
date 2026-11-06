/*
 * MicroTaskMonitor.h
 * ==================
 * Public entry point of the MicroTask Monitor (MTM) library.
 *
 * Usage:
 *
 *   #include <MicroTaskMonitor.h>
 *   MicroTaskMonitor monitor;
 *
 *   void setup() {
 *       Serial.begin(115200);
 *       monitor.begin();
 *       monitor.setSamplingInterval(1000);
 *       monitor.enable(RAM_MONITOR);
 *       monitor.enable(CPU_MONITOR);
 *       monitor.enable(TEMPERATURE_MONITOR);
 *   }
 *
 *   void loop() {
 *       monitor.update();
 *       if (monitor.available()) {
 *           monitor.printReport(Serial);
 *       }
 *   }
 *
 * Individual metric access:
 *
 *   auto ram = monitor.memory();
 *   Serial.println(ram.total);
 *   Serial.println(ram.free);
 *
 *   auto cpu = monitor.cpu();
 *   Serial.println(cpu.usage);
 *   Serial.println(cpu.frequency);
 *
 *   auto storage = monitor.storage();
 *   Serial.println(storage.total);
 *   Serial.println(storage.free);
 *
 *   auto temperature = monitor.temperature();
 *   Serial.println(temperature.value);
 *
 *   auto system = monitor.snapshot();
 *
 * Capability queries:
 *
 *   monitor.capabilities();
 *   monitor.isSupported(CPU_MONITOR);
 *   monitor.isSupported(PSRAM_MONITOR);
 *
 * Thresholds:
 *
 *   monitor.setRAMWarning(80);
 *   monitor.setCPUWarning(90);
 *   monitor.setTemperatureWarning(70);    // degrees C
 *   monitor.onWarning([](MonitorWarning w) {
 *       Serial.println("Warning raised!");
 *   });
 *
 * Transports:
 *
 *   monitor.addTransport(SerialTransport(Serial));
 *
 * License: MIT.
 */
#ifndef MICRO_TASK_MONITOR_H
#define MICRO_TASK_MONITOR_H

#include <stdint.h>
#include <stddef.h>

/* Public types — pulled in first so user code does not have to include
 * core/MTMTypes.h explicitly. */
#include "core/MTMTypes.h"

/* The Print base class used by the public API. On Arduino this is
 * Arduino's `Print`. On host builds (unit tests / dry runs) we use
 * the same minimal stub declared in MTMSerializers.h. */
#include "telemetry/MTMSerializers.h"
#define MTM_USER_PRINT MTM_PRINT_BASE

/* Capability enum is also in MTMTypes.h (RAM_MONITOR etc.). */

/* Re-export the warning type under the user-facing name used in the
 * README examples. */
typedef MTMMonitorWarning MonitorWarning;

namespace mtm {
class AlertsEngine;
class Profiler;
class IMTMTransport;

/* Forward declaration of monitor modules used by value below. */
class RAMMonitor;
class CPUMonitor;
class StorageMonitor;
class GPUMonitor;
class TemperatureMonitor;
class PowerMonitor;
class RTOSMonitor;
class NetworkMonitor;
class RuntimeMonitor;
} // namespace mtm

/*
 * The main engine class. Owns the platform backends and the monitor
 * module wrappers. Always create exactly ONE instance of this class.
 */
class MicroTaskMonitor {
public:
    MicroTaskMonitor();
    ~MicroTaskMonitor();

    /* One-time initialisation. Detects the platform, instantiates the
     * backends and primes the first sample. */
    void begin();

    /* Tick the engine. Call from loop(). The engine samples at most once
     * every `setSamplingInterval()` milliseconds. */
    void update();

    /* True once the first sample has been taken. */
    bool available() const;

    /* Sampling cadence in milliseconds. Default 1000. */
    void setSamplingInterval(uint32_t intervalMs);
    uint32_t samplingInterval() const;

    /* Enable / disable a monitoring module by capability. Disabled
     * modules report MTM_METRIC_DISABLED in their status fields. */
    void enable(MTMCapability cap);
    void disable(MTMCapability cap);
    bool isEnabled(MTMCapability cap) const;

    /* Capability queries. `capabilities()` returns the platform's static
     * capability bitmask. `isSupported()` returns true if the platform
     * can produce the requested metric (independent of whether the user
     * has enabled it). */
    uint32_t capabilities() const;
    bool isSupported(MTMCapability cap) const;

    /* The board name as reported by the platform backend
     * (e.g. "ESP32", "AVR", "STM32F4"). */
    const char* boardName() const;

    /* ---- individual metric access ---- */
    MTMRAMSnapshot        memory() const;       // alias of ram()
    MTMRAMSnapshot        ram() const;
    MTMCPUSnapshot        cpu() const;
    MTMStorageSnapshot    storage() const;
    MTMGPUSnapshot        gpu() const;
    MTMTemperatureSnapshot temperature() const;
    MTMPowerSnapshot       power() const;
    MTMRTOSSnapshot        rtos() const;
    MTMNetworkSnapshot     network() const;
    MTMRuntimeSnapshot     runtime() const;

    /* Convenience accessors that match the example API in the README.
     * These return POD structs with the most commonly used fields
     * exposed by name (`ram.used`, `cpu.usage`, `cpu.frequency`, etc.). */
    struct RAMView {
        uint32_t total, used, free, peak;
        int32_t  fragmentation;
        MTMMetricStatus status;
    };
    RAMView memoryView() const;

    struct CPUView {
        uint8_t  usage;
        uint32_t frequency;
        MTMMetricStatus status;
    };
    CPUView cpuView() const;

    struct StorageView {
        uint32_t total, used, free;
        MTMMetricStatus status;
    };
    StorageView storageView() const;

    struct TempView {
        int32_t value; // millidegrees C
        MTMMetricStatus status;
    };
    TempView temperatureView() const;

    /* Top-level snapshot — a copy of the internal system snapshot. */
    MTMSystemSnapshot snapshot() const;

    /* ---- telemetry output ---- */
    void printReport(MTM_USER_PRINT& out) const;
    void printJSON(MTM_USER_PRINT& out) const;
    void printCSV(MTM_USER_PRINT& out) const;
    void printCSVHeader(MTM_USER_PRINT& out) const;

    /* ---- alerts / thresholds ---- */
    void setRAMWarning(uint8_t percent);
    void setRAMCritical(uint8_t percent);
    void setHeapFreeWarning(uint32_t bytes);
    void setHeapFreeCritical(uint32_t bytes);
    void setCPUWarning(uint8_t percent);
    void setCPUCritical(uint8_t percent);
    void setTemperatureWarning(int32_t degreesC);
    void setTemperatureCritical(int32_t degreesC);
    void setLowVoltageWarning(int32_t millivolts);
    void setLowVoltageCritical(int32_t millivolts);
    void setStorageWarning(uint8_t percent);
    void setStorageCritical(uint8_t percent);
    void setStackWarning(uint16_t bytesRemaining);
    void setStackCritical(uint16_t bytesRemaining);

    /* Install a user callback. The callback runs from `update()` —
     * never from an ISR. */
    void onWarning(void (*cb)(const MonitorWarning&));

    /* ---- transports ---- */
    void addTransport(mtm::IMTMTransport& transport);
    void clearTransports();
    void sendToTransports(const char* buffer, size_t length);

    /* ---- profiler ---- */
    mtm::Profiler& profiler();
    /* Used by the MTM_PROFILE_NAME macro. Do not call directly. */
    mtm::Profiler* profilerPtr();

    /* ---- filesystem / SD binding ---- */
    void bindFilesystem(void* fs, uint32_t totalBytes);
    void bindSD(void* sd, uint32_t totalBytes);

    /* ---- external sensors ---- */
    void setExternalTemperatureSensor(int32_t (*reader)());
    void setVoltageSensor(int32_t (*readerMillivolts)());
    void setCurrentSensor(int32_t (*readerMicroamps)());

private:
    /* No copy. */
    MicroTaskMonitor(const MicroTaskMonitor&);
    MicroTaskMonitor& operator=(const MicroTaskMonitor&);

    struct Impl;
    Impl* _impl;
};

/* Convenience macro for scoped profiling. Requires a MicroTaskMonitor
 * instance (uses its internal profiler). */
#define MTM_PROFILE_NAME(monitor, name) \
    ::mtm::ProfileScopeRAII _mtm_prof_scope_(monitor.profilerPtr(), name)

#endif // MICRO_TASK_MONITOR_H
