# Transports

This document describes the transport layer of MicroTask Monitor and
how to write a custom transport.

## 1. Base interface

```cpp
class IMTMTransport {
public:
    virtual void begin() {}
    virtual void send(const char* buffer, size_t length) = 0;
    virtual const char* name() const = 0;
    virtual bool ready() const = 0;
};
```

The engine calls `send()` only when `ready()` returns true. The
`name()` accessor is used for diagnostics and does not need to be
unique across transport instances.

## 2. Built-in transports

| Class | Header | Available on | Notes |
|-------|--------|--------------|-------|
| `SerialTransport` | `<MicroTaskMonitor.h>` | every Arduino core | Wraps any `Print&`. |
| `UARTTransport` | `<MicroTaskMonitor.h>` | every Arduino core | Like SerialTransport but records a baud rate. |
| `WiFiTransport` | `<MicroTaskMonitor.h>` + user-included `<WiFi.h>` | ESP32, ESP8266, RP2040-Pico-W, SAMD-WINC | Pushes to an open `WiFiClient`. |
| `BLETransport` | `<MicroTaskMonitor.h>` + user-included `<BLEDevice.h>` | ESP32 only | Notify a `BLECharacteristic`. |

Optional transports are conditionally compiled — they only exist
when the relevant Arduino core is selected. The headers
`<WiFi.h>` / `<BLEDevice.h>` are **not** included by the library;
the user includes them in their sketch when they want the matching
transport.

## 3. Using a transport

```cpp
SerialTransport serial(Serial);
monitor.addTransport(serial);

#if MTM_HAS_WIFI
WiFiClient client;
client.connect("192.168.1.100", 9999);
WiFiTransport wifi(&client);
monitor.addTransport(wifi);
#endif
```

After `addTransport()`, every call to `monitor.printJSON(Serial)`
etc. will be mirrored to the transport. Transports do not have to
be passed the same Print as the direct call — the user controls
what goes where.

## 4. Writing a custom transport

```cpp
class MQTTTransport : public mtm::IMTMTransport {
public:
    MQTTTransport(PubSubClient& client, const char* topic)
        : _client(client), _topic(topic) {}

    void send(const char* buffer, size_t length) override {
        _client.publish(_topic, buffer);
    }
    const char* name() const override { return "mqtt"; }
    bool ready() const override { return _client.connected(); }
private:
    PubSubClient& _client;
    const char* _topic;
};
```

## 5. Lifetime and ownership

The engine does NOT take ownership of transport objects. The user
must keep the transport object alive for as long as it is registered.
Calling `clearTransports()` removes all registrations but does not
destruct the transport objects.

## 6. Concurrency

`send()` is called from the user's `loop()` context, never from an
ISR. It is safe to call any user-context function inside `send()`.

`ready()` is called twice per `sendToTransports()` invocation —
once before `send()` and once for the next transport. Keep it cheap.

## 7. Throughput considerations

The JSON snapshot is typically 1–4 KB on AVR-class boards and 4–10 KB
on 32-bit cores. At 115200 baud a 4 KB snapshot takes ~340 ms. For
real-time streaming consider:

- Using `printCSV()` instead of `printJSON()` — CSV is ~10× smaller.
- Bumping the UART baud rate to 921600 on platforms that support it.
- Using Wi-Fi or BLE transports on supported boards.
- Disabling modules you do not care about with `monitor.disable(cap)`.
