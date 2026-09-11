# Storm / Mist / Orbit — role comparison

Source: device firmware tree at 3cbb3a47ce1b3d50e77ec0c71fe58f2da7630572.
No firmware change in this unit. Reproducible exporter:
`python tools/render_core_role_comparison.py engine.so comparison.wav metrics.json`.

## What the comparison establishes

The three candidates differ in envelope, source architecture and stereo field.
Storm has a plausible centred sustained-pulse role beside Mist's ensemble and
Orbit's phase-related waveform morph. Retain Storm as a candidate, not as a
listening-approved production sound. Centroid/width measurements do not prove
beauty, perceived uniqueness or that all three deserve final product slots.

| Candidate | Role hypothesis | Measured dry behaviour on C4 |
|---|---|---|
| Storm | Compact pulse/reed foreground | Mono; 353 Hz spectral centroid; first 50 ms energy 3.8 dB below steady portion |
| Mist | Slow stereo ensemble | Side/mid energy -13.3 dB; 479 Hz centroid; first 50 ms energy 16.5 dB below steady portion |
| Orbit | More immediate harmonic morph | Mono; 524 Hz centroid; first 50 ms energy 1.9 dB above steady portion |

These are features of this default, this note and these analysis windows,
not universal descriptions or quality ratings. The eight-second excerpts do
not expose a full slow-modulation cycle. Native stereo measurements use FX
bypass; the user-facing comparison uses identical light shared-room settings.

## Concrete product gap: native output balance

Identical two-note phrase (Just C4/E4, velocity 0.4), master 0.4, native defaults:

| Core | Raw integrated LUFS | Constant demo gain | Matched LUFS |
|---|---:|---:|---:|
| Storm | -25.1 | +0.1 dB | -25.0 |
| Mist | -29.6 | +4.6 dB | -25.0 |
| Orbit | -18.3 | -6.7 dB | -25.0 |

Orbit is 11.3 LU above Mist for this phrase. Native envelope differences and
timbre contribute; this is not a command to apply these exact gain corrections
to the firmware. Next bounded technical unit: output calibration of these
three cores across relevant registers and velocities, preserving quiet attacks,
release behaviour and intentional dynamics. Prefer fixed core gain decisions
within the existing mixer; no automatic loudness pumping or extra audio layer.

## Listening asset and checks

Ambient_Storm_Mist_Orbit_26s.wav: 26 seconds total.
Storm 0–8 s, Mist 9–17 s, Orbit 18–26 s; one-second pauses.
Same two notes, gates, velocity, Shape, room and master. Only constant gain
matching is applied after native rendering; no reference sine, added EQ or
compression. Output -25.1 LUFS, -11.0 dBFS true peak.

Exporter assertions: finite/non-clipped raw PCM, exact duration, filled late
sustain windows (512-frame render contract), all three sections matched within
0.2 LU with at least 6 dB true-peak headroom. Exact results are retained in
CORE_ROLE_COMPARISON_METRICS.json. No repeated firmware suite required for
this tools/docs-only unit; firmware remains the previously tested Storm tree.

Still open: user listening verdict and keep/drop decision, long modulation,
higher-register aliasing and extreme controls, H743 load and physical outputs.
