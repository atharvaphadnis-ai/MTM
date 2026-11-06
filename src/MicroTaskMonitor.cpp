/*
 * MicroTaskMonitor.cpp
 * ====================
 * Engine implementation for the MicroTask Monitor library.
 *
 * The engine:
 *   - Holds the platform-specific backends (allocated once at begin()).
 *   - Wraps each backend in a `mtm::…Monitor` that owns the per-module
 *     history and running minima.
 *   - Calls update() on each enabled module at the configured sampling
 *     interval.
 *   - Aggregates the per-module snapshots into the public
 *     MTMSystemSnapshot used by the serializers.
 *   - Runs the alerts engine against the snapshot and dispatches the
 *     user callback from the main loop context.
 *   - Forwards serialized telemetry to all registered transports.
 *
 * License: MIT.
 */
#include "MicroTaskMonitor.h"

#include "hal/MTMPlatformFactory.h"
#include "hal/MTMHalInterfaces.h"
#include "metrics/MTMMonitorBase.h"
#include "alerts/MTMAlerts.h"
#include "profiling/MTMProfiler.h"
#include "telemetry/MTMSerializers.h"
#include "transports/MTMTransports.h"

#if defined(ARDUINO)
#  include <Arduino.h>
#else
/* Host build shims. We use clock_gettime(CLOCK_MONOTONIC) so the first
 * call to millis() / micros() returns a non-zero value even when the
 * process has only been alive for a few microseconds — important for
 * the engine's first sampling tick. */
#  include <time.h>
static uint32_t _mtmHostMono() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
static uint32_t millis() { return _mtmHostMono(); }
static uint32_t micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}
#endif

#define MTM_MAX_TRANSPORTS 4

struct MicroTaskMonitor::Impl {
    MTMBackendSet backends;
    mtm::RAMMonitor         ram;
    mtm::CPUMonitor         cpu;
    mtm::StorageMonitor     storage;
    mtm::GPUMonitor         gpu;
    mtm::TemperatureMonitor temperature;
    mtm::PowerMonitor       power;
    mtm::RTOSMonitor         rtos;
    mtm::NetworkMonitor      network;
    mtm::RuntimeMonitor      runtime;

    mtm::AlertsEngine alerts;
    mtm::Profiler     profiler;

    mtm::IMTMTransport* transports[MTM_MAX_TRANSPORTS];
    uint8_t            transportCount;

    uint32_t capabilitiesMask;
    uint32_t samplingIntervalMs;
    uint32_t lastSampleMs;
    uint32_t beginMs;
    bool     begun;
    bool     firstSampleTaken;
    char     boardName[24];

    Impl() {
        for (uint8_t i = 0; i < MTM_MAX_TRANSPORTS; ++i) transports[i] = 0;
        transportCount = 0;
        samplingIntervalMs = 1000;
        lastSampleMs = 0;
        beginMs = 0;
        begun = false;
        firstSampleTaken = false;
        boardName[0] = 0;
        capabilitiesMask = 0;
    }
};

MicroTaskMonitor::MicroTaskMonitor() : _impl(new Impl()) {}

MicroTaskMonitor::~MicroTaskMonitor() {
    /* Free the platform backends. The Null backends are also heap-allocated. */
    delete _impl->backends.ram;
    delete _impl->backends.cpu;
    delete _impl->backends.storage;
    delete _impl->backends.gpu;
    delete _impl->backends.temperature;
    delete _impl->backends.power;
    delete _impl->backends.rtos;
    delete _impl->backends.network;
    delete _impl->backends.runtime;
    delete _impl;
}

