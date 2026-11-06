/*
 * MTMESP32Backends.h
 * ==================
 * ESP32 backend factory declarations.
 *
 * License: MIT.
 */
#ifndef MTM_ESP32_BACKENDS_H
#define MTM_ESP32_BACKENDS_H
#include "hal/MTMHalInterfaces.h"

uint32_t     g_mtmESP32Capabilities();
const char*  g_mtmESP32BoardName();
IRAMBackend*         g_mtmESP32MakeRAM();
ICPUBackend*         g_mtmESP32MakeCPU();
IStorageBackend*     g_mtmESP32MakeStorage();
IGPUBackend*         g_mtmESP32MakeGPU();
ITemperatureBackend* g_mtmESP32MakeTemp();
IPowerBackend*       g_mtmESP32MakePower();
IRTOSBackend*        g_mtmESP32MakeRTOS();
INetworkBackend*     g_mtmESP32MakeNetwork();
IRuntimeBackend*     g_mtmESP32MakeRuntime();

#endif
