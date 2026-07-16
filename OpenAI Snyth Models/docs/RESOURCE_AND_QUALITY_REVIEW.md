# Resource and quality review

## Enforced budgets

| Resource | Limit | Enforcement |
|---|---:|---|
| active audio context | 32,768 bytes | C11 static assertion + host test |
| visual state | 2,048 bytes | C11 static assertion + host test |
| packed framebuffer | 27,200 bytes | API constant + host test |
| additional framebuffer | 0 bytes | direct packed rendering |
| audio allocation in render | 0 | fixed state and stack-only block output |

The audio context shares one chorus memory, one four-line spatial network, and
four compact feedback-string buffers across all models. Loading another model
reinitializes that same memory instead of keeping ten engines resident.

## Tests supplied

- deterministic output for an identical seed;
- exact output regardless of render block partitioning;
- silence before note input;
- signal, stereo, and clipping-margin checks for every model;
- control/event stress across every delay-line wrap point;
- framebuffer guard checks for every visual;
- animated/non-empty framebuffer checks;
- AddressSanitizer and UndefinedBehaviorSanitizer build;
- source-origin audit for media and external implementation references;
- SHA-256 source manifest.

## What still requires target hardware

Host speed is a regression indicator, not an STM32H743 proof. Before merging
into the product firmware, capture worst-case cycles in the real DMA callback
with ten active voices, maximum SPACE, display updates, storage traffic, and
the rest of the product mix enabled. The acceptance gate should be based on the
existing audio profiler and include a multi-hour underrun soak.

## Highest-value improvements under the small-capacity constraint

1. Keep only four to six simultaneous voices for the densest models on target;
   the spatial field supplies perceived size more cheaply than extra voices.
2. Replace host `sqrtf` panning with a 65-entry equal-power table if target
   profiling shows it is material.
3. Render visuals at 24 fps and reuse the product's control-rate band followers.
4. Give the feedback field two target-tuned damping profiles rather than adding
   a second reverb.
5. Add a hard voice-count telemetry counter so real songs reveal whether ten
   voices are ever perceptually necessary.

Large sample libraries, convolution, per-model reverbs, a second framebuffer,
and an FFT dedicated to visuals are intentionally excluded.
