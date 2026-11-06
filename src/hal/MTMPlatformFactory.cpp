/*
 * MTMPlatformFactory.cpp
 * ======================
 * Implementation of the platform backend factory.
 *
 * This is the single file that pulls in the platform-specific backend
 * implementations. It uses MTMPlatformDetect.h to select the right one at
 * compile time and fills the MTMBackendSet with Null backends for metrics
 * the platform does not provide.
 *
 * License: MIT.
 */
#include "MTMPlatformFactory.h"
#include "MTMNullBackends.h"

/* ----- platform-specific includes ----- */
#if defined(MTM_PLATFORM_AVR)
    #include "../platforms/avr/MTMAVRBackends.h"
#elif defined(MTM_PLATFORM_ESP32)
    #include "../platforms/esp32/MTMESP32Backends.h"
#elif defined(MTM_PLATFORM_ESP8266)
    #include "../platforms/esp8266/MTMESP8266Backends.h"
#elif defined(MTM_PLATFORM_STM32)
    #include "../platforms/stm32/MTMSTM32Backends.h"
#elif defined(MTM_PLATFORM_RP2040)
    #include "../platforms/rp2040/MTMRP2040Backends.h"
#elif defined(MTM_PLATFORM_SAMD)
    #include "../platforms/samd/MTMSAMDBackends.h"
#else
    /* HOST / unit tests — only Null backends. */
#endif

/*
 * Helper template to allocate a backend or fall back to its Null
 * implementation when the platform factory function returns 0.
 *
 * Marked __attribute__((unused)) — on platforms that supply every
 * backend the resolve*() for the missing one is never invoked and
 * would otherwise produce -Wunused-function warnings.
 */
static IRAMBackend* resolveRAM(IRAMBackend* p)        __attribute__((unused));
static ICPUBackend* resolveCPU(ICPUBackend* p)        __attribute__((unused));
static IStorageBackend* resolveStorage(IStorageBackend* p) __attribute__((unused));
static IGPUBackend* resolveGPU(IGPUBackend* p)        __attribute__((unused));
static ITemperatureBackend* resolveTemp(ITemperatureBackend* p) __attribute__((unused));
static IPowerBackend* resolvePower(IPowerBackend* p)  __attribute__((unused));
static IRTOSBackend* resolveRTOS(IRTOSBackend* p)    __attribute__((unused));
static INetworkBackend* resolveNet(INetworkBackend* p) __attribute__((unused));
static IRuntimeBackend* resolveRuntime(IRuntimeBackend* p) __attribute__((unused));

static IRAMBackend* resolveRAM(IRAMBackend* p)        { return p ? p : new MTMNullRAMBackend(); }
static ICPUBackend* resolveCPU(ICPUBackend* p)        { return p ? p : new MTMNullCPUBackend(); }
static IStorageBackend* resolveStorage(IStorageBackend* p) { return p ? p : new MTMNullStorageBackend(); }
static IGPUBackend* resolveGPU(IGPUBackend* p)        { return p ? p : new MTMNullGPUBackend(); }
static ITemperatureBackend* resolveTemp(ITemperatureBackend* p) { return p ? p : new MTMNullTemperatureBackend(); }
static IPowerBackend* resolvePower(IPowerBackend* p)  { return p ? p : new MTMNullPowerBackend(); }
static IRTOSBackend* resolveRTOS(IRTOSBackend* p)    { return p ? p : new MTMNullRTOSBackend(); }
static INetworkBackend* resolveNet(INetworkBackend* p) { return p ? p : new MTMNullNetworkBackend(); }
static IRuntimeBackend* resolveRuntime(IRuntimeBackend* p) { return p ? p : new MTMNullRuntimeBackend(); }

/*
 * Per-platform capability bitmasks and board-name strings. Implemented in
 * each platform's *Backends.cpp file. When the platform is HOST (no
 * embedded core), defaults are used.
 */
