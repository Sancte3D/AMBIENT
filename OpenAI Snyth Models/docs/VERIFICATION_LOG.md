# Verification log — 2026-07-16

## Final clean host run

- Strict C11 compilation with `-Wall -Wextra -Wpedantic -Wshadow
  -Wconversion -Werror`: passed.
- Functional/property verification: **2,210,398 checks, 0 failures**.
- AddressSanitizer + UndefinedBehaviorSanitizer: passed. Leak detection is
  disabled because the execution container does not support LeakSanitizer;
  the real-time implementation performs no allocation.
- Clean-origin implementation audit: passed.
- Preview signal-health gate: passed for all ten models.
- Artifact gate: ten stereo MP3s, five animated GIFs, and five matching stills
  at the expected formats and geometry.

## Measured static state

| Item | Measured | Budget | Margin |
|---|---:|---:|---:|
| audio context | 27,488 B | 32,768 B | 5,280 B |
| visual context | 1,444 B | 2,048 B | 604 B |
| packed 320 × 170 framebuffer | 27,200 B | existing buffer | — |
| additional framebuffer | 0 B | 0 B | 0 B |

## Relative host benchmark

Five seconds of four-note audio per model were rendered on the CI host. The
slowest result was TIDEGLASS at **88.54× real time** and the fastest was GLASS
ORBIT at **312.32× real time**. These numbers are useful only for host
regression; they are not a target-MCU cycle claim.

## Tooling limitation

The Make build was fully exercised. A parallel CMake definition is supplied,
but the recovery environment did not contain a `cmake` executable, so that
generator was not run here. Target integration and DMA-deadline verification
remain hardware release gates.
