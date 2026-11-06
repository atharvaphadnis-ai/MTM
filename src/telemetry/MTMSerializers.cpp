/*
 * MTMSerializers.cpp
 * =================
 * Implementation of the telemetry serializers.
 *
 * Notes on style:
 *   - The JSON writer is hand-rolled. It is intentionally compact and
 *     does NOT escape non-printable characters other than the JSON
 *     structural ones (`"`, `\`). Callers are responsible for keeping
 *     board names ASCII.
 *   - Field order is fixed and documented in docs/TELEMETRY_PROTOCOL.md.
 *
 * License: MIT.
 */
#include "MTMSerializers.h"
#include "../hal/MTMPlatformDetect.h"

/* On Arduino we use Arduino.h for millis()/micros() and for Print. */
#if defined(ARDUINO)
#  include <Arduino.h>
#endif

namespace mtm {

/* ---------- low-level helpers ---------- */

void writeU32(MTM_PRINT_BASE& out, uint32_t v) {
    char buf[12];
    int i = 0;
    if (v == 0) { out.print("0"); return; }
    while (v > 0 && i < 10) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i > 0) out.write((uint8_t)buf[--i]);
}

void writeI32(MTM_PRINT_BASE& out, int32_t v) {
    if (v < 0) { out.write('-'); v = -v; }
    writeU32(out, (uint32_t)v);
}

void writeQuotedString(MTM_PRINT_BASE& out, const char* s) {
    out.write('"');
    if (s) {
        while (*s) {
            char c = *s++;
            if (c == '"' || c == '\\') out.write('\\');
            out.write((uint8_t)c);
        }
    }
    out.write('"');
}

void writeIndent(MTM_PRINT_BASE& out, uint8_t spaces) {
    for (uint8_t i = 0; i < spaces; ++i) out.write(' ');
}

/* ---------- status pretty-printer ---------- */
static const char* statusStr(MTMMetricStatus s) {
    switch (s) {
        case MTM_METRIC_MEASURED:      return "MEASURED";
        case MTM_METRIC_ESTIMATE:      return "ESTIMATE";
        case MTM_METRIC_NOT_SUPPORTED: return "NOT_SUPPORTED";
        case MTM_METRIC_DISABLED:      return "DISABLED";
        case MTM_METRIC_ERROR:         return "ERROR";
    }
    return "UNKNOWN";
}

/* ---------- human-readable report ---------- */

