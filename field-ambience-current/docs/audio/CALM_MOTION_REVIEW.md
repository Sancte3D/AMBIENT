# AMBIENT — Chorus / Blur, 2026-09-23

## KERNURTEIL

Both inserts have useful possible roles: Chorus adds slow stereo movement;
Blur adds delayed, windowed fragments. Their previous gain structure also
let phase interference remove too much of a held note's body. The calming
register correction cannot fix this downstream behavior. This package makes
both inserts support a stable direct tone; it is not a listening approval.

## FUNDAMENTAL FALSCH

At full Blur, direct gain was 0.5476 and delayed gain 0.6084. A same-pitch
grain can oppose the source almost completely. This is a poor control range
for AMBIENT's stable, calming foreground. The active product-bus test found
up to 22.49 dB of windowed RMS variation from a constant-amplitude sine.
Chorus reached 8.82 dB, and the pair in Dream 22.65 dB.
These are controlled diagnostic results, not measured fluctuations of a song
or a claim that every user heard the maximum-settings case.

## NOCH NICHT SELBSTVERSTÄNDLICH

- Pitch modulation depth/rate and grain timing are unchanged. Less wet signal
  reduces their prominence; it does not prove that all siren-like impressions
  are gone. Voice vibrato, tape and moving resonances still need review.
- Motion simultaneously affects Chorus, tape/reverb movement and pad behavior.
  It remains a compound control. The musical ablation uses the FX setter only:
  it preserves Pad Motion but also removes tape/reverb motion with Chorus.
- Both effects still need a distinct perceptual purpose at matched level.
  If one stage cannot justify itself, remove it from Dream instead of raising
  its amount again. Their presence in an existing menu is not justification.
- Restoring dry energy makes the default Open Sea diagnostic 1.76 dB louder
  in RMS. The A/B uses fixed loudness matching; do not mistake this gain
  difference for better sound. Existing scenes use the gentler new ranges.

## LOCKED

Principles only: a present direct body, preserved tuning, continuous parameter
smoothing, one shared room, and modulation as supporting detail. Source
presets and envelopes remain isolated from this insert-balance correction.

## REMOVE / MERGE / REDESIGN

Change six coefficients in the active `firmware-c-next/src/ambient_effects.c`:

| Insert at full amount | Previous direct / wet | New direct / wet |
|---|---|---|
| Chorus | 0.791 / 0.396 | 0.94 / 0.18 |
| Blur | 0.5476 / 0.6084 | 0.92 / 0.24 |

No new filter, oscillator, allocation, buffer, state, transcendental or DSP
operation. The smoothed controls still drive the gain equations. Amount zero
keeps unity direct gain. The shared insert change affects standalone Chorus,
standalone Blur and Dream for every World/Character using the master effects.
The old separate effects handoff source is not the active product path.

### Verification

Regression in `effects-engine/tests/effects_verify.c`, compiled by the product
runner against the active source. 27 probes: Chorus / Blur / Dream × amounts
0 / 0.65 / 1 × 146.83 / 220 / 440 Hz. Quiet 0.1-peak sine, real bus API,
zero send, Age/Echo/Atmosphere/Shimmer zero, Tone/Width one. 100 ms RMS windows
from seconds 2..12, inspecting left, right and their mono sum. No audio file
of reference tones is delivered. The new bounds fail on old source (36 failed
checks), pass on candidate (488,805 FX checks, zero failures).

| Worst measured swing at full amount | Before | After |
|---|---:|---:|
| Chorus | 8.82 dB | 3.20 dB |
| Blur | 22.49 dB | 4.44 dB |
| Pair in Dream, other audible effects off | 22.65 dB | 6.57 dB |

This is effect-induced level variation, not a pitch-deviation metric. The
existing Blur pitch-preservation, parameter stress, memory guards and block
invariance tests remain in the suite. Full `bash test/run_tests.sh` passed;
only existing battery-test printf warnings. Host text/data/bss sizes are identical.
Current H743 map/stack/DWT and real-output listening remain open.

### Musical comparison and reduction test

`tools/review_calm_motion.py --before baseline.so --after candidate.so
--output /tmp/calm-motion` renders the real Open Sea pad, Bowed and Dream with
a D3 bed and D4 held note. 14 seconds per render, fresh process per case;
note-off at 11 seconds. Natural layer muted, World FX send preserved. These
are diagnostic notes, not an autonomous composition.

RMS in seconds 3..10:

| Path | Previous | New |
|---|---:|---:|
| Dream defaults | −33.04 dBFS | −31.28 dBFS |
| Without FX Motion | −31.80 | −30.90 |
| Without Blur | −31.98 | −31.10 |
| Without FX Motion and Blur | −30.72 | −30.72 |

No raw render clips. The combined removal is a simpler room candidate, not
an isolated reverb: tape coloration, echo and reverb remain. Numbers establish
signal contribution, not which version is musically best. Full measurements
and baseline provenance are in `CALM_MOTION_METRICS.json`.

## BESTE VERSION

The note remains settled in the landscape. Chorus gives quiet stereo life;
Blur extends texture without making the foreground periodically hollow.
A simpler Dream with one fewer stage is preferable if this distinction does
not survive listening. Next: Horn/Bowed body resonances and vibrato at D3..A4;
then combined tape/pitch motion. Shimmer remains a usefulness audit, not an
instruction to increase bright feedback. Display work stays deferred.

## TEST AM GERÄT

27.5-second A/B: old 0..13.5 s, 0.5 s gap, new from 14 s. Same notes/presets,
150 ms edge fades, one fixed gain per excerpt (+6.7 / +5.0 dB); each targets
−25 LUFS. Combined export −25 LUFS / −14.2 dBTP, no compressor or limiter.

At matched comfortable level, check sustained body, cyclical hollowness,
stereo width and mono solidity. Then default/max Motion and Blur on headphones
and device speakers: do they add distance or attract attention like a signal?
Compare removing either stage at matched level before declaring both necessary.
No claim of calming quality or adequate H743 headroom follows from host tests.
