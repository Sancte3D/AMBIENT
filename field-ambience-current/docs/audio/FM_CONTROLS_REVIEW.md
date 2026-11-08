# FM Glass — Index, Body and attack

## Confirmed issues

Before this change, FM Index set only the initial peak. An independently
absolute Body target (0..0.5 turns) could exceed a low peak, causing the stated
Index Decay to brighten toward Body instead. Once settled, changing Index on
a held note had practically no effect. New notes also jumped the running
modulation index even when the previous note was still audible.

## Change

- Body is now 0..80% of the velocity-scaled Index, using a normalized decay.
- Index follows the live knob during sustain. Its output is smoothed over
  80 ms, including reattacks; oscillator phase remains intact.
- Native attack is 18 ms instead of 6 ms, still scaled by global Shape.
- Default Index is 27% (was 59%), Body 45% (was 56%). Ratio, tone/filter,
  glide and output gain are unchanged. No additional post-filter is used.

Saved scene knob values remain readable, but Body now has relative semantics
and the natural attack is longer. Existing FM presets therefore change sound;
this is an intentional correction, not a transparent scene migration.

## Verification

A real dry-PCM regression fails on the previous implementation. It verifies
low Index with maximum Body, live Index on the same held note, Body zero,
finite/bounded output and release. New third/fundamental energy ratios are
0.010187 at minimum Index and 0.086415 after the live Index increase to 25%.
Full bash test/run_tests.sh passes: 16406 device and 478860 effects checks,
plus the new dedicated FM test; no failures.

Resource delta: two floats (8 bytes) and bounded arithmetic in the sample
loop, no new buffers, heap, or per-sample transcendentals. H743 build/map/DWT
and final listening are not established by these host tests.

Audio: Ambient_FM_Klang_AB_25s.wav, 25 seconds. Same three-note phrase,
velocities, room and master settings, using each version's native defaults.
Before 0–12 s, silence 12–13 s, after 13–25 s. Constant loudness matching,
no reference sine, post-EQ or compression. -25 LUFS integrated, -16.4 dBFS
true peak. Native raw integrated levels were -18.1 / -17.2 LUFS: a softer
spectrum does not automatically mean a lower output level. Cross-core level
calibration remains open. Reproduce with tools/render_fm_controls_review.py.

## Remaining FM work

Ratio switches still select integers 1..6 discretely. High register/high Index
can generate aliased sidebands; integer ratios alone do not guarantee harmonic
output at the sampled output. Review Ratio transitions, upper-register index
limits, Tone/Resonance extremes and keyboard balance next. The current change
is not a claim of complete FM or instrument quality acceptance.