void MicroTaskMonitor::begin() {
    if (_impl->begun) return;
    _impl->backends = mtmCreateBackends();
    _impl->capabilitiesMask = mtmPlatformCapabilities();

    /* Copy board name to our internal buffer so it is stable. */
    const char* n = mtmPlatformBoardName();
    uint8_t i = 0;
    if (n) while (i + 1 < sizeof(_impl->boardName) && n[i]) { _impl->boardName[i] = n[i]; ++i; }
    _impl->boardName[i] = 0;

    /* Bind each backend to its public monitor wrapper. */
    _impl->ram.bind(_impl->backends.ram);
    _impl->cpu.bind(_impl->backends.cpu);
    _impl->storage.bind(_impl->backends.storage);
    _impl->gpu.bind(_impl->backends.gpu);
    _impl->temperature.bind(_impl->backends.temperature);
    _impl->power.bind(_impl->backends.power);
    _impl->rtos.bind(_impl->backends.rtos);
    _impl->network.bind(_impl->backends.network);
    _impl->runtime.bind(_impl->backends.runtime);

    /* Begin each backend. */
    _impl->ram.begin();
    _impl->cpu.begin();
    _impl->storage.begin();
    _impl->gpu.begin();
    _impl->temperature.begin();
    _impl->power.begin();
    _impl->rtos.begin();
    _impl->network.begin();
    _impl->runtime.begin();

    /* If the runtime backend supports it, increment the persistent
     * reboot counter now. */
    _impl->runtime.incrementRebootCounter();

    _impl->beginMs = millis();
    _impl->lastSampleMs = 0;
    _impl->begun = true;
}

void MicroTaskMonitor::update() {
    if (!_impl->begun) return;
    uint32_t now = millis();
    if (now - _impl->lastSampleMs < _impl->samplingIntervalMs) return;
    _impl->lastSampleMs = now;

    _impl->ram.update(now);
    _impl->cpu.update(now);
    _impl->storage.update(now);
    _impl->gpu.update(now);
    _impl->temperature.update(now);
    _impl->power.update(now);
    _impl->rtos.update(now);
    _impl->network.update(now);
    _impl->runtime.update(now);

    _impl->firstSampleTaken = true;

    /* Evaluate alerts. */
    MTMSystemSnapshot s = snapshot();
    _impl->alerts.evaluate(s, now);
}

bool MicroTaskMonitor::available() const { return _impl->firstSampleTaken; }

void MicroTaskMonitor::setSamplingInterval(uint32_t intervalMs) {
    _impl->samplingIntervalMs = intervalMs;
}
uint32_t MicroTaskMonitor::samplingInterval() const { return _impl->samplingIntervalMs; }

