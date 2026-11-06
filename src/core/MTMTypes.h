/*
 * MTMTypes.h
 * ==========
 * Core public types shared by every MicroTask Monitor module.
 *
 * These types form the stable, versioned surface that platform backends,
 * serializers and user code all rely on. They MUST stay backward compatible
 * across patch releases.
 *
 * Design rules:
 *   - All types are POD (plain old data) where possible.
 *   - Every metric carries an `MTMMetricStatus` so callers can distinguish
 *     "measured", "estimated", "not supported" and "error".
 *   - Integer widths are explicit (uint32_t, int32_t, ...) so the same
 *     struct layout works on 8-bit AVR and 64-bit host tests.
 *   - Units are documented in the field doc comments.
 *
 * This file is part of MicroTask Monitor (MTM).
 * License: MIT — see LICENSE at the repository root.
 */
#ifndef MTM_TYPES_H
#define MTM_TYPES_H

#include <stdint.h>

/*
 * Library version exposed to user code and telemetry consumers.
 * Follows semantic versioning: MAJOR.MINOR.PATCH.
 */
#define MTM_VERSION_MAJOR 1
#define MTM_VERSION_MINOR 0
#define MTM_VERSION_PATCH 0
#define MTM_VERSION_STRING "1.0.0"

/*
 * Telemetry protocol version. Increment every time the JSON / CSV / binary
 * schema changes in a backwards-incompatible way. Desktop dashboards use
 * this field to decide which parser to invoke.
 */
#define MTM_PROTOCOL_VERSION 1

/*
 * Status of a single metric reading. Callers MUST check this before
 * interpreting the numeric `value` of any metric.
 *
 *   MTM_METRIC_MEASURED     - read from a real hardware counter or sensor
 *   MTM_METRIC_ESTIMATE     - derived from an indirect measurement
 *                            (e.g. AVR CPU usage from idle sampling)
 *   MTM_METRIC_NOT_SUPPORTED- the platform cannot provide this metric
 *   MTM_METRIC_DISABLED     - the user disabled this metric at runtime
 *   MTM_METRIC_ERROR        - the backend returned an error
 */
typedef enum : uint8_t {
    MTM_METRIC_MEASURED      = 0,
    MTM_METRIC_ESTIMATE      = 1,
    MTM_METRIC_NOT_SUPPORTED= 2,
    MTM_METRIC_DISABLED      = 3,
    MTM_METRIC_ERROR         = 4
} MTMMetricStatus;

/*
 * A single scalar reading with status. Used as a building block for the
 * larger `*Snapshot` structs below.
 */
struct MTMScalar {
    int32_t   value;   // Raw reading in the unit documented by the parent struct.
    MTMMetricStatus status;

    // Convenience: true when status == MTM_METRIC_MEASURED || ESTIMATE.
    bool isValid() const {
        return status == MTM_METRIC_MEASURED || status == MTM_METRIC_ESTIMATE;
    }
    bool isSupported() const {
        return status != MTM_METRIC_NOT_SUPPORTED;
    }
};

/*
 * Capability identifiers. These are the values passed to
 * MicroTaskMonitor::enable() / disable() / isSupported().
 *
 * Ordering is deliberate: related capabilities are grouped so that
 * capability-range checks (`>= RAM_MONITOR && <= STACK_MONITOR`) are cheap.
 *
 * NOTE: do not renumber existing entries in a patch release — user sketches
 * and telemetry consumers depend on these values.
 */
