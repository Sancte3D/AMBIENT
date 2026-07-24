# Field Ambience — Sound Catalog

Every sound source of the instrument, rendered in **isolation** so each
voice / bed / ambience layer / effect / world can be auditioned on its own.
Regenerate: `tools/render_catalog.sh`. Extend: add a line to its MANIFEST.

Status: **OK** = works & in the product · **PROTO** = exists, not yet wired
into the product · **TODO** = placeholder / own voice or layer still to build.


## VOICES

| sample | status | what it is |
|---|---|---|
| `1_voices/voice_bowed_opensea.wav` | OK | Bowed lyra — Open Sea character voice (warm) |
| `1_voices/voice_bowed_fjords.wav` | OK | Bowed Hardanger — Fjords voice (darker, more sympathetic ring) |
| `1_voices/voice_horn_alps.wav` | OK | Alphorn/brass — Alps character voice (wired r19.53) |
| `1_voices/voice_choir_moss.wav` | OK | Chor/Orgel — Moss-Charakterstimme (verdrahtet r19.61) |
| `1_voices/voice_guembri_desert.wav` | OK | Guembri/Sintir — Desert-Charakterstimme (verdrahtet r19.61) |
| `1_voices/voice_pluck_string.wav` | OK | Karplus-Strong pluck/string (VOICE: String) |
| `1_voices/voice_ember.wav` | OK | Warm subtractive analog (VOICE: Ember) |

## BEDS

| sample | status | what it is |
|---|---|---|
| `2_beds/bed_alps.wav` | OK | PADsynth pad bed — Alps timbre (warm odd harmonics) |
| `2_beds/bed_opensea.wav` | OK | PADsynth pad bed — Open Sea (glassy) |
| `2_beds/bed_fjords.wav` | OK | PADsynth pad bed — Fjords (darkest rolloff) |
| `2_beds/bed_moss.wav` | OK | PADsynth pad bed — Moss (dusty) |
| `2_beds/bed_desert.wav` | OK | PADsynth pad bed — Desert (dark-warm low-mid) |

## AMBIENCE

| sample | status | what it is |
|---|---|---|
| `3_ambience/amb_alps.wav` | OK | Alps ambience — wind only (clear air) |
| `3_ambience/amb_opensea.wav` | OK | Open Sea ambience — gentle waves + warm Mediterranean sea-hum |
| `3_ambience/amb_fjords.wav` | OK | Fjords ambience — dark water murmur + deep drips (r19.54) |
| `3_ambience/amb_moss.wav` | OK | Moss ambience — wind + rain |
| `3_ambience/amb_desert.wav` | OK | Desert ambience — dry heat haze + sparse sand grains (r19.54) |

## SYNTH

| sample | status | what it is |
|---|---|---|
| `7_synth/reso_00_off.wav` | OK | RESONANCE 0 % — Filter aus (Referenz, klingt wie vor r19.59) |
| `7_synth/reso_55_mid.wav` | OK | RESONANCE 55 % — der Moog-Ladder singt mit, BRIGHT fährt den Sweep |
| `7_synth/reso_90_high.wav` | OK | RESONANCE 90 % — kurz vor Selbstoszillation, klassisches Ambient-Timbre |
| `7_synth/motion_0_off.wav` | OK | MOTION aus — Filter steht still (Referenz) |
| `7_synth/motion_sweep.wav` | OK | MOTION Sweep 85 % — der Filter atmet von selbst (langsamer LFO, ~18 s) |
| `7_synth/motion_envmod.wav` | OK | MOTION EnvMod 90 % — der Filter oeffnet beim Anschlag (TD-3-EnvMod) |
| `7_synth/shape_0_pluck.wav` | OK | SHAPE Attack 0 % — dieselbe Bowed-Stimme wird perkussiv |
| `7_synth/shape_50_neutral.wav` | OK | SHAPE 50 % — neutral, exakt der Klang vor r19.60 |
| `7_synth/shape_100_swell.wav` | OK | SHAPE Attack 100 % — dieselbe Stimme wird zum atmenden Swell |

## LOW

| sample | status | what it is |
|---|---|---|
| `4_low/bass_root.wav` | OK | Bass voice — root mode |
| `4_low/bass_deep.wav` | OK | Bass voice — deep mode |
| `4_low/drone.wav` | OK | Drone voice (bloom in / tail out) |

## FX

| sample | status | what it is |
|---|---|---|
| `5_fx/fx_bypass.wav` | OK | Effects: Bypass (dry reference) |
| `5_fx/fx_reverb.wav` | OK | Effects: Reverb (dark FDN hall) |
| `5_fx/fx_delay.wav` | OK | Effects: filtered ping-pong Delay |
| `5_fx/fx_chorus.wav` | OK | Effects: Chorus |
| `5_fx/fx_tape.wav` | OK | Effects: Tape age (wow/flutter/hiss) |
| `5_fx/fx_swell.wav` | OK | Effects: reverse Swell |
| `5_fx/fx_shimmer.wav` | OK | Effects: octave Shimmer |
| `5_fx/fx_blur.wav` | OK | Effects: temporal Blur |
| `5_fx/fx_dream.wav` | OK | Effects: Dream Chain (boot default — everything) |

## WORLDS

| sample | status | what it is |
|---|---|---|
| `6_worlds/world_alps.wav` | OK | Full world — Alps (Horn voice + de-hissed wind) |
| `6_worlds/world_opensea.wav` | OK | Full world — Open Sea (bowed lyra + Mediterranean bed) |
| `6_worlds/world_fjords.wav` | OK | Full world — Fjords (bowed, darker) |
| `6_worlds/world_moss.wav` | OK | Full world — Moss Fields (Chor-Stimme + Regen) |
| `6_worlds/world_desert.wav` | OK | Full world — Desert (Guembri + Hitze-Flimmern) |
