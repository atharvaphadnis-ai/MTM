/*
 * MTMTestHostPrint.h
 * =================
 * Minimal Print-like helper used by the host unit tests to capture
 * serializer output into a fixed buffer for assertion.
 *
 * Inherits from MTM_PRINT_BASE so the serializer functions accept it
 * directly.
 *
 * License: MIT.
 */
#ifndef MTM_TEST_HOST_PRINT_H
#define MTM_TEST_HOST_PRINT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "telemetry/MTMSerializers.h"  /* for MTM_PRINT_BASE */

namespace mtm { namespace test {

class BufferPrint : public MTM_PRINT_BASE {
public:
    BufferPrint(char* dst, size_t cap) : _dst(dst), _cap(cap), _len(0) {
        if (cap > 0) _dst[0] = 0;
    }

    size_t write(const uint8_t* buf, size_t n) override {
        size_t free = (_cap > _len + 1) ? (_cap - _len - 1) : 0;
        size_t m = n < free ? n : free;
        if (m) { memcpy(_dst + _len, buf, m); _len += m; _dst[_len] = 0; }
        return m;
    }
    size_t write(uint8_t b) override { return write(&b, 1); }

    /* Compatibility shims that mirror Arduino's Print API. */
    size_t print(const char* s) { return write((const uint8_t*)s, strlen(s)); }
    size_t print(int v)         { char b[16]; snprintf(b, sizeof b, "%d", v); return print(b); }
    size_t print(unsigned v)    { char b[16]; snprintf(b, sizeof b, "%u", v); return print(b); }
    size_t println(const char* s) { size_t n = print(s); n += write((const uint8_t*)"\r\n", 2); return n; }
    size_t println()            { return write((const uint8_t*)"\r\n", 2); }

    size_t length() const { return _len; }
    const char* c_str() const { return _dst; }
    void clear() { _len = 0; if (_cap > 0) _dst[0] = 0; }
private:
    char*  _dst;
    size_t _cap;
    size_t _len;
};

}} // namespace mtm::test

#endif
