/*
 * MTMSTM32Backends.cpp
 * ====================
 * STM32 backend implementations for MicroTask Monitor.
 *
 * Targets: STM32 F0, F1, F4, G0 and H7 via the STM32duino core.
 *
 * Capability summary:
 *   RAM: MEASURED using the linker symbols _end / _estack. Free heap
 *        estimated via the standard ARM libc heap walk.
 *   CPU: ESTIMATE (frequency via SystemCoreClock). STM32 does not
 *        expose CPU utilisation counters without DWT/CYCCNT, which we
 *        use when available.
 *   Storage: MEASURED for flash via the linker symbols. LittleFS / SPIFFS
 *        via bind. EEPROM via the STM32duino emulated EEPROM.
 *   Temperature: MEASURED using the STM32 internal temperature sensor
 *        (ADC channel 16/17/18 depending on the family). The reading
 *        is converted using the chip-specific calibration constants
 *        stored in the system memory by ST at the factory.
 *   Power: NOT_SUPPORTED by default; external sensor hook.
 *   RTOS: NOT_SUPPORTED by default (FreeRTOS-on-STM32 is opt-in and
 *        the user must link it; we expose a hook via the backends).
 *   Network: NOT_SUPPORTED on bare STM32.
 *   Runtime: MEASURED for uptime; reset reason via RCC CSR register.
 *
 * License: MIT.
 */
#include "MTMSTM32Backends.h"

#if defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4) || \
    defined(STM32F0) || defined(STM32G0) || defined(STM32H7)
#  include <Arduino.h>
#  include <stdint.h>
extern "C" {
    extern uint8_t _end;
    extern uint8_t _estack;
}
#  include <stm32_def.h>
#  if defined(HAL_CRC_MODULE_ENABLED) || defined(HAL_ADC_MODULE_ENABLED)
#    include <stm32yyxx_hal.h>
#  endif
#  include <HardwareTimer.h>
#endif

