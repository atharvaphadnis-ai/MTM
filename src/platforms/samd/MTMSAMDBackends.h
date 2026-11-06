/*
 * MTMSAMDBackends.h
 * =================
 * SAMD backend factory declarations. License: MIT.
 */
#ifndef MTM_SAMD_BACKENDS_H
#define MTM_SAMD_BACKENDS_H
#include "hal/MTMHalInterfaces.h"

uint32_t     g_mtmSAMDCapabilities();
const char*  g_mtmSAMDBoardName();
IRAMBackend*         g_mtmSAMDMakeRAM();
ICPUBackend*         g_mtmSAMDMakeCPU();
IStorageBackend*     g_mtmSAMDMakeStorage();
IGPUBackend*         g_mtmSAMDMakeGPU();
ITemperatureBackend* g_mtmSAMDMakeTemp();
IPowerBackend*       g_mtmSAMDMakePower();
IRTOSBackend*        g_mtmSAMDMakeRTOS();
INetworkBackend*     g_mtmSAMDMakeNetwork();
IRuntimeBackend*     g_mtmSAMDMakeRuntime();

#endif
