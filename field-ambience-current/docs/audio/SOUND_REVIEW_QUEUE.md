# Sound review queue

User direction: review every sound, effect and setting in small completed units.
Each unit ends with a concrete change or finding, relevant verification and a
checkpoint. User-facing audio is at most 30 seconds unless explicitly requested.
Use correctly chunked firmware rendering, no diagnostic reference tone in a
musical demo, and constant loudness matching for tonal A/B comparisons.

Desired result: distinct instruments that share a coherent harmonic context,
usable registers/dynamics and a believable common space. Tuning tests alone do
not establish this. Subjective acceptance is open for every sound until heard.

## Product selection gate

See PRODUCT_SOUND_SELECTION.md: shipping the existing six-core roster is not
an assumption. Mist/Orbit are primary candidates, Bamboo/FM supporting,
Acid needs role-focused rework, Storm is the strongest redesign/drop candidate.
Storm now has a host-tested sustained-pulse candidate (STORM_ROLE_REVIEW.md).
The role comparison supports keeping it as a candidate; final listening approval
is open. Storm/Mist/Orbit now use fixed output trims, verified across 45 probes
(CORE_LEVEL_CALIBRATION.md). Next: Acid/FM/Bamboo levels and transient behaviour. FM Ratio/aliasing and
Ambient retuning stay open; do not expand layers before resource/routing review.

## Progress and next units

| Order | Unit | State / next concrete question |
|---|---|---|
| Done | String delay tuning | Filter-delay compensation tested; character/extreme ranges still open |
| Done | V2 Equal/Just | Six cores retune held notes without attack/priority reset |
| Done | Listening export | 512-frame clamp handled; old 13 s tuning WAV withdrawn |
| Done | FM Index / Body / attack | Coupled envelope, live Index and softer default; see FM_CONTROLS_REVIEW.md |
| Reviewed | Storm role | Architecturally distinct candidate; musical keep/drop remains open |
| Done | Three-core output balance | Storm/Mist/Orbit fixed trims, 45 probes; no AGC |
| Next | Remaining core levels | Acid/FM Glass/Bamboo compared with same reference; preserve transient roles |
| Open | FM Ratio and upper register | Ratio 1–6 changes, harmonic balance, aliasing, Tone/Resonance/Index extremes |
| Then | Ambient live tuning | Pad/Bowed/Horn/Choir, generated held notes, bass/pedal consistency |
| Then | Remaining cores | One core at a time, starting Storm pitch bend/PWM/drive |
| Then | Ambient and natural layers | One source at a time, including their actual routing |
| Then | Effects individually | Dry/send/wet response and tails, then combinations |
| Final | Whole instrument | Five worlds, scene/hold/clear transitions, mix balance and device limits |

## Complete inventory

| Area | Items to work through | Specific focus |
|---|---|---|
| Six synths | FM Glass, Storm, Acid, Mist, Orbit, Bamboo | Each core's A–F names/defaults from synth_controls.h; attack/body/release, pitch, useful register, velocity, live changes |
| Ambient sustained | Pad/PADsynth, Bowed, Horn, Choir | Held tuning, independent identity, formants without hollow/tubular dominance, overlaps and voice steals |
| Ambient transients | String/pluck + Body, Ember, Glass, Guembri | Fundamental vs partials, excitation noise, strike balance, decay, register floor; verify what VOICE actually routes to |
| Foundation | Bass, harmonic bass modes, drone/pedal | Native octaves, low-register spacing, tail pitch memory and speaker audibility |
| Natural layers | Wind/noise, waves/rain, landscape/texture, crackle/hiss | Lulls, irregularity, repeated shapes, resonant whistles, amount/brightness and masking |
| FX modes | Bypass, Dark Reverb, Ping Pong, Chorus/Detune, Tape/Age, Reverse Swell, Shimmer, Blur, Dream Chain | Pitch contribution, feedback stability, stereo/mono, decay, wet gain, mode-change tails |
| Other DSP paths | Reverb, echo, blur, tape, shimmer, Body, drive, DC/high-pass, soft limiter | Establish live device reachability first; no duplicate tuning of inactive legacy paths |
| FX settings | Space, Atmosphere, Echo, Motion, Age, Shimmer, Blur, Width, Tone, Level, Delay Time | Actual control ranges and dependencies; automation and saved-world values |
| Global musical controls | Key, Equal/Just, six modes, Vibe/Mood, Voice, Synth, Cell Note/Bloom/Land, Bass, Color | Harmony/role agreement, held notes and old tails during changes |
| Global sound controls | Volume, Drive, Brightness, Resonance, Sweep, Envmod, Attack, Release, Bass Depth, Send | Audible direction, extremes, smoothing, double filtering and redundant mappings |
| Performance/settings | Shift, Hold, Drone, Generate, Clear, core selection, scenes/recall | Ownership, no stuck notes, no abrupt gate/phase changes; old preset semantics |
| Composition | Bed, Eno, melody, motifs, ornaments, composer states | Spacing, pauses, repetition, register, transitions and simultaneous audible pitches |
| Worlds | Alps, Open Sea, Fjords, Moss Fields, Desert | Defaults, source/FX interaction, foreground vs bed vs bass, coherent level |
| Device | H743 RAM/flash, DWT peak/stack, speaker/headphone/line outputs | Fixed-buffer costs; <60% peak load and zero misses; real-device listening |

## Common review procedure (apply within one unit)

1. Trace source → pitch/envelope → native filter → dry/send → FX → master.
2. Render low/mid/high intended registers; soft/normal/hard strikes; held and
   released notes. Judge gain separately from timbre with matched playback.
3. Exercise each knob at min/default/max, then the interactions that can cause
   the identified failure (not an unbounded Cartesian sweep).
4. Check relevant PCM properties: finite/bounded output, pitch and unwanted
   sidebands, silent gaps/clicks, DC, stereo cancellation, envelope/tail behaviour.
5. Confirm musical role in a short phrase plus the shared room; retain useful
   character. Do not use a passing spectrum/test as a quality score.
6. Record patch, evidence, known limits, preset compatibility and device cost.
   Full host suite after sound changes; device build/DWT and human listening
   remain explicit gates. Stop optional testing once the current risk is covered.
