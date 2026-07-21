# Integration report — master-effects engine into the product (r19.41)

Status per INTEGRATION_TASK.md requirement, with measured results. Everything
below was verified in this environment; the three hardware-only steps are
explicitly reported open at the end (per the task's own rule: no completion
claims from host results alone).

## Done and verified

| # | Requirement | Result |
|---|---|---|
| 1 | Baseline host tests recorded | All suites green before edits (r19.40 state) |
| 2 | Sources into the active tree + CMake | `src/ambient_effects.c`, `include/ambient_effects.h` (product copy; this folder stays the reference), plus new adapter `src/fx_master.c` / `include/fx_master.h`. Both in `DSP_SOURCES` |
| 3 | One canonical path | `engine_render → render_ambient` (the path feeding `audio_set_renderer` on H743). Inserted there only; the V2 synth hosts keep their own guard chain untouched — no double processing |
| 4 | Legacy master processors removed, RAM reclaimed | `echo.c`, `blur.c`, `tape.c`, `shimmer.c` are out of the H743 link entirely (`DSP_SOURCES_H743` filter; **0 objects in the .map**) and out of the ambient render path. `reverb.c` stays linked for the V2 hosts but no longer renders in the ambient path. Modules remain host-test-only |
| 5 | Aligned storage + internal-SRAM arena, init outside callback, fail-closed | `fx_master.c` owns `AmbientFxStorage` + 245,760 B arena (`aligned(32)`); `fx_master_init()` runs in `engine_init()`; NULL init ⇒ bit-exact dry pass-through, setters no-op |
| 6 | `ambient_fx_process_f32` on the final float mix before the single limiter | Inserted after drive + DC-block + volume, before `soft_limit` → PCM16. `soft_limit` stays the ONE final limiter |
| 7 | Control mapping | Space→space (also still configures the V2 tank presets), Atmosphere→atmosphere (+ the per-world ambience layer, deliberate dual-route), Echo→echo, Motion→motion (+pad LFO), Age→age (tape character now lives here), Shimmer→shimmer, Blur→blur, Brightness→tone (hz −600..+800 → 0..1). Width/level: engine's curated per-world values (no product control exists) |
| 8 | World mapping | Product worlds 0-3 → TOKYO_CITY / CRYSTAL_COAST / MIDNIGHT_DRIVE / AFTER_HOURS in `engine_set_world` |
| 9 | Effect page via `ambient_fx_set_mode` | New menu slot **FX** (Bypass/Reverb/Delay/Chorus/Tape/Swell/Shimmer/Blur/Dream), default **Dream Chain**, wired through `menu → hal_set_fx → engine_set_fx_mode`. Scenes store it (SCENE_MAGIC → "SCN5") |
| 10 | Reverse swell only for scheduled notes | Wired to the Eno loops — the composer's genuinely pre-known events (fixed periods, next fire time exact). Triggered 1.5 s ahead with the same lead, pitch predicted from the current harmony, one-shot per cycle. Live key presses never trigger it. `ambient_fx_reverse_reverb_offline_f32` is not referenced from any render path |
| 11 | Property tests in the product runner | `effects_verify` suite added to `test/run_tests.sh` (478,857 checks). Full suite green. Two engine-level tests recalibrated with documented rationale: drone-audible threshold 300→250 (DREAM voices ~5.7 dB below the legacy chain — measured per-mode), reverb-decay check made relative (Atmosphere now also drives the never-decaying wind bed; relative check still catches a runaway hall). Hot-path lint updated (21 modules incl. `ambient_effects.c` + `fx_master.c`, clean — the engine has zero transcendental calls) |
| 12 | H743 build + section placement | **Cross-built with arm-none-eabi-gcc 13.2.1.** Map-verified: `.d2_bss` = 254,016 B entirely from `fx_master.c.obj`; `s_arena` @ 0x30001040 (RAM_D2, 32-byte aligned, 0x3C000). Memory: FLASH 11.3%, DTCM 90.9%, **D1 74.3%**, **D2 86.1%** (40 KB margin), D3 0% |

## Open — hardware required (cannot be proven here)

| # | Requirement | Why open |
|---|---|---|
| 13 | DWT worst-case callback measurement (<60% of the 11.61 ms window, zero misses) | Needs the physical H743. Host proxy: DREAM CHAIN 72.6× realtime + the product blocksize sweep passes, but the task correctly says the host benchmark is not the proof |
| 14 | Multi-hour hardware soak, mode-switch click listen, DC/peak at codec output, per-mode A/B vs bypass | Needs the bench + codec |

## Notes for the bench session

- The DREAM chain voices the final mix ~5.7 dB below the legacy chain at the
  Tokyo defaults (measured on host: drone peak 540 bypass → 280 dream). Speaker
  loudness should be re-judged after the r19.37 amp gain change lands on real
  hardware — both changes move level in the same session.
- `fx_master_process` runs inside the DMA ISR; the interleave scratch lives in
  D2 with the arena. If bench profiling shows D2 access cost matters, moving
  `s_buf` (4 KB) to D1/DTCM is a one-line linker experiment.
- Latency: `ambient_fx_latency_frames` reports 0 for the live modes in the
  default config; nothing shifted the dry path in host renders.