#if defined(MTM_PLATFORM_AVR)
    extern uint32_t     g_mtmAVRCapabilities();
    extern const char*  g_mtmAVRBoardName();
    extern IRAMBackend*        g_mtmAVRMakeRAM();
    extern ICPUBackend*        g_mtmAVRMakeCPU();
    extern IStorageBackend*    g_mtmAVRMakeStorage();
    extern IGPUBackend*        g_mtmAVRMakeGPU();
    extern ITemperatureBackend* g_mtmAVRMakeTemp();
    extern IPowerBackend*      g_mtmAVRMakePower();
    extern IRTOSBackend*       g_mtmAVRMakeRTOS();
    extern INetworkBackend*    g_mtmAVRMakeNetwork();
    extern IRuntimeBackend*    g_mtmAVRMakeRuntime();

    uint32_t     mtmPlatformCapabilities() { return g_mtmAVRCapabilities(); }
    const char*  mtmPlatformBoardName()    { return g_mtmAVRBoardName(); }
    MTMBackendSet mtmCreateBackends() {
        MTMBackendSet s;
        s.ram         = resolveRAM(g_mtmAVRMakeRAM());
        s.cpu         = resolveCPU(g_mtmAVRMakeCPU());
        s.storage     = resolveStorage(g_mtmAVRMakeStorage());
        s.gpu         = resolveGPU(g_mtmAVRMakeGPU());
        s.temperature = resolveTemp(g_mtmAVRMakeTemp());
        s.power       = resolvePower(g_mtmAVRMakePower());
        s.rtos        = resolveRTOS(g_mtmAVRMakeRTOS());
        s.network     = resolveNet(g_mtmAVRMakeNetwork());
        s.runtime     = resolveRuntime(g_mtmAVRMakeRuntime());
        return s;
    }
#elif defined(MTM_PLATFORM_ESP32)
    extern uint32_t     g_mtmESP32Capabilities();
    extern const char*  g_mtmESP32BoardName();
    extern IRAMBackend*        g_mtmESP32MakeRAM();
    extern ICPUBackend*        g_mtmESP32MakeCPU();
    extern IStorageBackend*    g_mtmESP32MakeStorage();
    extern IGPUBackend*        g_mtmESP32MakeGPU();
    extern ITemperatureBackend* g_mtmESP32MakeTemp();
    extern IPowerBackend*      g_mtmESP32MakePower();
    extern IRTOSBackend*       g_mtmESP32MakeRTOS();
    extern INetworkBackend*    g_mtmESP32MakeNetwork();
    extern IRuntimeBackend*    g_mtmESP32MakeRuntime();

    uint32_t     mtmPlatformCapabilities() { return g_mtmESP32Capabilities(); }
    const char*  mtmPlatformBoardName()    { return g_mtmESP32BoardName(); }
    MTMBackendSet mtmCreateBackends() {
        MTMBackendSet s;
        s.ram         = resolveRAM(g_mtmESP32MakeRAM());
        s.cpu         = resolveCPU(g_mtmESP32MakeCPU());
        s.storage     = resolveStorage(g_mtmESP32MakeStorage());
        s.gpu         = resolveGPU(g_mtmESP32MakeGPU());
        s.temperature = resolveTemp(g_mtmESP32MakeTemp());
        s.power       = resolvePower(g_mtmESP32MakePower());
        s.rtos        = resolveRTOS(g_mtmESP32MakeRTOS());
        s.network     = resolveNet(g_mtmESP32MakeNetwork());
        s.runtime     = resolveRuntime(g_mtmESP32MakeRuntime());
        return s;
    }
