# Verification log — 2026-07-16

## Final clean host run

- Strict C11 compilation with `-Wall -Wextra -Wpedantic -Wshadow
  -Wconversion -Werror`: passed.
- Functional/property verification: **2,211,898 checks, 0 failures**.
- AddressSanitizer + UndefinedBehaviorSanitizer: passed. Leak detection is
  disabled because the execution container does not support LeakSanitizer;
  the real-time implementation performs no allocation.
- Clean-origin implementation audit: passed.
- Preview signal-health gate: passed for all ten models.
- Artifact gate: ten stereo MP3s, 46 GIFs, 46 matching stills, and four
  overview PNGs at the expected formats and geometry.
- Colour gate: all fifteen studies contain 96 animation frames, at least twelve
  visible colours, a neon-accent saturation ratio of at least 65%, and less
  than 5 RGB levels mean drift between GIF midpoint and lossless PNG.
- Motion gate: all 26 new studies contain exactly 96 frames, animate across
  sampled frames, pass the same neon/fidelity gates, have distinct first
  frames, and remain below the loop-seam limit.

## Measured static state

| Item | Measured | Budget | Margin |
|---|---:|---:|---:|
| audio context | 27,488 B | 32,768 B | 5,280 B |
| visual context | 1,444 B | 2,048 B | 604 B |
| packed 320 × 170 framebuffer | 27,200 B | existing buffer | — |
| animated RGB565 palette LUT | 32 B | 32 B | 0 B |
| additional framebuffer | 0 B | 0 B | 0 B |

The twelve palettes were additionally checked for RGB565 uniqueness, animated
state change, visible range, monotonically increasing luminance by nibble
index, and strong tone-14 chroma at quiet and peak phases. This preserves the
renderer's maximum-tone blending semantics without washing the bright body to
white.

## Relative host benchmark

Five seconds of four-note audio per model were rendered on the CI host. The
slowest result was TIDEGLASS at **91.08× real time** and the fastest was GLASS
ORBIT at **337.68× real time**. These numbers are useful only for host
regression; they are not a target-MCU cycle claim.

All eighteen visual systems were also benchmarked for 360 frames on the review
host. The slowest was DREAM TOPOGRAPHY at **58.7 µs/frame**; the new modes did
not exceed it. This is likewise only a host regression baseline.

## Tooling limitation

The Make build was fully exercised. A parallel CMake definition is supplied,
but the recovery environment did not contain a `cmake` executable, so that
generator was not run here. Target integration and DMA-deadline verification
remain hardware release gates.
