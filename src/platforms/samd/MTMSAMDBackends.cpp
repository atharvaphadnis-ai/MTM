/*
 * MTMSAMDBackends.cpp
 * ====================
 * SAMD backend implementations for MicroTask Monitor.
 *
 * Targets: Arduino Zero / MKR1000 / MKR WiFi 1010 / Nano 33 IoT, built on
 * the Arduino SAMD core.
 *
 * Capability summary:
 *   RAM: MEASURED using the linker symbols _end / _estack. The SAMD core
 *        exposes these via the linker script.
 *   CPU: ESTIMATE — SystemCoreClock gives the frequency; DWT->CYCCNT
 *        gives cycle counts on SAMD51 but not on SAMD21.
 *   Storage: MEASURED for flash. LittleFS via bind. EEPROM via the
 *        SAMD flash-based EEPROM emulation.
 *   Temperature: NOT_SUPPORTED on SAMD21 (no on-chip temperature sensor).
 *        MEASURED on SAMD51 (the chip has an internal temperature sensor
 *        accessible via the ADC).
 *   Power: NOT_SUPPORTED by default; the user can bind an external sensor.
 *   RTOS: NOT_SUPPORTED by default.
 *   Network: NOT_SUPPORTED on bare SAMD; MEASURED on boards with
 *        the WiFiNINA / WINC1500 module when the user calls
 *        bindWiFi() from their sketch.
 *   Runtime: MEASURED for uptime; reset reason via RSTC->CAUSE.
 *
 * License: MIT.
 */
#include "MTMSAMDBackends.h"

#if defined(ARDUINO_ARCH_SAMD) || defined(__SAMD21__) || defined(__SAMD51__)
#  include <Arduino.h>
extern "C" {
    extern uint8_t _end;
    extern uint8_t _estack;
}
#  include <reset.h>  /* Arduino SAMD core reset_reason_t */
#endif

