# Fixed output calibration: Storm, Mist, Orbit

## Measurement and scope

Baseline: 1bafbd727ee89a960cf3a4fe61fefc2f387fe44d.
45 probes per version: three cores, MIDI 48/55/60/67/72 (C3–C5), velocity
0.2/0.5/0.85. Each uses native defaults, dry device path, master 0.6,
3 seconds gated plus 1 second release. Measure integrated LUFS/true peak
and steady RMS. See CORE_LEVEL_CALIBRATION_METRICS.json for every row;
reproduce with tools/calibrate_core_levels.py.

| Metric across comparable notes/velocities | Before | After |
|---|---:|---:|
| Median loudness spread | 9.5 LU | 0.6 LU |
| Largest loudness spread | 10.5 LU | 1.0 LU |
| Maximum true peak in grid | -6.3 dBFS | -6.8 dBFS |

This grid is distinct from the earlier two-note phrase's 11.3 LU gap.
Different attack structures remain; matching LUFS does not imply equal RMS
or equal transient peaks. No claim is made for all settings or registers.

## Implementation

Fixed host trims: Storm -4.8 dB, Mist 0 dB, Orbit -9.8 dB. The quieter Mist
is the reference, avoiding a boost. Other cores retain unity output trim.
The same trim applies to dry and send buses, including the departing core
while crossfading. Velocity and the envelope follower remain before the trim,
so the native Envmod response does not depend on the new output calibration.
No limiter/AGC, envelope rewriting, EQ or parameter smoothing stage is added.

Technical cost: a six-float const table (24 bytes) and bounded gain arithmetic
folded into the existing velocity/mix loops; no new audio buffer or mutable
DSP state. Physical H743 map/DWT/outputs still need device verification.
Scene IDs/values remain compatible; Storm/Orbit scenes now output at the lower
calibrated levels. Shared nonlinear drive/effects can still respond differently
to lower input; the default dry grid is not an extreme-FX qualification.

## Verification

All 45 before/after probes show the same per-core loudness offsets at every
velocity/register, confirming fixed rather than signal-dependent gain.
An added device regression checks three octaves and soft/hard velocities;
RMS spread is 1.07–1.59x across the distinct spectra (bound <2x).
Full bash test/run_tests.sh: exit 0, 16430 device checks, 478860 effects checks,
zero failures; existing velocity, shared-send, tuning and handover tests pass.

Audio: Ambient_Pegel_AB_25s.wav. Before 0–12 s, after 13–25 s. Each half plays
Storm, Mist, Orbit for 4 s each with identical C4/E4 motifs, light room and
velocity. One constant -0.5 dB export gain applies to the ENTIRE file; there
is no per-core/section loudness normalization hiding the firmware correction.
Output -25.0 LUFS, -12.1 dBFS true peak, 25 s total. Reproduce with
 tools/render_level_balance.py before.so after.so output.wav.

Next: calibrate Acid/FM Glass/Bamboo against the same reference with suitable
transient/sustain probes; do not flatten their different roles. Core-specific
extreme controls, FM Ratio/aliasing, Ambient held tuning and physical listening
remain separate open work.