void writeReport(MTM_PRINT_BASE& out, const MTMSystemSnapshot& s) {
    out.println("==== MicroTask Monitor v1.0.0 ====");
    out.print("Board:        "); out.println(s.boardName);
    out.print("Uptime (ms):  "); writeU32(out, s.uptimeMs); out.println("");
    out.println("");

    /* RAM */
    out.println("-- RAM --");
    out.print("  Status:        "); out.println(statusStr(s.ram.status));
    if (s.ram.status != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  Total:        "); writeU32(out, s.ram.total); out.println(" bytes");
        out.print("  Used:         "); writeU32(out, s.ram.used); out.println(" bytes");
        out.print("  Free:         "); writeU32(out, s.ram.free); out.println(" bytes");
        out.print("  Min ever free:"); writeU32(out, s.ram.minEverFree); out.println(" bytes");
        out.print("  Peak used:    "); writeU32(out, s.ram.peak); out.println(" bytes");
        out.print("  Fragmentation:"); if (s.ram.fragmentation < 0) out.print(" NOT_SUPPORTED");
                                       else { writeI32(out, s.ram.fragmentation); out.print(" %"); }
        out.println("");
        out.print("  Largest free: "); writeU32(out, s.ram.largestFreeBlock); out.println(" bytes");
        out.print("  Alloc fails:  "); writeU32(out, s.ram.allocFailures); out.println("");
        out.print("  PSRAM:        "); out.println(statusStr(s.ram.psramStatus));
        if (s.ram.psramStatus == MTM_METRIC_MEASURED) {
            out.print("    Total:     "); writeU32(out, s.ram.psramTotal); out.println("");
            out.print("    Used:      "); writeU32(out, s.ram.psramUsed); out.println("");
        }
        out.print("  Stack:        "); out.println(statusStr(s.ram.stackStatus));
        if (s.ram.stackStatus == MTM_METRIC_MEASURED) {
            out.print("    Size:      "); writeU32(out, s.ram.stackSize); out.println("");
            out.print("    HWM:       "); writeU32(out, s.ram.stackHighWater); out.println("");
        }
    }
    out.println("");

    /* CPU */
    out.println("-- CPU --");
    out.print("  Freq:          "); out.println(statusStr(s.cpu.freqStatus));
    if (s.cpu.freqStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("    "); writeU32(out, s.cpu.frequencyHz); out.println(" Hz");
    }
    out.print("  Usage:         "); out.println(statusStr(s.cpu.usageStatus));
    if (s.cpu.usageStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("    "); writeU32(out, s.cpu.usagePercent); out.println(" %");
        if (s.cpu.coreCount > 1) {
            for (uint8_t i = 0; i < s.cpu.coreCount; ++i) {
                out.print("    Core "); out.print(i); out.print(": ");
                writeU32(out, s.cpu.cores[i].usagePercent); out.println(" %");
            }
        }
    }
    if (s.cpu.loadStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  Load avg (1/5/60s): ");
        writeU32(out, s.cpu.loadAvg1s); out.print(" / ");
        writeU32(out, s.cpu.loadAvg5s); out.print(" / ");
        writeU32(out, s.cpu.loadAvg60s); out.println(" %");
    }
    out.println("");

    /* Storage */
    out.println("-- Storage --");
    if (s.storage.flashStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  Flash:    "); writeU32(out, s.storage.flashUsed); out.print(" / ");
        writeU32(out, s.storage.flashTotal); out.println(" bytes");
    }
    if (s.storage.fsStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  FS:       "); writeU32(out, s.storage.fsUsed); out.print(" / ");
        writeU32(out, s.storage.fsTotal); out.println(" bytes");
    }
    if (s.storage.sdStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  SD:       "); writeU32(out, s.storage.sdUsed); out.print(" / ");
        writeU32(out, s.storage.sdTotal); out.println(" bytes");
    }
    if (s.storage.eepromStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  EEPROM:   "); writeU32(out, s.storage.eepromSize); out.println(" bytes");
    }
    out.println("");

    /* Temperature */
    out.println("-- Temperature --");
    out.print("  Status:   "); out.println(statusStr(s.temperature.status));
    if (s.temperature.status != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  Now:      "); writeI32(out, s.temperature.value / 1000); out.print(".");
        writeU32(out, (uint32_t)(s.temperature.value < 0 ? -s.temperature.value : s.temperature.value) % 1000);
        out.println(" C");
        out.print("  Min:      "); writeI32(out, s.temperature.min / 1000); out.println(" C");
        out.print("  Max:      "); writeI32(out, s.temperature.max / 1000); out.println(" C");
    }
    out.println("");

    /* Power */
    out.println("-- Power --");
    if (s.power.voltageStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  Voltage:  "); writeI32(out, s.power.voltage); out.println(" mV");
    }
    if (s.power.currentStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  Current:  "); writeI32(out, s.power.current); out.println(" uA");
    }
    if (s.power.batteryStatus != MTM_METRIC_NOT_SUPPORTED) {
        out.print("  Battery:  "); writeI32(out, s.power.batteryPercent); out.println(" %");
    }
    out.println("");

    /* RTOS */
    if (s.rtos.status != MTM_METRIC_NOT_SUPPORTED) {
        out.println("-- RTOS --");
        out.print("  Tasks:    "); writeU32(out, s.rtos.taskCount); out.println("");
        for (uint8_t i = 0; i < s.rtos.taskCount && i < MTM_MAX_TASKS; ++i) {
            out.print("    "); out.print(s.rtos.tasks[i].name);
            out.print(" state="); writeU32(out, s.rtos.tasks[i].state);
            out.print(" prio="); writeU32(out, s.rtos.tasks[i].priority);
            out.print(" hwm="); writeU32(out, s.rtos.tasks[i].stackHighWater);
            out.println("");
        }
        out.println("");
    }

    /* Network */
    if (s.network.status != MTM_METRIC_NOT_SUPPORTED) {
        out.println("-- Network --");
        out.print("  Connected: "); out.println(s.network.connected ? "yes" : "no");
        if (s.network.connected) {
            out.print("  RSSI:      "); writeI32(out, s.network.rssi); out.println(" dBm");
            out.print("  IP:        ");
            out.print((s.network.ip >> 24) & 0xFF); out.write('.');
            out.print((s.network.ip >> 16) & 0xFF); out.write('.');
            out.print((s.network.ip >>  8) & 0xFF); out.write('.');
            out.print((s.network.ip >>  0) & 0xFF); out.println("");
            out.print("  TX:        "); writeU32(out, s.network.bytesSent); out.println(" bytes");
            out.print("  RX:        "); writeU32(out, s.network.bytesReceived); out.println(" bytes");
            out.print("  TX rate:   "); writeU32(out, s.network.txThroughputBps); out.println(" B/s");
            out.print("  RX rate:   "); writeU32(out, s.network.rxThroughputBps); out.println(" B/s");
        }
        out.println("");
    }

    /* Runtime */
    if (s.runtime.status != MTM_METRIC_NOT_SUPPORTED) {
        out.println("-- Runtime --");
        out.print("  Reset reason: "); writeU32(out, s.runtime.resetReason); out.println("");
        out.print("  Reboot count: "); writeU32(out, s.runtime.rebootCount); out.println("");
        out.print("  WDT reset:    "); out.println(s.runtime.watchdogReset ? "yes" : "no");
    }
    out.println("");
}

