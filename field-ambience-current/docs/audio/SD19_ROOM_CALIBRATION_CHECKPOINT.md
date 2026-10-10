# SD19 — calibrated shared Room return, 11 October 2026

The previous Room return barely contributed to held ensembles at the normal
control position. Product now uses a bounded linear return, `wet=5*r*enable`,
instead of `(0.12+0.50*r)*r*enable`. At Room0.5 the measured Room−Dry RMS ratio
increases from0.66–1.04% to8.86–13.99%. The same source body, send0.35, one FDN,
fixed delays, diffusion, damping, width and feedback remain in use. This is one
output calibration, not a new World, source, layer or feedback boost. Hearing
must still decide whether this stronger common room fits the instrument.

## Measured return

Relative to identical Dry PCM, actual3.0–9.5s held ensembles:

| World | Old Room0.5 | New Room0.24 | New Room0.5 | New Room1 |
|---|---:|---:|---:|---:|
| COAST | 0.663% | 3.535% | 8.859% | 22.719% |
| WOODLAND | 0.929% | 4.740% | 12.515% | 32.939% |
| HIGHLANDS | 1.037% | 5.267% | 13.992% | 38.866% |

These are approximate ratios of quantized, master-filtered RMS differences;
they are not energy percentages or perceptual quality ratings. The new scalar
is linear, monotonic, exactly0 at Dry, maximum5. The internal matrix/input
calibration and damping determine the actual return amplitude, so Room1 is not
100%wet. At default0.5 the return is13.51× the former scalar; at maximum8.06×.
The new curve makes a measurable contribution at ordinary control positions
while the full retained parameter extremes still pass headroom/mono tests.

## Three short exact-engine A/B files

`AMBIENT_COAST_Room_Calibrated_AB_27p5s.wav`

`AMBIENT_WOODLAND_Room_Calibrated_AB_27p5s.wav`

`AMBIENT_HIGHLANDS_Room_Calibrated_AB_27p5s.wav`

| Delivered section | Actual excerpt |
|---|---|
| 0–6.5s | Old Room0.5, held ensemble3.0–9.5s |
| 7–13.5s | New Room0.5, same held ensemble |
| 14–20.5s | Old Room0.5, real Stop/release16.0–22.5s |
| 21–27.5s | New Room0.5, same Stop/release |

Actual Product Engine, D-major/ET, seed1234, normal Activity/Color/Attack/Release0.5,
Volume0.6, Nature0. Silent1s settling,10ms Main grid,32s raw, GenerateStop16s.
No direct-source score, altered articulation, Clear or artificial raw ending.
Cut montages have three0.5s editorial gaps and40ms edge fades. They do not
represent live Room changes or shorten WOODLAND's long source articulation.
Full performances prove final sources retire24.82/18.00/23.13s, old and new.
Audition releases from Stop rather than the last owner-retirement second,
which can already be silent in PCM16.

Each version has one fixed gain determined by its held excerpt to−23LUFS;
apply that same gain to its release. No compression, AGC or tail normalization.
Old/new gains: COAST+13/+12.9dB, WOODLAND+10.3/+9.9dB,
HIGHLANDS+6.6/+6.0dB. Whole montage LUFS−23.6/−23.5/−23.7, true
peaks−11.1/−13.0/−16.0dBFS respectively. Every held section measures−23LUFS.
Each file: stereoPCM16/44100Hz,1,212,750frames,4,851,044bytes.

Baseline Room is read from exact PR152 commit
`09319610912b81a8c71860479a6eeba754f53427` and its source hash is verified against
the published baseline metrics. All other source/header inputs are identical.
New and old complete32s note-on/off CSVs are byte-identical. Baseline PCM and
traces also reproduce the previously published PR152 hashes on this host;
those matches are recorded, not required across arbitrary future host compilers.
The candidate audit reproduces all twelve full PCM/trace files at64/512 on the
same10ms grid. No global note-timing invariance is promised: the stronger return
can keep actual room tails above the existing quiet threshold for longer in
other/later phrases. The existing harmonic tail ledger and admission remain active.

## Decay, transitions and full regression

