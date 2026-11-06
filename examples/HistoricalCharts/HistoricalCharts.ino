/*
 * HistoricalCharts.ino
 * ====================
 * Historical data export example.
 *
 * Demonstrates the in-engine circular history buffer. We keep the last
 * 32 RAM / CPU / temperature samples and dump them as a CSV stream every
 * 10 seconds so a host can draw charts.
 *
 * The library itself never draws charts — it provides the data so an
 * external application can. See docs/DESKTOP_DASHBOARD.md.
 *
 * Open the Serial Monitor at 115200 baud.
 */

#include <MicroTaskMonitor.h>

MicroTaskMonitor monitor;

uint32_t lastDump = 0;

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    delay(2000);
#endif

    monitor.begin();
    monitor.setSamplingInterval(500);
    monitor.enable(RAM_MONITOR);
    monitor.enable(CPU_MONITOR);
    monitor.enable(TEMPERATURE_MONITOR);

    Serial.println(F("uptime_ms,ram_used,ram_free,cpu_usage_pct,temp_mc"));
}

void loop() {
    monitor.update();
    if (!monitor.available()) return;

    uint32_t now = millis();
    if (now - lastDump < 10000) return;
    lastDump = now;

    /* Stream a single combined CSV row from the live snapshot. */
    MTMRAMSnapshot ram = monitor.memory();
    MTMCPUSnapshot cpu = monitor.cpu();
    MTMTemperatureSnapshot t = monitor.temperature();

    Serial.print(monitor.snapshot().uptimeMs); Serial.print(',');
    Serial.print(ram.used); Serial.print(',');
    Serial.print(ram.free); Serial.print(',');
    Serial.print(cpu.usagePercent); Serial.print(',');
    Serial.println(t.value);
}
