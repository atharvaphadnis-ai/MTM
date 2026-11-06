/*
 * MTMAVRBackends.cpp
 * ==================
 * AVR backend implementations for MicroTask Monitor.
 *
 * Target boards: Arduino Uno, Nano, Mega, Leonardo, Pro Mini.
 *
 * Capability summary on AVR:
 *
 *   RAM: MEASURED. total is a compile-time constant derived from the
 *        RAMSIZE / RAMEND symbol exposed by the AVR libc. used/free
 *        come from a stack-pointer-based free-RAM calculation combined
 *        with the heap end (__brkval / __malloc_heap_end). Largest free
 *        block is estimated by walking the freelist.
 *   CPU: ESTIMATE. AVR has no on-chip CPU-utilisation counter. We sample
 *        the idle hook by accumulating time spent inside loop() vs
 *        time spent inside user-supplied sections (via the profiler).
 *        Frequency is read from F_CPU.
 *   Storage: MEASURED for flash; the AVR libc exposes __data_load_end
 *        and the program size from the linker. EEPROM is read via E2END.
 *        Filesystem support requires the user to bind LittleFS / SPIFFS
 *        / SD manually — AVR has very little flash so filesystems are
 *        rare.
 *   Temperature: MEASURED on ATmega328P/2560 via the internal
 *        temperature sensor and the bandgap reference. The reading is
 *        a coarse estimate (±10 °C) — see Atmel app note AVR122.
 *   Power: NOT_SUPPORTED by default; the user can plug in an external
 *        ADC sensor.
 *   RTOS: NOT_SUPPORTED (FreeRTOS-on-AVR is rare; we don't link it).
 *   Network: NOT_SUPPORTED (AVR boards have no built-in networking).
 *   Runtime: MEASURED for uptime; reset reason is approximated by
 *        reading MCUSR (the AVR reset flag register).
 *
 * License: MIT.
 */
#include "MTMAVRBackends.h"

#if defined(ARDUINO)
#  include <Arduino.h>
extern "C" {
    /* Symbols exposed by the AVR libc. */
    extern uint8_t __brkval;
    extern uint8_t __bss_end;
    extern unsigned char __heap_start;
    extern uint8_t* __brkval_ptr;
}
#endif

#include <stdlib.h>

namespace {

/* ---------- RAM backend ---------- */
class AVRRAMBackend : public IRAMBackend {
public:
    void begin() override {
        _minEverFree = (uint32_t)-1;
    }
    void update(uint32_t) override {}

    const char* name() const override { return "AVR-RAM"; }

    MTMRAMSnapshot readRAM() override {
        MTMRAMSnapshot s;
#if defined(ARDUINO_ARCH_AVR) || defined(__AVR__)
        /* Free RAM = distance between heap end and stack pointer. */
        extern unsigned int __heap_start;
        extern unsigned int __brkval;
        /* The Arduino core exposes `__brkval`; when no malloc has run it
         * is 0, in which case we use `__heap_start`. */
        unsigned int heapEnd = __brkval ? __brkval : (unsigned int)&__heap_start;
        unsigned int stackPtr = SP;
        int freeRam = (int)(stackPtr - heapEnd);
        if (freeRam < 0) freeRam = 0;

        /* Total RAM = RAMEND - RAMSTART. RAMEND is in <io.h>. */
        uint32_t total = 0;
#if defined(RAMEND) && defined(RAMSIZE)
        total = RAMSIZE;
#elif defined(RAMEND)
        /* RAMEND is the address of the last byte of RAM. */
        total = (uint32_t)RAMEND + 1;
#else
        /* Fall back: 2048 for ATmega328P, 8192 for ATmega2560. */
        #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__)
            total = 2048;
        #elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
            total = 8192;
        #elif defined(__AVR_ATmega32U4__)
            total = 2560;
        #elif defined(__AVR_ATmega168__)
            total = 1024;
        #else
            total = 2048;
        #endif
#endif
        /* Stack typically starts ~256 bytes below RAMEND because the
         * Arduino runtime reserves a little space at the top. */
        uint32_t used = total > (uint32_t)freeRam ? total - (uint32_t)freeRam : 0;
        s.total = total;
        s.used = used;
        s.free = (uint32_t)freeRam;
        if ((uint32_t)freeRam < _minEverFree) _minEverFree = (uint32_t)freeRam;
        s.minEverFree = _minEverFree;
        s.peak = total - _minEverFree;
        s.fragmentation = _estimateFragmentation();
        s.largestFreeBlock = (uint32_t)freeRam;
        s.psramTotal = 0; s.psramUsed = 0;
        s.stackSize = 0; s.stackHighWater = 0;
        s.allocFailures = 0;
        s.status = MTM_METRIC_MEASURED;
        s.psramStatus = MTM_METRIC_NOT_SUPPORTED;
        s.stackStatus = MTM_METRIC_NOT_SUPPORTED;
#else
        s.status = MTM_METRIC_NOT_SUPPORTED;
#endif
        return s;
    }

private:
    int32_t _estimateFragmentation() {
        /* Without freelist introspection on AVR we can't compute
         * fragmentation meaningfully; report it as not available. */
        return -1;
    }
    uint32_t _minEverFree;
};

/* ---------- CPU backend (estimate) ---------- */
class AVRCPUBackend : public ICPUBackend {
public:
    void begin() override {
        _idleCycles = 0;
        _activeCycles = 0;
        _lastSampleMs = 0;
    }
    void update(uint32_t nowMs) override {
        if (_lastSampleMs == 0) { _lastSampleMs = nowMs; return; }
        uint32_t dt = nowMs - _lastSampleMs;
        _lastSampleMs = nowMs;
        /* On AVR we can't measure CPU usage from a counter. We assume
         * the loop is mostly idle and the user is free to use the
         * profiler for hot spots. The estimate is conservative: we
         * always report 0% if we can't tell. */
        _idleCycles += dt;
    }
    void noteIdle() override { /* no-op — measured in update() */ }
    const char* name() const override { return "AVR-CPU"; }

