# Desktop dashboard integration

The Arduino library is the **only** deliverable required to make the
telemetry useful. A future standalone desktop application — modelled
after Windows Task Manager — will consume the JSON telemetry stream
emitted by the library over USB Serial, UART, Wi-Fi or BLE.

This document is the **interface contract** between the library and
any future dashboard. The library does NOT depend on the dashboard.

## 1. Dashboard responsibilities

The dashboard:

- opens a transport (Serial port, TCP socket, BLE characteristic);
- reads the versioned JSON telemetry stream documented in
  [TELEMETRY_PROTOCOL.md](TELEMETRY_PROTOCOL.md);
- validates the `protocol` and `version` fields;
- renders live graphs for CPU, RAM, heap, fragmentation, storage,
  temperature, network throughput;
- renders a per-task table on FreeRTOS-enabled cores;
- raises toast notifications when the alerts engine's thresholds
  are crossed.

The dashboard MUST NOT invent fields. Anything outside the protocol
document is unspecified.

## 2. Recommended panes

| Pane | Source fields | Rendering |
|------|---------------|-----------|
| **CPU** | `cpu.usage_pct`, `cpu.frequency_hz`, `cpu.cores[*].usage_pct` | line graph, last 60 samples |
| **RAM** | `ram.used`, `ram.free`, `ram.peak`, `ram.min_ever_free` | stacked area + min/peak markers |
| **Heap fragmentation** | `ram.fragmentation_pct`, `ram.largest_free_block` | line graph, log scale on the block size |
| **PSRAM** | `ram.psram_total`, `ram.psram_used` (when `psram_total > 0`) | pie chart |
| **Storage** | `storage.flash_*`, `storage.fs_*`, `storage.sd_*` | donut chart per media |
| **Temperature** | `temperature.value_mc`, `temperature.min_mc`, `temperature.max_mc` | line graph with min/max bands |
| **Power** | `power.voltage_mv`, `power.current_ua`, `power.battery_pct` | gauges |
| **RTOS tasks** | `rtos.tasks[*]` | sortable table with stack-HWM heat indicator |
| **Network** | `network.connected`, `network.rssi_dbm`, `network.tx_bps`, `network.rx_bps` | link-state badge + throughput graph |
| **Uptime / reboot** | `runtime.uptime_ms`, `runtime.reboot_count`, `runtime.reset_reason` | big-number tiles |
| **Alerts** | emitted by `MonitorWarning` callbacks as separate JSON events | toast notifications |

## 3. Alert events

Alerts are NOT embedded in the periodic JSON snapshot. They are
emitted as separate JSON messages when a threshold is crossed. The
recommended wire format is:

```json
{"type":"alert","source":"CPU","severity":"WARNING","value":92,
 "threshold":90,"uptime_ms":1247}
```

The library already passes `MonitorWarning` structs to the user
callback; the sketch is responsible for the JSON encoding if it
wants the dashboard to receive them.

## 4. Wire format example

```
{"protocol":"mtm-telemetry","version":1,"board":"ESP32","uptime_ms":1247,...}
{"type":"alert","source":"CPU","severity":"WARNING","value":92,"threshold":90,"uptime_ms":1247}
{"protocol":"mtm-telemetry","version":1,"board":"ESP32","uptime_ms":2247,...}
{"protocol":"mtm-telemetry","version":1,"board":"ESP32","uptime_ms":3247,...}
```

Each line is a self-contained JSON document. The dashboard parses
line-by-line and dispatches based on the `protocol` field (telemetry)
or `type` field (alert).

## 5. Reference dashboard architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  Dashboard (Electron / Qt / native)                              │
│                                                                  │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐  │
│  │  CPU       │  │  RAM       │  │  Storage   │  │  Temp      │  │
│  │  graph     │  │  graph     │  │  donut     │  │  graph     │  │
│  └────────────┘  └────────────┘  └────────────┘  └────────────┘  │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐  │
│  │  Power     │  │  Net       │  │  RTOS      │  │  Uptime    │  │
│  │  gauges    │  │  graph     │  │  table     │  │  tiles     │  │
│  └────────────┘  └────────────┘  └────────────┘  └────────────┘  │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  Alerts stream (toast notifications)                       │  │
│  └────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                            ▲
                            │ JSON over Serial / TCP / BLE
                            │
┌─────────────────────────────────────────────────────────────────┐
│  Microcontroller (ESP32 / RP2040 / STM32 / AVR / SAMD)         │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  MicroTaskMonitor engine                                  │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## 6. Out of scope

The library **does not**:

- Draw any charts on a TFT / OLED. The user is free to do so by
  reading the snapshot directly, but the library provides no
  graphics primitives.
- Implement a TCP server. The user chooses whether to push telemetry
  out (e.g. via `WiFiTransport`) or expose it passively.
- Implement MQTT. The transport layer is pluggable; the user can
  write a 30-line `IMTMTransport` adapter that publishes JSON to any
  MQTT broker.
- Auto-discover the dashboard. Discovery is left to the host.

The library deliberately stays small. A future `mtm-gui` repository
will host the desktop dashboard; it will not change the Arduino
library at all.
