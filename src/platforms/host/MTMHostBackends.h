/*
 * MTMHostBackends.h
 * =================
 * Host backend factory declarations (used by unit tests).
 *
 * The host backend does NOT measure real hardware — it returns
 * configurable values so unit tests can exercise the engine and
 * serializers deterministically. It is NOT included in the Arduino
 * build (the platform factory only includes it when no embedded
 * toolchain is detected).
 *
 * License: MIT.
 */
#ifndef MTM_HOST_BACKENDS_H
#define MTM_HOST_BACKENDS_H
#include "hal/MTMHalInterfaces.h"
#include "hal/MTMNullBackends.h"

namespace mtm {

class HostRAMBackend : public IRAMBackend {
public:
    HostRAMBackend() : _total(64*1024), _used(8*1024) {}
    void setValues(uint32_t total, uint32_t used) { _total = total; _used = used; }
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "Host-RAM"; }
    MTMRAMSnapshot readRAM() override {
        MTMRAMSnapshot s;
        s.total = _total; s.used = _used;
        s.free = _total > _used ? _total - _used : 0;
        s.minEverFree = s.free;
        s.peak = _used;
        s.fragmentation = 12;
        s.largestFreeBlock = s.free;
        s.psramTotal = 0; s.psramUsed = 0;
        s.psramStatus = MTM_METRIC_NOT_SUPPORTED;
        s.stackSize = 4096; s.stackHighWater = 1024;
        s.stackStatus = MTM_METRIC_MEASURED;
        s.allocFailures = 0;
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    uint32_t _total, _used;
};

class HostCPUBackend : public ICPUBackend {
public:
    HostCPUBackend() : _freq(240000000), _usage(42), _coreCount(2) {}
    void setValues(uint32_t freq, uint8_t usage, uint8_t coreCount) {
        _freq = freq; _usage = usage; _coreCount = coreCount;
    }
    void begin() override {}
    void update(uint32_t) override {}
    void noteIdle() override {}
    const char* name() const override { return "Host-CPU"; }
    MTMCPUSnapshot readCPU() override {
        MTMCPUSnapshot s;
        s.frequencyHz = _freq;
        s.usagePercent = _usage;
        s.coreCount = _coreCount > MTM_MAX_CPU_CORES ? MTM_MAX_CPU_CORES : _coreCount;
        for (uint8_t i = 0; i < s.coreCount; ++i) {
            s.cores[i].coreId = i;
            s.cores[i].usagePercent = _usage;
            s.cores[i].idleMs = 1000; s.cores[i].activeMs = 2000;
            s.cores[i].status = MTM_METRIC_MEASURED;
        }
        s.usageStatus = MTM_METRIC_MEASURED;
        s.freqStatus = MTM_METRIC_MEASURED;
        s.loadStatus = MTM_METRIC_ESTIMATE;
        s.loadAvg1s = _usage; s.loadAvg5s = _usage; s.loadAvg60s = _usage;
        return s;
    }
private:
    uint32_t _freq;
    uint8_t  _usage, _coreCount;
};

class HostStorageBackend : public IStorageBackend {
public:
    HostStorageBackend() : _flashTotal(4*1024*1024), _fsTotal(0), _sdTotal(0) {}
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "Host-Storage"; }
    MTMStorageSnapshot readStorage() override {
        MTMStorageSnapshot s;
        s.flashTotal = _flashTotal; s.flashUsed = _flashTotal / 3;
        s.flashStatus = MTM_METRIC_MEASURED;
        s.fsTotal = _fsTotal; s.fsUsed = 0;
        s.fsStatus = _fsTotal ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
        s.sdTotal = _sdTotal; s.sdUsed = 0;
        s.sdStatus = _sdTotal ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
        s.eepromSize = 4096;
        s.eepromStatus = MTM_METRIC_MEASURED;
        s.fsWriteCycles = 0;
        s.wearStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
private:
    uint32_t _flashTotal, _fsTotal, _sdTotal;
};

class HostTemperatureBackend : public ITemperatureBackend {
public:
    HostTemperatureBackend() : _external(0), _value(25000) {}
    void setMilliC(int32_t v) { _value = v; }
    void begin() override {}
    void update(uint32_t) override {}
    void setExternalSensor(int32_t (*r)()) override { _external = r; }
    const char* name() const override { return "Host-Temp"; }
    MTMTemperatureSnapshot readTemperature() override {
        MTMTemperatureSnapshot s;
        s.value = _external ? _external() : _value;
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    int32_t (*_external)();
    int32_t _value;
};

} // namespace mtm

#endif
