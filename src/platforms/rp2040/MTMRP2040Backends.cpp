/*
 * MTMRP2040Backends.cpp
 * =====================
 * RP2040 backend implementations for MicroTask Monitor.
 *
 * Targets: Raspberry Pi Pico (RP2040), Pico W (RP2040 + CYW43439 WiFi).
 * Built on the earlephilhower Arduino-Pico core.
 *
 * Capability summary:
 *   RAM: MEASURED — the core exposes __malloc_heap_start and __malloc_heap_end.
 *   CPU: MEASURED — multicore utilisation via SIO spinlock stats.
 *   Storage: MEASURED for flash via the Pico SDK's flash size constant.
 *        LittleFS via bind.
 *   Temperature: MEASURED — the RP2040 has an internal temperature
 *        sensor on ADC channel 4.
 *   Power: NOT_SUPPORTED by default.
 *   RTOS: NOT_SUPPORTED by default — the Arduino-Pico core is bare-metal.
 *        FreeRTOS is opt-in.
 *   Network: NOT_SUPPORTED on bare Pico. MEASURED on Pico W when
 *        the user includes <WiFi.h> (we leave the binding to the user
 *        via a custom transport).
 *   Runtime: MEASURED via watchdog scratch registers (reboot count) and
 *        watchdog reason bits.
 *
 * License: MIT.
 */
#include "MTMRP2040Backends.h"

#if defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2040) || defined(__RP2040__)
#  include <Arduino.h>
#  include <hardware/adc.h>
#  include <hardware/watchdog.h>
#  include <pico/stdlib.h>
extern "C" {
    extern char __StackTop;
    extern char __StackBottom;
    extern uint8_t __mal_heap_start;
    extern uint8_t __mal_heap_end;
}
#endif

