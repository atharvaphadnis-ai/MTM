/*
 * RAMMonitoring.ino
 * =================
 * Detailed RAM / heap / fragmentation monitoring example.
 *
 * Demonstrates:
 *   - monitor.memory() structured access
 *   - min-ever-free and peak used tracking
 *   - Fragmentation on ESP8266 / ESP32
 *   - PSRAM availability check (ESP32 only)
 *   - Stack high-water mark (FreeRTOS cores)
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
    monitor.setSamplingInterval(500);
    monitor.enable(RAM_MONITOR);
    monitor.enable(PSRAM_MONITOR); /* no-op on boards without PSRAM */

    Serial.print(F("\n[RAM Monitor] board="));
    Serial.println(monitor.boardName());
}

void loop() {
    monitor.update();
    if (!monitor.available()) return;

    MTMRAMSnapshot ram = monitor.memory();

    Serial.println(F("\n--- RAM Snapshot ---"));
    Serial.print(F("status:         ")); Serial.println(ram.status == MTM_METRIC_MEASURED ? "MEASURED" :
                                                          (ram.status == MTM_METRIC_NOT_SUPPORTED ? "NOT_SUPPORTED" : "OTHER"));

    if (ram.status == MTM_METRIC_NOT_SUPPORTED) return;

    Serial.print(F("total:          ")); Serial.print(ram.total);    Serial.println(F(" bytes"));
    Serial.print(F("used:           ")); Serial.print(ram.used);     Serial.println(F(" bytes"));
    Serial.print(F("free:           ")); Serial.print(ram.free);     Serial.println(F(" bytes"));
    Serial.print(F("min ever free:  ")); Serial.print(ram.minEverFree); Serial.println(F(" bytes"));
    Serial.print(F("peak used:      ")); Serial.print(ram.peak);    Serial.println(F(" bytes"));

    if (ram.fragmentation >= 0) {
        Serial.print(F("fragmentation:  ")); Serial.print(ram.fragmentation); Serial.println(F(" %"));
    } else {
        Serial.println(F("fragmentation:  NOT_SUPPORTED"));
    }

    Serial.print(F("largest free:   ")); Serial.print(ram.largestFreeBlock); Serial.println(F(" bytes"));

    if (ram.psramStatus == MTM_METRIC_MEASURED) {
        Serial.print(F("PSRAM total:    ")); Serial.print(ram.psramTotal); Serial.println(F(" bytes"));
        Serial.print(F("PSRAM used:     ")); Serial.print(ram.psramUsed);  Serial.println(F(" bytes"));
    } else {
        Serial.println(F("PSRAM:          NOT_SUPPORTED"));
    }

    if (ram.stackStatus == MTM_METRIC_MEASURED) {
        Serial.print(F("stack size:     ")); Serial.print(ram.stackSize); Serial.println(F(" bytes"));
        Serial.print(F("stack HWM:      ")); Serial.print(ram.stackHighWater); Serial.println(F(" bytes"));
    }

    delay(2000);
}