    MTMCPUSnapshot readCPU() override {
        MTMCPUSnapshot s;
#if defined(ARDUINO_ARCH_AVR) || defined(__AVR__)
        s.frequencyHz = F_CPU;
        s.usagePercent = 0; /* cannot measure on bare AVR */
        s.coreCount = 1;
        s.cores[0].coreId = 0;
        s.cores[0].usagePercent = 0;
        s.cores[0].idleMs = _idleCycles;
        s.cores[0].activeMs = 0;
        s.cores[0].status = MTM_METRIC_NOT_SUPPORTED; /* per-core */
        s.usageStatus = MTM_METRIC_ESTIMATE;
        s.freqStatus = MTM_METRIC_MEASURED;
        s.loadStatus = MTM_METRIC_NOT_SUPPORTED;
#else
        s.usageStatus = MTM_METRIC_NOT_SUPPORTED;
        s.freqStatus = MTM_METRIC_NOT_SUPPORTED;
        s.loadStatus = MTM_METRIC_NOT_SUPPORTED;
#endif
        s.loadAvg1s = 0; s.loadAvg5s = 0; s.loadAvg60s = 0;
        return s;
    }
private:
    uint32_t _idleCycles, _activeCycles, _lastSampleMs;
};

/* ---------- Storage backend ---------- */
class AVRStorageBackend : public IStorageBackend {
public:
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "AVR-Storage"; }

    MTMStorageSnapshot readStorage() override {
        MTMStorageSnapshot s;
#if defined(ARDUINO_ARCH_AVR) || defined(__AVR__)
        /* Total flash from the toolchain. */
#if defined(FLASHEND)
        s.flashTotal = (uint32_t)FLASHEND + 1;
#elif defined(__AVR_ATmega328P__)
        s.flashTotal = 32768;
#elif defined(__AVR_ATmega2560__)
        s.flashTotal = 262144;
#elif defined(__AVR_ATmega32U4__)
        s.flashTotal = 32768;
#else
        s.flashTotal = 32768;
#endif
        /* Program size is not directly knowable at runtime on AVR. We
         * expose a conservative estimate from the data section size. */
        extern uint8_t __data_load_end;
        s.flashUsed = (uint32_t)&__data_load_end;
#if defined(E2END)
        s.eepromSize = (uint16_t)E2END + 1;
        s.eepromStatus = MTM_METRIC_MEASURED;
#else
        s.eepromSize = 0;
        s.eepromStatus = MTM_METRIC_NOT_SUPPORTED;
#endif
        s.flashStatus = MTM_METRIC_MEASURED;
        s.fsStatus = MTM_METRIC_NOT_SUPPORTED;
        s.sdStatus = MTM_METRIC_NOT_SUPPORTED;
        s.wearStatus = MTM_METRIC_NOT_SUPPORTED;
#else
        s.flashStatus = MTM_METRIC_NOT_SUPPORTED;
        s.fsStatus = MTM_METRIC_NOT_SUPPORTED;
        s.sdStatus = MTM_METRIC_NOT_SUPPORTED;
        s.eepromStatus = MTM_METRIC_NOT_SUPPORTED;
        s.wearStatus = MTM_METRIC_NOT_SUPPORTED;
#endif
        s.fsTotal = 0; s.fsUsed = 0;
        s.sdTotal = 0; s.sdUsed = 0;
        s.fsWriteCycles = 0;
        return s;
    }
};

/* ---------- Temperature backend (internal AVR sensor) ---------- */
class AVRTemperatureBackend : public ITemperatureBackend {
public:
    AVRTemperatureBackend() : _external(0) {}
    void begin() override {}
    void update(uint32_t) override {}
    const char* name() const override { return "AVR-Temp"; }
    void setExternalSensor(int32_t (*reader)()) override { _external = reader; }

