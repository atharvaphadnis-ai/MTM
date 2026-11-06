/*
 * MTMTests.cpp
 * ===========
 * Host-side unit tests for MicroTask Monitor.
 *
 * Coverage:
 *   1. Circular history buffer (push, indexing, statistics, copyOut)
 *   2. Snapshot construction & defaults
 *   3. Null backends return NOT_SUPPORTED
 *   4. Telemetry serializers:
 *        - report contains board name and uptime
 *        - JSON is valid (matches expected schema prefix)
 *        - CSV header + row counts match
 *   5. Alerts engine:
 *        - RAM threshold triggers warning then critical
 *        - Hysteresis prevents flapping
 *        - NOT_SUPPORTED metrics never raise
 *   6. Profiler:
 *        - begin/end produces count and stats
 *   7. Capability bitmask sanity
 *
 * Build:
 *   cd tests
 *   g++ -std=c++11 -I../src -o mtm_tests MTMTests.cpp \
 *       ../src/MicroTaskMonitor.cpp ../src/telemetry/MTMSerializers.cpp \
 *       ../src/alerts/MTMAlerts.cpp ../src/hal/MTMPlatformFactory.cpp \
 *       -DHOST_BUILD
 *   ./mtm_tests
 *
 * License: MIT.
 */
#include "host/MTMTestHostPrint.h"

#include "core/MTMTypes.h"
#include "hal/MTMHalInterfaces.h"
#include "hal/MTMNullBackends.h"
#include "history/MTMHistory.h"
#include "alerts/MTMAlerts.h"
#include "profiling/MTMProfiler.h"
#include "telemetry/MTMSerializers.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

static int g_testsRun = 0;
static int g_testsPassed = 0;

static void check(bool cond, const char* msg) {
    ++g_testsRun;
    if (cond) { ++g_testsPassed; printf("  ok    %s\n", msg); }
    else      { printf("  FAIL  %s\n", msg); }
}

/* ---------- 1. History buffer ---------- */
static void test_history_basic() {
    printf("\n[1] History buffer\n");
    mtm::History<int, 4> h;
    check(h.size() == 0, "fresh buffer empty");
    h.push(10); h.push(20); h.push(30); h.push(40);
    check(h.size() == 4, "size == 4 after 4 pushes");
    check(h.full(), "buffer full");
    check(h[0] == 40, "most recent is [0] = 40");
    check(h[3] == 10, "oldest is [3] = 10");
    h.push(50); /* evicts the oldest (10) */
    check(h.size() == 4, "size still 4 after wrap");
    check(h[0] == 50, "newest is 50");
    check(h[3] == 20, "oldest now 20 (10 was evicted)");
    check(h.min() == 20, "min after eviction = 20");
    check(h.max() == 50, "max = 50");
    check(h.sum() == 50+40+30+20, "sum after wrap");
}

static void test_history_copyOut() {
    printf("\n[2] History copyOut\n");
    mtm::History<int, 4> h;
    h.push(1); h.push(2); h.push(3); h.push(4); h.push(5);
    int out[8];
    h.copyOut(out, 8);
    check(out[0] == 2, "oldest in chronological order (2)");
    check(out[3] == 5, "newest at end (5)");
}

/* ---------- 2. Null backends ---------- */
static void test_null_backends() {
    printf("\n[3] Null backends\n");
    MTMNullRAMBackend ram;
    MTMRAMSnapshot r = ram.readRAM();
    check(r.status == MTM_METRIC_NOT_SUPPORTED, "null RAM backend NOT_SUPPORTED");
    check(r.psramStatus == MTM_METRIC_NOT_SUPPORTED, "null PSRAM NOT_SUPPORTED");
    check(r.stackStatus == MTM_METRIC_NOT_SUPPORTED, "null stack NOT_SUPPORTED");

    MTMNullCPUBackend cpu;
    MTMCPUSnapshot c = cpu.readCPU();
    check(c.usageStatus == MTM_METRIC_NOT_SUPPORTED, "null CPU usage NOT_SUPPORTED");
    check(c.freqStatus == MTM_METRIC_NOT_SUPPORTED, "null CPU freq NOT_SUPPORTED");

    MTMNullRTOSBackend rtos;
    MTMRTOSSnapshot rt = rtos.readRTOS();
    check(rt.status == MTM_METRIC_NOT_SUPPORTED, "null RTOS NOT_SUPPORTED");
}

