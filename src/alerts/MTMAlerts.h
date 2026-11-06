/*
 * MTMAlerts.h
 * ===========
 * Threshold and alert engine for MicroTask Monitor.
 *
 * The engine evaluates every metric against configured warning and
 * critical thresholds on each sampling tick. When a threshold is crossed
 * the user-supplied callback is invoked — but ONLY from the main `update()`
 * call, never from an ISR.
 *
 * Hysteresis is built in: a metric that is currently in warning state must
 * drop below (warningThreshold - hysteresis) before the alert clears. This
 * prevents flapping when a value oscillates near the threshold.
 *
 * The engine keeps no dynamic memory. Up to MTM_ALERT_MAX active alerts
 * can be tracked simultaneously; older alerts are dropped.
 *
 * License: MIT.
 */
#ifndef MTM_ALERTS_H
#define MTM_ALERTS_H

#include "../core/MTMTypes.h"

#define MTM_ALERT_MAX 8

namespace mtm {

typedef void (*AlertCallback)(const MTMMonitorWarning&);

class AlertsEngine {
public:
    AlertsEngine()
        : _cb(0),
          _ramWarn(85), _ramCrit(95),
          _heapFreeWarn(0), _heapFreeCrit(0),
          _fragWarn(50), _fragCrit(70),
          _cpuWarn(90), _cpuCrit(100),
          _tempWarn(70000), _tempCrit(85000),
          _voltageWarn(3300), _voltageCrit(3000),
          _storageWarn(90), _storageCrit(98),
          _stackWarn(128), _stackCrit(32),
          _hysteresis(2) {}

    /* Install the user callback. Called from the engine's update(). */
    void onAlert(AlertCallback cb) { _cb = cb; }

    /* Threshold setters — values are in the units documented in MTMTypes.h.
     * A threshold of 0 disables the check (with the exception of those
     * where 0 is a meaningful value, like heapFree — those use a separate
     * `_enabled` bit). */
    void setRAMWarning(uint8_t percent)              { _ramWarn = percent; }
    void setRAMCritical(uint8_t percent)            { _ramCrit = percent; }
    void setHeapFreeWarning(uint32_t bytes)         { _heapFreeWarn = bytes; }
    void setHeapFreeCritical(uint32_t bytes)        { _heapFreeCrit = bytes; }
    void setFragmentationWarning(uint8_t percent)  { _fragWarn = percent; }
    void setFragmentationCritical(uint8_t percent)  { _fragCrit = percent; }
    void setCPUWarning(uint8_t percent)            { _cpuWarn = percent; }
    void setCPUCritical(uint8_t percent)           { _cpuCrit = percent; }
    void setTemperatureWarning(int32_t milliC)      { _tempWarn = milliC; }
    void setTemperatureCritical(int32_t milliC)    { _tempCrit = milliC; }
    void setLowVoltageWarning(int32_t millivolts)  { _voltageWarn = millivolts; }
    void setLowVoltageCritical(int32_t millivolts) { _voltageCrit = millivolts; }
    void setStorageWarning(uint8_t percent)        { _storageWarn = percent; }
    void setStorageCritical(uint8_t percent)       { _storageCrit = percent; }
    void setStackWarning(uint16_t bytesRemaining)  { _stackWarn = bytesRemaining; }
    void setStackCritical(uint16_t bytesRemaining) { _stackCrit = bytesRemaining; }
    void setHysteresis(uint8_t percent)            { _hysteresis = percent; }

    /* Evaluate the engine against a fresh snapshot. Caller passes the
     * system uptime so the warning events carry a meaningful timestamp. */
    void evaluate(const MTMSystemSnapshot& snap, uint32_t uptimeMs);

    /* True if the given alert source is currently in an active state. */
    bool isActive(MTMAlertSource src) const {
        for (uint8_t i = 0; i < _activeCount; ++i) if (_active[i] == src) return true;
        return false;
    }

    /* Reset all active states — used by tests. */
    void reset() { _activeCount = 0; }

private:
    void _raise(MTMAlertSource src, MTMAlertSeverity sev, int32_t v, int32_t thr, uint32_t uptime) {
        if (!_cb) return;
        /* Mark as active. If the array is full, drop the oldest. */
        if (_activeCount == MTM_ALERT_MAX) {
            for (uint8_t i = 0; i + 1 < _activeCount; ++i) _active[i] = _active[i+1];
            --_activeCount;
        }
        _active[_activeCount++] = src;
        MTMMonitorWarning w;
        w.source = src; w.severity = sev;
        w.value = v; w.threshold = thr; w.uptimeMs = uptime;
        _cb(w);
    }
    void _clear(MTMAlertSource src) {
        for (uint8_t i = 0; i < _activeCount; ++i) {
            if (_active[i] == src) {
                for (uint8_t j = i; j + 1 < _activeCount; ++j) _active[j] = _active[j+1];
                --_activeCount;
                return;
            }
        }
    }

    AlertCallback _cb;
    uint8_t       _ramWarn, _ramCrit;
    uint32_t      _heapFreeWarn, _heapFreeCrit;
    uint8_t       _fragWarn, _fragCrit;
    uint8_t       _cpuWarn, _cpuCrit;
    int32_t       _tempWarn, _tempCrit;
    int32_t       _voltageWarn, _voltageCrit;
    uint8_t       _storageWarn, _storageCrit;
    uint16_t      _stackWarn, _stackCrit;
    uint8_t       _hysteresis;

    MTMAlertSource _active[MTM_ALERT_MAX];
    uint8_t       _activeCount;
};

} // namespace mtm

#endif // MTM_ALERTS_H
