# Musical playability and listening review — 2026-09-05

This pass makes the existing instrument more expressive and makes its controls
reach the sound. It is based on sound PR #126 at
`24d0a2be2a0cd3f2c04e210824ea9eee89a795d5`, not the divergent main/UI branch.

## Changes

- Ambient and the six V2 cores now feed the same float dry/send buses, effects,
  DC/high-pass stages, smoothed master volume and final safety limiter. Mute
  includes reverb tails and generated tape noise. The standalone V2 host API
  remains available for existing host tools.
- Per-voice sends feed delay and reverb again. The external-bus API has unity
  dry gain and calibrated returns (4x FDN, 2x echo, compensating conservative
  send levels); its caller owns final level/DC/limiting. The standalone effects
  API and its existing calibration remain unchanged. Inserts still colour the
  dry signal; FX-mode changes fade their contribution without muting the note.
- A held-source stack gives the mono cores last-note priority. Releasing an
  older key leaves the newer one playing; releasing the newest returns to the
  previous held key. Switching clears ownership and crossfades dry/send before
  the shared effects. The host publishes a prepared core request last and
  changes renderer ownership at chunk boundaries.
- Every synth has six named menu controls, with values retained per core and
  80-ms control smoothing. Orbit no longer overwrites Wave Shape on each note.
  All six cores gain an 8-ms-smoothed velocity-to-level response, preserving
  their existing timbral accent behaviour.
- Bowed, Horn and Choir have source-owned gates for live cells, while generated
  notes retain their timed one-shots. Live strikes no longer use minimum-level
  floors. Softer playing darkens their excitation; bowed/choir vibrato fades in.
- Actual foreground audio, including its release, gently reduces the pad by
  at most 2.2 dB (80-ms entrance / 1.2-s recovery), leaving master gain alone.
- Scenes now include shape and all 36 core values. SCN5 presets migrate to SCN6
  in memory with neutral new defaults; no flash write occurs until Save. A
  static assertion keeps the store inside the hardware's 512-byte staging area.

## Core controls (menu order)

| Core | A | B | C | D | E | F |
|---|---|---|---|---|---|---|
| Acid | Cutoff | Resonance | Decay | Core Drive | Glide | Filter Env |
| FM Glass | FM Index | FM Ratio | Index Decay | Tone | Glide | FM Body |
| Mist | Cutoff | Detune | Chorus | Chorus Rate | Glide | Core Attack |
| Storm | Cutoff | Detune | PWM Depth | Core Drive | Glide | PWM Rate |
| Orbit | Wave Shape | Orbit Rate | Cutoff | Spread | Glide | Sustain |
| Bamboo | Pluck Tone | LPG Decay | Metal | Tone Floor | Glide | Send |

Core pages appear when a synth is selected. Percent recipes are in
`include/synth_controls.h`; FM Ratio displays its integer ratio.
The ambient Shape/Resonance/Sweep/EnvMod controls retain their existing scope.

## Verification and listening

`bash test/run_tests.sh` passes. This includes 80 device-path checks (all-core
mute/retrigger, velocity, exact-phase half-volume comparison, held-note priority,
gated voices and separate sends), scene migration/recall, the block-size sweep,
the hot-path guard, and 478,857 existing effects checks. The default FX arena
remains 214,489 bytes; no allocation or blocking calls were added to rendering.

`tools/render_musical_review.py` compiles and drives the actual C device path.
It creates five 50-second pentatonic performances, six synths dry/Dream, and all
nine effects. Run the five performances again with `--engine baseline.so` for
the original commit. Each clip uses a fresh process so file-static PRNGs reset.
The event list and raw peak/RMS/DC/band-energy measurements accompany the files.
All 31 renders have zero clipped int16 samples in this programme.

`tools/package_musical_review.py` makes the listening pack: constant-gain EBU
R128 matching for before/after and dry/Dream pairs, shared gain for the nine FX
examples, FLAC at native 44.1 kHz / 16 bit, and two WAV previews. It verifies the
delivered true peaks; no compressor or dynamic normalization is applied.
The Fjords preview is 0:00 before / 0:51 after. The sound tour lasts 2:10.

These are host firmware renders, not recordings of the DAC/amplifier/speakers.
The H743 cross-build/map and DWT peak-load gate (<60%) still require device/CI
validation, especially with both cores active during a switch. Added static
scratch buffers stay in internal RAM. Hardware deployment and final musical
approval are pending listening on the intended headphones and loudspeaker.