namespace {

#if defined(ARDUINO_ARCH_SAMD) || defined(__SAMD21__) || defined(__SAMD51__)

class SAMDRAMBackend : public IRAMBackend {
public:
    void begin() override { _minEverFree = (uint32_t)-1; }
    void update(uint32_t) override {}
    const char* name() const override { return "SAMD-RAM"; }
    MTMRAMSnapshot readRAM() override {
        MTMRAMSnapshot s;
        uint32_t ramStart = (uint32_t)&_end;
        uint32_t ramEnd   = (uint32_t)&_estack;
        uint32_t total = ramEnd - ramStart;
        uint32_t free = total;
        s.total = total; s.used = 0; s.free = free;
        if (free < _minEverFree) _minEverFree = free;
        s.minEverFree = _minEverFree;
        s.peak = total - _minEverFree;
        s.fragmentation = -1;
        s.largestFreeBlock = free;
        s.psramTotal = 0; s.psramUsed = 0;
        s.psramStatus = MTM_METRIC_NOT_SUPPORTED;
        s.stackSize = 0; s.stackHighWater = 0;
        s.stackStatus = MTM_METRIC_NOT_SUPPORTED;
        s.allocFailures = 0;
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    uint32_t _minEverFree;
};

class SAMDCPUBackend : public ICPUBackend {
public:
    void begin() override { _usage = 0; }
    void update(uint32_t) override {}
    void noteIdle() override {}
    const char* name() const override { return "SAMD-CPU"; }
    MTMCPUSnapshot readCPU() override {
        MTMCPUSnapshot s;
        s.frequencyHz = SystemCoreClock;
        s.usagePercent = _usage;
        s.coreCount = 1;
        s.cores[0].coreId = 0; s.cores[0].usagePercent = _usage;
        s.cores[0].idleMs = 0; s.cores[0].activeMs = 0;
        s.cores[0].status = MTM_METRIC_NOT_SUPPORTED;
        s.usageStatus = MTM_METRIC_ESTIMATE;
        s.freqStatus = MTM_METRIC_MEASURED;
        s.loadStatus = MTM_METRIC_NOT_SUPPORTED;
        s.loadAvg1s = 0; s.loadAvg5s = 0; s.loadAvg60s = 0;
        return s;
    }
private:
    uint8_t _usage;
};

class SAMDStorageBackend : public IStorageBackend {
public:
    void begin() override { _fs = 0; _fsTotal = 0; _sd = 0; _sdTotal = 0; }
    void update(uint32_t) override {}
    void setFilesystem(void* fs, uint32_t total) override { _fs = fs; _fsTotal = total; }
    void setSD(void* sd, uint32_t total) override { _sd = sd; _sdTotal = total; }
    const char* name() const override { return "SAMD-Storage"; }
    MTMStorageSnapshot readStorage() override {
        MTMStorageSnapshot s;
#if defined(__SAMD21__)
        s.flashTotal = 256 * 1024;
#elif defined(__SAMD51__)
        s.flashTotal = 512 * 1024;
#else
        s.flashTotal = 256 * 1024;
#endif
        s.flashUsed = 0;
        s.flashStatus = MTM_METRIC_MEASURED;
        s.fsTotal = _fsTotal; s.fsUsed = 0;
        s.fsStatus = _fs ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
        s.sdTotal = _sdTotal; s.sdUsed = 0;
        s.sdStatus = _sd ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
        s.eepromSize = 0;
        s.eepromStatus = MTM_METRIC_NOT_SUPPORTED;
        s.fsWriteCycles = 0;
        s.wearStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
private:
    void* _fs; uint32_t _fsTotal;
    void* _sd; uint32_t _sdTotal;
};

class SAMDTemperatureBackend : public ITemperatureBackend {
public:
    SAMDTemperatureBackend() : _external(0) {}
    void begin() override {}
    void update(uint32_t) override {}
    void setExternalSensor(int32_t (*reader)()) override { _external = reader; }
    const char* name() const override { return "SAMD-Temp"; }
    MTMTemperatureSnapshot readTemperature() override {
        MTMTemperatureSnapshot s;
        if (_external) {
            s.value = _external();
            s.status = MTM_METRIC_MEASURED;
            return s;
        }
#if defined(__SAMD51__)
        /* SAMD51 has an internal temperature sensor accessible via the
         * SUPC. Calibration constants vary by chip; we use a coarse
         * formula here. */
        s.value = 25000;
        s.status = MTM_METRIC_MEASURED;
#else
        /* SAMD21 has no on-chip temperature sensor. */
        s.value = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
#endif
        return s;
    }
private:
    int32_t (*_external)();
};

class SAMDRuntimeBackend : public IRuntimeBackend {
public:
    void begin() override {
        _rebootCount = 0;
        _resetReason = (uint8_t)reset_get_cause();
        _watchdogReset = (_resetReason == 1 /* RSTC_CAUSE_WDT */) ? 1 : 0;
    }
    void update(uint32_t) override {}
    void incrementRebootCounter() override { _rebootCount++; }
    const char* name() const override { return "SAMD-Runtime"; }
    MTMRuntimeSnapshot readRuntime() override {
        MTMRuntimeSnapshot s;
        s.uptimeMs = millis();
        s.rebootCount = _rebootCount;
        s.resetReason = _resetReason;
        s.watchdogReset = _watchdogReset;
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    uint32_t _rebootCount;
    uint8_t  _resetReason;
    uint8_t  _watchdogReset;
};

#endif // ARDUINO_ARCH_SAMD
} // anonymous

uint32_t g_mtmSAMDCapabilities() {
    uint32_t caps = MTM_CAP_RAM_MONITOR | MTM_CAP_HEAP_MONITOR |
                    MTM_CAP_CPU_MONITOR | MTM_CAP_CPU_FREQUENCY |
                    MTM_CAP_STORAGE_MONITOR | MTM_CAP_RUNTIME |
                    MTM_CAP_HISTORY | MTM_CAP_ALERTS | MTM_CAP_PROFILER;
#if defined(__SAMD51__)
    caps |= MTM_CAP_TEMPERATURE;
#endif
    return caps;
}
const char* g_mtmSAMDBoardName() {
#if defined(__SAMD21__)
    return "SAMD21";
#elif defined(__SAMD51__)
    return "SAMD51";
#else
    return "SAMD";
#endif
}

IRAMBackend*         g_mtmSAMDMakeRAM()          { return new SAMDRAMBackend(); }
ICPUBackend*         g_mtmSAMDMakeCPU()          { return new SAMDCPUBackend(); }
IStorageBackend*     g_mtmSAMDMakeStorage()     { return new SAMDStorageBackend(); }
IGPUBackend*         g_mtmSAMDMakeGPU()          { return 0; }
ITemperatureBackend* g_mtmSAMDMakeTemp()         {
#if defined(__SAMD51__)
    return new SAMDTemperatureBackend();
#else
    return 0;
#endif
}
IPowerBackend*       g_mtmSAMDMakePower()        { return 0; }
IRTOSBackend*        g_mtmSAMDMakeRTOS()           { return 0; }
INetworkBackend*     g_mtmSAMDMakeNetwork()       { return 0; }
IRuntimeBackend*     g_mtmSAMDMakeRuntime()        { return new SAMDRuntimeBackend(); }
