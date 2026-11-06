/*
 * MTMMonitorBase.h
 * ===============
 * Thin wrapper classes that adapt the platform-agnostic backend
 * interfaces to the public `MTM*Monitor` classes the user includes via
 * MicroTaskMonitor.h.
 *
 * The wrappers add three things on top of the raw backend:
 *   1. History bookkeeping (sampling pushes into a fixed circular buffer)
 *   2. Threshold checks (delegate to the alerts engine)
 *   3. Convenience accessors for the user-facing API
 *
 * The wrappers do not own the backends — the MTM engine owns them.
 *
 * License: MIT.
 */
#ifndef MTM_MONITOR_BASE_H
#define MTM_MONITOR_BASE_H

#include "../core/MTMTypes.h"
#include "../hal/MTMHalInterfaces.h"
#include "../history/MTMHistory.h"

namespace mtm {

/*
 * Generic monitor wrapper. Subclasses bind a specific backend interface.
 */
class MonitorModule {
public:
    virtual ~MonitorModule() {}
    virtual void begin() = 0;
    virtual void update(uint32_t nowMs) = 0;
    virtual const char* name() const = 0;
};

/* ------------------------------------------------------------------ */
/* RAM                                                                 */
/* ------------------------------------------------------------------ */
class RAMMonitor : public MonitorModule {
public:
    RAMMonitor() : _backend(0), _minEverFree((uint32_t)-1), _peak(0), _enabled(true) {}

    void bind(IRAMBackend* b) { _backend = b; }

    void begin() override {
        if (_backend) _backend->begin();
        /* Prime minEverFree from the first reading. */
        MTMRAMSnapshot s = read();
        if (s.status != MTM_METRIC_NOT_SUPPORTED && s.free > 0) {
            _minEverFree = s.free;
        }
    }

    void update(uint32_t nowMs) override {
        (void)nowMs;
        if (!_enabled || !_backend) return;
        _backend->update(nowMs);
        MTMRAMSnapshot s = read();
        if (s.status != MTM_METRIC_NOT_SUPPORTED) {
            if (s.free < _minEverFree) _minEverFree = s.free;
            uint32_t usedPeak = s.total - _minEverFree;
            if (usedPeak > _peak) _peak = usedPeak;
        }
    }

    MTMRAMSnapshot read() const {
        if (!_backend) {
            MTMRAMSnapshot s; s.status = MTM_METRIC_NOT_SUPPORTED; return s;
        }
        MTMRAMSnapshot s = _backend->readRAM();
        /* Fill the engine-level running minima onto the backend's snapshot. */
        if (s.status != MTM_METRIC_NOT_SUPPORTED) {
            s.minEverFree = _minEverFree;
            s.peak = _peak;
        }
        return s;
    }

    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }

    const char* name() const override { return "RAM"; }

private:
    IRAMBackend* _backend;
    uint32_t    _minEverFree;
    uint32_t    _peak;
    bool        _enabled;
};

/* ------------------------------------------------------------------ */
/* CPU                                                                 */
/* ------------------------------------------------------------------ */
class CPUMonitor : public MonitorModule {
public:
    CPUMonitor() : _backend(0), _enabled(true) {}

    void bind(ICPUBackend* b) { _backend = b; }

    void begin() override { if (_backend) _backend->begin(); }
    void update(uint32_t nowMs) override {
        if (!_enabled || !_backend) return;
        _backend->update(nowMs);
    }
    MTMCPUSnapshot read() const {
        if (!_backend) { MTMCPUSnapshot s; s.usageStatus = MTM_METRIC_NOT_SUPPORTED; return s; }
        return _backend->readCPU();
    }

    void noteIdle() { if (_backend) _backend->noteIdle(); }

    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }

    const char* name() const override { return "CPU"; }

private:
    ICPUBackend* _backend;
    bool        _enabled;
};

/* ------------------------------------------------------------------ */
/* Storage                                                             */
/* ------------------------------------------------------------------ */
class StorageMonitor : public MonitorModule {
public:
    StorageMonitor() : _backend(0), _enabled(true) {}

    void bind(IStorageBackend* b) { _backend = b; }

    void begin() override { if (_backend) _backend->begin(); }
    void update(uint32_t nowMs) override { if (_enabled && _backend) _backend->update(nowMs); }
    MTMStorageSnapshot read() const {
        if (!_backend) { MTMStorageSnapshot s; s.flashStatus = MTM_METRIC_NOT_SUPPORTED; return s; }
        return _backend->readStorage();
    }

