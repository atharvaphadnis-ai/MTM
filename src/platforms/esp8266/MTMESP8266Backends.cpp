/*
 * MTMESP8266Backends.cpp
 * =====================
 * ESP8266 backend implementations for MicroTask Monitor.
 *
 * The ESP8266 (NodeMCU, Wemos D1) is single-core Xtensa, has no PSRAM,
 * no internal temperature sensor and no FreeRTOS statistics exposed to
 * Arduino sketches — so RTOS is NOT_SUPPORTED.
 *
 * Capability summary:
 *   RAM: MEASURED via ESP.getFreeHeap() / ESP.getMaxFreeBlockSize() /
 *        ESP.getHeapFragmentation().
 *   CPU: ESTIMATE (frequency via ESP.getCpuFreqMHz).
 *   Storage: MEASURED (flash via ESP.getFlashChipSize; EEPROM via
 *        the ESP8266 EEPROM emulation; LittleFS / SPIFFS via bind).
 *   Temperature: NOT_SUPPORTED by default; external sensor hook.
 *   Power: NOT_SUPPORTED by default; external sensor hook.
 *   RTOS: NOT_SUPPORTED (FreeRTOS is not exposed to user sketches).
 *   Network: MEASURED when WiFi is connected.
 *   Runtime: MEASURED for uptime; reset reason via ESP.getResetInfoPtr.
 *
 * License: MIT.
 */
#include "MTMESP8266Backends.h"

#if defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
#  include <Arduino.h>
#  include <ESP.h>
#endif

namespace {

#if defined(ARDUINO_ARCH_ESP8266)

class ESP8266RAMBackend : public IRAMBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "ESP8266-RAM"; }
    MTMRAMSnapshot readRAM() override {
        MTMRAMSnapshot s;
        uint32_t total = 50000; /* approx user-heap size on ESP8266 */
        uint32_t free  = ESP.getFreeHeap();
        uint32_t used  = total > free ? total - free : 0;
        s.total = total; s.used = used; s.free = free;
        s.minEverFree = ESP.getMinFreeHeap();
        s.peak = total > s.minEverFree ? total - s.minEverFree : 0;
        s.fragmentation = (int32_t)ESP.getHeapFragmentation();
        s.largestFreeBlock = ESP.getMaxFreeBlockSize();
        s.psramTotal = 0; s.psramUsed = 0;
        s.psramStatus = MTM_METRIC_NOT_SUPPORTED;
        s.stackSize = 0;
        s.stackHighWater = 0;
        s.stackStatus = MTM_METRIC_NOT_SUPPORTED;
        s.allocFailures = 0;
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
};

class ESP8266CPUBackend : public ICPUBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    void noteIdle() override {}
    const char* name() const override { return "ESP8266-CPU"; }
    MTMCPUSnapshot readCPU() override {
        MTMCPUSnapshot s;
        s.frequencyHz = (uint32_t)ESP.getCpuFreqMHz() * 1000000u;
        s.usagePercent = 0; /* not measurable on ESP8266 without the SDK */
        s.coreCount = 1;
        s.cores[0].coreId = 0; s.cores[0].usagePercent = 0;
        s.cores[0].idleMs = 0; s.cores[0].activeMs = 0;
        s.cores[0].status = MTM_METRIC_NOT_SUPPORTED;
        s.usageStatus = MTM_METRIC_ESTIMATE;
        s.freqStatus = MTM_METRIC_MEASURED;
        s.loadStatus = MTM_METRIC_NOT_SUPPORTED;
        s.loadAvg1s = 0; s.loadAvg5s = 0; s.loadAvg60s = 0;
        return s;
    }
};

class ESP8266StorageBackend : public IStorageBackend {
public:
    void begin() override { _fs = 0; _fsTotal = 0; _sd = 0; _sdTotal = 0; }
    void update(uint32_t) override {}
    void setFilesystem(void* fs, uint32_t total) override { _fs = fs; _fsTotal = total; }
    void setSD(void* sd, uint32_t total) override { _sd = sd; _sdTotal = total; }
    const char* name() const override { return "ESP8266-Storage"; }
    MTMStorageSnapshot readStorage() override {
        MTMStorageSnapshot s;
        s.flashTotal = ESP.getFlashChipRealSize();
        s.flashUsed = 0; /* not knowable without the partition table */
        s.flashStatus = MTM_METRIC_MEASURED;
        s.fsTotal = _fsTotal; s.fsUsed = 0;
        s.fsStatus = _fs ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
        s.sdTotal = _sdTotal; s.sdUsed = 0;
        s.sdStatus = _sd ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
        s.eepromSize = 4096; /* ESP8266 EEPROM is flash-backed, 4KB */
        s.eepromStatus = MTM_METRIC_MEASURED;
        s.fsWriteCycles = 0;
        s.wearStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
private:
    void* _fs; uint32_t _fsTotal;
    void* _sd; uint32_t _sdTotal;
};

class ESP8266NetworkBackend : public INetworkBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "ESP8266-Network"; }
    MTMNetworkSnapshot readNetwork() override {
        MTMNetworkSnapshot s;
        s.connected = 0; s.rssi = 0; s.ip = 0;
        s.bytesSent = 0; s.bytesReceived = 0;
        s.txThroughputBps = 0; s.rxThroughputBps = 0;
        s.linkUptimeMs = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class ESP8266RuntimeBackend : public IRuntimeBackend {
public:
    void begin() override { _rebootCount = 0; }
    void update(uint32_t) override {}
    void incrementRebootCounter() override { _rebootCount++; }
    const char* name() const override { return "ESP8266-Runtime"; }
    MTMRuntimeSnapshot readRuntime() override {
        MTMRuntimeSnapshot s;
        s.uptimeMs = millis();
        s.rebootCount = _rebootCount;
        s.resetReason = (uint8_t)ESP.getResetInfoPtr()->reason;
        s.watchdogReset = (s.resetReason == 6 /* REASON_WDT */) ? 1 : 0;
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    uint32_t _rebootCount;
};

#endif // ARDUINO_ARCH_ESP8266
} // anonymous

uint32_t g_mtmESP8266Capabilities() {
    return MTM_CAP_RAM_MONITOR | MTM_CAP_HEAP_MONITOR | MTM_CAP_FRAGMENTATION |
           MTM_CAP_CPU_MONITOR | MTM_CAP_CPU_FREQUENCY |
           MTM_CAP_STORAGE_MONITOR | MTM_CAP_EEPROM_MONITOR |
           MTM_CAP_NETWORK | MTM_CAP_RUNTIME |
           MTM_CAP_HISTORY | MTM_CAP_ALERTS | MTM_CAP_PROFILER;
}
const char* g_mtmESP8266BoardName() { return "ESP8266"; }

IRAMBackend*         g_mtmESP8266MakeRAM()         { return new ESP8266RAMBackend(); }
ICPUBackend*         g_mtmESP8266MakeCPU()         { return new ESP8266CPUBackend(); }
IStorageBackend*     g_mtmESP8266MakeStorage()    { return new ESP8266StorageBackend(); }
IGPUBackend*         g_mtmESP8266MakeGPU()         { return 0; }
ITemperatureBackend* g_mtmESP8266MakeTemp()         { return 0; }
IPowerBackend*       g_mtmESP8266MakePower()        { return 0; }
IRTOSBackend*        g_mtmESP8266MakeRTOS()        { return 0; }
INetworkBackend*     g_mtmESP8266MakeNetwork()     { return new ESP8266NetworkBackend(); }
IRuntimeBackend*     g_mtmESP8266MakeRuntime()     { return new ESP8266RuntimeBackend(); }
