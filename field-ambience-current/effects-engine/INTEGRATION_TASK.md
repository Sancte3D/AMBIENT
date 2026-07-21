# Integration task: the complete effects engine

## Goal

Integrate the implementation in this directory into the active product at
`field-ambience-current/firmware-c-next`. Do not stop after compiling the
standalone handoff. Wire it into the real audio path, controls, world state,
memory map, tests, and hardware profiler.

## Non-negotiable target contract

- MCU: STM32H743VIT6.
- Audio: 44,100 Hz, stereo, current 512-frame DMA half-buffer.
- C11, bounded deterministic work.
- No heap allocation, locks, blocking, I/O, or logging in the audio callback.
- Preserve parameter smoothing and click-free mode transitions.
- Preserve the existing synth, harmony, note, and world behavior unless a
  change is strictly required for the master-effects replacement.
- Place the effects arena in internal SRAM. Never put hot delay histories in
  QSPI PSRAM.
- Keep one final limiter. Do not cascade multiple master limiters.

## Critical architecture rule

The default engine needs 214,489 bytes of hot arena memory. The current product
already owns large echo, blur, reverb, tape, and shimmer histories. Replace or
reuse those old regions; do not keep both complete chains resident. Confirm the
result in the linker `.map` file instead of assuming it fits.

True reverse reverb cannot precede an unpredictable live key press without
look-ahead latency. Use `ambient_fx_trigger_reverse_swell()` only for notes the
composer has already scheduled. Keep
`ambient_fx_reverse_reverb_offline_f32()` out of the DMA callback.

## Required implementation work

1. Run the current product host tests and record the baseline before editing.
2. Add `firmware/include/ambient_effects.h` and
   `firmware/src/ambient_effects.c` to the active firmware include/source tree
   and CMake targets.
3. Determine which engine actually feeds `audio_set_renderer()` on the H743
   build. Integrate into that one canonical path; do not process the signal in
   both legacy and V2 hosts.
4. Remove or disable the superseded global master processors and reclaim their
   delay storage: legacy echo, blur, master reverb, tape-wow/saturation, and
   shimmer regeneration. Keep any voice-local synthesis effect that is part of
   an instrument model rather than the master bus.
5. Allocate one aligned `AmbientFxStorage` and one aligned internal-SRAM arena.
   Initialize outside the callback with `ambient_fx_default_config()`. Fail
   closed to dry audio if initialization fails.
6. Prefer `ambient_fx_process_f32()` on the final interleaved floating-point
   mix immediately before the single final limiter and PCM16 conversion. Use
   `ambient_fx_process_i16()` only if the current architecture makes a safe
   float-bus insertion impossible.
7. Map current product controls directly:

   | Product control | `AmbientFxParameters` |
   |---|---|
   | Space | `space` |
   | Atmosphere | `atmosphere` |
   | Echo | `echo` |
   | Motion | `motion` |
   | Age | `age` |
   | Shimmer | `shimmer` |
   | Blur | `blur` |
   | Stereo width | `width` |
   | Tone/brightness | `tone` |
   | Engine trim | `level` |

8. Map the current four product worlds to
   `AMBIENT_FX_TOKYO_CITY`, `AMBIENT_FX_CRYSTAL_COAST`,
   `AMBIENT_FX_MIDNIGHT_DRIVE`, and `AMBIENT_FX_AFTER_HOURS`.
9. Make effect-page selection call `ambient_fx_set_mode()`. DREAM CHAIN is the
   default complete path; individual modes must remain available for A/B tests.
10. When the generative composer knows a note in advance, trigger the reverse
    swell with exactly the same lead time as the scheduled note.
11. Adapt the supplied property tests to the product test runner. Retain exact
    bypass, block-partition invariance, arena guards, delay timing, long-tail,
    reverse alignment, parameter stress, and sanitizer coverage.
12. Build the H743 firmware and inspect section placement. Verify the arena is
    aligned and located in internal SRAM and that stack/static RAM margins
    remain valid.
13. Measure the worst callback with the existing DWT profiler: maximum voices,
    DREAM CHAIN, display/LED traffic, controls, and storage activity. Acceptance
    is zero deadline misses and less than 60% peak use of the 11.61 ms half-
    buffer window.
14. Run a multi-hour hardware soak, listen for mode-switch clicks, check DC and
    peaks at the codec output, and compare every individual mode against bypass.

## Required result

Return an integration commit/PR containing the actual product wiring, linker
placement, updated tests, and measured target results. Explicitly report any
requirement that could not be proven on hardware. Do not claim completion from
the host benchmark alone.
