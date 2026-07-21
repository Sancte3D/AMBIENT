# AMBIENT effects engine — integration handoff

This directory is a self-contained, buildable handoff of the complete C11
ambient/dreamy/nostalgic effects engine. It is deliberately separate from the
live product firmware so it can be reviewed, tested, and integrated without
silently overwriting the current engine.

Start with [`INTEGRATION_TASK.md`](INTEGRATION_TASK.md). The DSP contract and algorithm
details are in [`docs/EFFECTS_ENGINE.md`](docs/EFFECTS_ENGINE.md).

## Included

- bit-exact bypass;
- long dark eight-line FDN reverb;
- filtered ping-pong delay;
- slow chorus and detune;
- tape wow, flutter, drift, bandwidth loss, saturation, hiss, and hum;
- scheduled live reverse swell;
- true offline/look-ahead reverse reverb;
- restrained octave shimmer;
- deterministic temporal blur;
- complete DREAM CHAIN;
- property tests, sanitizers, benchmark, and deterministic WAV renderer.

## Standalone host build

```sh
make verify
make sanitize
make benchmark
make previews
```

Or with CMake:

```sh
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake
ctest --test-dir build-cmake --output-on-failure
```

Generated binaries and WAV files stay below `build/` and are not required for
target integration.

## Verified handoff baseline

- strict C11 build with warnings treated as errors;
- 478,857 effects checks, zero failures;
- AddressSanitizer and UndefinedBehaviorSanitizer pass;
- private state: 680 / 4,096 bytes;
- default hot arena: 214,489 / 245,760 bytes;
- DREAM CHAIN host regression benchmark: approximately 85–95× real time across
  the recorded host runs.

Host speed is not evidence of STM32H743 deadline safety. The product integration
is complete only after linker-map inspection, DWT profiling, and an underrun
soak on hardware.
