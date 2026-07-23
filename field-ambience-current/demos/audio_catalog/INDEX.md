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
| `1_voices/voice_horn_alps.wav` | PROTO | Alphorn/brass — Alps voice (prototype, NOT yet wired into the engine) |
| `1_voices/voice_pluck_string.wav` | OK | Karplus-Strong pluck/string (VOICE: String) |
| `1_voices/voice_glass.wav` | OK | FM glass bell (VOICE: Glass) |
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
| `3_ambience/amb_fjords.wav` | TODO | Fjords ambience — wind only (fjord-water texture still to build) |
| `3_ambience/amb_moss.wav` | OK | Moss ambience — wind + rain |
| `3_ambience/amb_desert.wav` | TODO | Desert ambience — wind only (heat-shimmer texture still to build) |

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
| `6_worlds/world_alps.wav` | OK | Full world — Alps (Pad voice; own voice TODO) |
| `6_worlds/world_opensea.wav` | OK | Full world — Open Sea (bowed lyra + Mediterranean bed) |
| `6_worlds/world_fjords.wav` | OK | Full world — Fjords (bowed, darker) |
| `6_worlds/world_moss.wav` | OK | Full world — Moss Fields (Pad; own voice TODO) |
| `6_worlds/world_desert.wav` | OK | Full world — Desert (Pad; own voice TODO) |
