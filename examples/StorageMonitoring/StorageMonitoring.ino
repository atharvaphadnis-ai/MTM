/*
 * StorageMonitoring.ino
 * =====================
 * Flash, filesystem and SD card monitoring example.
 *
 * Demonstrates:
 *   - monitor.storage() structured access
 *   - Binding LittleFS as a filesystem (ESP32 / ESP8266 / RP2040)
 *   - SD card binding
 *   - EEPROM size on AVR
 *
 * Open the Serial Monitor at 115200 baud.
 */

#include <MicroTaskMonitor.h>

MicroTaskMonitor monitor;

#if defined(ARDUINO_ARCH_ESP32)
#  include <LittleFS.h>
#  define HAS_LITTLEFS 1
#elif defined(ARDUINO_ARCH_ESP8266)
#  include <LittleFS.h>
#  define HAS_LITTLEFS 1
#elif defined(ARDUINO_ARCH_RP2040)
#  include <LittleFS.h>
#  define HAS_LITTLEFS 1
#endif

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    delay(2000);
#endif

    monitor.begin();
    monitor.setSamplingInterval(2000);
    monitor.enable(STORAGE_MONITOR);

#if HAS_LITTLEFS
    if (LittleFS.begin(true /* format on fail */)) {
        Serial.println(F("[Storage] LittleFS mounted"));
        monitor.bindFilesystem(&LittleFS, /* totalBytes */ 0);
    } else {
        Serial.println(F("[Storage] LittleFS mount FAILED"));
    }
#endif
}

void loop() {
    monitor.update();
    if (!monitor.available()) return;

    MTMStorageSnapshot s = monitor.storage();

    Serial.println(F("\n--- Storage ---"));

    if (s.flashStatus == MTM_METRIC_MEASURED) {
        Serial.print(F("Flash:   ")); Serial.print(s.flashUsed); Serial.print(F(" / "));
        Serial.print(s.flashTotal); Serial.println(F(" bytes"));
    }
    if (s.fsStatus == MTM_METRIC_MEASURED) {
        Serial.print(F("FS:      ")); Serial.print(s.fsUsed); Serial.print(F(" / "));
        Serial.print(s.fsTotal); Serial.println(F(" bytes"));
    }
    if (s.sdStatus == MTM_METRIC_MEASURED) {
        Serial.print(F("SD:      ")); Serial.print(s.sdUsed); Serial.print(F(" / "));
        Serial.print(s.sdTotal); Serial.println(F(" bytes"));
    }
    if (s.eepromStatus == MTM_METRIC_MEASURED) {
        Serial.print(F("EEPROM:  ")); Serial.print(s.eepromSize); Serial.println(F(" bytes"));
    }
}
