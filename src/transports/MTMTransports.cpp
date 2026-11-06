/*
 * MTMTransports.cpp
 * =================
 * Implementation of the optional Wi-Fi and BLE transports.
 *
 * The implementations live in a .cpp file so that the heavy headers are
 * only pulled in when the user actually wants Wi-Fi / BLE. On platforms
 * without these libraries the file compiles to nothing.
 *
 * License: MIT.
 */
#include "MTMTransports.h"

#if MTM_HAS_WIFI
#  if defined(MTM_PLATFORM_ESP32)
#    include <WiFi.h>
#    include <WiFiClient.h>
#  elif defined(MTM_PLATFORM_ESP8266)
#    include <ESP8266WiFi.h>
#  elif defined(MTM_PLATFORM_RP2040)
#    include <WiFi.h>
#  elif defined(MTM_PLATFORM_SAMD)
#    include <WiFi.h>
#  endif

namespace mtm {
void WiFiTransport::send(const char* buffer, size_t length) {
    if (!_client) return;
    WiFiClient* c = (WiFiClient*)_client;
    if (c && c->connected()) {
        c->write((const uint8_t*)buffer, length);
    } else {
        _connected = false;
    }
}
} // namespace mtm
#endif

#if MTM_HAS_BLE
#  include <BLEDevice.h>
#  include <BLEUtils.h>
namespace mtm {
void BLETransport::send(const char* buffer, size_t length) {
    if (!_ch) return;
    BLECharacteristic* c = (BLECharacteristic*)_ch;
    c->setValue((uint8_t*)buffer, (int)length);
    c->notify();
}
} // namespace mtm
#endif