#elif defined(MTM_PLATFORM_ESP8266)
    extern uint32_t     g_mtmESP8266Capabilities();
    extern const char*  g_mtmESP8266BoardName();
    extern IRAMBackend*        g_mtmESP8266MakeRAM();
    extern ICPUBackend*        g_mtmESP8266MakeCPU();
    extern IStorageBackend*    g_mtmESP8266MakeStorage();
    extern IGPUBackend*        g_mtmESP8266MakeGPU();
    extern ITemperatureBackend* g_mtmESP8266MakeTemp();
    extern IPowerBackend*      g_mtmESP8266MakePower();
    extern IRTOSBackend*       g_mtmESP8266MakeRTOS();
    extern INetworkBackend*    g_mtmESP8266MakeNetwork();
    extern IRuntimeBackend*    g_mtmESP8266MakeRuntime();

    uint32_t     mtmPlatformCapabilities() { return g_mtmESP8266Capabilities(); }
    const char*  mtmPlatformBoardName()    { return g_mtmESP8266BoardName(); }
    MTMBackendSet mtmCreateBackends() {
        MTMBackendSet s;
        s.ram         = resolveRAM(g_mtmESP8266MakeRAM());
        s.cpu         = resolveCPU(g_mtmESP8266MakeCPU());
        s.storage     = resolveStorage(g_mtmESP8266MakeStorage());
        s.gpu         = resolveGPU(g_mtmESP8266MakeGPU());
        s.temperature = resolveTemp(g_mtmESP8266MakeTemp());
        s.power       = resolvePower(g_mtmESP8266MakePower());
        s.rtos        = resolveRTOS(g_mtmESP8266MakeRTOS());
        s.network     = resolveNet(g_mtmESP8266MakeNetwork());
        s.runtime     = resolveRuntime(g_mtmESP8266MakeRuntime());
        return s;
    }
#elif defined(MTM_PLATFORM_STM32)
    extern uint32_t     g_mtmSTM32Capabilities();
    extern const char*  g_mtmSTM32BoardName();
    extern IRAMBackend*        g_mtmSTM32MakeRAM();
    extern ICPUBackend*        g_mtmSTM32MakeCPU();
    extern IStorageBackend*    g_mtmSTM32MakeStorage();
    extern IGPUBackend*        g_mtmSTM32MakeGPU();
    extern ITemperatureBackend* g_mtmSTM32MakeTemp();
    extern IPowerBackend*      g_mtmSTM32MakePower();
    extern IRTOSBackend*       g_mtmSTM32MakeRTOS();
    extern INetworkBackend*    g_mtmSTM32MakeNetwork();
    extern IRuntimeBackend*    g_mtmSTM32MakeRuntime();

    uint32_t     mtmPlatformCapabilities() { return g_mtmSTM32Capabilities(); }
    const char*  mtmPlatformBoardName()    { return g_mtmSTM32BoardName(); }
    MTMBackendSet mtmCreateBackends() {
        MTMBackendSet s;
        s.ram         = resolveRAM(g_mtmSTM32MakeRAM());
        s.cpu         = resolveCPU(g_mtmSTM32MakeCPU());
        s.storage     = resolveStorage(g_mtmSTM32MakeStorage());
        s.gpu         = resolveGPU(g_mtmSTM32MakeGPU());
        s.temperature = resolveTemp(g_mtmSTM32MakeTemp());
        s.power       = resolvePower(g_mtmSTM32MakePower());
        s.rtos        = resolveRTOS(g_mtmSTM32MakeRTOS());
        s.network     = resolveNet(g_mtmSTM32MakeNetwork());
        s.runtime     = resolveRuntime(g_mtmSTM32MakeRuntime());
        return s;
    }
