# Dusk: tracked filter and rounded bloom

Baseline 57dd35d60b02eda963cd43497cbfc87926aca44a. Goal: a compact, warm
evening character with a stable body, distinct from the broad Mist ensemble.
Host evidence and implementation below do not constitute listening approval.

## Confirmed cause

The old default filter floor was fixed at 325.28 Hz. After the fast filter
envelope closed, C5's 523.25 Hz fundamental lay above that four-pole low-pass
floor. At velocity .5, dry body RMS (1..1.4 s) fell 16.65 dB from C3 to C5.
A controlled baseline experiment changed ONLY cutoff, scaling it with pitch
relative to C3: 325.28/650.56/1301.12 Hz at C3/C4/C5. The remaining body spread
was 0.40 dB. Oscillator, envelopes, resonance, drive and output gain stayed
unchanged. This isolates cutoff/register mismatch as the dominant cause.
Reproduce that intervention through parameter A = sqrt((cutoff-100)/2200),
allow one second to settle, then hold the note. DUSK_REVIEW_METRICS.json
contains these six diagnostic probes and the final before/after grid.

## Implementation

The filter floor and envelope range now track smoothed frequency relative
to C4. Thus cutoff also glides during held-note retuning. The existing
80..8000 Hz clamp and native/shared controls remain in the same signal path.
No post-filter EQ, register gain compensation or extra oscillator is added.

| Behaviour | Before | Dusk candidate |
|---|---|---|
| Native amp attack | 6 ms | 60 ms |
| Amp decay time constant | 80 ms | 280 ms |
| Release time constant | 60 ms | 300 ms |
| Default cutoff | fixed ~325 Hz | ~2.48 times note frequency |
| Default filter envelope | ~5490 Hz, ~180 ms decay | 780 Hz at C4, 620 ms decay, 60 ms smoothing |
| Native resonance range | .30..1.70 | .15..1.10; default .4825 |
| Native drive range | .60..3.60 | 1.00..2.20; default 1.30 |
| Filter Env range | mandatory 1500..8500 Hz | 0..2600 Hz at C4; zero disables bloom |

Filter excitation reattacks through the smoother, preserving the running
filter envelope instead of jumping it to full. Oscillator phases and amp
level remain continuous. Shape still scales native amp attack/release.
Default A..F percentages: 50/35/35/25/18/30. Output trim stays -4.1 dB.

## Verification

Five notes C3/G3/C4/G4/C5 x velocities .2/.5/.85, native defaults, dry device
path, master .6, 3 s held + 1 s released; all rendering chunked <=512 frames.
Body spread across registers: before 16.22..16.95 dB, after 0.476..0.482 dB.
Maximum true peak: -11.0 -> -14.5 dBFS. Against the unchanged Mist reference,
integrated loudness offset is -1.0..+2.1 LU, median +0.1 LU. This preserves
the common reference without another gain adjustment.

New native regression covers soft onset, C3..C5 body consistency at three
velocities, audible release followed by silence, and low/high control plus
global-colour interactions at C2/C4/C6. Full bash test/run_tests.sh passes,
including 16436 device checks and 478860 effects checks, zero failures.
Existing held Equal/Just retuning, velocity, switching, send/master and
realtime lint checks remain green.

Native static state symbol: 164 -> 168 bytes in the host build (one float).
No new buffers/heap/transcendentals in the sample loop. The additional work is
bounded envelope arithmetic and pitch scaling at the existing filter rate.
H743 map/DWT, <60% peak load, zero deadline misses and physical output listening
remain required; host timing is not a hardware qualification.

Scene IDs/order and file format are stable. Existing Dusk scenes adopt the
new tracking, envelope and parameter ranges; they do not reproduce the old
acid timbre exactly. Other core parameters and world presets are unchanged.

## Listening and remaining work

Ambient_Dusk_AB_25s.wav: 12 s before, 1 s pause, 12 s after. Original Just
C3/C4/E4/G4/C5 phrase, light shared room, identical velocity .5 and timing.
Constant export gain per half (+3.7/+4.1 dB) matches the phrase loudness;
individual notes are not normalized. No reference sine, external samples or
compression. Output -25.0 LUFS, -12.8 dBFS true peak. Reproduce using
tools/render_dusk_review.py before.so after.so output.wav.

Judge warmth, register continuity and whether Dusk earns a distinct role;
the measured improvement alone does not prove the imagined atmosphere.
Wide-register/extreme-control timbre and world/FX combinations remain open.
Next bounded unit: Dew's uneven attack levels versus register, with the
excitation and LPG filter examined separately. Glimmer Ratio/aliasing and
Ambient held tuning remain queued.
