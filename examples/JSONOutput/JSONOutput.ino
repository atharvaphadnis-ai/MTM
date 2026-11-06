/*
 * JSONOutput.ino
 * ==============
 * Compact JSON telemetry output over serial.
 *
 * The JSON follows the stable schema documented in
 * docs/TELEMETRY_PROTOCOL.md. The "protocol" and "version" fields let
 * external desktop dashboards pick the right parser.
 *
 * Use this sketch with `microtask-monitor-gui` or any JSON consumer
 * like `jq` on a host machine:
 *
 *   stty -F /dev/ttyUSB0 115200 raw -echo
 *   cat /dev/ttyUSB0 | jq .
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
    monitor.setSamplingInterval(2000);
    monitor.enable(RAM_MONITOR);
    monitor.enable(CPU_MONITOR);
    monitor.enable(STORAGE_MONITOR);
    monitor.enable(TEMPERATURE_MONITOR);
    monitor.enable(RUNTIME_MONITOR);
}

void loop() {
    monitor.update();
    if (monitor.available()) {
        monitor.printJSON(Serial);
        Serial.println();
    }
}