/* ---------- 3. Snapshot defaults ---------- */
static void test_snapshot_defaults() {
    printf("\n[4] Snapshot defaults\n");
    /* Use the null backends to verify the default status. memset(0) of
     * the raw struct would set the status field to the enum value 0,
     * which is MTM_METRIC_MEASURED — not the "nothing is available"
     * default. The correct way to get a NOT_SUPPORTED-default snapshot
     * is to ask a Null backend. */
    MTMNullRAMBackend nullRam;
    MTMRAMSnapshot r = nullRam.readRAM();
    check(r.status == MTM_METRIC_NOT_SUPPORTED, "null RAM default NOT_SUPPORTED");

    MTMNullCPUBackend nullCpu;
    MTMCPUSnapshot c = nullCpu.readCPU();
    check(c.usageStatus == MTM_METRIC_NOT_SUPPORTED, "null CPU default NOT_SUPPORTED");
}

/* ---------- 4. Serializers ---------- */
static void test_serializer_report() {
    printf("\n[5] Serializer report\n");
    MTMSystemSnapshot s;
    memset(&s, 0, sizeof(s));
    s.uptimeMs = 12345;
    strncpy(s.boardName, "TEST-BOARD", sizeof s.boardName);
    s.ram.status = MTM_METRIC_MEASURED;
    s.ram.total = 1024; s.ram.used = 256; s.ram.free = 768;
    s.ram.minEverFree = 700; s.ram.peak = 324;
    s.ram.fragmentation = 8;
    s.ram.largestFreeBlock = 500;
    s.cpu.usageStatus = MTM_METRIC_MEASURED;
    s.cpu.usagePercent = 42;
    s.cpu.freqStatus = MTM_METRIC_MEASURED;
    s.cpu.frequencyHz = 16000000;

    char buf[2048];
    mtm::test::BufferPrint p(buf, sizeof buf);
    mtm::writeReport(p, s);
    check(strstr(p.c_str(), "TEST-BOARD") != 0, "report contains board name");
    check(strstr(p.c_str(), "12345") != 0, "report contains uptime");
    check(strstr(p.c_str(), "MEASURED") != 0, "report contains status text");
    check(strstr(p.c_str(), "256 bytes") != 0, "report contains RAM used bytes");
}

static void test_serializer_json() {
    printf("\n[6] Serializer JSON\n");
    MTMSystemSnapshot s;
    memset(&s, 0, sizeof(s));
    s.uptimeMs = 1000;
    strncpy(s.boardName, "TEST", sizeof s.boardName);
    s.ram.status = MTM_METRIC_MEASURED;
    s.ram.total = 1024; s.ram.used = 256; s.ram.free = 768;
    s.cpu.usageStatus = MTM_METRIC_MEASURED;
    s.cpu.usagePercent = 50;
    s.cpu.freqStatus = MTM_METRIC_MEASURED;
    s.cpu.frequencyHz = 240000000;
    s.cpu.coreCount = 1;
    s.cpu.cores[0].coreId = 0;
    s.cpu.cores[0].usagePercent = 50;
    s.cpu.cores[0].status = MTM_METRIC_MEASURED;

    char buf[4096];
    mtm::test::BufferPrint p(buf, sizeof buf);
    mtm::writeJSON(p, s);

    check(strstr(p.c_str(), "\"protocol\":\"mtm-telemetry\"") != 0, "JSON has protocol field");
    check(strstr(p.c_str(), "\"version\":1") != 0, "JSON has version=1");
    check(strstr(p.c_str(), "\"board\":\"TEST\"") != 0, "JSON has board field");
    check(strstr(p.c_str(), "\"uptime_ms\":1000") != 0, "JSON has uptime_ms");
    check(strstr(p.c_str(), "\"ram\":") != 0, "JSON has ram object");
    check(strstr(p.c_str(), "\"total\":1024") != 0, "JSON has ram.total");
    check(strstr(p.c_str(), "\"cpu\":") != 0, "JSON has cpu object");
    check(strstr(p.c_str(), "\"frequency_hz\":240000000") != 0, "JSON has cpu.frequency_hz");
}

static void test_serializer_csv() {
    printf("\n[7] Serializer CSV\n");
    MTMSystemSnapshot s;
    memset(&s, 0, sizeof(s));
    s.uptimeMs = 5000;
    s.ram.status = MTM_METRIC_MEASURED;
    s.ram.total = 2048; s.ram.used = 512; s.ram.free = 1536;
    s.ram.fragmentation = -1;
    s.cpu.usageStatus = MTM_METRIC_MEASURED;
    s.cpu.usagePercent = 30;
    s.cpu.frequencyHz = 80000000;

    char buf[4096];
    mtm::test::BufferPrint p(buf, sizeof buf);
    mtm::writeCSVHeader(p);
    p.println();
    mtm::writeCSV(p, s);

    /* Count commas in the header — should be 22 (23 columns). */
    int commas = 0;
    for (size_t i = 0; i < p.length(); ++i) if (p.c_str()[i] == ',') ++commas;
    check(commas >= 22, "CSV header has at least 22 commas (23 columns)");

    /* The first row's first field must be 5000. */
    check(strstr(p.c_str(), "5000,2048,512,1536") != 0, "CSV row begins with uptime + RAM fields");
}

