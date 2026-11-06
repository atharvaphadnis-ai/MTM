/*
 * MTMSTM32Backends.h
 * =================
 * STM32 backend factory declarations. License: MIT.
 */
#ifndef MTM_STM32_BACKENDS_H
#define MTM_STM32_BACKENDS_H
#include "hal/MTMHalInterfaces.h"

uint32_t     g_mtmSTM32Capabilities();
const char*  g_mtmSTM32BoardName();
IRAMBackend*         g_mtmSTM32MakeRAM();
ICPUBackend*         g_mtmSTM32MakeCPU();
IStorageBackend*     g_mtmSTM32MakeStorage();
IGPUBackend*         g_mtmSTM32MakeGPU();
ITemperatureBackend* g_mtmSTM32MakeTemp();
IPowerBackend*       g_mtmSTM32MakePower();
IRTOSBackend*        g_mtmSTM32MakeRTOS();
INetworkBackend*     g_mtmSTM32MakeNetwork();
IRuntimeBackend*     g_mtmSTM32MakeRuntime();

#endif