void MicroTaskMonitor::enable(MTMCapability cap) {
    switch (cap) {
        case MTM_CAP_RAM_MONITOR:    case MTM_CAP_HEAP_MONITOR:
        case MTM_CAP_STACK_MONITOR:  case MTM_CAP_PSRAM_MONITOR:
        case MTM_CAP_FRAGMENTATION:
            _impl->ram.setEnabled(true); break;
        case MTM_CAP_CPU_MONITOR:    case MTM_CAP_CPU_FREQUENCY:
        case MTM_CAP_CPU_MULTICORE:  case MTM_CAP_CPU_LOAD_AVERAGES:
            _impl->cpu.setEnabled(true); break;
        case MTM_CAP_STORAGE_MONITOR: case MTM_CAP_FILESYSTEM:
        case MTM_CAP_SD_CARD:         case MTM_CAP_EEPROM_MONITOR:
        case MTM_CAP_FLASH_WEAR:
            _impl->storage.setEnabled(true); break;
        case MTM_CAP_GPU_MONITOR:    case MTM_CAP_DMA_MONITOR:
            _impl->gpu.setEnabled(true); break;
        case MTM_CAP_TEMPERATURE:
            _impl->temperature.setEnabled(true); break;
        case MTM_CAP_POWER:
            _impl->power.setEnabled(true); break;
        case MTM_CAP_RTOS:
            _impl->rtos.setEnabled(true); break;
        case MTM_CAP_NETWORK:
            _impl->network.setEnabled(true); break;
        case MTM_CAP_RUNTIME:
            _impl->runtime.setEnabled(true); break;
        default: break;
    }
}
void MicroTaskMonitor::disable(MTMCapability cap) {
    switch (cap) {
        case MTM_CAP_RAM_MONITOR:    case MTM_CAP_HEAP_MONITOR:
        case MTM_CAP_STACK_MONITOR:  case MTM_CAP_PSRAM_MONITOR:
        case MTM_CAP_FRAGMENTATION:
            _impl->ram.setEnabled(false); break;
        case MTM_CAP_CPU_MONITOR:    case MTM_CAP_CPU_FREQUENCY:
        case MTM_CAP_CPU_MULTICORE:  case MTM_CAP_CPU_LOAD_AVERAGES:
            _impl->cpu.setEnabled(false); break;
        case MTM_CAP_STORAGE_MONITOR: case MTM_CAP_FILESYSTEM:
        case MTM_CAP_SD_CARD:         case MTM_CAP_EEPROM_MONITOR:
        case MTM_CAP_FLASH_WEAR:
            _impl->storage.setEnabled(false); break;
        case MTM_CAP_GPU_MONITOR:    case MTM_CAP_DMA_MONITOR:
            _impl->gpu.setEnabled(false); break;
        case MTM_CAP_TEMPERATURE:
            _impl->temperature.setEnabled(false); break;
        case MTM_CAP_POWER:
            _impl->power.setEnabled(false); break;
        case MTM_CAP_RTOS:
            _impl->rtos.setEnabled(false); break;
        case MTM_CAP_NETWORK:
            _impl->network.setEnabled(false); break;
        case MTM_CAP_RUNTIME:
            _impl->runtime.setEnabled(false); break;
        default: break;
    }
}
bool MicroTaskMonitor::isEnabled(MTMCapability cap) const {
    switch (cap) {
        case MTM_CAP_RAM_MONITOR:    case MTM_CAP_HEAP_MONITOR:
        case MTM_CAP_STACK_MONITOR:  case MTM_CAP_PSRAM_MONITOR:
        case MTM_CAP_FRAGMENTATION:   return _impl->ram.enabled();
        case MTM_CAP_CPU_MONITOR:    case MTM_CAP_CPU_FREQUENCY:
        case MTM_CAP_CPU_MULTICORE:  case MTM_CAP_CPU_LOAD_AVERAGES:
                                       return _impl->cpu.enabled();
        case MTM_CAP_STORAGE_MONITOR: case MTM_CAP_FILESYSTEM:
        case MTM_CAP_SD_CARD:         case MTM_CAP_EEPROM_MONITOR:
        case MTM_CAP_FLASH_WEAR:      return _impl->storage.enabled();
        case MTM_CAP_GPU_MONITOR:    case MTM_CAP_DMA_MONITOR:
                                       return _impl->gpu.enabled();
        case MTM_CAP_TEMPERATURE:    return _impl->temperature.enabled();
        case MTM_CAP_POWER:           return _impl->power.enabled();
        case MTM_CAP_RTOS:            return _impl->rtos.enabled();
        case MTM_CAP_NETWORK:         return _impl->network.enabled();
        case MTM_CAP_RUNTIME:         return _impl->runtime.enabled();
        default: return false;
    }
}

uint32_t MicroTaskMonitor::capabilities() const { return _impl->capabilitiesMask; }
bool MicroTaskMonitor::isSupported(MTMCapability cap) const {
    return (_impl->capabilitiesMask & (uint32_t)cap) != 0;
}

const char* MicroTaskMonitor::boardName() const { return _impl->boardName; }

