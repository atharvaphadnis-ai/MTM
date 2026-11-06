/*
 * MTMHistory.h
 * ============
 * Fixed-size circular history buffer used by every monitoring module.
 *
 * Design:
 *   - Header-only, templated on the sample type and capacity.
 *   - No heap allocation. Storage is a fixed C array.
 *   - O(1) push, O(N) iteration. N is bounded by CAPACITY.
 *   - Thread-safety: none. The engine calls push() from the same context
 *     (loop or update()) so no locking is needed.
 *
 * Used by:
 *   - RAMMonitor for heap-free history
 *   - CPUMonitor for utilisation history
 *   - TemperatureMonitor for temperature history
 *   - NetworkMonitor for throughput history
 *   - PerformanceProfiler for execution-time history
 *
 * License: MIT.
 */
#ifndef MTM_HISTORY_H
#define MTM_HISTORY_H

#include <stdint.h>
#include <stddef.h>

namespace mtm {

template <typename T, size_t CAPACITY>
class History {
public:
    History() : _head(0), _size(0), _sum() {
        for (size_t i = 0; i < CAPACITY; ++i) _buf[i] = T();
    }

    /* Add a sample. If the buffer is full, the oldest sample is dropped. */
    void push(const T& sample) {
        _sum += sample;
        if (_size == CAPACITY) {
            _sum -= _buf[_head];
        } else {
            ++_size;
        }
        _buf[_head] = sample;
        _head = (_head + 1) % CAPACITY;
    }

    /* Number of samples currently stored (0..CAPACITY). */
    size_t size() const { return _size; }
    size_t capacity() const { return CAPACITY; }
    bool empty() const { return _size == 0; }
    bool full() const { return _size == CAPACITY; }

    /* Reset the buffer to empty. */
    void clear() {
        _head = 0; _size = 0; _sum = T();
        for (size_t i = 0; i < CAPACITY; ++i) _buf[i] = T();
    }

    /* Index the most recent sample as [0]. Older samples as [1], [2]...
     * Returns the default-constructed T when idx >= size(). */
    T operator[](size_t idx) const {
        if (idx >= _size) return T();
        /* Most recent entry is at (_head - 1) circularly. */
        size_t pos = (_head + CAPACITY - 1 - idx) % CAPACITY;
        return _buf[pos];
    }

    /* Statistical helpers — only meaningful for arithmetic types. */
    T sum() const { return _sum; }

    T average() const {
        if (_size == 0) return T();
        return (T)(_sum / (typename _Integral<T>::type)_size);
    }

    T min() const {
        if (_size == 0) return T();
        T m = _buf[0];
        for (size_t i = 1; i < _size; ++i) {
            if (_buf[i] < m) m = _buf[i];
        }
        return m;
    }

    T max() const {
        if (_size == 0) return T();
        T m = _buf[0];
        for (size_t i = 1; i < _size; ++i) {
            if (_buf[i] > m) m = _buf[i];
        }
        return m;
    }

    /* Raw access — used by serializers that need to dump the whole
     * buffer in chronological order (oldest first). */
    void copyOut(T* dst, size_t dstCap) const {
        size_t n = _size < dstCap ? _size : dstCap;
        if (n == 0) return;
        /* Oldest entry is at (_head - _size) mod CAPACITY. */
        size_t start = (_head + CAPACITY - _size) % CAPACITY;
        for (size_t i = 0; i < n; ++i) {
            dst[i] = _buf[(start + i) % CAPACITY];
        }
    }

private:
    /* Helper to pick an integral divisor for average(). For non-integral
     * T the user is expected to call sum()/size() themselves. */
    template <typename U>
    struct _Integral { typedef U type; };

    T       _buf[CAPACITY];
    size_t  _head;
    size_t  _size;
    T       _sum;
};

} // namespace mtm

#endif // MTM_HISTORY_H