typedef enum : uint32_t {
    MTM_CAP_NONE                = 0,

    // Memory family
    MTM_CAP_RAM_MONITOR         = 0x0001,
    MTM_CAP_HEAP_MONITOR        = 0x0002,
    MTM_CAP_STACK_MONITOR       = 0x0004,
    MTM_CAP_PSRAM_MONITOR       = 0x0008,
    MTM_CAP_FRAGMENTATION       = 0x0010,

    // CPU family
    MTM_CAP_CPU_MONITOR         = 0x0020,
    MTM_CAP_CPU_FREQUENCY       = 0x0040,
    MTM_CAP_CPU_MULTICORE       = 0x0080,
    MTM_CAP_CPU_LOAD_AVERAGES   = 0x0100,

    // Storage family
    MTM_CAP_STORAGE_MONITOR    = 0x0200,
    MTM_CAP_FILESYSTEM          = 0x0400,
    MTM_CAP_SD_CARD             = 0x0800,
    MTM_CAP_EEPROM_MONITOR      = 0x1000,
    MTM_CAP_FLASH_WEAR          = 0x2000,

    // Accelerator family
    MTM_CAP_GPU_MONITOR         = 0x4000,
    MTM_CAP_DMA_MONITOR         = 0x8000,

    // Temperature / Power / RTOS / Network / Runtime / Profiler
    MTM_CAP_TEMPERATURE         = 0x00010000,
    MTM_CAP_POWER               = 0x00020000,
    MTM_CAP_RTOS                 = 0x00040000,
    MTM_CAP_NETWORK              = 0x00080000,
    MTM_CAP_RUNTIME              = 0x00100000,
    MTM_CAP_PROFILER             = 0x00200000,

    // History + alerts are always available, but listed for completeness.
    MTM_CAP_HISTORY             = 0x00400000,
    MTM_CAP_ALERTS              = 0x00800000,

    // Convenience aliases used by the user-facing example API.
    RAM_MONITOR         = MTM_CAP_RAM_MONITOR,
    HEAP_MONITOR        = MTM_CAP_HEAP_MONITOR,
    STACK_MONITOR       = MTM_CAP_STACK_MONITOR,
    PSRAM_MONITOR       = MTM_CAP_PSRAM_MONITOR,
    FRAGMENTATION       = MTM_CAP_FRAGMENTATION,
    CPU_MONITOR         = MTM_CAP_CPU_MONITOR,
    CPU_FREQUENCY       = MTM_CAP_CPU_FREQUENCY,
    CPU_MULTICORE       = MTM_CAP_CPU_MULTICORE,
    CPU_LOAD_AVERAGES   = MTM_CAP_CPU_LOAD_AVERAGES,
    STORAGE_MONITOR    = MTM_CAP_STORAGE_MONITOR,
    FILESYSTEM          = MTM_CAP_FILESYSTEM,
    SD_CARD             = MTM_CAP_SD_CARD,
    EEPROM_MONITOR      = MTM_CAP_EEPROM_MONITOR,
    FLASH_WEAR          = MTM_CAP_FLASH_WEAR,
    GPU_MONITOR         = MTM_CAP_GPU_MONITOR,
    DMA_MONITOR         = MTM_CAP_DMA_MONITOR,
    TEMPERATURE_MONITOR = MTM_CAP_TEMPERATURE,
    POWER_MONITOR       = MTM_CAP_POWER,
    RTOS_MONITOR         = MTM_CAP_RTOS,
    NETWORK_MONITOR      = MTM_CAP_NETWORK,
    RUNTIME_MONITOR      = MTM_CAP_RUNTIME,
    PROFILER_MONITOR    = MTM_CAP_PROFILER
} MTMCapability;

/*
 * Severity of an alert raised by the alerts engine.
 */
typedef enum : uint8_t {
    MTM_ALERT_INFO     = 0,
    MTM_ALERT_WARNING  = 1,
    MTM_ALERT_CRITICAL = 2
} MTMAlertSeverity;

/*
 * Source metric that triggered an alert. The values mirror MTMCapability
 * but are kept as a separate enum so the alerts engine can be extended
 * independently.
 */