/* ---- per-module snapshots ---- */
MTMRAMSnapshot        MicroTaskMonitor::memory() const { return _impl->ram.read(); }
MTMRAMSnapshot        MicroTaskMonitor::ram() const    { return _impl->ram.read(); }
MTMCPUSnapshot        MicroTaskMonitor::cpu() const    { return _impl->cpu.read(); }
MTMStorageSnapshot    MicroTaskMonitor::storage() const { return _impl->storage.read(); }
MTMGPUSnapshot        MicroTaskMonitor::gpu() const    { return _impl->gpu.read(); }
MTMTemperatureSnapshot MicroTaskMonitor::temperature() const { return _impl->temperature.read(); }
MTMPowerSnapshot       MicroTaskMonitor::power() const { return _impl->power.read(); }
MTMRTOSSnapshot        MicroTaskMonitor::rtos() const   { return _impl->rtos.read(); }
MTMNetworkSnapshot     MicroTaskMonitor::network() const { return _impl->network.read(); }
MTMRuntimeSnapshot     MicroTaskMonitor::runtime() const { return _impl->runtime.read(); }

MicroTaskMonitor::RAMView MicroTaskMonitor::memoryView() const {
    MTMRAMSnapshot s = _impl->ram.read();
    RAMView v;
    v.total = s.total; v.used = s.used; v.free = s.free; v.peak = s.peak;
    v.fragmentation = s.fragmentation;
    v.status = s.status;
    return v;
}
MicroTaskMonitor::CPUView MicroTaskMonitor::cpuView() const {
    MTMCPUSnapshot s = _impl->cpu.read();
    CPUView v;
    v.usage = s.usagePercent;
    v.frequency = s.frequencyHz;
    v.status = s.usageStatus;
    return v;
}
MicroTaskMonitor::StorageView MicroTaskMonitor::storageView() const {
    MTMStorageSnapshot s = _impl->storage.read();
    StorageView v;
    v.total = s.flashTotal ? s.flashTotal : s.fsTotal;
    v.used  = s.flashTotal ? s.flashUsed  : s.fsUsed;
    v.free  = v.total > v.used ? v.total - v.used : 0;
    v.status = s.flashStatus != MTM_METRIC_NOT_SUPPORTED ? s.flashStatus : s.fsStatus;
    return v;
}
MicroTaskMonitor::TempView MicroTaskMonitor::temperatureView() const {
    MTMTemperatureSnapshot s = _impl->temperature.read();
    TempView v;
    v.value = s.value;
    v.status = s.status;
    return v;
}

MTMSystemSnapshot MicroTaskMonitor::snapshot() const {
    MTMSystemSnapshot s;
    s.uptimeMs = millis() - _impl->beginMs;
    /* Copy the board name. */
    uint8_t i = 0;
    while (i + 1 < sizeof(s.boardName) && _impl->boardName[i]) { s.boardName[i] = _impl->boardName[i]; ++i; }
    s.boardName[i] = 0;
    s.ram         = _impl->ram.read();
    s.cpu         = _impl->cpu.read();
    s.storage     = _impl->storage.read();
    s.gpu         = _impl->gpu.read();
    s.temperature = _impl->temperature.read();
    s.power       = _impl->power.read();
    s.rtos        = _impl->rtos.read();
    s.network     = _impl->network.read();
    s.runtime     = _impl->runtime.read();
    return s;
}

/* ---- telemetry ---- */
void MicroTaskMonitor::printReport(MTM_USER_PRINT& out) const {
    mtm::writeReport(out, snapshot());
}
void MicroTaskMonitor::printJSON(MTM_USER_PRINT& out) const {
    mtm::writeJSON(out, snapshot());
}
void MicroTaskMonitor::printCSV(MTM_USER_PRINT& out) const {
    mtm::writeCSV(out, snapshot());
}
void MicroTaskMonitor::printCSVHeader(MTM_USER_PRINT& out) const {
    mtm::writeCSVHeader(out);
}

