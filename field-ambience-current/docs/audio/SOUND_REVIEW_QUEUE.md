# Sound review queue

User direction: review every sound, effect and setting in small completed units.
Each unit ends with a concrete change or finding, relevant verification and a
checkpoint. User-facing audio is at most 30 seconds unless explicitly requested.
Use correctly chunked firmware rendering, no diagnostic reference tone in a
musical demo, and constant loudness matching for tonal A/B comparisons.

Binding direction 2026-09-22: calming, warm and usually low-mid; no attention-grabbing
alarm/siren/beep character. Bright detail is allowed only as restrained texture,
not an exposed notification-like lead. Register limits alone cannot certify this.

Desired result: distinct instruments that share a coherent harmonic context,
usable registers/dynamics and a believable common space. Tuning tests alone do
not establish this. Subjective acceptance is open for every sound until heard.

## Product selection gate

AMBIENT_CONCEPT_REVIEW.md prioritizes World/Character continuity before the
next solo timbre review. The lost return-pause history is fixed and host-tested;
the Character still replaces the bed. Next: a concrete resource/routing plan
for bed + one foreground role, without adding a second full engine or FX tank.

See PRODUCT_SOUND_SELECTION.md: shipping the existing six-core roster is not
an assumption. Mist/Horizon are primary candidates, Dew/Glimmer supporting,
Dusk needs role-focused rework, Tide is the strongest redesign/drop candidate.
Tide now has a host-tested sustained-pulse candidate (STORM_ROLE_REVIEW.md).
The role comparison supports keeping it as a candidate; final listening approval
is open. Tide/Mist/Horizon now use fixed output trims, verified across 45 probes
(CORE_LEVEL_CALIBRATION.md). Dusk/Glimmer/Dew fixed trims now follow in
SUPPORT_LEVEL_CALIBRATION.md (60 probes per version). SOUND_NAMES.md records
the evocative atmosphere names and unchanged IDs. DUSK_REVIEW.md confirms and
fixes its register/filter mismatch with a rounded bloom. User found that
candidate too sci-fi: DUSK_CALM_REVIEW.md corrects partial fundamental cancellation
and reduces native colour. Listening acceptance stays open; Dew's uneven attacks,
FM Ratio/aliasing and
Ambient retuning stay open; do not expand layers before resource/routing review.

## Progress and next units

**User priority 2026-09-22: sound first; defer display idle.**
Open Sea dry voice / pad / Dream diagnosis is in OPEN_SEA_SOUND_REVIEW.md.
Fixed periodic fundamental cancellation in the shared Bowed voice without
raising its average level. CALM_REGISTER_REVIEW.md then lowers the automatic
foreground to D3..A4 with tonic-centred openings and intact collision checks.
Next: chorus/blur and combined pitch movement, followed by Horn/Bowed resonance
at the new register. Shimmer must justify its role; no blanket brightness boost.

The detailed product execution order is now in PRODUCT_REVIEW_EXECUTION.md.
The user subsequently selected autonomous listening with locked cells.
LISTENING_WORLD_REVIEW.md supersedes simultaneous Generate/play as the contract.
The ownership cleanup remains; source/FX inventory below remains open.

| Order | Unit | State / next concrete question |
|---|---|---|
| Done | String delay tuning | Filter-delay compensation tested; character/extreme ranges still open |
| Done | V2 Equal/Just | Six cores retune held notes without attack/priority reset |
| Done | Listening export | 512-frame clamp handled; old 13 s tuning WAV withdrawn |
| Done | FM Index / Body / attack | Coupled envelope, live Index and softer default; see FM_CONTROLS_REVIEW.md |
| Reviewed | Tide role | Architecturally distinct candidate; musical keep/drop remains open |
| Done | Three-core output balance | Tide/Mist/Horizon fixed trims, 45 probes; no AGC |
| Done | Remaining core levels | Fixed trims; separate Dew attack measurement preserves transient role |
| Done | Ambient naming | Six shared evocative atmosphere names; scene IDs unchanged; sound/imagery fit remains under review |
| Done | Dusk register / defaults | Pitch-tracked filter, rounded bloom; body spread <0.5 dB; listening approval open |
| Listening | Dusk calming character | Partial root cancellation corrected; gentler defaults; current device body spread ~1.1 dB |
| Done | Concept / return pause | Cross-mode presence history fixed; 28 s system probe; continuity gap remains |
| Done | Generate / listening | Cells/Hold/Drone locked; manual Character remembered; World voice and phrasing; 4 s LED pulse; host verification |
| Done / device gate | Listening exit | Independent source drain, two-second background fade, immediate manual Character; Harmony→Generate bass ownership corrected; see LISTENING_EXIT_REVIEW.md |
| Deferred | Quiet UI | Sound takes priority; 15 min human-idle display timer and neutral wake still required |
| Done / listening | Open Sea dry Bowed | Periodic root collapse corrected; same average level; both colours and four pitches; 28.25 s stem A/B |
| Done / listening | Calming automatic register | D3..A4; tonic-centred opening; 72 key/mode regression, actual Generate A/B; manual/FX registers still open |
| Next sound | Open Sea space and movement | Compare chorus/blur against simpler room; default Shimmer has tiny measured contribution in diagnostic scene; range and feedback audit before keep/reduce/remove |
| Open | Dew attack consistency | Register-dependent attack level; excitation/filter cause still unverified |
| Open | FM Ratio and upper register | Ratio 1–6 changes, harmonic balance, aliasing, Tone/Resonance/Index extremes |
| Then | Ambient live tuning | Pad/Bowed/Horn/Choir, generated held notes, bass/pedal consistency |
| Then | Remaining core controls | One core at a time; verify useful ranges and interactions |
| Then | Ambient and natural layers | One source at a time, including their actual routing |
| Then | Effects individually | Dry/send/wet response and tails, then combinations |
| Final | Whole instrument | Five worlds, scene/hold/clear transitions, mix balance and device limits |

## Complete inventory

| Area | Items to work through | Specific focus |
|---|---|---|
| Six synths | Glimmer, Tide, Dusk, Mist, Horizon, Dew | Each core's A–F names/defaults from synth_controls.h; attack/body/release, pitch, useful register, velocity, live changes |
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
