/*
 * MTMNullBackends.h
 * =================
 * "Null" backend implementations that report MTM_METRIC_NOT_SUPPORTED for
 * every metric. The platform factory uses these to fill in metrics a
 * particular platform cannot supply, so the engine never has to deal with
 * null pointers.
 *
 * The null backends are also used by host-side unit tests to verify the
 * behaviour of the engine when a metric is unavailable.
 *
 * License: MIT.
 */
#ifndef MTM_NULL_BACKENDS_H
#define MTM_NULL_BACKENDS_H

#include "MTMHalInterfaces.h"

class MTMNullRAMBackend : public IRAMBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "RAM(null)"; }
    MTMRAMSnapshot readRAM() override {
        MTMRAMSnapshot s;
        s.total = 0; s.used = 0; s.free = 0; s.minEverFree = 0; s.peak = 0;
        s.fragmentation = -1; s.largestFreeBlock = 0;
        s.psramTotal = 0; s.psramUsed = 0;
        s.stackSize = 0; s.stackHighWater = 0; s.allocFailures = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
        s.psramStatus = MTM_METRIC_NOT_SUPPORTED;
        s.stackStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class MTMNullCPUBackend : public ICPUBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "CPU(null)"; }
    MTMCPUSnapshot readCPU() override {
        MTMCPUSnapshot s;
        s.frequencyHz = 0; s.usagePercent = 0; s.coreCount = 0;
        for (int i = 0; i < MTM_MAX_CPU_CORES; ++i) {
            s.cores[i].coreId = 0; s.cores[i].usagePercent = 0;
            s.cores[i].idleMs = 0; s.cores[i].activeMs = 0;
            s.cores[i].status = MTM_METRIC_NOT_SUPPORTED;
        }
        s.loadAvg1s = 0; s.loadAvg5s = 0; s.loadAvg60s = 0;
        s.usageStatus = MTM_METRIC_NOT_SUPPORTED;
        s.freqStatus = MTM_METRIC_NOT_SUPPORTED;
        s.loadStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class MTMNullStorageBackend : public IStorageBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "Storage(null)"; }
    MTMStorageSnapshot readStorage() override {
        MTMStorageSnapshot s;
        s.flashTotal = 0; s.flashUsed = 0; s.fsTotal = 0; s.fsUsed = 0;
        s.sdTotal = 0; s.sdUsed = 0; s.eepromSize = 0; s.fsWriteCycles = 0;
        s.flashStatus = MTM_METRIC_NOT_SUPPORTED;
        s.fsStatus = MTM_METRIC_NOT_SUPPORTED;
        s.sdStatus = MTM_METRIC_NOT_SUPPORTED;
        s.eepromStatus = MTM_METRIC_NOT_SUPPORTED;
        s.wearStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class MTMNullGPUBackend : public IGPUBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "GPU(null)"; }
    MTMGPUSnapshot readGPU() override {
        MTMGPUSnapshot s;
        s.usagePercent = 0; s.dmaUsagePercent = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
        s.dmaStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class MTMNullTemperatureBackend : public ITemperatureBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "Temp(null)"; }
    MTMTemperatureSnapshot readTemperature() override {
        MTMTemperatureSnapshot s;
        s.value = 0; s.min = 0; s.max = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class MTMNullPowerBackend : public IPowerBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "Power(null)"; }
    MTMPowerSnapshot readPower() override {
        MTMPowerSnapshot s;
        s.voltage = 0; s.current = 0; s.powerMicrowatts = 0;
        s.batteryPercent = -1;
        s.voltageStatus = MTM_METRIC_NOT_SUPPORTED;
        s.currentStatus = MTM_METRIC_NOT_SUPPORTED;
        s.batteryStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class MTMNullRTOSBackend : public IRTOSBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "RTOS(null)"; }
    MTMRTOSSnapshot readRTOS() override {
        MTMRTOSSnapshot s;
        s.taskCount = 0; s.totalRuntime = 0;
        for (int i = 0; i < MTM_MAX_TASKS; ++i) {
            s.tasks[i].name[0] = 0; s.tasks[i].state = 0; s.tasks[i].priority = 0;
            s.tasks[i].stackHighWater = 0; s.tasks[i].runtime = 0;
            s.tasks[i].coreAffinity = 0;
        }
        s.status = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class MTMNullNetworkBackend : public INetworkBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "Network(null)"; }
    MTMNetworkSnapshot readNetwork() override {
        MTMNetworkSnapshot s;
        s.connected = 0; s.rssi = 0; s.ip = 0;
        s.bytesSent = 0; s.bytesReceived = 0; s.linkUptimeMs = 0;
        s.txThroughputBps = 0; s.rxThroughputBps = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

class MTMNullRuntimeBackend : public IRuntimeBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "Runtime(null)"; }
    MTMRuntimeSnapshot readRuntime() override {
        MTMRuntimeSnapshot s;
        s.uptimeMs = 0; s.rebootCount = 0; s.resetReason = 0; s.watchdogReset = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
};

#endif // MTM_NULL_BACKENDS_H