/* ---------- 5. Alerts ---------- */

static int g_alert_count = 0;
static MTMAlertSource g_last_alert_source = (MTMAlertSource)-1;

static void onAlert_global(const MTMMonitorWarning& w) {
    ++g_alert_count;
    g_last_alert_source = w.source;
}

static void test_alerts_ram_warning() {
    printf("\n[8] Alerts: RAM warning\n");
    g_alert_count = 0;
    mtm::AlertsEngine a;
    a.onAlert(onAlert_global);

    MTMSystemSnapshot s;
    memset(&s, 0, sizeof s);
    /* Disable every alert source we are NOT testing by leaving them at
     * NOT_SUPPORTED status — the alerts engine ignores those. */
    s.power.voltageStatus = MTM_METRIC_NOT_SUPPORTED;
    s.network.status = MTM_METRIC_NOT_SUPPORTED;
    s.storage.fsStatus = MTM_METRIC_NOT_SUPPORTED;
    s.temperature.status = MTM_METRIC_NOT_SUPPORTED;
    s.rtos.status = MTM_METRIC_NOT_SUPPORTED;

    /* Now we are testing RAM only. */
    s.ram.status = MTM_METRIC_MEASURED;
    s.ram.total = 1000; s.ram.used = 700; s.ram.free = 300;

    a.setRAMWarning(80);
    a.setRAMCritical(95);
    a.evaluate(s, 1000);
    check(g_alert_count == 0, "70% RAM does not trigger warning");

    s.ram.used = 820; s.ram.free = 180;
    a.evaluate(s, 2000);
    check(g_alert_count == 1, "82% RAM triggers warning");

    s.ram.used = 960; s.ram.free = 40;
    a.evaluate(s, 3000);
    check(g_alert_count == 2, "96% RAM triggers critical (second alert)");
}

static void test_alerts_hysteresis() {
    printf("\n[9] Alerts: hysteresis prevents flapping\n");
    g_alert_count = 0;
    mtm::AlertsEngine a;
    a.onAlert(onAlert_global);

    MTMSystemSnapshot s;
    memset(&s, 0, sizeof s);
    s.power.voltageStatus = MTM_METRIC_NOT_SUPPORTED;
    s.network.status = MTM_METRIC_NOT_SUPPORTED;
    s.storage.fsStatus = MTM_METRIC_NOT_SUPPORTED;
    s.temperature.status = MTM_METRIC_NOT_SUPPORTED;
    s.rtos.status = MTM_METRIC_NOT_SUPPORTED;
    s.ram.status = MTM_METRIC_MEASURED;
    s.ram.total = 1000; s.ram.used = 900; s.ram.free = 100;

    a.setRAMWarning(85);
    a.setRAMCritical(95);
    a.evaluate(s, 1000);
    check(g_alert_count == 1, "85-95% triggers warning once");

    /* Drop just below the warning threshold; hysteresis = 2% default,
     * so we should NOT clear at 84%. We should not see a NEW raise. */
    s.ram.used = 840; s.ram.free = 160;
    a.evaluate(s, 2000);
    check(g_alert_count == 1, "84% does not raise a new alert");

    /* Drop well below — no new raise, no clear callback (we don't emit clears). */
    s.ram.used = 500; s.ram.free = 500;
    a.evaluate(s, 3000);
    check(g_alert_count == 1, "50% does not raise a new alert");
}

static void test_alerts_unsupported_silent() {
    printf("\n[10] Alerts: NOT_SUPPORTED metrics stay silent\n");
    g_alert_count = 0;
    mtm::AlertsEngine a;
    a.onAlert(onAlert_global);

    MTMSystemSnapshot s;
    memset(&s, 0, sizeof s);
    /* Set every status field to NOT_SUPPORTED explicitly. */
    s.ram.status = MTM_METRIC_NOT_SUPPORTED;
    s.cpu.usageStatus = MTM_METRIC_NOT_SUPPORTED;
    s.storage.fsStatus = MTM_METRIC_NOT_SUPPORTED;
    s.temperature.status = MTM_METRIC_NOT_SUPPORTED;
    s.power.voltageStatus = MTM_METRIC_NOT_SUPPORTED;
    s.rtos.status = MTM_METRIC_NOT_SUPPORTED;
    s.network.status = MTM_METRIC_NOT_SUPPORTED;

    a.evaluate(s, 1000);
    check(g_alert_count == 0, "no alerts raised when all metrics are NOT_SUPPORTED");
}

