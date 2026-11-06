/*
 * CSVOutput.ino
 * =============
 * CSV telemetry output over serial.
 *
 * Emits a single header row, then one row per sample. Suitable for
 * piping into `awk`, `python -c "import csv,sys; ..."`, or Excel.
 *
 *   stty -F /dev/ttyUSB0 115200 raw -echo
 *   head -n 100 /dev/ttyUSB0 > samples.csv
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
    monitor.enable(RAM_MONITOR);
    monitor.enable(CPU_MONITOR);
    monitor.enable(STORAGE_MONITOR);
    monitor.enable(TEMPERATURE_MONITOR);
    monitor.enable(RUNTIME_MONITOR);

    monitor.printCSVHeader(Serial);
}

void loop() {
    monitor.update();
    if (monitor.available()) {
        monitor.printCSV(Serial);
    }
}
