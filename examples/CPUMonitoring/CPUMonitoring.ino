/*
 * CPUMonitoring.ino
 * =================
 * CPU frequency and (where measurable) utilisation monitoring.
 *
 * Demonstrates:
 *   - monitor.cpu() structured access
 *   - Distinguishing MEASURED vs ESTIMATE
 *   - Per-core monitoring on ESP32 / RP2040
 *   - Load averages
 *
 * Open the Serial Monitor at 115200 baud.
 */

#include <MicroTaskMonitor.h>

MicroTaskMonitor monitor;

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    delay(2000);
#endif

    monitor.begin();
    monitor.setSamplingInterval(1000);
    monitor.enable(CPU_MONITOR);
}

void loop() {
    monitor.update();
    if (!monitor.available()) return;

    MTMCPUSnapshot cpu = monitor.cpu();

    Serial.println(F("\n--- CPU Snapshot ---"));
    Serial.print(F("frequency:      "));
    if (cpu.freqStatus != MTM_METRIC_NOT_SUPPORTED) {
        Serial.print(cpu.frequencyHz); Serial.println(F(" Hz"));
    } else {
        Serial.println(F("NOT_SUPPORTED"));
    }

    Serial.print(F("usage:          "));
    if (cpu.usageStatus == MTM_METRIC_MEASURED) {
        Serial.print(cpu.usagePercent); Serial.println(F(" % (MEASURED)"));
    } else if (cpu.usageStatus == MTM_METRIC_ESTIMATE) {
        Serial.print(cpu.usagePercent); Serial.println(F(" % (ESTIMATE)"));
    } else {
        Serial.println(F("NOT_SUPPORTED"));
    }

    if (cpu.coreCount > 1) {
        for (uint8_t i = 0; i < cpu.coreCount; ++i) {
            Serial.print(F("core "));
            Serial.print(i);
            Serial.print(F(":    "));
            Serial.print(cpu.cores[i].usagePercent); Serial.println(F(" %"));
        }
    }

    if (cpu.loadStatus != MTM_METRIC_NOT_SUPPORTED) {
        Serial.print(F("load avg (1/5/60s): "));
        Serial.print(cpu.loadAvg1s); Serial.print(F("/"));
        Serial.print(cpu.loadAvg5s); Serial.print(F("/"));
        Serial.print(cpu.loadAvg60s); Serial.println(F(" %"));
    }
}
