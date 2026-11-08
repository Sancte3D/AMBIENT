# Sonic review — 10 September 2026

User feedback: wind repeats; noise is homogeneous; several sounds feel tubular,
filtered and reminiscent of old science-fiction effects. Every change must fit
STM32H743. This review starts from PR129 `d69ad9e`, not from the older demos.

## Findings and changes

| Area | Evidence in the previous signal path | Change |
|---|---|---|
| Universal wind | 14-second sine sweep of Q1.4 bandpass; Q18 whistles; constant gate floor | Broadband lowpass air, pressure-dependent brightness, independent irregular gust/eddy clocks, no whistles or fixed cycle; continuous sample-rate level smoothing |
| Noise texture | Fixed .052/.040Hz filter/amplitude LFOs, Q1.6 band, independent permanent high-frequency hiss | Random slow pressure/colour trajectory, broad lowpass, high band follows quiet intervals |
| Other weather | Steady rain wash, regular desert haze; resonant low-mid Fjord drops | Uneven rain activity/density, irregular desert haze, less tonal drips; sea/Fjord swell targets include deeper rests |
| BLUR, including DREAM | Reading grains 1.18–1.40 times as fast transposed their content by 2.9–5.8 semitones | Fixed-rate grain reads; overlapping windows and random time positions retained |
| Orbit | Saw fundamental opposed pulse; triangle quarter-cycle offset. Morph could cancel its fundamental | Align fundamental phase across all four shapes, retaining different harmonic spectra |
| Mist | Near-equal direct/delayed blend (.6/.7), before another common chorus | Direct-led .85/.30 blend; ensemble motion, detune and full controls retained |
| Ambient pad | Additional wandering Q2.2 formant above the PADsynth timbre | Broad Q.85 emphasis at .30 instead of .55; keep spectral movement without pronounced moving vowel |

The weather remains procedural, not a claim of field-recording realism. Fixed
seeds permit repeatable comparisons; absence of a fixed LFO is not proof that
listeners will prefer it. No external samples or new sound layers were added.

## Whole-system review and distinct roles

| Core / layer | Musical role and review decision |
|---|---|
| Acid | Resonant articulated bass, ladder/filter envelope and accent. Resonance is its deliberate character; do not flatten it into a generic pad. Shared BLUR fix applies. |
| FM Glass | Harmonic integer-ratio keys with transient-to-body index envelope. Distinct from short LPG plucks. Native index/ratio/decay controls retained. |
| Mist | Sustained saw ensemble. Less comb dominance in the direct tone; still the widest native core. |
| Storm | Driven detuned saw/PWM voice. Keep its edge and transient; slow PWM already bounded in prior pass. |
| Orbit | Slowly evolving harmonic shape. Correct phase cancellation rather than compensating the lost fundamental with bass gain. |
| Bamboo | Short coupled filter/amplitude LPG decay, wood-to-metal FM. Keep strike/release identity and its small fixed state. |
| Pad/PADsynth | Harmonic bed, now less pronounced moving formant; retain gentle foreground ducking and sustained voice gain. |
| String/body | Pluck plus world material modes. Body only affects string, not every synth: not the global cause of tubular sound. |
| Bowed | Bow noise, softened saw body, quiet sympathetic fifth/octave. Keep its own three-voice budget and held-note lifecycle. |
| Horn | Attack air and intentional brass vowel. Its formant is intrinsic character, not a global filter; hear Alps in context. |
| Choir | Soft harmonic stack, breath, vowel/damping. Retain distinction from Mist rather than removing every formant. |
| Glass/Guembri | Separate transient materials within Ambient; no extra stacking or new polyphony. |
| Bass/drone | Existing shared pitch and conservative tail protection retained; no additional sub layer. |
| Common effects | Single dry/send topology retained. BLUR now preserves pitch; delay, FDN, controlled shimmer and tape retain their functions. Native/direct and room renders distinguish source colour from common processing. |

This is a source and measurable-behaviour review, not a claim that every voice
has passed human listening. Extreme FM settings can intentionally be metallic;
modulated delay, tape, chorus and shimmer can intentionally change pitch.
This pass does not promise alias-free output for every high-register/extreme
setting. Long-note naturalness, masking and excessive character at useful knob
positions must be judged from the comparisons and the prototype.

## Hardware and verification

Baseline H743 CI job 102678112323 (9 September, PR head d69ad9e) linked:

| Region | Used / available |
|---|---|
| Flash, excluding scene sector | 293,076 / 1,966,080 bytes |
| DTCM, including 16KB stack reservation | 119,440 / 131,072 bytes |
| D1, including heap reservation | 417,672 / 524,288 bytes |
| D2, effects adapter | 258,100 / 294,912 bytes |

No delay/voice buffer added. Wind uses two SVFs instead of four and updates
coefficients every 64 instead of 16 samples. FX arena remains 214,489 used of
245,760 bytes; effect state 680 of 4,096. Host shared-library static data+BSS
falls 80 bytes (not an ARM memory-map substitute).

Regression checks:
- 120-second wind: deep lulls and nonstationary envelope; sample-identical
  output at 64/256 frames after reset, including macro ramp.
- BLUR stationary-pitch predictor residual: old .100969 fails; new .004757
  passes <.01, same excitation. This tests unwanted transposition, not taste.
- Orbit: 37 Shape positions including the previous cancellation point;
  minimum measured fundamental .3133, required >.15.
- Existing complete host suite covers tuning, macro/Shape response, voice
  handovers, composition, clipping, block sizes, effect state and hot-path lint.

Final ARM linking/region usage is verified on the published commit's CI.
No hardware DWT measurement is available. Release gate remains peak_load <.60,
zero deadline misses, stack high-water measurement and listening on speaker,
headphones, line out and mono. Do not call this final AAA acceptance.

## Reproduce and listen

Build two host libraries with `tools/render_musical_review.py:build_engine`, one
from d69ad9e and one from this change. Then:

```sh
python3 tools/render_sonic_review.py --before /tmp/before.so --after /tmp/after.so --output /tmp/sonic
```

38 renders: 120s wind/noise A/B; all six cores both dry and with room A/B;
five world performances A/B. Same notes, controls and fresh process per clip.
Constant per-pair integrated-loudness matching (maximum -25 LUFS, at least
3dB true-peak reserve), no compression/EQ. Raw gain metrics included. Every
FLAC must decode byte-identically to its input PCM before packaging.

The four-minute `Ambient_Klangreview_AB.wav` contains Wind A/B, Noise A/B,
Orbit with room A/B and Mist with room A/B. Full lossless comparisons and
instructions: `Ambient_Klangreview_Hoerpaket.zip`.
