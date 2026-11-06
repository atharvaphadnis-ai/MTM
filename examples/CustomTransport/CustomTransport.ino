/*
 * CustomTransport.ino
 * ====================
 * User-defined transport adapter example.
 *
 * Demonstrates how to implement the IMTMTransport interface and stream
 * telemetry to anywhere — here, an ESP32 hardware UART (Serial1) on
 * GPIO 16/17 instead of USB.
 *
 * The same pattern works for any target: implement IMTMTransport,
 * instantiate your adapter, and pass it to monitor.addTransport().
 *
 * Open the Serial Monitor at 115200 baud for the human-readable
 * secondary output, and connect GPIO 16 (RX on the receiving device's
 * TX) / 17 (TX on the receiving device's RX) to capture telemetry.
 */

#include <MicroTaskMonitor.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
#  include <HardwareSerial.h>
#endif

MicroTaskMonitor monitor;

/*
 * Custom transport: writes telemetry to a hardware UART (Serial1) on
 * boards that have one, and falls back to the USB Serial on boards
 * that do not (AVR Uno, Nano, etc.).
 */
class MyCustomTransport : public mtm::IMTMTransport {
public:
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
    MyCustomTransport() {
        Serial1.begin(115200, SERIAL_8N1, 16, 17);
    }
    void send(const char* buffer, size_t length) override {
        Serial1.write((const uint8_t*)buffer, length);
    }
#else
    MyCustomTransport() {}
    void send(const char* buffer, size_t length) override {
        Serial.write((const uint8_t*)buffer, length);
    }
#endif
    const char* name() const override { return "custom"; }
    bool ready() const override { return true; }
};

MyCustomTransport myTransport;

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    delay(2000);
#endif

    monitor.begin();
    monitor.setSamplingInterval(1000);
    monitor.enable(RAM_MONITOR);
    monitor.enable(CPU_MONITOR);
    monitor.enable(TEMPERATURE_MONITOR);

    /* Send every JSON snapshot to our custom transport. */
    monitor.addTransport(myTransport);
}

void loop() {
    monitor.update();
    if (monitor.available()) {
        /* Build a JSON payload into a local buffer and ship it to the
         * transport. We avoid Arduino String by writing directly into
         * a fixed char array. */
        char buf[1024];
        /* Use the serializer to a Print-to-memory helper, then forward
         * the result. For the example we just call printJSON on a
         * capturing Print subclass — left as an exercise for the user. */
        monitor.printJSON(Serial);  /* mirror to USB for inspection */
        Serial.println();
    }
}