/* ---------- JSON ---------- */

static void writeJSONKV(MTM_PRINT_BASE& out, const char* key, uint32_t v, bool first) {
    if (!first) out.write(',');
    writeQuotedString(out, key); out.write(':'); writeU32(out, v);
}
static void writeJSONKVs(MTM_PRINT_BASE& out, const char* key, int32_t v, bool first) {
    if (!first) out.write(',');
    writeQuotedString(out, key); out.write(':'); writeI32(out, v);
}
static void writeJSONKVb(MTM_PRINT_BASE& out, const char* key, bool v, bool first) {
    if (!first) out.write(',');
    writeQuotedString(out, key); out.write(':'); out.print(v ? "true" : "false");
}
static void writeJSONKQstr(MTM_PRINT_BASE& out, const char* key, const char* v, bool first) {
    if (!first) out.write(',');
    writeQuotedString(out, key); out.write(':'); writeQuotedString(out, v);
}

void writeJSON(MTM_PRINT_BASE& out, const MTMSystemSnapshot& s) {
    out.write('{');
    /* Header */
    writeJSONKQstr(out, "protocol", "mtm-telemetry", true);
    writeJSONKV(out, "version", MTM_PROTOCOL_VERSION, false);
    writeJSONKQstr(out, "board", s.boardName, false);
    writeJSONKV(out, "uptime_ms", s.uptimeMs, false);

    /* RAM */
    out.write(','); writeQuotedString(out, "ram"); out.write(':'); out.write('{');
    writeJSONKV(out, "total", s.ram.total, true);
    writeJSONKV(out, "used", s.ram.used, false);
    writeJSONKV(out, "free", s.ram.free, false);
    writeJSONKV(out, "min_ever_free", s.ram.minEverFree, false);
    writeJSONKV(out, "peak", s.ram.peak, false);
    writeJSONKVs(out, "fragmentation_pct", s.ram.fragmentation, false);
    writeJSONKV(out, "largest_free_block", s.ram.largestFreeBlock, false);
    writeJSONKV(out, "psram_total", s.ram.psramTotal, false);
    writeJSONKV(out, "psram_used", s.ram.psramUsed, false);
    writeJSONKV(out, "stack_size", s.ram.stackSize, false);
    writeJSONKV(out, "stack_hwm", s.ram.stackHighWater, false);
    writeJSONKV(out, "alloc_failures", s.ram.allocFailures, false);
    writeJSONKQstr(out, "status", statusStr(s.ram.status), false);
    out.write('}');

    /* CPU */
    out.write(','); writeQuotedString(out, "cpu"); out.write(':'); out.write('{');
    writeJSONKV(out, "frequency_hz", s.cpu.frequencyHz, true);
    writeJSONKV(out, "usage_pct", s.cpu.usagePercent, false);
    writeJSONKV(out, "core_count", s.cpu.coreCount, false);
    out.write(','); writeQuotedString(out, "cores"); out.write(':'); out.write('[');
    for (uint8_t i = 0; i < s.cpu.coreCount; ++i) {
        if (i) out.write(',');
        out.write('{');
        writeJSONKV(out, "id", s.cpu.cores[i].coreId, true);
        writeJSONKV(out, "usage_pct", s.cpu.cores[i].usagePercent, false);
        writeJSONKV(out, "idle_ms", s.cpu.cores[i].idleMs, false);
        writeJSONKV(out, "active_ms", s.cpu.cores[i].activeMs, false);
        out.write('}');
    }
    out.write(']');
    writeJSONKV(out, "load_avg_1s", s.cpu.loadAvg1s, false);
    writeJSONKV(out, "load_avg_5s", s.cpu.loadAvg5s, false);
    writeJSONKV(out, "load_avg_60s", s.cpu.loadAvg60s, false);
    writeJSONKQstr(out, "usage_status", statusStr(s.cpu.usageStatus), false);
    out.write('}');

    /* Storage */
    out.write(','); writeQuotedString(out, "storage"); out.write(':'); out.write('{');
    writeJSONKV(out, "flash_total", s.storage.flashTotal, true);
    writeJSONKV(out, "flash_used", s.storage.flashUsed, false);
    writeJSONKV(out, "fs_total", s.storage.fsTotal, false);
    writeJSONKV(out, "fs_used", s.storage.fsUsed, false);
    writeJSONKV(out, "sd_total", s.storage.sdTotal, false);
    writeJSONKV(out, "sd_used", s.storage.sdUsed, false);
    writeJSONKV(out, "eeprom_size", s.storage.eepromSize, false);
    writeJSONKV(out, "fs_write_cycles", s.storage.fsWriteCycles, false);
    out.write('}');

    /* Temperature */
    out.write(','); writeQuotedString(out, "temperature"); out.write(':'); out.write('{');
    writeJSONKVs(out, "value_mc", s.temperature.value, true);
    writeJSONKVs(out, "min_mc", s.temperature.min, false);
    writeJSONKVs(out, "max_mc", s.temperature.max, false);
    writeJSONKQstr(out, "status", statusStr(s.temperature.status), false);
    out.write('}');

    /* Power */
    out.write(','); writeQuotedString(out, "power"); out.write(':'); out.write('{');
    writeJSONKVs(out, "voltage_mv", s.power.voltage, true);
    writeJSONKVs(out, "current_ua", s.power.current, false);
    writeJSONKVs(out, "power_uw", s.power.powerMicrowatts, false);
    writeJSONKVs(out, "battery_pct", s.power.batteryPercent, false);
    out.write('}');

    /* GPU */
    out.write(','); writeQuotedString(out, "gpu"); out.write(':'); out.write('{');
    writeJSONKV(out, "usage_pct", s.gpu.usagePercent, true);
    writeJSONKV(out, "dma_usage_pct", s.gpu.dmaUsagePercent, false);
    writeJSONKQstr(out, "status", statusStr(s.gpu.status), false);
    out.write('}');

    /* RTOS */
    out.write(','); writeQuotedString(out, "rtos"); out.write(':'); out.write('{');
    writeJSONKV(out, "task_count", s.rtos.taskCount, true);
    writeJSONKV(out, "total_runtime", s.rtos.totalRuntime, false);
    out.write(','); writeQuotedString(out, "tasks"); out.write(':'); out.write('[');
    for (uint8_t i = 0; i < s.rtos.taskCount && i < MTM_MAX_TASKS; ++i) {
        if (i) out.write(',');
        out.write('{');
        writeJSONKQstr(out, "name", s.rtos.tasks[i].name, true);
        writeJSONKV(out, "state", s.rtos.tasks[i].state, false);
        writeJSONKV(out, "priority", s.rtos.tasks[i].priority, false);
        writeJSONKV(out, "stack_hwm", s.rtos.tasks[i].stackHighWater, false);
        writeJSONKV(out, "runtime", s.rtos.tasks[i].runtime, false);
        out.write('}');
    }
    out.write(']');
    out.write('}');

    /* Network */
    out.write(','); writeQuotedString(out, "network"); out.write(':'); out.write('{');
    writeJSONKVb(out, "connected", s.network.connected != 0, true);
    writeJSONKVs(out, "rssi_dbm", s.network.rssi, false);
    writeJSONKV(out, "ip", s.network.ip, false);
    writeJSONKV(out, "bytes_sent", s.network.bytesSent, false);
    writeJSONKV(out, "bytes_recv", s.network.bytesReceived, false);
    writeJSONKV(out, "link_uptime_ms", s.network.linkUptimeMs, false);
    writeJSONKV(out, "tx_bps", s.network.txThroughputBps, false);
    writeJSONKV(out, "rx_bps", s.network.rxThroughputBps, false);
    out.write('}');

    /* Runtime */
    out.write(','); writeQuotedString(out, "runtime"); out.write(':'); out.write('{');
    writeJSONKV(out, "uptime_ms", s.runtime.uptimeMs, true);
    writeJSONKV(out, "reboot_count", s.runtime.rebootCount, false);
    writeJSONKV(out, "reset_reason", s.runtime.resetReason, false);
    writeJSONKVb(out, "watchdog_reset", s.runtime.watchdogReset != 0, false);
    out.write('}');

    out.write('}');
}