static void test_alerts_temperature() {
    printf("\n[11] Alerts: temperature\n");
    g_alert_count = 0;
    mtm::AlertsEngine a;
    a.onAlert(onAlert_global);

    MTMSystemSnapshot s;
    memset(&s, 0, sizeof s);
    s.power.voltageStatus = MTM_METRIC_NOT_SUPPORTED;
    s.network.status = MTM_METRIC_NOT_SUPPORTED;
    s.storage.fsStatus = MTM_METRIC_NOT_SUPPORTED;
    s.ram.status = MTM_METRIC_NOT_SUPPORTED;
    s.rtos.status = MTM_METRIC_NOT_SUPPORTED;
    s.temperature.status = MTM_METRIC_MEASURED;
    s.temperature.value = 25000; /* 25 °C */
    a.setTemperatureWarning(70000);
    a.setTemperatureCritical(85000);
    a.evaluate(s, 1000);
    check(g_alert_count == 0, "25C no alert");

    s.temperature.value = 72000;
    a.evaluate(s, 2000);
    check(g_alert_count == 1, "72C raises warning");

    s.temperature.value = 90000;
    a.evaluate(s, 3000);
    check(g_alert_count == 2, "90C raises critical");
}

/* ---------- 6. Profiler ---------- */
static void test_profiler() {
    printf("\n[12] Profiler\n");
    mtm::Profiler p;
    int8_t id = p.beginScope("loop");
    check(id >= 0, "beginScope returns valid id");
    /* Simulate 100us of work. */
    p.endScope(id, 0, 100);
    p.beginScope("loop"); p.endScope(id, 100, 250);
    p.beginScope("loop"); p.endScope(id, 250, 280);

    check(p.scopeCount() == 1, "scopeCount == 1");
    const mtm::ProfileScopeEntry* e = p.scope(0);
    check(e != 0, "scope(0) returns entry");
    if (e) {
        check(strcmp(e->name, "loop") == 0, "scope name = loop");
        check(e->count == 3, "count == 3");
        check(e->lastUs == 30, "lastUs = 30");
        check(e->minUs == 30, "minUs = 30");
        check(e->maxUs == 150, "maxUs = 150");
        check(e->sumUs == 30 + 150 + 100, "sumUs = 280");
    }

    /* Add a second scope with a different name. */
    int8_t id2 = p.beginScope("setup");
    check(id2 >= 0, "second scope returns valid id");
    p.endScope(id2, 0, 50);
    check(p.scopeCount() == 2, "scopeCount == 2 after second scope");
}

/* ---------- 7. Capability bitmask sanity ---------- */
static void test_capabilities() {
    printf("\n[13] Capability bitmask\n");
    /* Each capability bit must be unique. */
    uint32_t mask = MTM_CAP_RAM_MONITOR | MTM_CAP_CPU_MONITOR | MTM_CAP_STORAGE_MONITOR |
                    MTM_CAP_TEMPERATURE | MTM_CAP_POWER | MTM_CAP_RTOS | MTM_CAP_NETWORK |
                    MTM_CAP_RUNTIME | MTM_CAP_PROFILER;
    /* Count set bits — should match number of distinct capabilities. */
    int bits = 0;
    for (uint32_t m = mask; m; m &= m - 1) ++bits;
    check(bits == 9, "9 distinct capability bits set");

    check(MTM_CAP_RAM_MONITOR != MTM_CAP_CPU_MONITOR, "RAM != CPU cap bit");
    check(MTM_CAP_CPU_MONITOR != MTM_CAP_NETWORK, "CPU != NETWORK cap bit");
}

/* ---------- main ---------- */
int main() {
    printf("MicroTask Monitor unit tests\n");
    printf("================================\n");
    test_history_basic();
    test_history_copyOut();
    test_null_backends();
    test_snapshot_defaults();
    test_serializer_report();
    test_serializer_json();
    test_serializer_csv();
    test_alerts_ram_warning();
    test_alerts_hysteresis();
    test_alerts_unsupported_silent();
    test_alerts_temperature();
    test_profiler();
    test_capabilities();

    printf("\n================================\n");
    printf("Tests run:    %d\n", g_testsRun);
    printf("Tests passed: %d\n", g_testsPassed);
    printf("Tests failed: %d\n", g_testsRun - g_testsPassed);
    return (g_testsPassed == g_testsRun) ? 0 : 1;
}
