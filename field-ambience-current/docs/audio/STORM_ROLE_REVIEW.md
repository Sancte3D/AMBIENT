# Storm — restrained sustained pulse candidate

Product hypothesis: a centred, sustained pulse/reed colour, distinct from
Mist's chorus ensemble and Orbit's waveform morph. This is a review candidate,
not a decision that Storm deserves its place in the final palette.

## Implemented in the existing core

| Property | Before | Candidate |
|---|---|---|
| Attack pitch | Velocity-dependent downward offset relaxing to target | No attack offset; existing note-to-note glide retained |
| Native amp attack | 4 ms | 80 ms, still scaled by Shape |
| Release time constant | 120 ms | 600 ms, still scaled by Shape |
| Detune range / default | +/-4..34 cents / 16 | +/-0..12 cents / 6 |
| PWM depth range / default | 0..0.45 / 0.351 | 0..0.18 / 0.081 |
| Native drive range / default | 1..4 / 1.6 | 1..2 / 1.15 |
| PWM rate range / default | 0.025..0.125 Hz / 0.060 | 0.015..0.080 Hz / approximately 0.035 |
| Ladder resonance | 0.55 | 0.25 before global macro |
| Default cutoff | approximately 2.2 kHz | 1.8 kHz |

Same oscillator count, mono output, ladder, voice limit, send and output gain.
No new buffer or DSP stage. The two bend floats and their per-sample operations
are removed: native object state shrinks from 192 to 184 bytes. This is not an
H743 map/CPU measurement. Physical peak-load/stack/output acceptance stays open.

## Verification

The old implementation fails the new real-PCM onset test at -16.298 cents.
With detune/PWM/drive set to their minima, the new early-onset measurements are
+0.009 cents at 440 Hz and +0.005 cents at 880 Hz. This probes the attack bend;
it is not a claim that detuned default oscillators all play exactly one pitch.
The regression also checks restrained early attack energy, audible release,
retirement, and nine min/mid/max native-control/register combinations.

Full bash test/run_tests.sh: exit 0, 16406 device checks and 478860 effects
checks, zero failures, including the new Storm regression. Hardware and
subjective sound acceptance are separate from these tests.

## Listening and compatibility

Ambient_Storm_AB_25s.wav: 25 seconds, before 0–12 s, silence 12–13 s, after
13–25 s. Same C4/E4/G4 phrase, velocities and light shared room. Native defaults,
constant loudness matching, no reference sine or post-EQ/compression.
Integrated loudness -25.7 LUFS, true peak -13.8 dBFS. Raw A/B loudness before
matching was -25.1/-25.7 LUFS. Reproduce using tools/render_storm_review.py.

Synth ID, menu name, control slots and scene byte format stay the same.
Existing Storm scenes keep their values but sound different because the
ranges/envelope changed. This is an intentional product rework, not a
transparent reproduction of the hoover voice.

Next: compare the candidate with Mist and Orbit at matched playback level to
judge a distinct role. Keep/drop is still open. Higher-register aliasing,
combined extreme macros, very short taps/maximum Shape and physical output
balance require their own targeted checks. Ambient/V2 coexistence is not
implemented; existing mono routing and generation guards remain in place.
