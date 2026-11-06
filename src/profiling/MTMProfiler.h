/*
 * MTMProfiler.h
 * =============
 * Lightweight performance profiler.
 *
 * Usage:
 *   MTM_PROFILE("loop");          // begin a scope
 *   ...
 *   MTM_PROFILE_END("loop");      // end the scope
 *
 * Or use the RAII helper:
 *   {
 *       MTMProfileScope scope("loop");
 *       ...
 *   }
 *
 * The profiler maintains one entry per named scope. Scopes are hashed
 * by a simple FNV-1a of their name, which keeps lookup O(1) for the
 * small number of scopes used in a typical sketch.
 *
 * Statistics kept per scope:
 *   - last execution time (microseconds)
 *   - min, max
 *   - count (number of completed invocations)
 *   - average (sum/count)
 *
 * A small fixed ring buffer per scope keeps the last
 * MTM_PROFILER_HISTORY samples for "execution time history".
 *
 * License: MIT.
 */
#ifndef MTM_PROFILER_H
#define MTM_PROFILER_H

#include <stdint.h>
#include <stddef.h>
#include "../history/MTMHistory.h"

#define MTM_PROFILER_MAX_SCOPES 16
#define MTM_PROFILER_HISTORY    32

namespace mtm {

struct ProfileScopeEntry {
    uint32_t hash;
    char     name[16];
    uint32_t lastUs;
    uint32_t minUs;
    uint32_t maxUs;
    uint32_t sumUs;
    uint32_t count;
    History<uint32_t, MTM_PROFILER_HISTORY> history;
};

class Profiler {
public:
    Profiler() : _count(0) {
        for (size_t i = 0; i < MTM_PROFILER_MAX_SCOPES; ++i) {
            _entries[i].hash = 0;
            _entries[i].name[0] = 0;
            _entries[i].lastUs = 0;
            _entries[i].minUs = (uint32_t)-1;
            _entries[i].maxUs = 0;
            _entries[i].sumUs = 0;
            _entries[i].count = 0;
        }
    }

    /* Record the start of a scope. Returns the scope index for endScope().
     * If the table is full, returns -1 and the call becomes a no-op. */
    int8_t beginScope(const char* name) {
        uint32_t h = _hash(name);
        int8_t idx = _find(h, name);
        if (idx < 0) idx = _alloc(h, name);
        return idx;
    }

    /* Record the end of a scope. `startUs` is the value of micros() at the
     * matching beginScope(). */
    void endScope(int8_t idx, uint32_t startUs, uint32_t nowUs) {
        if (idx < 0 || idx >= MTM_PROFILER_MAX_SCOPES) return;
        uint32_t elapsed = nowUs - startUs;
        ProfileScopeEntry& e = _entries[idx];
        e.lastUs = elapsed;
        if (elapsed < e.minUs) e.minUs = elapsed;
        if (elapsed > e.maxUs) e.maxUs = elapsed;
        e.sumUs += elapsed;
        ++e.count;
        e.history.push(elapsed);
    }

    size_t scopeCount() const { return _count; }

    const ProfileScopeEntry* scope(size_t idx) const {
        if (idx >= _count) return 0;
        return &_entries[idx];
    }

    void reset() {
        for (size_t i = 0; i < _count; ++i) {
            _entries[i].lastUs = 0;
            _entries[i].minUs = (uint32_t)-1;
            _entries[i].maxUs = 0;
            _entries[i].sumUs = 0;
            _entries[i].count = 0;
            _entries[i].history.clear();
        }
    }

private:
    uint32_t _hash(const char* s) const {
        /* FNV-1a 32-bit */
        uint32_t h = 2166136261u;
        while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
        return h;
    }
    int8_t _find(uint32_t h, const char* name) const {
        for (uint8_t i = 0; i < _count; ++i) {
            if (_entries[i].hash == h) {
                /* Confirm the names match to avoid hash collisions. */
                uint8_t j = 0;
                while (j < 16 && _entries[i].name[j] && name[j]) {
                    if (_entries[i].name[j] != name[j]) break;
                    ++j;
                }
                if (j == 16 || _entries[i].name[j] == 0 || name[j] == 0) return (int8_t)i;
            }
        }
        return -1;
    }
    int8_t _alloc(uint32_t h, const char* name) {
        if (_count >= MTM_PROFILER_MAX_SCOPES) return -1;
        ProfileScopeEntry& e = _entries[_count];
        e.hash = h;
        uint8_t i = 0;
        while (i + 1 < 16 && name[i]) { e.name[i] = name[i]; ++i; }
        e.name[i] = 0;
        e.lastUs = 0;
        e.minUs = (uint32_t)-1;
        e.maxUs = 0;
        e.sumUs = 0;
        e.count = 0;
        return (int8_t)(_count++);
    }

    ProfileScopeEntry _entries[MTM_PROFILER_MAX_SCOPES];
    uint8_t           _count;
};

/*
 * RAII helper. Construct with the scope name; destruction records the
 * elapsed time. Use the MTM_PROFILE macro below to make this easy.
 */
class ProfileScopeRAII {
public:
    ProfileScopeRAII(Profiler* p, const char* name)
        : _p(p), _idx(p ? p->beginScope(name) : -1), _startUs(_mtmMicros()) {}
    ~ProfileScopeRAII() { if (_p && _idx >= 0) _p->endScope(_idx, _startUs, _mtmMicros()); }
private:
    Profiler* _p;
    int8_t    _idx;
    uint32_t  _startUs;
    /* micros() is provided by Arduino; on host tests the platform
     * shim defines it. */
    static uint32_t _mtmMicros();
};

} // namespace mtm

/* User-facing convenience macro. The scope name must be a string literal
 * or a const char* with static storage duration. */
#define MTM_PROFILE(profiler, name) \
    ::mtm::ProfileScopeRAII _mtm_prof_scope_(profiler, name)

#endif // MTM_PROFILER_H
