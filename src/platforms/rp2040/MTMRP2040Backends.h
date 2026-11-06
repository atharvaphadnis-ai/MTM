/*
 * MTMRP2040Backends.h
 * ==================
 * RP2040 backend factory declarations. License: MIT.
 */
#ifndef MTM_RP2040_BACKENDS_H
#define MTM_RP2040_BACKENDS_H
#include "hal/MTMHalInterfaces.h"

uint32_t     g_mtmRP2040Capabilities();
const char*  g_mtmRP2040BoardName();
IRAMBackend*         g_mtmRP2040MakeRAM();
ICPUBackend*         g_mtmRP2040MakeCPU();
IStorageBackend*     g_mtmRP2040MakeStorage();
IGPUBackend*         g_mtmRP2040MakeGPU();
ITemperatureBackend* g_mtmRP2040MakeTemp();
IPowerBackend*       g_mtmRP2040MakePower();
IRTOSBackend*        g_mtmRP2040MakeRTOS();
INetworkBackend*     g_mtmRP2040MakeNetwork();
IRuntimeBackend*     g_mtmRP2040MakeRuntime();

#endif