Four32s float impulses0/.24/.50/1: finite, exact-zero Dry, byte-identical64/512,
positive firstwet1117samples/25.33ms, mono-energy≥0.7465, late9s energy<1e−6.
Compared with the exact old Room, normalized broadband SchroederT30, mono energy
ratio and firstwet onset are unchanged within1e−6. T30≈1.21155/1.79848/4.04051s,
distinct from nominal feedback1.40736/2.1/4.8s. Only output weighting changes.
The same fixed delays and feedback avoid delay-time pitch movement.

The complete `test/run_tests.sh` finishes exit0 on the new core, including:
12min Product PCM and input/tuning/ownership invariance; actual COAST48,
WOODLAND48 and HIGHLANDS52 key/collection/tuning/seed/wrap phrases;2304 pure
long-form cases×16phrases and90min actual PCM; Room/Nature impulse/low-sustain/
Dry-revival/score independence;24 source-limit cases and36min high-Room/Volume/
envelope/Activity/Color/tuning stress; all six directed World transitions,
Mute/Clear, scenes/journal/control dispatch and the remaining reference tests.
Output-limiter/NaN counters stay0 in the normal retained stress cases, sample
headroom remains>6dB and mono checks pass. The current12 default-macro review
performances have raw true peaks≤−20.2dBFS and mono≥0.9225.

One initial diagnostic capture was incomplete and failed strict comparison.
It was discarded. Final full captures and final WAV sizes/hashes pass. The
renderer now checks stream error flags as well as close status; a prior failed
write must not masquerade as a successful capture. No crash cause is inferred
from that observation. Audio-protect assertions were not relaxed.

## Real H743 baseline and candidate build

Both Product Release builds use the same verifiedGCC13.2.1/Newlib4.4 toolchain.
All four Ubuntu package sizes and SHA256s were checked before extraction/use.
Fresh PR152 baseline ELF exactly matches the prior recorded baseline
`c6e67d391826b4a51ed685445230580a278ed0151c44cc29fe0de107a809f130`.
Both real link/bank/DMA/compiler-frame audits pass.

| Resource | PR152 baseline | Candidate | Delta |
|---|---:|---:|---:|
| Flash / Bank1 load | 195936B | 195928B | −8B |
| D1 RAM | 65664B | 65664B | 0 |
| D2 RAM | 60808B | 60808B | 0 |
| Reserved DTCM stack | 16384B | 16384B | 0 |
| Largest individual core compiler frame | 192B | 192B | 0 |

182 core frames, Room frame192B, engine-render88B; one D2 tank55352B plus
diffusion5296B, aligned4096B D1 audioDMA. No reference/archive DSP families
retained in Product. Actual CPU/ISR latency, nested call-stack high-water and
the DAC/amplifier/speaker remain device gates; compiler frames are not those
measurements. Exact fingerprints and compiler/package/bank data:
[SD19_ROOM_CALIBRATION_ARM_METRICS.json](SD19_ROOM_CALIBRATION_ARM_METRICS.json).

## Reproduction and remaining decision

From firmware-c-next, with PR152 object available in local Git:

```sh
python3 tools/review_room_calibration.py build-room-calibration
```

This runs the current Room audit then makes the three old/new comparisons.
An already verified current audit can be supplied with `--candidate-audit PATH`.
CI fetches only the exact baseline commit and reuses its prior current audit;
uploads short three WAVs, metrics and note histories. Original baseline metrics
remain frozen in `SD19_ROOM_METRICS.json`; current audit is separately saved in
`SD19_ROOM_CALIBRATED_AUDIT.json`, A/B in `SD19_ROOM_CALIBRATION_METRICS.json`.

Spec0.8 explicitly records the changed return curve at the same SCN7 normalized
Room position. Default position0.5, nominal decay, source send and memory schema
are retained; the Product candidate now sounds with more return at that position.
Reference remains CMake default, JournalOFF. SD19 hearing and final room choice
remain open; same TODO24 closed/30 open. Next independent preparation is SD28
matched Nature on/off with the same musical performance, while final Nature
selection still depends on the musical source/room hearing decision.