typedef enum : uint8_t {
    MTM_ALERT_SOURCE_RAM              = 0,
    MTM_ALERT_SOURCE_HEAP_FREE        = 1,
    MTM_ALERT_SOURCE_HEAP_FRAGMENTATION = 2,
    MTM_ALERT_SOURCE_CPU              = 3,
    MTM_ALERT_SOURCE_TEMPERATURE      = 4,
    MTM_ALERT_SOURCE_VOLTAGE          = 5,
    MTM_ALERT_SOURCE_STORAGE          = 6,
    MTM_ALERT_SOURCE_STACK            = 7,
    MTM_ALERT_SOURCE_NETWORK          = 8,
    MTM_ALERT_SOURCE_WATCHDOG         = 9
} MTMAlertSource;

/*
 * A single alert event delivered to a user callback.
 */
struct MTMMonitorWarning {
    MTMAlertSource   source;
    MTMAlertSeverity severity;
    int32_t          value;     // Reading that triggered the alert.
    int32_t          threshold; // Threshold that was crossed.
    uint32_t         uptimeMs;  // System uptime at the time of the alert.
};

/*
 * RAM / heap snapshot.
 *
 * Units:
 *   - total, used, free, peak, largestFreeBlock, psramTotal, psramUsed,
 *     stackSize, stackHighWater: bytes
 *   - fragmentation: percent (0..100), -1 when not supported
 *   - minEverFree: bytes (running minimum of `free` since begin())
 *
 * status fields follow MTMMetricStatus. For PSRAM, when the platform has no
 * PSRAM the status is NOT_SUPPORTED and the values are zero.
 */
struct MTMRAMSnapshot {
    uint32_t total;            // Total SRAM in bytes (compile-time constant on most platforms).
    uint32_t used;             // Estimated bytes currently in use.
    uint32_t free;             // Estimated bytes free.
    uint32_t minEverFree;      // Minimum free RAM observed since begin().
    uint32_t peak;             // Peak utilisation in bytes (total - minEverFree).
    int32_t  fragmentation;    // Percent 0..100, or -1 when unsupported.
    uint32_t largestFreeBlock; // Largest contiguous free block, 0 if unknown.
    uint32_t psramTotal;
    uint32_t psramUsed;
    uint32_t stackSize;        // Stack size if known (RTOS), 0 otherwise.
    uint32_t stackHighWater;   // Remaining stack, in bytes.
    uint32_t allocFailures;    // Counter of allocation failures observed.
    MTMMetricStatus status;
    MTMMetricStatus psramStatus;
    MTMMetricStatus stackStatus;
};

/*
 * Per-core CPU statistics. `MTMCPUSnapshot` carries up to
 * MTM_MAX_CPU_CORES entries.
 */
#define MTM_MAX_CPU_CORES 2

struct MTMCPUCore {
    uint8_t  coreId;
    uint8_t  usagePercent;     // 0..100, or 0 when status != MEASURED/ESTIMATE.
    uint32_t idleMs;           // Cumulative idle time in ms.
    uint32_t activeMs;         // Cumulative active time in ms.
    MTMMetricStatus status;
};

struct MTMCPUSnapshot {
    uint32_t frequencyHz;      // Current CPU frequency.
    uint8_t  usagePercent;     // Aggregate CPU usage 0..100.
    uint8_t  coreCount;
    MTMCPUCore cores[MTM_MAX_CPU_CORES];
    uint32_t loadAvg1s;        // 1-second load average (0..100 percent).
    uint32_t loadAvg5s;
    uint32_t loadAvg60s;
    MTMMetricStatus usageStatus;   // MEASURED vs ESTIMATE.
    MTMMetricStatus freqStatus;
    MTMMetricStatus loadStatus;
};

/*
 * Storage snapshot. Each member is independent because some platforms
 * expose flash but no filesystem, or vice-versa.
 */
struct MTMStorageSnapshot {
    uint32_t flashTotal;        // Total flash in bytes.
    uint32_t flashUsed;         // Program + static data, where available.
    uint32_t fsTotal;           // Filesystem capacity, 0 if no FS mounted.
    uint32_t fsUsed;
    uint32_t sdTotal;           // SD card capacity, 0 if no card.
    uint32_t sdUsed;
    uint16_t eepromSize;        // EEPROM size in bytes (0 if none).
    uint32_t fsWriteCycles;     // Wear counter, or 0 when unknown.
    MTMMetricStatus flashStatus;
    MTMMetricStatus fsStatus;
    MTMMetricStatus sdStatus;
    MTMMetricStatus eepromStatus;
    MTMMetricStatus wearStatus;
};