/* ---------- CSV ---------- */

static void writeCSVField(MTM_PRINT_BASE& out, uint32_t v, bool first) {
    if (!first) out.write(',');
    writeU32(out, v);
}
static void writeCSVFieldS(MTM_PRINT_BASE& out, int32_t v, bool first) {
    if (!first) out.write(',');
    writeI32(out, v);
}

void writeCSVHeader(MTM_PRINT_BASE& out) {
    out.println("uptime_ms,ram_total,ram_used,ram_free,ram_peak,ram_frag_pct,"
                "cpu_freq_hz,cpu_usage_pct,flash_total,flash_used,fs_total,fs_used,"
                "temp_mc,voltage_mv,battery_pct,net_connected,net_rssi,net_tx_b,"
                "net_rx_b,net_tx_bps,net_rx_bps,rtos_task_count,reboot_count");
}

void writeCSV(MTM_PRINT_BASE& out, const MTMSystemSnapshot& s) {
    bool first = true;
    writeCSVField(out, s.uptimeMs, first); first = false;
    writeCSVField(out, s.ram.total, first);
    writeCSVField(out, s.ram.used, first);
    writeCSVField(out, s.ram.free, first);
    writeCSVField(out, s.ram.peak, first);
    writeCSVFieldS(out, s.ram.fragmentation, first);
    writeCSVField(out, s.cpu.frequencyHz, first);
    writeCSVField(out, s.cpu.usagePercent, first);
    writeCSVField(out, s.storage.flashTotal, first);
    writeCSVField(out, s.storage.flashUsed, first);
    writeCSVField(out, s.storage.fsTotal, first);
    writeCSVField(out, s.storage.fsUsed, first);
    writeCSVFieldS(out, s.temperature.value, first);
    writeCSVFieldS(out, s.power.voltage, first);
    writeCSVFieldS(out, s.power.batteryPercent, first);
    writeCSVField(out, s.network.connected, first);
    writeCSVFieldS(out, s.network.rssi, first);
    writeCSVField(out, s.network.bytesSent, first);
    writeCSVField(out, s.network.bytesReceived, first);
    writeCSVField(out, s.network.txThroughputBps, first);
    writeCSVField(out, s.network.rxThroughputBps, first);
    writeCSVField(out, s.rtos.taskCount, first);
    writeCSVField(out, s.runtime.rebootCount, first);
    out.write('\r'); out.write('\n');
}

} // namespace mtm
