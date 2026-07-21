# Firmware size & build-flag audit (r19.42)

External review guidance: "reduce the compiled firmware, not source lines —
and first establish WHAT is too large (flash vs RAM vs CPU)." This document
is that measurement for the H743 product build, plus the checklist status.
Numbers from the real cross-build (arm-none-eabi-gcc 13.2.1), read from the
`.map` / `--print-memory-usage`, not from any transport format.

## 1 · What is actually too large? (measured, r19.41 state)

| Resource | Used | Budget | Verdict |
|---|---|---|---|
| FLASH | 222,920 B | 1920 KB | **11.3% — NOT a problem** (1.7 MB free) |
| RAM_D1 (AXI) | 389,632 B | 512 KB | 74% — managed, watch on growth |
| RAM_D2 | 254,016 B | 288 KB | 86% — fx arena; 40 KB margin |
| DTCM | 119,144 B | 128 KB | 91% — pad voices + stack; tightest region |
| CPU / WCET | unproven | <60% of 11.61 ms | **the real open risk — needs bench DWT** |

Conclusion: flash-size work has near-zero value here. RAM placement is
already explicitly managed by object file in the linker script. The genuine
unknown is worst-case callback time on silicon.

## 2 · Checklist status (what was already in place)

| Recommendation | Status |
|---|---|
| `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections` | ✅ since r18.86 (`cmake/arm-none-eabi-m7.cmake`); all boot-critical sections carry `KEEP()` in the .ld |
| Linker `.map` generated | ✅ (`-Wl,-Map=…` + `--print-memory-usage`) |
| newlib-nano, no printf-float | ✅ (`--specs=nano.specs`; float formatter deliberately not linked) |
| No C++ runtime (exceptions/RTTI/iostream) | ✅ moot — the firmware is pure C11 |
| No framework layers (Arduino/MBed) | ✅ native vendor HAL + CMSIS only |
| One shared immutable table per kind | ✅ single PADsynth table, single sine LUT (dsp.c), no per-voice copies |
| Tables generated at startup where cheap | ✅ PADsynth builds at boot/world-change; per-world variants are NOT stored in flash |
| Transcendentals out of the audio rate | ✅ enforced awareness via `test/lint_hotpath.sh`; the r19.41 effects engine has **zero** transcendental calls (LUT-only) |
| No dynamic allocation | ✅ hard rule (REALTIME_AUDIO_RULES.md) + lint gate |
| Fonts/assets | baked 1-bit font already (`baked_font_data.c`) |

## 3 · Optimization-level comparison (measured, do not assume)

Same tree, same toolchain, FLASH used:

| Flags | FLASH | vs -O3 |
|---|---|---|
| `-O3` (CMake Release — current default) | 222,920 B (11.3%) | — |
| `-O2` | 197,608 B (10.1%) | −25.3 KB |
| `-Os` (MinSizeRel) | 181,956 B (9.3%) | −41.0 KB |

**Decision: keep `-O3` as the product default.** Rationale: every variant
fits with >1.6 MB to spare, so the 41 KB saving buys nothing — while the
open risk is CPU (WCET unproven on silicon), where -O3 gives the DSP the
most headroom. Revisit only if (a) bench DWT shows the callback comfortably
inside budget at -O2/-Os too, or (b) flash ever becomes scarce. A per-file
split (-Os for UI/menu, -O3 for DSP) is the prepared fallback; CMake
`set_source_files_properties` makes it a 5-line change.

## 4 · Deliberately NOT applied (and why)

- **LTO** — medium risk (can surface latent UB, complicates the map audit)
  for a resource that is at 11%. No need, no change.
- **-Oz / global -Os** — solves a non-problem, costs DSP throughput margin.
- **Fixed-point conversion, math approximation swaps, unroll reduction** —
  these are CPU/quality trades, gated on the bench DWT measurement per the
  review's own rule: "do not modify the audio algorithm until the map proves
  it is the actual size problem." The map proves it is not.
- **-Ofast** — never globally (FTZ/denormal handling in the engine is
  deliberate; see REALTIME_AUDIO_RULES.md).

## 5 · Standing practice

On any size regression question: build `FAM_TARGET=h743`, read
`--print-memory-usage` + the `.map`, and update the table above. Judge from
the ELF/map — never from a `.uf2`/transport artifact (bench-Pico builds
included).