#elif defined(MTM_PLATFORM_RP2040)
    extern uint32_t     g_mtmRP2040Capabilities();
    extern const char*  g_mtmRP2040BoardName();
    extern IRAMBackend*        g_mtmRP2040MakeRAM();
    extern ICPUBackend*        g_mtmRP2040MakeCPU();
    extern IStorageBackend*    g_mtmRP2040MakeStorage();
    extern IGPUBackend*        g_mtmRP2040MakeGPU();
    extern ITemperatureBackend* g_mtmRP2040MakeTemp();
    extern IPowerBackend*      g_mtmRP2040MakePower();
    extern IRTOSBackend*       g_mtmRP2040MakeRTOS();
    extern INetworkBackend*    g_mtmRP2040MakeNetwork();
    extern IRuntimeBackend*    g_mtmRP2040MakeRuntime();

    uint32_t     mtmPlatformCapabilities() { return g_mtmRP2040Capabilities(); }
    const char*  mtmPlatformBoardName()    { return g_mtmRP2040BoardName(); }
    MTMBackendSet mtmCreateBackends() {
        MTMBackendSet s;
        s.ram         = resolveRAM(g_mtmRP2040MakeRAM());
        s.cpu         = resolveCPU(g_mtmRP2040MakeCPU());
        s.storage     = resolveStorage(g_mtmRP2040MakeStorage());
        s.gpu         = resolveGPU(g_mtmRP2040MakeGPU());
        s.temperature = resolveTemp(g_mtmRP2040MakeTemp());
        s.power       = resolvePower(g_mtmRP2040MakePower());
        s.rtos        = resolveRTOS(g_mtmRP2040MakeRTOS());
        s.network     = resolveNet(g_mtmRP2040MakeNetwork());
        s.runtime     = resolveRuntime(g_mtmRP2040MakeRuntime());
        return s;
    }
#elif defined(MTM_PLATFORM_SAMD)
    extern uint32_t     g_mtmSAMDCapabilities();
    extern const char*  g_mtmSAMDBoardName();
    extern IRAMBackend*        g_mtmSAMDMakeRAM();
    extern ICPUBackend*        g_mtmSAMDMakeCPU();
    extern IStorageBackend*    g_mtmSAMDMakeStorage();
    extern IGPUBackend*        g_mtmSAMDMakeGPU();
    extern ITemperatureBackend* g_mtmSAMDMakeTemp();
    extern IPowerBackend*      g_mtmSAMDMakePower();
    extern IRTOSBackend*       g_mtmSAMDMakeRTOS();
    extern INetworkBackend*    g_mtmSAMDMakeNetwork();
    extern IRuntimeBackend*    g_mtmSAMDMakeRuntime();

    uint32_t     mtmPlatformCapabilities() { return g_mtmSAMDCapabilities(); }
    const char*  mtmPlatformBoardName()    { return g_mtmSAMDBoardName(); }
    MTMBackendSet mtmCreateBackends() {
        MTMBackendSet s;
        s.ram         = resolveRAM(g_mtmSAMDMakeRAM());
        s.cpu         = resolveCPU(g_mtmSAMDMakeCPU());
        s.storage     = resolveStorage(g_mtmSAMDMakeStorage());
        s.gpu         = resolveGPU(g_mtmSAMDMakeGPU());
        s.temperature = resolveTemp(g_mtmSAMDMakeTemp());
        s.power       = resolvePower(g_mtmSAMDMakePower());
        s.rtos        = resolveRTOS(g_mtmSAMDMakeRTOS());
        s.network     = resolveNet(g_mtmSAMDMakeNetwork());
        s.runtime     = resolveRuntime(g_mtmSAMDMakeRuntime());
        return s;
    }
#else
    /* HOST build — Null backends everywhere. */
    uint32_t    mtmPlatformCapabilities() { return 0; }
    const char* mtmPlatformBoardName()    { return "HOST"; }
    MTMBackendSet mtmCreateBackends() {
        MTMBackendSet s;
        s.ram         = new MTMNullRAMBackend();
        s.cpu         = new MTMNullCPUBackend();
        s.storage     = new MTMNullStorageBackend();
        s.gpu         = new MTMNullGPUBackend();
        s.temperature = new MTMNullTemperatureBackend();
        s.power       = new MTMNullPowerBackend();
        s.rtos        = new MTMNullRTOSBackend();
        s.network     = new MTMNullNetworkBackend();
        s.runtime     = new MTMNullRuntimeBackend();
        return s;
    }
#endif
