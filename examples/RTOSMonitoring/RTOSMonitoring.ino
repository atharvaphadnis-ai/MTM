/*
 * RTOSMonitoring.ino
 * ===================
 * Per-task FreeRTOS statistics example.
 *
 * Only useful on FreeRTOS-enabled cores: ESP32, ESP32-S2/S3, RP2040 with
 * FreeRTOS, STM32 with FreeRTOS. On bare-metal cores the RTOS monitor
 * reports NOT_SUPPORTED and this sketch prints a polite message.
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
    monitor.enable(RTOS_MONITOR);

    Serial.print(F("\n[RTOS] board="));
    Serial.println(monitor.boardName());
    Serial.print(F("RTOS supported? "));
    Serial.println(monitor.isSupported(RTOS_MONITOR) ? "yes" : "no");
}

void loop() {
    monitor.update();
    if (!monitor.available()) return;

    MTMRTOSSnapshot s = monitor.rtos();
    if (s.status != MTM_METRIC_MEASURED) {
        Serial.println(F("RTOS monitor: NOT_SUPPORTED on this board."));
        delay(5000);
        return;
    }

    Serial.println(F("\n--- RTOS Tasks ---"));
    Serial.print(F("task count: ")); Serial.println(s.taskCount);

    for (uint8_t i = 0; i < s.taskCount && i < MTM_MAX_TASKS; ++i) {
        Serial.print(F("  "));
        Serial.print(s.tasks[i].name);
        Serial.print(F(" state=")); Serial.print(s.tasks[i].state);
        Serial.print(F(" prio=")); Serial.print(s.tasks[i].priority);
        Serial.print(F(" hwm=")); Serial.print(s.tasks[i].stackHighWater);
        Serial.print(F(" runtime=")); Serial.println(s.tasks[i].runtime);
    }
}
