/*
 * MTMSerializers.h
 * ===============
 * Telemetry serializers: human-readable serial report, JSON, CSV.
 *
 * All serializers write to a `Print&` so they work with `Serial`,
 * `SoftwareSerial`, `WiFiClient`, `BLECharacteristic` writers, etc.
 *
 * JSON is written with a small hand-rolled writer — the library does
 * NOT depend on ArduinoJson or any other JSON library. Output is
 * compact (no whitespace) and always stable in field order.
 *
 * CSV uses CRLF line terminators and a header row.
 *
 * License: MIT.
 */
#ifndef MTM_SERIALIZERS_H
#define MTM_SERIALIZERS_H

#include "../core/MTMTypes.h"

#if defined(ARDUINO)
#  include <Print.h>
#  define MTM_PRINT_BASE Print
#else
/* Host build: provide a minimal Print base class for the unit tests. */
#  include <stdio.h>
class MTM_PRINT_BASE {
public:
    virtual size_t write(const uint8_t* buf, size_t n) = 0;
    virtual size_t write(uint8_t b) { return write(&b, 1); }
    size_t print(const char* s) { return write((const uint8_t*)s, _strlen(s)); }
    size_t print(int v) { char b[16]; return print(_itoa(v, b)); }
    size_t print(unsigned v) { char b[16]; return print(_uitoa(v, b)); }
    size_t println(const char* s) { size_t n = print(s); n += write('\r'); n += write('\n'); return n; }
private:
    static size_t _strlen(const char* s) { size_t n = 0; while (s && *s++) ++n; return n; }
    static char* _itoa(int v, char* b) { sprintf(b, "%d", v); return b; }
    static char* _uitoa(unsigned v, char* b) { sprintf(b, "%u", v); return b; }
};
#endif

namespace mtm {

/*
 * Human-readable report. ~1.2 KB on AVR. Multi-line, ASCII-only.
 * Suitable for `Serial` output during debugging.
 */
void writeReport(MTM_PRINT_BASE& out, const MTMSystemSnapshot& s);

/*
 * JSON telemetry. Stable schema, versioned via `MTM_PROTOCOL_VERSION`.
 * Compact form, no whitespace.
 */
void writeJSON(MTM_PRINT_BASE& out, const MTMSystemSnapshot& s);

/*
 * CSV row. The first time `writeCSV` is called on a given Print the user
 * should call `writeCSVHeader(out)` to emit the column names. After that
 * one call per sample is sufficient.
 */
void writeCSVHeader(MTM_PRINT_BASE& out);
void writeCSV(MTM_PRINT_BASE& out, const MTMSystemSnapshot& s);

/*
 * Generic helpers exposed for custom transports.
 */
void writeU32(MTM_PRINT_BASE& out, uint32_t v);
void writeI32(MTM_PRINT_BASE& out, int32_t v);
void writeQuotedString(MTM_PRINT_BASE& out, const char* s);
void writeIndent(MTM_PRINT_BASE& out, uint8_t spaces);

} // namespace mtm

#endif // MTM_SERIALIZERS_H
