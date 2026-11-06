/*
 * MTMESP8266Backends.h
 * ===================
 * ESP8266 backend factory declarations. License: MIT.
 */
#ifndef MTM_ESP8266_BACKENDS_H
#define MTM_ESP8266_BACKENDS_H
#include "hal/MTMHalInterfaces.h"

uint32_t     g_mtmESP8266Capabilities();
const char*  g_mtmESP8266BoardName();
IRAMBackend*         g_mtmESP8266MakeRAM();
ICPUBackend*         g_mtmESP8266MakeCPU();
IStorageBackend*     g_mtmESP8266MakeStorage();
IGPUBackend*         g_mtmESP8266MakeGPU();
ITemperatureBackend* g_mtmESP8266MakeTemp();
IPowerBackend*       g_mtmESP8266MakePower();
IRTOSBackend*        g_mtmESP8266MakeRTOS();
INetworkBackend*     g_mtmESP8266MakeNetwork();
IRuntimeBackend*     g_mtmESP8266MakeRuntime();

#endif
