# SD19 — matched ensemble Room baseline, 10 October 2026

Three compact comparisons now isolate Room in the actual COAST, long WOODLAND
and HIGHLANDS generators. The four versions contain identical real note-on/off
histories, pitches, velocities and owners. No source, macro, generator, default,
send or Room DSP was changed. SD19 remains open: this is a measured baseline and
hearing preparation, with a concrete calibration finding.

## What the current Room actually contributes

The RMS difference between the Room and Dry PCM in the same 3.0–9.5s held
ensemble section, divided by Dry RMS, is small:

| World | Room 0.24 | Room 0.50 | Room 1.00 |
|---|---:|---:|---:|
| COAST | 0.197% | 0.663% | 2.819% |
| WOODLAND | 0.238% | 0.929% | 4.085% |
| HIGHLANDS | 0.257% | 1.037% | 4.820% |

These are approximate wet contributions in matched, quantized, master-filtered
PCM, not wet energy percentages or perceptual judgements. Near Dry there is a
PCM16 quantization floor. The natural source pitches, register and articulation
also affect the Room excitation. The modest contribution gives a concrete
reason to investigate the previously reported dry impression; it does not prove
which new room is musically preferable. The next single unit should calibrate
one bounded version of this same FDN against this baseline, keeping its fixed
delays, one arena, owner/tail rules and Nature 0. Do not select a final setting
from loudness or decay numbers alone.

## Listen

Files: `AMBIENT_COAST_Room_AB_27p5s.wav`,
`AMBIENT_WOODLAND_Room_AB_27p5s.wav`,
`AMBIENT_HIGHLANDS_Room_AB_27p5s.wav`.

| Delivered position | Actual fixed Room control |
|---|---|
| 0.0–6.5s | Dry / 0 |
| 7.0–13.5s | 0.24, earlier audition value |
| 14.0–20.5s | 0.50, current parameter default |
| 21.0–27.5s | 1.00, current maximum |

Room 1 means the maximum control value, not 100% wet. Each section is the exact
same 3.0–9.5s excerpt from a separate 32s normal-engine performance. Three 0.5s
editorial gaps and 40ms excerpt-edge fades make the cuts clean; they do not
shorten the WOODLAND articulation or represent live Room changes. Listen for
held chord separation, warmth, space and whether a common room is identifiable
across all three Worlds. The earlier long World phrases remain available to
judge full phrasing; this short montage does not replace them.

One constant scalar per entire variant matches excerpts to −23 LUFS, with a
common feasible target and at least 6dB true-peak reserve. No compression, AGC,
firmware gain change or dynamic matching. Applied scalar gains are +13dB COAST,
+10.2–10.3dB WOODLAND and +6.6–6.7dB HIGHLANDS. Raw and delivered levels are both
retained in [SD19_ROOM_METRICS.json](SD19_ROOM_METRICS.json). Final montages are
−23.0/−23.0/−23.1 LUFS, true peaks −11.0/−13.0/−15.9dBFS respectively. Every
individual delivered variant measures −23.0 LUFS. All files are complete stereo
PCM16, 44.1kHz, 1,212,750 frames, 4,851,044 bytes.

## Actual-engine evidence

`tools/render_room_comparison.c` calls the unchanged Product Engine APIs.
`tools/review_room_comparison.py` compiles those actual sources with C11/O2 and
warnings as errors, renders and packages the result. D-major/ET, seed1234,
Activity/Color/Attack/Release0.5, Volume0.6, Nature0. A silent second settles
parameter smoothing before Generate. Every Main tick is on the same10ms grid.
Generate stops16s into each32s recorded performance; no Clear or artificial
fade on the raw performances. Maximum actual sources3/2/3, including releases.
Starts5/3/5; all actual sources retire24.82/18.00/23.13s respectively at every
Room amount. Finite counters and output-limiter counters remain0.

All12 performances were rendered at both64 and512 frames on the identical Main
grid. Complete PCM WAVs and actual on/off CSVs are byte-identical across block
sizes. Each World's complete32s on/off CSV is also byte-identical across all
four Room amounts. This comparison therefore does not bypass harmonic/tail
admission or conceal a changed musical schedule. No broader timing invariance
is claimed: Room tail-quiet admission may alter later phrases in other cases.
Raw true peaks stay below−20.7dBFS; mono energy ratios stay≥0.92632, DC<0.0002.

## Float impulse, decay and mono evidence

The separate `impulse` diagnostic sends a0.3 impulse to both real Room input
buses after1s silent settling. It is explicitly a diagnostic, not a musical
World score. A32s IEEE-float32 stereo capture preserves the tail below PCM16
resolution. All four captures are byte-identical at64/512 frames; Amount0
produces exact zero. Positive settings share first wet output at frame1117
(25.33ms), consistent with fixed delays and no Room-time pitch movement.

| Amount | Nominal feedback T60 | Broadband T20 | Broadband T30 | Mono energy ratio | Energy fraction after9s |
|---|---:|---:|---:|---:|---:|
| 0.24 | 1.40736s | 1.17426s | 1.21155s | 0.75348 | 0 |
| 0.50 | 2.10000s | 1.71951s | 1.79848s | 0.75126 | 1.07e−27 |
| 1.00 | 4.80000s | 3.87700s | 4.04051s | 0.74651 | 3.47e−13 |

T20/T30 are linear fits of the −5..−25/−35dB Schroeder broadband energy decay,
extrapolated to60dB. Fit R²≥0.9984. They are distinct from the feedback parameter
and do not imply uniform decay at every frequency. The late9s energy fraction
stays below1e−6 at every amount, finite output and mono criteria pass.

The existing `test/test_room_nature.c` also passes unchanged: impulse0/.5/1,
low146.8324Hz sustain through maximum Room, complete Dry transition with no old
tail revival, actual Clear, cold idle, Nature seed/block/invalid-input behaviour,
and independent actual90s note histories with/without Nature for all Worlds.
That test is broader than the Nature0 audition; it is not Nature hearing approval.

Room initialization uses statically linked storage, `void ambient_room_init`,
and no fallible allocation. No allocation-failure injection is represented as a
passed Init-Fail test. Exact Dry and completed bypass are exercised. Real target
boot/fault recovery, output hardware, CPU and stack high-water remain device gates.

## Scope and reproduction

Baseline: PR151 commit `0317077806ca67e35fadd6b053ecb70dd76d0120`,
tree `169dca2667c94c0236e39ef6c75cab7950b002bc`. Its full CI37997896683/run677
completed successfully. This checkpoint only adds tools, documentation and a CI
review step; every firmware source/header/CMake input is unchanged. No new local
ARM build or complete host-suite run is claimed. The exact green baseline
already covers H743 Product/Reference/Journal; this unit runs targeted real
Room/audio checks. New CI also retains those existing cross-build gates.

Run from `field-ambience-current/firmware-c-next`:

```sh
python3 tools/review_room_comparison.py build-room-comparison
```

Metrics fingerprint all renderer/test/core/header inputs, complete raw WAV/trace
hashes and delivered montage hashes. CI uploads only three small comparisons,
metrics and actual note histories, not the full raw PCM/float diagnostic files.
Spec0.7, reference default, JournalOFF, source algorithms and long WOODLAND remain
unchanged. Same TODO:24 technically closed /30 with hearing/device/storage/UX
gates open. User requests to continue authorize work, not final hearing approval.