/* ---- alerts ---- */
void MicroTaskMonitor::setRAMWarning(uint8_t percent)         { _impl->alerts.setRAMWarning(percent); }
void MicroTaskMonitor::setRAMCritical(uint8_t percent)        { _impl->alerts.setRAMCritical(percent); }
void MicroTaskMonitor::setHeapFreeWarning(uint32_t bytes)     { _impl->alerts.setHeapFreeWarning(bytes); }
void MicroTaskMonitor::setHeapFreeCritical(uint32_t bytes)    { _impl->alerts.setHeapFreeCritical(bytes); }
void MicroTaskMonitor::setCPUWarning(uint8_t percent)         { _impl->alerts.setCPUWarning(percent); }
void MicroTaskMonitor::setCPUCritical(uint8_t percent)        { _impl->alerts.setCPUCritical(percent); }
void MicroTaskMonitor::setTemperatureWarning(int32_t dC)      { _impl->alerts.setTemperatureWarning(dC * 1000); }
void MicroTaskMonitor::setTemperatureCritical(int32_t dC)     { _impl->alerts.setTemperatureCritical(dC * 1000); }
void MicroTaskMonitor::setLowVoltageWarning(int32_t mV)       { _impl->alerts.setLowVoltageWarning(mV); }
void MicroTaskMonitor::setLowVoltageCritical(int32_t mV)      { _impl->alerts.setLowVoltageCritical(mV); }
void MicroTaskMonitor::setStorageWarning(uint8_t percent)     { _impl->alerts.setStorageWarning(percent); }
void MicroTaskMonitor::setStorageCritical(uint8_t percent)    { _impl->alerts.setStorageCritical(percent); }
void MicroTaskMonitor::setStackWarning(uint16_t bytes)        { _impl->alerts.setStackWarning(bytes); }
void MicroTaskMonitor::setStackCritical(uint16_t bytes)       { _impl->alerts.setStackCritical(bytes); }

void MicroTaskMonitor::onWarning(void (*cb)(const MonitorWarning&)) {
    _impl->alerts.onAlert(cb);
}

/* ---- transports ---- */
void MicroTaskMonitor::addTransport(mtm::IMTMTransport& transport) {
    if (_impl->transportCount >= MTM_MAX_TRANSPORTS) return;
    _impl->transports[_impl->transportCount++] = &transport;
}
void MicroTaskMonitor::clearTransports() {
    _impl->transportCount = 0;
}
void MicroTaskMonitor::sendToTransports(const char* buffer, size_t length) {
    for (uint8_t i = 0; i < _impl->transportCount; ++i) {
        if (_impl->transports[i] && _impl->transports[i]->ready()) {
            _impl->transports[i]->send(buffer, length);
        }
    }
}

/* ---- profiler ---- */
mtm::Profiler& MicroTaskMonitor::profiler() { return _impl->profiler; }
mtm::Profiler* MicroTaskMonitor::profilerPtr() { return &_impl->profiler; }

/* ---- filesystem / SD ---- */
void MicroTaskMonitor::bindFilesystem(void* fs, uint32_t totalBytes) {
    _impl->storage.bindFilesystem(fs, totalBytes);
}
void MicroTaskMonitor::bindSD(void* sd, uint32_t totalBytes) {
    _impl->storage.bindSD(sd, totalBytes);
}

/* ---- external sensors ---- */
void MicroTaskMonitor::setExternalTemperatureSensor(int32_t (*reader)()) {
    _impl->temperature.setExternalSensor(reader);
}
void MicroTaskMonitor::setVoltageSensor(int32_t (*readerMillivolts)()) {
    _impl->power.setVoltageSensor(readerMillivolts);
}
void MicroTaskMonitor::setCurrentSensor(int32_t (*readerMicroamps)()) {
    _impl->power.setCurrentSensor(readerMicroamps);
}

/* ---- Profiler RAII helper: micros() stub ---- */
namespace mtm {
uint32_t ProfileScopeRAII::_mtmMicros() {
#if defined(ARDUINO)
    return ::micros();
#else
    return ::micros();
#endif
}
} // namespace mtm