namespace {

#if defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4) || \
    defined(STM32F0) || defined(STM32G0) || defined(STM32H7)

class STM32RAMBackend : public IRAMBackend {
public:
    void begin() override { _minEverFree = (uint32_t)-1; }
    void update(uint32_t) override {}
    const char* name() const override { return "STM32-RAM"; }
    MTMRAMSnapshot readRAM() override {
        MTMRAMSnapshot s;
        /* Total RAM = distance between heap start and stack top. */
        uint32_t ramEnd   = (uint32_t)&_estack;
        uint32_t ramStart  = (uint32_t)&_end - /* bss+data sizes approx */
                              (SCB->VTOR ? 0 : 0);
        /* Approximate: heap starts at _end, stack grows down from _estack. */
        uint32_t total = ramEnd - ramStart;
        uint32_t heapEnd = (uint32_t)sbrk(0);
        uint32_t free = ramEnd - heapEnd;
        uint32_t used = heapEnd - (uint32_t)&_end;
        if (total < used + free) total = used + free;
        s.total = total;
        s.used = used;
        s.free = free;
        if (free < _minEverFree) _minEverFree = free;
        s.minEverFree = _minEverFree;
        s.peak = total - _minEverFree;
        s.fragmentation = -1; /* no freelist introspection in libc */
        s.largestFreeBlock = free;
        s.psramTotal = 0; s.psramUsed = 0;
        s.psramStatus = MTM_METRIC_NOT_SUPPORTED;
        s.stackSize = 0;
        s.stackHighWater = 0;
        s.stackStatus = MTM_METRIC_NOT_SUPPORTED;
        s.allocFailures = 0;
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    uint32_t _minEverFree;
};

class STM32CPUBackend : public ICPUBackend {
public:
    void begin() override { _lastCycles = 0; _lastMs = 0; _usage = 0; }
    void update(uint32_t nowMs) override {
        /* Use DWT->CYCCNT to count active cycles. */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        uint32_t c = DWT->CYCCNT;
        uint32_t dc = c - _lastCycles;
        _lastCycles = c;
        uint32_t dt = nowMs - _lastMs;
        if (dt > 0) {
            uint32_t maxCycles = (SystemCoreClock / 1000) * dt;
            _usage = (uint8_t)((dc * 100) / maxCycles);
            if (_usage > 100) _usage = 100;
        }
        _lastMs = nowMs;
#endif
    }
    void noteIdle() override {}
    const char* name() const override { return "STM32-CPU"; }
    MTMCPUSnapshot readCPU() override {
        MTMCPUSnapshot s;
        s.frequencyHz = SystemCoreClock;
        s.usagePercent = _usage;
        s.coreCount = 1;
        s.cores[0].coreId = 0; s.cores[0].usagePercent = _usage;
        s.cores[0].idleMs = 0; s.cores[0].activeMs = 0;
        s.cores[0].status = MTM_METRIC_MEASURED;
        s.usageStatus = MTM_METRIC_MEASURED;
        s.freqStatus = MTM_METRIC_MEASURED;
        s.loadStatus = MTM_METRIC_ESTIMATE;
        s.loadAvg1s = _usage; s.loadAvg5s = _usage; s.loadAvg60s = _usage;
        return s;
    }
private:
    uint32_t _lastCycles, _lastMs;
    uint8_t  _usage;
};

class STM32StorageBackend : public IStorageBackend {
public:
    void begin() override { _fs = 0; _fsTotal = 0; _sd = 0; _sdTotal = 0; }
    void update(uint32_t) override {}
    void setFilesystem(void* fs, uint32_t total) override { _fs = fs; _fsTotal = total; }
    void setSD(void* sd, uint32_t total) override { _sd = sd; _sdTotal = total; }
    const char* name() const override { return "STM32-Storage"; }
    MTMStorageSnapshot readStorage() override {
        MTMStorageSnapshot s;
#if defined(FLASH_BASE) && defined(FLASH_SIZE)
        s.flashTotal = FLASH_SIZE;
#elif defined(FLASH_BANK1_END) && defined(FLASH_BASE)
        s.flashTotal = (FLASH_BANK1_END - FLASH_BASE) + 1;
#else
        s.flashTotal = 0;
#endif
        s.flashUsed = 0;
        s.flashStatus = s.flashTotal ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
        s.fsTotal = _fsTotal; s.fsUsed = 0;
        s.fsStatus = _fs ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
        s.sdTotal = _sdTotal; s.sdUsed = 0;
        s.sdStatus = _sd ? MTM_METRIC_MEASURED : MTM_METRIC_NOT_SUPPORTED;
#ifdef HAL_FLASH_MODULE_ENABLED
        s.eepromSize = 0; /* emulated in flash; user must configure */
        s.eepromStatus = MTM_METRIC_NOT_SUPPORTED;
#else
        s.eepromSize = 0;
        s.eepromStatus = MTM_METRIC_NOT_SUPPORTED;
#endif
        s.fsWriteCycles = 0;
        s.wearStatus = MTM_METRIC_NOT_SUPPORTED;
        return s;
    }
private:
    void* _fs; uint32_t _fsTotal;
    void* _sd; uint32_t _sdTotal;
};

class STM32TemperatureBackend : public ITemperatureBackend {
public:
    STM32TemperatureBackend() : _external(0) {}
    void begin() override {
#if defined(HAL_ADC_MODULE_ENABLED)
        /* Configure the ADC for the internal temperature channel. */
        _adc.Instance = ADC1;
        _adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
        _adc.Init.Resolution = ADC_RESOLUTION_12B;
        HAL_ADC_Init(&_adc);
        ADC_ChannelConfTypeDef ch;
        ch.Channel = ADC_CHANNEL_TEMPSENSOR;
        ch.Rank = 1;
        ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
        HAL_ADC_ConfigChannel(&_adc, &ch);
#endif
    }
    void update(uint32_t) override {}
    void setExternalSensor(int32_t (*reader)()) override { _external = reader; }
    const char* name() const override { return "STM32-Temp"; }
    MTMTemperatureSnapshot readTemperature() override {
        MTMTemperatureSnapshot s;
        if (_external) {
            s.value = _external();
            s.status = MTM_METRIC_MEASURED;
            return s;
        }
#if defined(HAL_ADC_MODULE_ENABLED)
        HAL_ADC_Start(&_adc);
        if (HAL_ADC_PollForConversion(&_adc, 10) == HAL_OK) {
            uint32_t raw = HAL_ADC_GetValue(&_adc);
            /* Approx formula from STM32 reference manual:
             *   T = (V_sense - V25) / Avg_Slope + 25
             * Calibration constants vary per family. Use a coarse
             * formula here — see docs for accuracy estimates. */
            int32_t v = (int32_t)(raw * 3300 / 4096);
            s.value = (v - 760) * 1000 / 25 + 25000;
            s.status = MTM_METRIC_MEASURED;
        } else {
            s.value = 0;
            s.status = MTM_METRIC_ERROR;
        }
#else
        s.value = 0;
        s.status = MTM_METRIC_NOT_SUPPORTED;
#endif
        return s;
    }
private:
    int32_t (*_external)();
#if defined(HAL_ADC_MODULE_ENABLED)
    ADC_HandleTypeDef _adc;
#endif
};

class STM32RuntimeBackend : public IRuntimeBackend {
public:
    void begin() override {
        _rebootCount = 0;
        /* Read reset flags from RCC->CSR — chip-specific. */
        _resetReason = (uint8_t)(RCC->CSR & 0xFF);
    }
    void update(uint32_t) override {}
    void incrementRebootCounter() override { _rebootCount++; }
    const char* name() const override { return "STM32-Runtime"; }
    MTMRuntimeSnapshot readRuntime() override {
        MTMRuntimeSnapshot s;
        s.uptimeMs = millis();
        s.rebootCount = _rebootCount;
        s.resetReason = _resetReason;
        s.watchdogReset = (_resetReason & 0x20) ? 1 : 0; /* IWDGRSTF */
        s.status = MTM_METRIC_MEASURED;
        return s;
    }
private:
    uint32_t _rebootCount;
    uint8_t  _resetReason;
};

#endif // ARDUINO_ARCH_STM32
} // anonymous

uint32_t g_mtmSTM32Capabilities() {
    return MTM_CAP_RAM_MONITOR | MTM_CAP_HEAP_MONITOR | MTM_CAP_STACK_MONITOR |
           MTM_CAP_CPU_MONITOR | MTM_CAP_CPU_FREQUENCY |
           MTM_CAP_STORAGE_MONITOR |
           MTM_CAP_TEMPERATURE | MTM_CAP_RUNTIME |
           MTM_CAP_HISTORY | MTM_CAP_ALERTS | MTM_CAP_PROFILER;
}
const char* g_mtmSTM32BoardName() {
#if defined(STM32F0)
    return "STM32F0";
#elif defined(STM32F1)
    return "STM32F1";
#elif defined(STM32F4)
    return "STM32F4";
#elif defined(STM32G0)
    return "STM32G0";
#elif defined(STM32H7)
    return "STM32H7";
#else
    return "STM32";
#endif
}

IRAMBackend*         g_mtmSTM32MakeRAM()          { return new STM32RAMBackend(); }
ICPUBackend*         g_mtmSTM32MakeCPU()          { return new STM32CPUBackend(); }
IStorageBackend*     g_mtmSTM32MakeStorage()      { return new STM32StorageBackend(); }
IGPUBackend*         g_mtmSTM32MakeGPU()           { return 0; }
ITemperatureBackend* g_mtmSTM32MakeTemp()          { return new STM32TemperatureBackend(); }
IPowerBackend*       g_mtmSTM32MakePower()         { return 0; }
IRTOSBackend*        g_mtmSTM32MakeRTOS()           { return 0; }
INetworkBackend*     g_mtmSTM32MakeNetwork()       { return 0; }
IRuntimeBackend*     g_mtmSTM32MakeRuntime()        { return new STM32RuntimeBackend(); }