    void bindFilesystem(void* fs, uint32_t total) { if (_backend) _backend->setFilesystem(fs, total); }
    void bindSD(void* sd, uint32_t total)         { if (_backend) _backend->setSD(sd, total); }

    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }

    const char* name() const override { return "Storage"; }

private:
    IStorageBackend* _backend;
    bool            _enabled;
};

/* ------------------------------------------------------------------ */
/* GPU                                                                 */
/* ------------------------------------------------------------------ */
class GPUMonitor : public MonitorModule {
public:
    GPUMonitor() : _backend(0), _enabled(true) {}
    void bind(IGPUBackend* b) { _backend = b; }
    void begin() override { if (_backend) _backend->begin(); }
    void update(uint32_t nowMs) override { if (_enabled && _backend) _backend->update(nowMs); }
    MTMGPUSnapshot read() const {
        if (!_backend) { MTMGPUSnapshot s; s.status = MTM_METRIC_NOT_SUPPORTED; return s; }
        return _backend->readGPU();
    }
    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }
    const char* name() const override { return "GPU"; }
private:
    IGPUBackend* _backend;
    bool        _enabled;
};

/* ------------------------------------------------------------------ */
/* Temperature                                                         */
/* ------------------------------------------------------------------ */
class TemperatureMonitor : public MonitorModule {
public:
    TemperatureMonitor()
        : _backend(0), _enabled(true),
          _min(INT32_MAX), _max(INT32_MIN),
          _warningThreshold(70000), _criticalThreshold(85000),
          _warningActive(false) {}

    void bind(ITemperatureBackend* b) { _backend = b; }
    void begin() override {
        if (_backend) _backend->begin();
        MTMTemperatureSnapshot s = read();
        if (s.status != MTM_METRIC_NOT_SUPPORTED) { _min = s.value; _max = s.value; }
    }
    void update(uint32_t nowMs) override {
        if (!_enabled || !_backend) return;
        _backend->update(nowMs);
        MTMTemperatureSnapshot s = read();
        if (s.status != MTM_METRIC_NOT_SUPPORTED) {
            if (s.value < _min) _min = s.value;
            if (s.value > _max) _max = s.value;
            if (s.value >= _criticalThreshold)        _warningActive = true;
            else if (s.value < _warningThreshold - 2000) _warningActive = false;
        }
    }
    MTMTemperatureSnapshot read() const {
        if (!_backend) { MTMTemperatureSnapshot s; s.status = MTM_METRIC_NOT_SUPPORTED; return s; }
        MTMTemperatureSnapshot s = _backend->readTemperature();
        if (s.status != MTM_METRIC_NOT_SUPPORTED) { s.min = _min; s.max = _max; }
        return s;
    }

    void setExternalSensor(int32_t (*reader)()) { if (_backend) _backend->setExternalSensor(reader); }
    void setWarning(int32_t milliC) { _warningThreshold = milliC; }
    void setCritical(int32_t milliC) { _criticalThreshold = milliC; }
    bool warningActive() const { return _warningActive; }

    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }

    const char* name() const override { return "Temperature"; }

private:
    ITemperatureBackend* _backend;
    bool                _enabled;
    int32_t             _min, _max;
    int32_t             _warningThreshold, _criticalThreshold;
    bool                _warningActive;
};

/* ------------------------------------------------------------------ */
/* Power                                                               */
/* ------------------------------------------------------------------ */
class PowerMonitor : public MonitorModule {
public:
    PowerMonitor() : _backend(0), _enabled(true), _lowVoltageWarn(3300) {}
    void bind(IPowerBackend* b) { _backend = b; }
    void begin() override { if (_backend) _backend->begin(); }
    void update(uint32_t nowMs) override { if (_enabled && _backend) _backend->update(nowMs); }
    MTMPowerSnapshot read() const {
        if (!_backend) { MTMPowerSnapshot s; s.voltageStatus = MTM_METRIC_NOT_SUPPORTED; return s; }
        return _backend->readPower();
    }
    void setVoltageSensor(int32_t (*r)()) { if (_backend) _backend->setVoltageSensor(r); }
    void setCurrentSensor(int32_t (*r)()) { if (_backend) _backend->setCurrentSensor(r); }
    void setLowVoltageWarning(int32_t millivolts) { _lowVoltageWarn = millivolts; }
    int32_t lowVoltageWarning() const { return _lowVoltageWarn; }

    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }

    const char* name() const override { return "Power"; }
