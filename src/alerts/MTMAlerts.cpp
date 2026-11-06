/*
 * MTMAlerts.cpp
 * =============
 * Implementation of the alerts engine. Logic kept here to keep the header
 * small and to make the threshold logic testable from the unit tests.
 *
 * License: MIT.
 */
#include "MTMAlerts.h"

namespace mtm {

void AlertsEngine::evaluate(const MTMSystemSnapshot& s, uint32_t uptimeMs) {
    /* ---- RAM utilisation (percent) ---- */
    if (s.ram.status != MTM_METRIC_NOT_SUPPORTED && s.ram.total > 0) {
        uint8_t usedPct = (uint8_t)((uint64_t)s.ram.used * 100 / s.ram.total);
        uint8_t clearPct = (uint8_t)(usedPct > _hysteresis ? usedPct - _hysteresis : 0);
        if (usedPct >= _ramCrit)               _raise(MTM_ALERT_SOURCE_RAM, MTM_ALERT_CRITICAL, usedPct, _ramCrit, uptimeMs);
        else if (usedPct >= _ramWarn)          _raise(MTM_ALERT_SOURCE_RAM, MTM_ALERT_WARNING,  usedPct, _ramWarn, uptimeMs);
        else if (clearPct < _ramWarn)          _clear(MTM_ALERT_SOURCE_RAM);
    }

    /* ---- Heap free (bytes) ---- */
    if (s.ram.status != MTM_METRIC_NOT_SUPPORTED && _heapFreeCrit > 0) {
        if (s.ram.free <= _heapFreeCrit)       _raise(MTM_ALERT_SOURCE_HEAP_FREE, MTM_ALERT_CRITICAL, (int32_t)s.ram.free, (int32_t)_heapFreeCrit, uptimeMs);
        else if (s.ram.free <= _heapFreeWarn)  _raise(MTM_ALERT_SOURCE_HEAP_FREE, MTM_ALERT_WARNING,  (int32_t)s.ram.free, (int32_t)_heapFreeWarn, uptimeMs);
        else if (s.ram.free > _heapFreeWarn + 64) _clear(MTM_ALERT_SOURCE_HEAP_FREE);
    }

    /* ---- Heap fragmentation (percent) ---- */
    if (s.ram.status != MTM_METRIC_NOT_SUPPORTED && s.ram.fragmentation >= 0) {
        uint8_t frag = (uint8_t)s.ram.fragmentation;
        if (frag >= _fragCrit)                _raise(MTM_ALERT_SOURCE_HEAP_FRAGMENTATION, MTM_ALERT_CRITICAL, frag, _fragCrit, uptimeMs);
        else if (frag >= _fragWarn)           _raise(MTM_ALERT_SOURCE_HEAP_FRAGMENTATION, MTM_ALERT_WARNING,  frag, _fragWarn, uptimeMs);
        else if (frag < _fragWarn - _hysteresis) _clear(MTM_ALERT_SOURCE_HEAP_FRAGMENTATION);
    }

    /* ---- CPU usage (percent) ---- */
    if (s.cpu.usageStatus != MTM_METRIC_NOT_SUPPORTED) {
        uint8_t u = s.cpu.usagePercent;
        uint8_t clear = (uint8_t)(u > _hysteresis ? u - _hysteresis : 0);
        if (u >= _cpuCrit)                     _raise(MTM_ALERT_SOURCE_CPU, MTM_ALERT_CRITICAL, u, _cpuCrit, uptimeMs);
        else if (u >= _cpuWarn)                _raise(MTM_ALERT_SOURCE_CPU, MTM_ALERT_WARNING,  u, _cpuWarn, uptimeMs);
        else if (clear < _cpuWarn)             _clear(MTM_ALERT_SOURCE_CPU);
    }

    /* ---- Temperature (millidegrees C) ---- */
    if (s.temperature.status != MTM_METRIC_NOT_SUPPORTED) {
        int32_t v = s.temperature.value;
        if (v >= _tempCrit)                    _raise(MTM_ALERT_SOURCE_TEMPERATURE, MTM_ALERT_CRITICAL, v, _tempCrit, uptimeMs);
        else if (v >= _tempWarn)               _raise(MTM_ALERT_SOURCE_TEMPERATURE, MTM_ALERT_WARNING,  v, _tempWarn, uptimeMs);
        else if (v < _tempWarn - 2000)         _clear(MTM_ALERT_SOURCE_TEMPERATURE);
    }

    /* ---- Voltage (millivolts) ---- */
    if (s.power.voltageStatus != MTM_METRIC_NOT_SUPPORTED) {
        int32_t v = s.power.voltage;
        if (v <= _voltageCrit)                 _raise(MTM_ALERT_SOURCE_VOLTAGE, MTM_ALERT_CRITICAL, v, _voltageCrit, uptimeMs);
        else if (v <= _voltageWarn)            _raise(MTM_ALERT_SOURCE_VOLTAGE, MTM_ALERT_WARNING,  v, _voltageWarn, uptimeMs);
        else if (v > _voltageWarn + 100)       _clear(MTM_ALERT_SOURCE_VOLTAGE);
    }

    /* ---- Storage utilisation (percent) ---- */
    if (s.storage.fsStatus != MTM_METRIC_NOT_SUPPORTED && s.storage.fsTotal > 0) {
        uint8_t usedPct = (uint8_t)((uint64_t)s.storage.fsUsed * 100 / s.storage.fsTotal);
        if (usedPct >= _storageCrit)          _raise(MTM_ALERT_SOURCE_STORAGE, MTM_ALERT_CRITICAL, usedPct, _storageCrit, uptimeMs);
        else if (usedPct >= _storageWarn)     _raise(MTM_ALERT_SOURCE_STORAGE, MTM_ALERT_WARNING,  usedPct, _storageWarn, uptimeMs);
        else if (usedPct < _storageWarn - _hysteresis) _clear(MTM_ALERT_SOURCE_STORAGE);
    }

    /* ---- RTOS stack exhaustion (bytes remaining) ---- */
    if (s.rtos.status != MTM_METRIC_NOT_SUPPORTED) {
        for (uint8_t i = 0; i < s.rtos.taskCount; ++i) {
            uint16_t hw = s.rtos.tasks[i].stackHighWater;
            if (hw <= _stackCrit)             _raise(MTM_ALERT_SOURCE_STACK, MTM_ALERT_CRITICAL, hw, _stackCrit, uptimeMs);
            else if (hw <= _stackWarn)        _raise(MTM_ALERT_SOURCE_STACK, MTM_ALERT_WARNING,  hw, _stackWarn, uptimeMs);
            /* Clearing is conservative: stack alerts only clear via reset(). */
        }
    }

    /* ---- Network disconnection ---- */
    if (s.network.status != MTM_METRIC_NOT_SUPPORTED) {
        if (!s.network.connected)             _raise(MTM_ALERT_SOURCE_NETWORK, MTM_ALERT_WARNING, 0, 1, uptimeMs);
        else                                  _clear(MTM_ALERT_SOURCE_NETWORK);
    }
}

} // namespace mtm
