/*
 * MTMPlatformFactory.h
 * ====================
 * Selects the platform-specific backends at compile time.
 *
 * The factory is responsible for instantiating the correct set of
 * backend objects for the current architecture. On platforms where a
 * particular metric is not available, the factory returns the
 * `Null<…>Backend`, which reports MTM_METRIC_NOT_SUPPORTED for every
 * reading. This keeps the engine code identical across platforms and
 * avoids `#ifdef` pollution in user sketches.
 *
 * The engine OWNS the backends it receives from the factory and will
 * `delete` them at destruction. Backends are therefore allocated with
 * `new` here — this is the only place in the library that uses heap
 * allocation, and it happens once at `begin()`. The library otherwise
 * uses no dynamic memory.
 *
 * This file is part of MicroTask Monitor (MTM). License: MIT.
 */
#ifndef MTM_PLATFORM_FACTORY_H
#define MTM_PLATFORM_FACTORY_H

#include "MTMHalInterfaces.h"

/*
 * Backend factory. Implemented in MTMPlatformFactory.cpp. Each platform
 * implements its own .cpp; the right one is selected by #include of the
 * matching platform header below.
 *
 * The factory returns raw owning pointers. The engine wraps them in
 * unique_ptr-style ownership internally (we use plain pointers + delete
 * to stay C++11 compatible with the oldest Arduino cores).
 */
struct MTMBackendSet {
    IRAMBackend*         ram;
    ICPUBackend*        cpu;
    IStorageBackend*    storage;
    IGPUBackend*        gpu;
    ITemperatureBackend* temperature;
    IPowerBackend*       power;
    IRTOSBackend*         rtos;
    INetworkBackend*      network;
    IRuntimeBackend*      runtime;

    MTMBackendSet()
        : ram(0), cpu(0), storage(0), gpu(0), temperature(0),
          power(0), rtos(0), network(0), runtime(0) {}
};

/* Capability bitmask reported by the platform. Built at the same time as
 * the backends so the engine can answer `isSupported()` immediately. */
uint32_t mtmPlatformCapabilities();
const char* mtmPlatformBoardName();

/* Allocate the backend set for the current platform. Caller takes ownership. */
MTMBackendSet mtmCreateBackends();

#endif // MTM_PLATFORM_FACTORY_H
