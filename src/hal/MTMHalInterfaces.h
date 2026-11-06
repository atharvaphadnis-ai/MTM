/*
 * MTMHalInterfaces.h
 * =================
 * Hardware abstraction interfaces for MicroTask Monitor.
 *
 * Each monitoring module has a corresponding pure interface (IRAMBackend,
 * ICPUBackend, ...). Platform backends implement the subset of these
 * interfaces they can support and leave the rest returning
 * MTM_METRIC_NOT_SUPPORTED.
 *
 * The MTM engine owns exactly one of each backend, selected at compile
 * time by MTMPlatformFactory (see MTMPlatformFactory.h).
 *
 * Concurrency contract:
 *   - All `update()` / `read*()` calls execute in user context, never
 *     from an ISR. Backends must not acquire long-held locks.
 *   - Backends must not block. If a metric needs an expensive operation
 *     (e.g. flash wear read) the backend should cache the result and
 *     refresh it lazily.
 *
 * Memory contract:
 *   - Backends never allocate on the heap.
 *   - Backends never use Arduino `String`.
 *
 * This file is part of MicroTask Monitor (MTM). License: MIT.
 */
#ifndef MTM_HAL_INTERFACES_H
#define MTM_HAL_INTERFACES_H

#include "MTMPlatformDetect.h"
#include "../core/MTMTypes.h"

/*
 * Common base interface shared by all backends. `begin()` is called once
 * at engine startup, `update()` on every sampling tick.
 */
class IMTMBackend {
public:
    virtual ~IMTMBackend() {}
    virtual void begin() = 0;
    virtual void update(uint32_t nowMs) = 0;
    virtual const char* name() const = 0;
};

/* ------------------------------------------------------------------ */
/* RAM / heap / stack                                                  */
/* ------------------------------------------------------------------ */
class IRAMBackend : public IMTMBackend {
public:
    virtual MTMRAMSnapshot readRAM() = 0;
};

/* ------------------------------------------------------------------ */
/* CPU                                                                 */
/* ------------------------------------------------------------------ */
class ICPUBackend : public IMTMBackend {
public:
    virtual MTMCPUSnapshot readCPU() = 0;
    /* Called from the engine's idle path so the backend can account
     * idle cycles without consuming a hardware timer. Implementations
     * may leave this empty. */
    virtual void noteIdle() {}
};

/* ------------------------------------------------------------------ */
/* Storage                                                             */
/* ------------------------------------------------------------------ */
class IStorageBackend : public IMTMBackend {
public:
    virtual MTMStorageSnapshot readStorage() = 0;
    /* Optional: bind to an externally-mounted filesystem. */
    virtual void setFilesystem(void* fs, uint32_t total) { (void)fs; (void)total; }
    virtual void setSD(void* sd, uint32_t total) { (void)sd; (void)total; }
};

/* ------------------------------------------------------------------ */
/* GPU / accelerator — most platforms return NOT_SUPPORTED.           */
/* ------------------------------------------------------------------ */
class IGPUBackend : public IMTMBackend {
public:
    virtual MTMGPUSnapshot readGPU() = 0;
};

/* ------------------------------------------------------------------ */
/* Temperature                                                         */
/* ------------------------------------------------------------------ */
class ITemperatureBackend : public IMTMBackend {
public:
    virtual MTMTemperatureSnapshot readTemperature() = 0;
    /* Allow an external sensor (e.g. DS18B20) to be plugged in. */
    virtual void setExternalSensor(int32_t (*reader)()) { (void)reader; }
};

/* ------------------------------------------------------------------ */
/* Power                                                               */
/* ------------------------------------------------------------------ */
class IPowerBackend : public IMTMBackend {
public:
    virtual MTMPowerSnapshot readPower() = 0;
    virtual void setVoltageSensor(int32_t (*readerMillivolts)()) { (void)readerMillivolts; }
    virtual void setCurrentSensor(int32_t (*readerMicroamps)()) { (void)readerMicroamps; }
};

/* ------------------------------------------------------------------ */
/* RTOS — only present on FreeRTOS-enabled cores.                      */
/* ------------------------------------------------------------------ */
class IRTOSBackend : public IMTMBackend {
public:
    virtual MTMRTOSSnapshot readRTOS() = 0;
};

/* ------------------------------------------------------------------ */
/* Network                                                             */
/* ------------------------------------------------------------------ */
class INetworkBackend : public IMTMBackend {
public:
    virtual MTMNetworkSnapshot readNetwork() = 0;
};

/* ------------------------------------------------------------------ */
/* Runtime                                                             */
/* ------------------------------------------------------------------ */
class IRuntimeBackend : public IMTMBackend {
public:
    virtual MTMRuntimeSnapshot readRuntime() = 0;
    /* Persist the reboot counter — backends that have no NV storage
     * will simply ignore this. */
    virtual void incrementRebootCounter() {}
};

#endif // MTM_HAL_INTERFACES_H
