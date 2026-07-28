---
name: ambient-embedded-verification
description: Validate AMBIENT UI, animation, display firmware, LCD hardware, component, and PCB changes with deterministic tests, render comparisons, timing and memory measurement, logic-analyzer or scope evidence, audio-stress testing, and manufacturing gates.
---

# AMBIENT Embedded Verification

Turn every important claim into a reproducible check.

## Build an evidence matrix

Read `../ambient-ui-lcd-smt/references/project-baseline.md`.

Use `references/evidence-matrix-template.md`. For each requirement record:

| Requirement | Method | Artifact/measurement | Pass threshold | Result | Open hardware check |
|---|---|---|---|---|---|

Do not use “looks smooth,” “should fit,” or “datasheet compatible” as a result.

## Software checks

- Run the repository's full host test suite.
- Run the realtime hot-path lint.
- Build every supported panel configuration affected by the change.
- Compile H743 with warnings treated as errors where the project supports it.
- Inspect the linker map after any buffer, asset, table, or driver change.
- Add deterministic tests for clipping, coordinate transforms, panel offsets, RGB565 byte order, state transitions, animation retargeting, dirty-region merging, and busy/error recovery.
- Render fixed input states through the host simulator and compare exact or tolerance-bounded outputs.

## Performance checks

Measure on the real target:

- frame update/render/flush duration distributions;
- bytes and region area per submitted frame;
- SPI clock and DMA gaps with a logic analyzer;
- input-to-visible-feedback latency;
- audio ISR worst-case latency and underrun count;
- DMA errors, cache artifacts, missed frames, and recovery behavior;
- CPU and memory high-water marks.

Report p50, p95, and p99 rather than one best-case timing.

## Visual checks

Use panel photographs or video at:

- actual scale and normal viewing distance;
- lowest and typical backlight;
- straight-on and expected off-axis angles;
- fast encoder motion and rapid mode changes;
- long static periods for burn-in/image-retention observation where relevant.

Check borders, single-pixel lines, text, gradients, color order, inversion, clipping, tearing, flicker, and animation interruption.

## Hardware and manufacturing checks

- Verify connector pin order and orientation with continuity measurement.
- Measure LCD rail, reset, backlight current, startup behavior, and SPI signal integrity.
- Check backlight PWM coupling into audio and power rails.
- Run KiCad ERC/DRC, 3D, 1:1 mechanical comparison, and fabrication-output checks.
- Verify exact MPN/package/footprint and JLCPCB preview for every changed component.

## Stress matrix

At minimum combine:

- maximum audio/FX load;
- continuous display animation;
- rapid encoder and key input;
- world/preset switching;
- phones/line and speaker states;
- USB and battery operation;
- minimum/maximum backlight;
- multi-hour soak.

State what was not testable without production hardware. Never convert an unrun hardware check into a pass.