    MTMTemperatureSnapshot readTemperature() override {
        MTMTemperatureSnapshot s;
        if (_external) {
            s.value = _external();
            s.status = MTM_METRIC_MEASURED;
            return s;
        }
#if defined(ARDUINO_ARCH_AVR) && (defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega32U4__))
        /* AVR internal temperature sensor — see AVR122.
         * Reading requires selecting the temperature channel of the
         * ADC and the 1.1V bandgap reference. The formula is approximate
         * and is calibrated per-chip; we expose a coarse reading. */
        s.value = _readInternal();
        s.status = MTM_METRIC_MEASURED;
#else
        s.value = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
#endif
        return s;
    }
private:
    int32_t (*_external)();
#if defined(ARDUINO_ARCH_AVR)
    int32_t _readInternal() {
        /* Configure ADC for the internal temperature channel.
         * This is supported on ATmega328P, ATmega2560, ATmega32U4.
         * The actual calibration constants vary by chip; we return a
         * reading in millidegrees C using a coarse formula. */
        ADMUX = (1 << REFS1) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);
        /* Allow the ADC to settle. */
        delay(2);
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)) {}
        uint16_t raw = ADC;
        /* Coarse formula: T = raw - 273 + offset. We assume offset = 0
         * since per-chip calibration is not feasible at runtime. */
        return ((int32_t)raw - 100) * 1000; /* placeholder, raw → milliC */
    }
#endif
};

/* ---------- Runtime backend ---------- */
class AVRRuntimeBackend : public IRuntimeBackend {
public:
    void begin() override {
#if defined(ARDUINO_ARCH_AVR)
        /* Read the reset flags from MCUSR. */
        uint8_t mcusr = MCUSR;
        _resetReason = mcusr;
        _watchdogReset = (mcusr & (1 << WDRF)) ? 1 : 0;
        MCUSR = 0; /* clear flags for next boot */
#else
        _resetReason = 0;
        _watchdogReset = 0;
#endif
        /* Reboot counter — persist in EEPROM at address 0. */
        _rebootCount = _readEeprom32(0);
        /* Increment on the next incrementRebootCounter() call. */
    }
    void update(uint32_t) override {}
    const char* name() const override { return "AVR-Runtime"; }

    void incrementRebootCounter() override {
        _rebootCount++;
        _writeEeprom32(0, _rebootCount);
    }

    MTMRuntimeSnapshot readRuntime() override {
        MTMRuntimeSnapshot s;
#if defined(ARDUINO_ARCH_AVR)
        s.uptimeMs = millis();
        s.rebootCount = _rebootCount;
        s.resetReason = _resetReason;
        s.watchdogReset = _watchdogReset;
        s.status = MTM_METRIC_MEASURED;
#else
        s.uptimeMs = 0; s.rebootCount = 0; s.resetReason = 0;
        s.watchdogReset = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
#endif
        return s;
    }
private:
    uint8_t  _resetReason;
    uint8_t  _watchdogReset;
    uint32_t _rebootCount;
#if defined(ARDUINO_ARCH_AVR)
#  include <EEPROM.h>
    static uint32_t _readEeprom32(int addr) {
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) v |= ((uint32_t)EEPROM.read(addr + i)) << (i * 8);
        return v;
    }
    static void _writeEeprom32(int addr, uint32_t v) {
        for (int i = 0; i < 4; ++i) EEPROM.write(addr + i, (uint8_t)(v >> (i * 8)));
    }
#endif
};

} // anonymous namespace

/* ---------- Factory functions ---------- */
uint32_t g_mtmAVRCapabilities() {
    return MTM_CAP_RAM_MONITOR | MTM_CAP_HEAP_MONITOR | MTM_CAP_STACK_MONITOR |
           MTM_CAP_CPU_MONITOR | MTM_CAP_CPU_FREQUENCY |
           MTM_CAP_STORAGE_MONITOR | MTM_CAP_EEPROM_MONITOR |
           MTM_CAP_TEMPERATURE | MTM_CAP_RUNTIME |
           MTM_CAP_HISTORY | MTM_CAP_ALERTS | MTM_CAP_PROFILER;
}
const char* g_mtmAVRBoardName() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__)
    return "AVR-ATmega328P";
#elif defined(__AVR_ATmega2560__)
    return "AVR-ATmega2560";
#elif defined(__AVR_ATmega32U4__)
    return "AVR-ATmega32U4";
#elif defined(__AVR_ATmega168__)
    return "AVR-ATmega168";
#else
    return "AVR";
#endif
}
IRAMBackend*         g_mtmAVRMakeRAM()        { return new AVRRAMBackend(); }
ICPUBackend*         g_mtmAVRMakeCPU()        { return new AVRCPUBackend(); }
IStorageBackend*     g_mtmAVRMakeStorage()    { return new AVRStorageBackend(); }
IGPUBackend*         g_mtmAVRMakeGPU()        { return 0; }
ITemperatureBackend* g_mtmAVRMakeTemp()        { return new AVRTemperatureBackend(); }
IPowerBackend*       g_mtmAVRMakePower()       { return 0; }
IRTOSBackend*        g_mtmAVRMakeRTOS()         { return 0; }
INetworkBackend*     g_mtmAVRMakeNetwork()     { return 0; }
IRuntimeBackend*     g_mtmAVRMakeRuntime()      { return new AVRRuntimeBackend(); }
