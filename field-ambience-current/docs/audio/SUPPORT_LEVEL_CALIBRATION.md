# Resonant / Keys / Pluck output calibration

Baseline f83bad19ab24737f631015f2f5620a8711b8171b. Old-name mapping is in
SOUND_NAMES.md. This is level calibration, not final voicing or listening approval.

## Method and result

60 probes before and 60 after: four cores including Ensemble reference,
MIDI 48/55/60/67/72, velocity .2/.5/.85, native defaults, dry device path,
master .6, three seconds held plus one second released. The offline renderer
chunks every engine call to <=512 frames. SUPPORT_LEVEL_METRICS.json contains
all results; tools/calibrate_support_levels.py reproduces them.

For Resonant/Keys compare integrated four-second loudness to Ensemble.
For Pluck compare its first 400 ms to Ensemble's established body at 1..1.4 s.
This avoids raising a short event merely because its tail is silent. These
are distinct role-based comparisons, not one universal LUFS ranking.

| Core | Fixed output trim | Median offset before / after | Remaining offset range |
|---|---:|---:|---:|
| Resonant | -4.1 dB | +4.1 / 0.0 LU | -4.6..+3.1 LU |
| Keys | -12.4 dB | +12.4 / 0.0 LU | -0.8..+0.9 LU |
| Pluck | -6.8 dB | +6.8 / 0.0 LU | -6.4..+4.2 LU |

Native velocity/envelope/filter behaviour is preserved; no per-note gain or
AGC is introduced. Full-file RMS offsets match each fixed trim within 0.004 dB
in all 45 changed probes; Ensemble is unchanged. Integrated LUFS differences
can deviate because the measurement gate includes/excludes different portions
of a decaying signal. Do not interpret that as dynamic gain processing.
Maximum dry true peaks after: Resonant -11.0, Keys -15.3, Pluck -13.8 dBFS.

## Unresolved source behaviour

Keys' gross default level mismatch is addressed across the measured range.
Its Ratio changes, high-register aliasing and extreme controls remain open.
Resonant still varies substantially with register; a fixed filter floor and
fast-closing filter envelope are source-level candidates to investigate.
Pluck's attack level is notably lower at G3 than nearby sampled C3/C4, and
velocity changes its native excitation as well as host gain. The measurements
establish uneven response, not the exact cause or that all variation is wrong.
Investigate excitation/filter/register behaviour before further normalization.

The trims set a usable common reference without claiming these remaining
ranges are product-approved. Preserved dynamics are intentional; the tonal
palette and useful registers still need listening decisions.

## Compatibility, cost and verification

Only three values in the existing const gain table change. Same trim on dry
and send, including the departing crossfade leg; envelope following stays
before the trim. No new DSP state, buffers, allocations or processing stage.
Native parameters, scene IDs/order and format remain compatible; recalled
scenes have the new lower outputs. Shared nonlinear drive/FX may respond to
the lower input; this dry calibration does not qualify extreme FX settings.

Full bash test/run_tests.sh passes: 16436 device checks, 478860 effects checks,
zero failures. The sustained-core RMS regression now includes Keys and checks
three octaves at soft/hard velocity (1.08..1.60x spread, required <2x).
Hardware map/DWT, <60% peak load, zero deadline misses and listening on the
actual outputs remain unmeasured release gates.

Audio: Ambient_Klangtypen_AB_25s.wav, Resonant -> Keys -> Pluck, four seconds
each. Before 0..12 s, pause 12..13 s, after 13..25 s. Identical C4/E4 motifs,
velocity .5, light room; one constant -4.1 dB export gain across the whole file.
No per-section matching, diagnostic tones or compression. Output -25.0 LUFS,
-13.3 dBFS true peak. Reproduce with tools/render_support_balance.py.

Next bounded unit: Resonant default filter/envelope and register response,
then Pluck's register-dependent attacks. Keep the FM Ratio/aliasing and Ambient
held-retuning tasks open; do not add layers or finalize names to hide these gaps.
