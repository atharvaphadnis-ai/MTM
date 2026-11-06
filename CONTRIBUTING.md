# Contributing to MicroTask Monitor

Thanks for your interest in improving **MicroTask Monitor**! This document
explains how to contribute code, examples, tests and platform backends.

## Code of conduct

Be respectful, be precise, and assume good faith. Embedded systems touch a
lot of platforms — disagreements about trade-offs are normal and welcome as
long as they stay technical.

## Project priorities

When contributing, keep the four project priorities in mind, in this order:

1. **Portability** — code must compile across every supported platform.
2. **Reliability** — no fake metrics, no undefined behaviour, no blocking
   monitoring operations.
3. **Extensibility** — new platforms and metrics must slot in without
   touching the public API.
4. **Minimal resource overhead** — no mandatory heap allocation, no
   mandatory `String`, no mandatory RTOS, no background threads.

## Repository layout

```
src/
  MicroTaskMonitor.h         public API
  MicroTaskMonitor.cpp        engine
  core/                       public types, capability enum
  hal/                        hardware abstraction interfaces
  metrics/                    monitoring module interfaces
  platforms/<arch>/           platform-specific backends
  profiling/                  performance profiler
  history/                    circular buffers
  alerts/                     threshold engine
  telemetry/                  JSON / CSV / report serializers
  transports/                 Serial / UART / Wi-Fi / BLE / custom
examples/                     Arduino IDE examples
tests/                        host-side unit tests
docs/                         documentation and diagrams
```

## Adding a new platform backend

1. Add a folder under `src/platforms/<arch>/`.
2. Implement every `I…Backend` interface declared in `src/hal/` that the
   platform can support. Functions that the platform cannot implement
   must return `MTM_NOT_SUPPORTED`.
3. Add the architecture detection macro to `src/hal/MTMPlatformDetect.h`.
4. Add the new backend to the factory in `src/MicroTaskMonitor.cpp`.
5. Add a row to the compatibility table in `README.md` and `docs/`.
6. Add at least one host-side mock in `tests/mocks/` if the platform
   introduces new metric semantics.

## Style

- Modern C++ compatible with the Arduino Core's minimum toolchain (C++11).
- Two-space indentation, no tabs.
- `lowerCamelCase` for functions and variables, `UpperCamelCase` for types,
  `UPPER_SNAKE_CASE` for macros and enum constants.
- Every public function has a `/** … */` Doxygen comment.
- No `String` in the public API. Use `char*` + size or `Print&`.
- No `malloc`/`new` in the public API. Use static or stack storage.

## Tests

Run the host-side test suite before opening a pull request:

```bash
cd tests
make
./mtm_tests
```

Every new calculation, serializer, threshold or history feature must come
with a unit test. Platform-specific behaviour that cannot be tested on the
host must be documented in `tests/PLATFORM_NOTES.md`.

## Pull request checklist

- [ ] Code compiles on at least two platforms (one AVR-class, one 32-bit).
- [ ] No new mandatory dependencies.
- [ ] Public API surface unchanged unless the PR is a major version bump.
- [ ] New metrics are guarded by capability checks, not by `#ifdef`.
- [ ] Tests added and passing.
- [ ] `README.md` compatibility table updated.
- [ ] `CHANGELOG.md` entry added under `[Unreleased]`.

## Reporting issues

When reporting an issue, include:

- Board and Arduino core version.
- PlatformIO or Arduino IDE version.
- A minimal sketch that reproduces the problem.
- The exact `printReport(Serial)` output.
- Whether the issue is reproducible on a second board of the same type.

## Licensing

By contributing you agree that your contributions are licensed under the
project's MIT license, as described in `LICENSE`.
