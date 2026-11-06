/*
 * Alerts.ino
 * ==========
 * Threshold-driven alerts example.
 *
 * Demonstrates:
 *   - setRAMWarning() / setRAMCritical()
 *   - setCPUWarning() / setCPUCritical()
 *   - setTemperatureWarning(degreesC)
 *   - onWarning() callback
 *
 * The callback is invoked from the engine's update() call, so it can
 * use Serial safely without locking.
 *
 * Open the Serial Monitor at 115200 baud.
 */

#include <MicroTaskMonitor.h>

MicroTaskMonitor monitor;

void onAlert(const MonitorWarning& w) {
    const char* src = "?";
    switch (w.source) {
        case MTM_ALERT_SOURCE_RAM:               src = "RAM"; break;
        case MTM_ALERT_SOURCE_HEAP_FREE:         src = "HEAP_FREE"; break;
        case MTM_ALERT_SOURCE_HEAP_FRAGMENTATION: src = "FRAGMENTATION"; break;
        case MTM_ALERT_SOURCE_CPU:               src = "CPU"; break;
        case MTM_ALERT_SOURCE_TEMPERATURE:       src = "TEMP"; break;
        case MTM_ALERT_SOURCE_VOLTAGE:           src = "VOLTAGE"; break;
        case MTM_ALERT_SOURCE_STORAGE:           src = "STORAGE"; break;
        case MTM_ALERT_SOURCE_STACK:             src = "STACK"; break;
        case MTM_ALERT_SOURCE_NETWORK:           src = "NET"; break;
        case MTM_ALERT_SOURCE_WATCHDOG:           src = "WDT"; break;
    }
    const char* sev = (w.severity == MTM_ALERT_CRITICAL) ? "CRITICAL" : "WARNING";
    Serial.print(F("\n[ALERT] "));
    Serial.print(sev); Serial.print(F(" source="));
    Serial.print(src);
    Serial.print(F(" value=")); Serial.print(w.value);
    Serial.print(F(" threshold=")); Serial.print(w.threshold);
    Serial.print(F(" uptime_ms=")); Serial.println(w.uptimeMs);
}

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    delay(2000);
#endif

    monitor.begin();
    monitor.setSamplingInterval(1000);

    monitor.enable(RAM_MONITOR);
    monitor.enable(CPU_MONITOR);
    monitor.enable(TEMPERATURE_MONITOR);

    monitor.setRAMWarning(80);          // % used
    monitor.setRAMCritical(95);
    monitor.setCPUWarning(90);          // %
    monitor.setCPUCritical(99);
    monitor.setTemperatureWarning(70);  // degrees C
    monitor.setTemperatureCritical(85);

    monitor.onWarning(onAlert);

    Serial.println(F("[Alerts] armed"));
}

void loop() {
    monitor.update();
}