private:
    IPowerBackend* _backend;
    bool          _enabled;
    int32_t       _lowVoltageWarn;
};

/* ------------------------------------------------------------------ */
/* RTOS                                                                */
/* ------------------------------------------------------------------ */
class RTOSMonitor : public MonitorModule {
public:
    RTOSMonitor() : _backend(0), _enabled(true) {}
    void bind(IRTOSBackend* b) { _backend = b; }
    void begin() override { if (_backend) _backend->begin(); }
    void update(uint32_t nowMs) override { if (_enabled && _backend) _backend->update(nowMs); }
    MTMRTOSSnapshot read() const {
        if (!_backend) { MTMRTOSSnapshot s; s.status = MTM_METRIC_NOT_SUPPORTED; return s; }
        return _backend->readRTOS();
    }
    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }
    const char* name() const override { return "RTOS"; }
private:
    IRTOSBackend* _backend;
    bool         _enabled;
};

/* ------------------------------------------------------------------ */
/* Network                                                             */
/* ------------------------------------------------------------------ */
class NetworkMonitor : public MonitorModule {
public:
    NetworkMonitor()
        : _backend(0), _enabled(true),
          _lastBytesSent(0), _lastBytesRecv(0), _lastSampleMs(0),
          _txBps(0), _rxBps(0) {}
    void bind(INetworkBackend* b) { _backend = b; }
    void begin() override {
        if (_backend) _backend->begin();
        MTMNetworkSnapshot s = read();
        if (s.status != MTM_METRIC_NOT_SUPPORTED) {
            _lastBytesSent = s.bytesSent;
            _lastBytesRecv = s.bytesReceived;
        }
    }
    void update(uint32_t nowMs) override {
        if (!_enabled || !_backend) return;
        _backend->update(nowMs);
        MTMNetworkSnapshot s = read();
        if (s.status != MTM_METRIC_NOT_SUPPORTED && _lastSampleMs != 0) {
            uint32_t dt = nowMs - _lastSampleMs;
            if (dt > 0) {
                uint32_t dTx = s.bytesSent - _lastBytesSent;
                uint32_t dRx = s.bytesReceived - _lastBytesRecv;
                _txBps = (uint32_t)((uint64_t)dTx * 1000 / dt);
                _rxBps = (uint32_t)((uint64_t)dRx * 1000 / dt);
            }
        }
        _lastBytesSent = s.bytesSent;
        _lastBytesRecv = s.bytesReceived;
        _lastSampleMs  = nowMs;
    }
    MTMNetworkSnapshot read() const {
        if (!_backend) { MTMNetworkSnapshot s; s.status = MTM_METRIC_NOT_SUPPORTED; return s; }
        MTMNetworkSnapshot s = _backend->readNetwork();
        if (s.status != MTM_METRIC_NOT_SUPPORTED) {
            s.txThroughputBps = _txBps;
            s.rxThroughputBps = _rxBps;
        }
        return s;
    }
    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }
    const char* name() const override { return "Network"; }
private:
    INetworkBackend* _backend;
    bool            _enabled;
    uint32_t        _lastBytesSent, _lastBytesRecv, _lastSampleMs;
    uint32_t        _txBps, _rxBps;
};

/* ------------------------------------------------------------------ */
/* Runtime                                                             */
/* ------------------------------------------------------------------ */
class RuntimeMonitor : public MonitorModule {
public:
    RuntimeMonitor() : _backend(0), _enabled(true) {}
    void bind(IRuntimeBackend* b) { _backend = b; }
    void begin() override { if (_backend) _backend->begin(); }
    void update(uint32_t nowMs) override { if (_enabled && _backend) _backend->update(nowMs); }
    MTMRuntimeSnapshot read() const {
        if (!_backend) { MTMRuntimeSnapshot s; s.status = MTM_METRIC_NOT_SUPPORTED; return s; }
        return _backend->readRuntime();
    }
    void incrementRebootCounter() { if (_backend) _backend->incrementRebootCounter(); }
    bool enabled() const { return _enabled; }
    void setEnabled(bool e) { _enabled = e; }
    const char* name() const override { return "Runtime"; }
private:
    IRuntimeBackend* _backend;
    bool            _enabled;
};

} // namespace mtm

#endif // MTM_MONITOR_BASE_H
