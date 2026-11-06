/*
 * BasicMonitoring.ino
 * ===================
 * Minimal MicroTask Monitor sketch. Reports RAM, CPU, storage and
 * temperature once per second over USB serial.
 *
 * Tested on: Arduino Uno, Nano, Mega, Leonardo, ESP32, ESP8266,
 *           STM32 F1/F4, Raspberry Pi Pico, Arduino Zero.
 *
 * Open the Serial Monitor at 115200 baud.
 */

#include <MicroTaskMonitor.h>

MicroTaskMonitor monitor;

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    delay(2000); /* give the USB-CDC stack time to enumerate */
#endif

    Serial.println(F("\n[MicroTask Monitor] starting..."));

    monitor.begin();
    monitor.setSamplingInterval(1000);
    monitor.enable(RAM_MONITOR);
    monitor.enable(CPU_MONITOR);
    monitor.enable(STORAGE_MONITOR);
    monitor.enable(TEMPERATURE_MONITOR);
    monitor.enable(RUNTIME_MONITOR);

    Serial.print(F("Detected board: "));
    Serial.println(monitor.boardName());

    Serial.print(F("Platform capabilities: 0x"));
    Serial.println(monitor.capabilities(), HEX);
    Serial.print(F("PSRAM supported? "));
    Serial.println(monitor.isSupported(PSRAM_MONITOR) ? "yes" : "no");
}

void loop() {
    monitor.update();
    if (monitor.available()) {
        monitor.printReport(Serial);
        Serial.println(F(""));
        delay(5000); /* avoid flooding the monitor */
    }
}