/*
 * Temperature snapshot. `value` is in millidegrees Celsius (so 25.0 °C is
 * 25000). This keeps the integer precision high enough for telemetry
 * without using float.
 */
struct MTMTemperatureSnapshot {
    int32_t  value;             // millidegrees Celsius
    int32_t  min;               // rolling minimum since begin()
    int32_t  max;               // rolling maximum
    MTMMetricStatus status;
};

/*
 * Power snapshot.
 *   - voltage: millivolts
 *   - current: microamps (when measurable)
 *   - powerMicrowatts: derived power
 *   - batteryPercent: 0..100, or -1 when not available
 */
struct MTMPowerSnapshot {
    int32_t  voltage;
    int32_t  current;
    int32_t  powerMicrowatts;
    int32_t  batteryPercent;
    MTMMetricStatus voltageStatus;
    MTMMetricStatus currentStatus;
    MTMMetricStatus batteryStatus;
};

/*
 * GPU / accelerator. Most MCUs do not have a GPU; the structure is present
 * for forward compatibility.
 */
struct MTMGPUSnapshot {
    uint8_t  usagePercent;
    uint8_t  dmaUsagePercent;
    MTMMetricStatus status;
    MTMMetricStatus dmaStatus;
};

/*
 * RTOS task entry. The number of tasks is bounded by MTM_MAX_TASKS.
 */
#define MTM_MAX_TASKS 16

struct MTMTaskInfo {
    char     name[16];          // Truncated to 15 chars + NUL.
    uint8_t  state;             // FreeRTOS: eTaskState.
    uint8_t  priority;
    uint16_t stackHighWater;    // Bytes remaining.
    uint32_t runtime;           // Total runtime in ticks, 0 if unsupported.
    uint8_t  coreAffinity;      // Bitmask, 0 if unknown.
};

struct MTMRTOSSnapshot {
    uint8_t  taskCount;
    uint32_t totalRuntime;
    MTMTaskInfo tasks[MTM_MAX_TASKS];
    MTMMetricStatus status;
};

/*
 * Network snapshot. `linkUptimeMs` is 0 if not connected.
 */
struct MTMNetworkSnapshot {
    uint8_t  connected;         // 0 / 1
    int8_t   rssi;               // dBm, 0 when not connected.
    uint32_t ip;                 // IPv4 in network byte order, 0 if none.
    uint32_t bytesSent;
    uint32_t bytesReceived;
    uint32_t linkUptimeMs;
    uint32_t txThroughputBps;
    uint32_t rxThroughputBps;
    MTMMetricStatus status;
};

/*
 * Runtime snapshot. `resetReason` is platform-specific (see platform docs).
 */
struct MTMRuntimeSnapshot {
    uint32_t uptimeMs;
    uint32_t rebootCount;
    uint8_t  resetReason;       // 0 = unknown.
    uint8_t  watchdogReset;     // 1 if last reset was caused by WDT.
    MTMMetricStatus status;
};

/*
 * Top-level system snapshot returned by `MicroTaskMonitor::snapshot()`.
 * Each sub-snapshot is independent — fields are NOT_SUPPORTED on platforms
 * that cannot supply them.
 */
struct MTMSystemSnapshot {
    uint32_t uptimeMs;
    char     boardName[24];
    MTMRAMSnapshot        ram;
    MTMCPUSnapshot        cpu;
    MTMStorageSnapshot    storage;
    MTMTemperatureSnapshot temperature;
    MTMPowerSnapshot       power;
    MTMGPUSnapshot         gpu;
    MTMRTOSSnapshot        rtos;
    MTMNetworkSnapshot     network;
    MTMRuntimeSnapshot     runtime;
};

#endif // MTM_TYPES_H
