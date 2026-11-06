# Telemetry protocol (v1)

This document is the **stable** specification of the telemetry payload
emitted by MicroTask Monitor. Downstream consumers (the future desktop
dashboard, MQTT bridges, cloud aggregators) MUST treat this protocol
as the contract they parse against.

## 1. Transport-agnostic shape

The protocol is the **payload**, independent of transport. The same
payload can be sent over USB Serial, UART, Wi-Fi TCP, ESP32 BLE or any
user-defined transport. Transports are documented separately in
[TRANSPORTS.md](TRANSPORTS.md).

## 2. JSON shape

JSON output is compact (no whitespace) and starts with a stable
header followed by per-module objects. The complete schema:

```json
{
  "protocol": "mtm-telemetry",
  "version": 1,
  "board": "ESP32",
  "uptime_ms": 1247,

  "ram": {
    "total": 319232,
    "used": 14432,
    "free": 304800,
    "min_ever_free": 294112,
    "peak": 25120,
    "fragmentation_pct": 5,
    "largest_free_block": 291840,
    "psram_total": 0,
    "psram_used": 0,
    "stack_size": 8192,
    "stack_hwm": 5300,
    "alloc_failures": 0,
    "status": "MEASURED"
  },

  "cpu": {
    "frequency_hz": 240000000,
    "usage_pct": 7,
    "core_count": 2,
    "cores": [
      { "id": 0, "usage_pct": 7, "idle_ms": 1234, "active_ms": 89 },
      { "id": 1, "usage_pct": 4, "idle_ms": 1290, "active_ms": 33 }
    ],
    "load_avg_1s": 7,
    "load_avg_5s": 7,
    "load_avg_60s": 7,
    "usage_status": "MEASURED"
  },

  "storage": {
    "flash_total": 4194304,
    "flash_used": 1332016,
    "fs_total": 524288,
    "fs_used": 12288,
    "sd_total": 3191496704,
    "sd_used": 1048576,
    "eeprom_size": 0,
    "fs_write_cycles": 0
  },

  "temperature": {
    "value_mc": 53000,
    "min_mc": 50000,
    "max_mc": 53000,
    "status": "MEASURED"
  },

  "power": {
    "voltage_mv": 3300,
    "current_ua": 0,
    "power_uw": 0,
    "battery_pct": -1
  },

  "gpu": {
    "usage_pct": 0,
    "dma_usage_pct": 0,
    "status": "NOT_SUPPORTED"
  },

  "rtos": {
    "task_count": 8,
    "total_runtime": 12345,
    "tasks": [
      { "name": "loop0", "state": 1, "priority": 1, "stack_hwm": 1200, "runtime": 4221 },
      { "name": "IDLE0", "state": 1, "priority": 0, "stack_hwm": 60,  "runtime": 8124 }
    ]
  },

  "network": {
    "connected": true,
    "rssi_dbm": -55,
    "ip": 16909060,
    "bytes_sent": 2048,
    "bytes_recv": 4096,
    "link_uptime_ms": 60000,
    "tx_bps": 12,
    "rx_bps": 50
  },

  "runtime": {
    "uptime_ms": 1247,
    "reboot_count": 3,
    "reset_reason": 1,
    "watchdog_reset": false
  }
}
```

## 3. Field semantics

Every numeric field can be absent of meaning when the platform does
not support the metric. The `status` field of each sub-object tells
the consumer whether the numeric fields are valid:

| Status | Meaning |
|--------|---------|
| `MEASURED`      | Read from a real counter / sensor. Numeric fields are valid. |
| `ESTIMATE`     | Derived from an indirect measurement. Numeric fields are best-effort. |
| `NOT_SUPPORTED`| The platform cannot supply this metric. Numeric fields are 0. |
| `DISABLED`     | The user disabled this metric at runtime. Numeric fields are 0. |
| `ERROR`        | The backend returned an error. Numeric fields are 0. |

When `status == NOT_SUPPORTED`, **consumers MUST NOT interpret the
numeric fields**. They are guaranteed to be 0 but contain no
information.

## 4. Units

| Field group | Unit |
|-------------|------|
| RAM `*`, `largest_free_block`, `stack_*` | bytes |
| `fragmentation_pct`, `usage_pct`, `battery_pct` | percent (0..100) |
| `frequency_hz` | hertz |
| `idle_ms`, `active_ms`, `uptime_ms`, `link_uptime_ms` | milliseconds |
| `value_mc`, `min_mc`, `max_mc` (temperature) | millidegrees Celsius |
| `voltage_mv` | millivolts |
| `current_ua` | microamps |
| `power_uw` | microwatts |
| `bytes_sent`, `bytes_recv` | bytes |
| `tx_bps`, `rx_bps` | bytes / second |
| `runtime` (task runtime) | FreeRTOS tick-equivalent microseconds |
| `stack_hwm` | bytes remaining (lower = closer to overflow) |
| `fs_write_cycles` | count |

## 5. IP encoding

`network.ip` is the IPv4 address in **network byte order** as a 32-bit
unsigned integer. To render it dotted:

```python
ip = snapshot["network"]["ip"]
dotted = f"{ip >> 24 & 0xFF}.{ip >> 16 & 0xFF}.{ip >> 8 & 0xFF}.{ip & 0xFF}"
```

## 6. JSON Schema (formal)

The full JSON Schema is published as `docs/telemetry-v1.schema.json`
alongside this document. Consumers can use any JSON-Schema validator
to verify the payload before processing it.

## 7. Versioning policy

- `version` is bumped on every **backwards-incompatible** change.
  Consumers MUST check this value before parsing.
- Adding a new optional field is **backwards-compatible** and does
  not bump the version.
- Renaming or removing a field, or changing its type or unit, is
  **backwards-incompatible** and requires a version bump.

The current version is `1`.

## 8. CSV shape

CSV uses CRLF line terminators. The first row is a fixed header
emitted by `printCSVHeader()`:

```
uptime_ms,ram_total,ram_used,ram_free,ram_peak,ram_frag_pct,
cpu_freq_hz,cpu_usage_pct,flash_total,flash_used,fs_total,fs_used,
temp_mc,voltage_mv,battery_pct,net_connected,net_rssi,net_tx_b,
net_rx_b,net_tx_bps,net_rx_bps,rtos_task_count,reboot_count
```

Each subsequent row is one sample. The field order is fixed and
matches the header exactly. Unlike JSON, CSV does NOT carry status
fields — consumers should expect 0 for unsupported metrics.

## 9. Human-readable report

The `printReport()` output is intended for human reading during
debugging. It is multi-line, ASCII-only, and uses `--` and `==`
separators. The format is **not** versioned; consumers should prefer
JSON or CSV for machine parsing.
