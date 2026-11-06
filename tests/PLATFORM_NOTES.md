# Platform validation notes

This document records the validation status of every supported
platform. It is the canonical source of truth for "has this been
tested on real hardware?".

## Validation status legend

- ✅ **Compiled + run on real hardware.** Telemetry output has been
  eyeballed against the documented schema.
- 🟡 **Compiled but not run on real hardware.** PlatformIO or
  Arduino IDE builds succeed; live hardware validation pending.
- ⛔ **Not yet ported.** Listed for future work.

## Status table

| Platform | Compile | Live test | Notes |
|----------|:-------:|:---------:|-------|
| AVR — Arduino Uno (ATmega328P) | ✅ | ✅ | Real `SP` and `__brkval` based free-RAM reading; on-chip temperature calibrated ±10 °C. |
| AVR — Arduino Nano (ATmega328P) | ✅ | ✅ | Identical to Uno. |
| AVR — Arduino Mega 2560 | ✅ | 🟡 | Compiles clean. Stack-pointer free-RAM verified in unit tests; full live test pending. |
| AVR — Arduino Leonardo (ATmega32U4) | ✅ | 🟡 | USB CDC serial; needs ~2 s warm-up before telemetry starts. |
| ESP32 (classic) | ✅ | ✅ | FreeRTOS per-task stats verified; multicore usage via `vTaskGetInfo`. |
| ESP32-S2 | ✅ | 🟡 | Single-core Xtensa. PSRAM absent on Saola. |
| ESP32-S3 | ✅ | 🟡 | Dual-core Xtensa LX7. PSRAM present on DevKitC-1. |
| ESP32-C3 | ✅ | 🟡 | Single-core RISC-V. No internal temperature sensor — temperature reads `NOT_SUPPORTED`. |
| ESP32-C6 | ✅ | 🟡 | Single-core RISC-V. No internal temperature sensor. |
| ESP8266 (NodeMCU v2) | ✅ | ✅ | `ESP.getHeapFragmentation()` and `ESP.getMaxFreeBlockSize()` verified. |
| STM32 F1 (BluePill F103C8) | ✅ | ✅ | DWT cycle counter for CPU usage; on-chip temperature via ADC channel 16. |
| STM32 F4 (BlackPill F411) | ✅ | 🟡 | Compiles clean. Live test pending. |
| STM32 G0 (Nucleo-G070RB) | ✅ | 🟡 | Compiles clean. Live test pending. |
| STM32 H7 (Nucleo-H743ZI) | ✅ | 🟡 | Compiles clean. Per-core multicore stats on H7 dual-core not implemented yet. |
| STM32 F0 (Nucleo-F091RC) | ✅ | 🟡 | Compiles clean. Live test pending. |
| RP2040 (Raspberry Pi Pico) | ✅ | ✅ | Internal temperature via ADC channel 4; multicore stats via SIO spinlock. |
| RP2040 (Pico W) | ✅ | 🟡 | Network metric compiled; live Wi-Fi test pending. |
| SAMD21 (Arduino Zero) | ✅ | 🟡 | Compiles clean. No on-chip temperature sensor. |
| SAMD51 (Adafruit Grand Central M4) | ✅ | 🟡 | Compiles clean. On-chip temperature sensor needs SUPC enable. |

## Mocks for unit tests

`tests/mocks/` (created as the need arises) contains deterministic
mock backends that return pre-canned values. The library currently
ships the **Null** backends (`src/hal/MTMNullBackends.h`) which serve
as the canonical "everything is `NOT_SUPPORTED`" mocks.

## How to validate a board yourself

1. Install the matching `pio run -e <env>` toolchain.
2. Plug in the board. Identify the serial port.
3. Run `pio run -e <env> -t upload`.
4. Run `pio device monitor -p <port> -b 115200`.
5. Open the `BasicMonitoring` example, upload and watch for:
   - The `Board:` line shows the expected board name.
   - `Platform capabilities: 0x` shows a non-zero mask.
   - The `-- RAM --` section reports a non-zero `Total`.
   - At least one status field shows `MEASURED`.

If any of these fail, please open an issue with the full Serial
output and your `pio run --verbose` log.
