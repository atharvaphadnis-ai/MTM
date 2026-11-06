/*
 * NetworkMonitoring.ino
 * =====================
 * Wi-Fi link monitoring example.
 *
 * Tested on ESP32 and ESP8266. Other platforms will print
 * NOT_SUPPORTED.
 *
 * Connect the board to a 2.4 GHz Wi-Fi network (set SSID/PASSWORD below)
 * and open the Serial Monitor at 115200 baud.
 */

#include <MicroTaskMonitor.h>

MicroTaskMonitor monitor;

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || \
    defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_SAMD)
#  include <WiFi.h>
#  define HAS_WIFI 1
#endif

const char* WIFI_SSID     = "your-ssid";
const char* WIFI_PASSWORD = "your-password";

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    delay(2000);
#endif

    monitor.begin();
    monitor.setSamplingInterval(2000);
    monitor.enable(NETWORK_MONITOR);

    Serial.print(F("\n[Network] board="));
    Serial.println(monitor.boardName());

#if HAS_WIFI
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print(F("[Network] connecting"));
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); Serial.print('.');
    }
    Serial.println(F(" connected."));
    Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
#endif
}

void loop() {
    monitor.update();
    if (!monitor.available()) return;

    MTMNetworkSnapshot s = monitor.network();
    if (s.status != MTM_METRIC_MEASURED) {
        Serial.println(F("Network: NOT_SUPPORTED"));
        delay(5000);
        return;
    }

    Serial.println(F("\n--- Network ---"));
    Serial.print(F("connected: ")); Serial.println(s.connected ? "yes" : "no");
    if (s.connected) {
        Serial.print(F("RSSI:      ")); Serial.print(s.rssi); Serial.println(F(" dBm"));
        Serial.print(F("bytes sent: ")); Serial.println(s.bytesSent);
        Serial.print(F("bytes recv: ")); Serial.println(s.bytesReceived);
        Serial.print(F("TX rate:    ")); Serial.print(s.txThroughputBps); Serial.println(F(" B/s"));
        Serial.print(F("RX rate:    ")); Serial.print(s.rxThroughputBps); Serial.println(F(" B/s"));
    }
}
