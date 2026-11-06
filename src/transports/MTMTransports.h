/*
 * MTMTransports.h
 * ===============
 * Modular transport adapters for MicroTask Monitor telemetry.
 *
 * Each transport is an `IMTMTransport` that the engine calls with the
 * serialized telemetry payload. The engine does not know whether the
 * transport is Serial, UART, Wi-Fi, BLE or a user adapter.
 *
 * Optional transports (Wi-Fi, BLE) are conditionally compiled:
 *   - WiFiTransport is only available if the sketch includes <WiFi.h> /
 *     <ESP8266WiFi.h>. We detect this by checking for the presence of
 *     `WiFi` as a class — the relevant Arduino cores expose it.
 *   - BLETransport is only available on ESP32 with the built-in
 *     `BLEDevice.h` include. We do NOT include it here; the user must
 *     include it themselves and pass the BLE characteristic into the
 *     transport constructor.
 *
 * User-defined transports: implement IMTMTransport and pass an instance
 * to MicroTaskMonitor::addTransport().
 *
 * License: MIT.
 */
#ifndef MTM_TRANSPORTS_H
#define MTM_TRANSPORTS_H

#include "../core/MTMTypes.h"
#include "../telemetry/MTMSerializers.h"

namespace mtm {

/*
 * Base interface. `send()` is called by the engine from user context.
 * Implementations MUST NOT block for long periods (>= the sampling
 * interval) — doing so would corrupt monitoring cadence.
 */
class IMTMTransport {
public:
    virtual ~IMTMTransport() {}
    virtual void begin() {}
    virtual void send(const char* buffer, size_t length) = 0;
    virtual const char* name() const = 0;
    virtual bool ready() const = 0;
};

/*
 * USB / hardware serial transport. Works with any `Stream`/`Print`
 * subclass that the Arduino core provides.
 */
class SerialTransport : public IMTMTransport {
public:
    SerialTransport(MTM_PRINT_BASE& port) : _port(port) {}
    void send(const char* buffer, size_t length) override {
        _port.write((const uint8_t*)buffer, length);
    }
    const char* name() const override { return "serial"; }
    bool ready() const override { return true; }
private:
    MTM_PRINT_BASE& _port;
};

/*
 * Generic UART transport. Identical to SerialTransport except it
 * records a baud rate for diagnostic output.
 */
class UARTTransport : public IMTMTransport {
public:
    UARTTransport(MTM_PRINT_BASE& port, uint32_t baud = 115200)
        : _port(port), _baud(baud) {}
    void send(const char* buffer, size_t length) override {
        _port.write((const uint8_t*)buffer, length);
    }
    const char* name() const override { return "uart"; }
    bool ready() const override { return true; }
private:
    MTM_PRINT_BASE& _port;
    uint32_t        _baud;
};

} // namespace mtm

/* ---- Optional Wi-Fi transport ----
 * Available only on cores that expose a `WiFi` global. We detect at
 * compile time using the platform macros to avoid including heavy
 * headers on AVR.
 */
#if defined(MTM_PLATFORM_ESP32) || defined(MTM_PLATFORM_ESP8266) || \
    defined(MTM_PLATFORM_RP2040) || defined(MTM_PLATFORM_SAMD)
#  define MTM_HAS_WIFI 1
#else
#  define MTM_HAS_WIFI 0
#endif

#if MTM_HAS_WIFI
namespace mtm {
/*
 * The user is expected to have included the WiFi library themselves.
 * We forward-declare only what we need.
 */
class WiFiTransport : public IMTMTransport {
public:
    /* `client` is an open TCP connection to the dashboard.
     * `client` lifetime is owned by the user. */
    WiFiTransport(/* WiFiClient */ void* client) : _client(client), _connected(true) {}
    void send(const char* buffer, size_t length) override;
    const char* name() const override { return "wifi"; }
    bool ready() const override { return _connected; }
    void setConnected(bool c) { _connected = c; }
private:
    void* _client;
    bool  _connected;
};
} // namespace mtm
#endif // MTM_HAS_WIFI

/* ---- Optional BLE transport (ESP32 only) ---- */
#if defined(MTM_PLATFORM_ESP32)
#  define MTM_HAS_BLE 1
#else
#  define MTM_HAS_BLE 0
#endif

#if MTM_HAS_BLE
namespace mtm {
/*
 * The user is expected to include BLEDevice.h themselves and pass the
 * characteristic they want telemetry written to.
 */
class BLETransport : public IMTMTransport {
public:
    /* `characteristic` is a `BLECharacteristic*`. We store it as void*
     * here to keep the header dependency-free. */
    BLETransport(void* characteristic) : _ch(characteristic) {}
    void send(const char* buffer, size_t length) override;
    const char* name() const override { return "ble"; }
    bool ready() const override { return _ch != 0; }
private:
    void* _ch;
};
} // namespace mtm
#endif // MTM_HAS_BLE

#endif // MTM_TRANSPORTS_H
