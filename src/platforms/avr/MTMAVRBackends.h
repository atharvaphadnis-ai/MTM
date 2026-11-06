/*
 * MTMAVRBackends.h
 * ================
 * Declarations of the AVR backend factory functions.
 *
 * The actual backend implementations are in MTMAVRBackends.cpp.
 *
 * License: MIT.
 */
#ifndef MTM_AVR_BACKENDS_H
#define MTM_AVR_BACKENDS_H
#include "hal/MTMHalInterfaces.h"

uint32_t     g_mtmAVRCapabilities();
const char*  g_mtmAVRBoardName();
IRAMBackend*         g_mtmAVRMakeRAM();
ICPUBackend*         g_mtmAVRMakeCPU();
IStorageBackend*     g_mtmAVRMakeStorage();
IGPUBackend*         g_mtmAVRMakeGPU();
ITemperatureBackend* g_mtmAVRMakeTemp();
IPowerBackend*       g_mtmAVRMakePower();
IRTOSBackend*        g_mtmAVRMakeRTOS();
INetworkBackend*     g_mtmAVRMakeNetwork();
IRuntimeBackend*     g_mtmAVRMakeRuntime();

#endif
