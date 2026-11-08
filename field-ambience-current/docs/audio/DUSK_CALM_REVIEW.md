# Dusk: stronger fundamental, quieter colour

User feedback on the tracked-filter candidate: improved, but still "spicy
sci fi" rather than calming ambient. Keep Dusk under review; meter balance
is not sonic acceptance. Baseline: 388be4a7559b880979dd526759b96059a925c048.

## Concrete finding and change

dsp_poly_saw and dsp_poly_square have opposite fundamental polarity at the
same phase. The old .8*saw + .2*square combination partially cancelled the
root while retaining a strong octave. At default C4/velocity .5, measured dry
octave amplitude was +4.20 dB relative to the fundamental. This is a plausible
contributor to the hollow/synthetic impression, not a proof of its sole cause.

Use .8*saw - .2*square so fundamentals reinforce. Native level .85 -> .50
compensates for the stronger oscillator. Also change native defaults:
resonance .4825 -> .1975, drive 1.30 -> 1.00, filter bloom at C4 780 -> 312 Hz,
glide time constant ~30 -> 8 ms, amp attack 60 -> 120 ms, release time constant
300 -> 400 ms. New A..F percentages: 50/5/35/0/0/12. Control ranges and the
existing pitch tracking, filter smoothing and host output trim stay intact.
The goal is a warm, direct body with less octave emphasis, sweep and portamento.

## Evidence and practical limits

tools/review_dusk_tone.py projects harmonics 1..8 through a Hann window over
the 1..2 s dry C4 body. Octave/root becomes -13.63 dB; third/root -22.19 dB.
The new native regression requires the octave to remain at least ~8 dB below
the root at C4/three velocities; measured -14.4..-13.3 dB. Existing register,
soft-onset, release and extreme-colour bounds also pass.

The same 15-probe device grid (C3/G3/C4/G4/C5, .2/.5/.85) now has body spread
1.06..1.07 dB, versus <.5 dB previously. Native body spread is only ~.03 dB;
the downstream path affects the changed spectrum differently. Both are
reported rather than claiming the previous <.5 dB device result still applies.
Maximum dry true peak is -17.4 dBFS; integrated offset to unchanged Mist is
-2.3..+.9 LU, median -.8 LU. No further output normalization is added.
See DUSK_CALM_METRICS.json; review_dusk.py reproduces the level grid.

Full bash test/run_tests.sh passes: 16436 device checks, 478860 effects checks,
zero failures. No new DSP state, buffers, oscillators, filters, or expensive
sample operations; this reuses the existing primitives and parameter paths.
H743 DWT/output listening remain open. Saved Dusk scenes keep IDs and values,
but the oscillator polarity, fixed native level and amp timings change their
sound; new parameter defaults apply to newly initialized settings.

## Listening checkpoint

Ambient_Dusk_Calm_AB_25s.wav compares the rejected candidate (0..12 s) to this
revision (13..25 s), identical Just phrase and light room. Constant gain per
half +4.1/+5.0 dB matches phrase loudness, with no per-note normalization,
new effects, reference tones or compression. Output -25.0 LUFS, -17.3 dBFS
true peak. Reproduce using render_dusk_review.py before.so after.so output.wav.

This is a calmer-voicing candidate, not a claim to have heard or approved it.
Dusk's fit with the intended atmosphere remains a listening decision. Dew's
register-dependent attacks and the other queue items remain pending.
