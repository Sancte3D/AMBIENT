# ADR-0021 — Swappable synth-engine host (OP-1-Field model)

**Status:** Accepted · **Date:** 2026-06-28

## Context

The device should offer distinct electronic-synth sounds (303 acid, DX7 FM
glass, Juno chorus, hoover, wavetable, west-coast LPG), calibrated against
reference audio (`ambient_iconic_synth_worlds_pack`). Two wrong turns to avoid:

1. **Preset ≠ engine.** The V2 ambient stack (`engine_v2` → harmony_field /
   field_voice / texture / diffuser / mod_delay / arp / beat / reverb /
   beauty_guard) is **one** engine; its "worlds" (GLASS, WARM, DUST, FOG, TAPE,
   CRYSTAL) only change *parameters*. Renaming presets as engines, or adding
   more world presets, does **not** produce an acid/FM/wavetable sound — those
   need their **own synthesis**. `field_voice` only has 3 generic voice types
   (GLASS/TAPE/PARTICLE); no parameter set turns it into a 303.

2. **Layer ≠ core.** Tinting the ambient engine with an "acid layer" makes
   everything sound like *ambient + a tint*, never like a real acid instrument.

## Decision

Adopt the **OP-1-Field model**: the device (UI, clock, transport, FX, master,
output) stays constant; only the **sound-core is swapped**.

- **`synth_engine_t`** (`include/v2/synth_engine.h`) — the contract every
  selectable core implements: `init / activate / deactivate / note_on(midi,vel)
  / note_off / set_param(0..1) / render_mix(dry,send) / panic`.
  - `render_mix` **ADDS** into a dry + reverb-send bus (matches the existing
    voice contract). Chosen over a bare `render(L,R)` so engines keep
    per-voice reverb send — ambient cores would otherwise lose their space.
  - 6 param slots (`SP_A..SP_F`); the same 4 push-encoders (+ shift for E/F)
    drive any engine, each mapping them to its own meaning.
- **`synth_host`** (`src/v2/synth_host.c`) — registry + active-engine routing +
  the **global** signal path shared by all engines:
  `active.render_mix → reverb (send) → master → beauty_guard → int16`.
  FX live **outside** the core, so switching engines never re-plumbs FX.
- **The V2 ambient stack stays the FIELD engine, untouched.** The host does not
  modify `engine_v2`. A thin FIELD adapter into the host is a later step.

### Engine constraints (embedded)

No malloc · no samples · no per-sample `powf`/`sinf` · no UI logic in the audio
path. Build only on the `dsp.h` toolkit (`dsp_sin`, `dsp_poly_saw`,
`dsp_poly_square`, `dsp_svf_*`, `dsp_smooth_coef`, `dsp_midi_to_hz`).

### Golden-reference calibration (process)

Each engine is calibrated against its reference WAV, not "by vibe": measure the
reference (tempo, register, filter-sweep / spectral centroid envelope), build
the DSP to match, render offline, and A/B. First engine **ACID RAIN** matched
the reference at 117 BPM @ 8ths, bass register A2–D3, resonant sweep ~3.7–6×
(ref 3.9×).

## Scope / sequence

1. ✅ Host + contract + **ACID RAIN** (`engine_acid.c`) + host test + offline
   render (`tools/render_synth.c`).
2. Next engines, one per PR, each reference-calibrated: FM Glass (force
   harmonic ratios — the reference sounds unmelodic), Chorus Mist, Ion Storm,
   Glass Orbit, Bamboo Circuit.
3. FIELD adapter so the V2 ambient engine is selectable in the same host.
4. UI/mode layer (FIELD / SYNTH / …), encoder→param mapping, display.

## Consequences

- Distinct, real synth cores instead of one ambient engine wearing preset hats.
- A World/preset becomes "a saved parameter set **of an engine**" (e.g. engine
  = FM Glass, preset = "Glass Station").
- All engines compiled into firmware; the user selects the active one (no
  re-flash). Memory is a non-issue — each engine is a few KB of DSP code, far
  inside the H743's 2 MB flash; the reference WAVs are previews, not shipped.

## Supersedes

The drift where world presets were treated as engines. `engine_v2` keeps its
role as the FIELD engine.
