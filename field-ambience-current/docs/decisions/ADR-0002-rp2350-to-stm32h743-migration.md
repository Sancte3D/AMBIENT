# ADR-0002: MCU Migration — Pico 2 (RP2350) → STM32H743 Bare-Chip

**Status:** ACCEPTED (schematic-side migration complete in r18.5/r18.6; firmware-side migration planned for Phase 2-5 of `NATIVE_PORT_PLAN.md`)
**Date:** 2026-06-07 (decision); 2026-06-11 (schematic completed)

## Context

The original Step-6 hardware was a Raspberry Pi Pico 2 module (RP2350, SC1631) on through-hole pads. During a UX review of the display simulator, the question came up: is the Pico 2 actually CPU-headroom enough for an audio product?

The honest first answer was "Pico 2 fits, barely." On closer inspection that answer was methodologically wrong:

- **Counting instructions, not measuring**: cycle-counting from operation counts doesn't capture cache, branch prediction, or peripheral DMA stalls
- **Both M33 cores as DSP**: assumed Core 1 was free for audio, but the UI needs it
- **"Daisy is drop-in"**: the Pico-SDK HAL → Daisy migration is non-trivial

ChatGPT + Gemini cross-reviews confirmed the analysis was unsafe. Decision needed: trust the Pico 2 with safety margin, or migrate.

## Decision

**Migrate to STM32H743VIT6 bare-chip** (LQFP-100, hand-solderable). Specifically:
- **Not Daisy** — too much abstraction lock-in
- **Not Pico 2 with optimism** — methodologically unsafe
- **Bare-chip on our own board** — full control, JLC-orderable (LCSC C114409, $6.62, Mouser/DigiKey in stock)

This gives:
- 480 MHz Cortex-M7 + DTCM/ITCM + double-precision FPU = 3-4× CPU headroom vs. RP2350 single-M7-equivalent
- 1 MB internal RAM, 2 MB flash
- ~80 GPIOs in LQFP-100 vs. 24 on Pico (50+ reserve for Rev-B expansion)
- Hardware QEI (TIM1/2/3/4) for encoder quadrature → no software polling
- USART2 for MIDI (no PIO workaround)
- SAI for I²S (Audio-grade peripheral, not bit-banged)

## Consequences

**Positive:**
- ~3-4× CPU headroom → comfortably room for convolution reverb later, polyphony aufstockung, generative-bed expansion
- Native I²S via SAI, native QEI for encoders, native UART for MIDI — no Pico-PIO workarounds
- Schematic and BOM are JLC-orderable (vs. Pico module which JLC doesn't stock as a part)
- ~50 GPIO reserve for future features

**Negative:**
- All schematic work since v0.6 needs to migrate (done r18.5)
- Firmware HAL abstraction needed (Phase 2 — pending)
- New STM32 hardware to acquire for Phase-5 profiling gate (Nucleo-H743 + PCM5102A breakout)
- The repo carries `legacy_pico2/` for the old sheets and a `firmware-c/` frozen snapshot — overhead

**Hard mitigation:**
- **Profiling Acceptance Gate** before any PCB layout (Phase 5): the audio engine must measurably fit in < 40% block-time worst-case on real STM32H743 hardware (DWT-CYCCNT counter). If it doesn't, the decision is wrong and we'd need to revisit. This gate exists precisely because the original Pico assessment was wrong.

## Phases

1. ✅ Decision + ChatGPT/Gemini review (2026-06-07)
2. ✅ Schematic spec (`field_ambience_pcb_SPEC_v0.7.md` r18) — full §5 pin allocation against DS12110
3. ✅ Schematic generation (r18.5)
4. ✅ Engineering verification (r18.6) — LDO pinout, footprints, BOM LCSC numbers
5. ⏳ Firmware HAL abstraction (Phase 2 of NATIVE_PORT_PLAN)
6. ⏳ STM32H743 firmware (Phase 3-4)
7. ⏳ Profiling on real hardware (Phase 5 = the gate)
8. ⏳ PCB layout (Phase 6, only if profiling passes)

## Related

- ADR-0001 — Repo structure refactor was driven in part by needing to separate legacy Pico-era material clearly
- ADR-0003 — Manufacturing gates institutionalize the "no layout without profiling" rule
- ADR-0004 — MIDI design (different problem, but blocks gate 4 with this MCU)