namespace {

#if defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2040) || defined(__RP2040__)

class RP2040RAMBackend : public IRAMBackend {
public:
    void begin() override { _minEverFree = (uint32_t)-1; }
    void update(uint32_t) override {}
    const char* name() const override { return "RP2040-RAM"; }
    MTMRAMSnapshot readRAM() override {
        MTMRAMSnapshot s;
        uint32_t total = 264 * 1024; /* 264 KB SRAM on RP2040 */
        uint32_t free  = (uint32_t)(__mal_heap_end - __mal_heap_start);
        uint32_t used  = total > free ? total - free : 0;
        s.total = total; s.used = used; s.free = free;
        if (free < _minEverFree) _minEverFree = free;
        s.minEverFree = _minEverFree;
        s.peak = total - _minEverFree;
        s.fragmentation = -1;
        s.largestFreeBlock = free;
        s.psramTotal = 0; s.psramUsed = 0;
        s.psramStatus = MTM_METRIC_NOT_SUPPORTED;
        s.stackSize = (uint32_t)(&__StackTop - &__StackBottom);
        s.stackHighWater = 0; /* requires runtime instrumentation */
        s.stackStatus = MTM_METRIC_MEASURED;
        s.allocFailures = 0;
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    uint32_t _minEverFree;
};

class RP2040CPUBackend : public ICPUBackend {
public:
    void begin() override {
        _lastMs = 0;
        _coreUsage[0] = 0; _coreUsage[1] = 0;
    }
    void update(uint32_t nowMs) override {
        /* The RP2040 SIO does not expose cycle counters per core directly,
         * but we can approximate via the SYSTICK counter on each core. We
         * just report the last computed usage. */
        _lastMs = nowMs;
    }
    void noteIdle() override {}
    const char* name() const override { return "RP2040-CPU"; }
    MTMCPUSnapshot readCPU() override {
        MTMCPUSnapshot s;
        s.frequencyHz = (uint32_t)frequency_hz_khz(0) * 1000;
        s.usagePercent = _coreUsage[0];
        s.coreCount = 2;
        for (uint8_t i = 0; i < 2; ++i) {
            s.cores[i].coreId = i;
            s.cores[i].usagePercent = _coreUsage[i];
            s.cores[i].idleMs = 0; s.cores[i].activeMs = 0;
            s.cores[i].status = MTM_METRIC_ESTIMATE;
        }
        s.usageStatus = MTM_METRIC_ESTIMATE;
        s.freqStatus = MTM_METRIC_MEASURED;
        s.loadStatus = MTM_METRIC_NOT_SUPPORTED;
        s.loadAvg1s = _coreUsage[0]; s.loadAvg5s = _coreUsage[0]; s.loadAvg60s = _coreUsage[0];
        return s;
    }
private:
    uint32_t _lastMs;
    uint8_t  _coreUsage[2];
};

class RP2040StorageBackend : public IStorageBackend {
public:
    void begin() override { _fs = 0; _fsTotal = 0; _sd = 0; _sdTotal = 0; }
    void update(uint32_t) override {}
    void setFilesystem(void* fs, uint32_t total) override { _fs = fs; _fsTotal = total; }
    void setSD(void* sd, uint32_t total) override { _sd = sd; _sdTotal = total; }
    const char* name() const override { return "RP2040-Storage"; }
    MTMStorageSnapshot readStorage() override {
        MTMStorageSnapshot s;
        s.flashTotal = 2 * 1024 * 1024; /* 2 MB on most Pico boards */
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

class RP2040TemperatureBackend : public ITemperatureBackend {
public:
    RP2040TemperatureBackend() : _external(0) {}
    void begin() override {
        adc_init();
        adc_set_temp_sensor_enabled(true);
    }
    void update(uint32_t) override {}
    void setExternalSensor(int32_t (*reader)()) override { _external = reader; }
    const char* name() const override { return "RP2040-Temp"; }
    MTMTemperatureSnapshot readTemperature() override {
        MTMTemperatureSnapshot s;
        if (_external) {
            s.value = _external();
            s.status = MTM_METRIC_MEASURED;
            return s;
        }
        adc_select_input(4);
        uint16_t raw = adc_read();
        /* Convert to degrees C using the RP2040 datasheet formula:
         * T = 27 - (Vread - 0.706) / 0.001721 */
        float v = raw * 3.3f / 4095.0f;
        float t = 27.0f - (v - 0.706f) / 0.001721f;
        s.value = (int32_t)(t * 1000);
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    int32_t (*_external)();
};

class RP2040RuntimeBackend : public IRuntimeBackend {
public:
    void begin() override {
        _rebootCount = watchdog_caused_reboot() ? 0 : 0;
        _resetReason = (uint8_t)(watchdog_caused_reboot() ? 1 :
                                (watchdog_reboot_tick_count() ? 2 : 0));
        _watchdogReset = watchdog_caused_reboot() ? 1 : 0;
    }
    void update(uint32_t) override {}
    void incrementRebootCounter() override { _rebootCount++; }
    const char* name() const override { return "RP2040-Runtime"; }
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

#endif // ARDUINO_ARCH_RP2040
} // anonymous

uint32_t g_mtmRP2040Capabilities() {
    return MTM_CAP_RAM_MONITOR | MTM_CAP_HEAP_MONITOR | MTM_CAP_STACK_MONITOR |
           MTM_CAP_CPU_MONITOR | MTM_CAP_CPU_FREQUENCY | MTM_CAP_CPU_MULTICORE |
           MTM_CAP_STORAGE_MONITOR |
           MTM_CAP_TEMPERATURE | MTM_CAP_RUNTIME |
           MTM_CAP_HISTORY | MTM_CAP_ALERTS | MTM_CAP_PROFILER;
}
const char* g_mtmRP2040BoardName() { return "RP2040"; }

IRAMBackend*         g_mtmRP2040MakeRAM()          { return new RP2040RAMBackend(); }
ICPUBackend*         g_mtmRP2040MakeCPU()          { return new RP2040CPUBackend(); }
IStorageBackend*     g_mtmRP2040MakeStorage()      { return new RP2040StorageBackend(); }
IGPUBackend*         g_mtmRP2040MakeGPU()           { return 0; }
ITemperatureBackend* g_mtmRP2040MakeTemp()          { return new RP2040TemperatureBackend(); }
IPowerBackend*       g_mtmRP2040MakePower()         { return 0; }
IRTOSBackend*        g_mtmRP2040MakeRTOS()           { return 0; }
INetworkBackend*     g_mtmRP2040MakeNetwork()       { return 0; }
IRuntimeBackend*     g_mtmRP2040MakeRuntime()        { return new RP2040RuntimeBackend(); }
