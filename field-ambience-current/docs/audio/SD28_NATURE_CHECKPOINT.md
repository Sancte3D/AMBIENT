# SD28 — optional Nature against actual music, 11 October 2026

Three short comparisons expose the existing optional Nature under held ensembles
and in later real musical passages. The same notes, owners and phrase histories
run with Nature off/on. This prepares the hearing decision; it does not choose
Nature for any World or certify masking, calmness or geographic identity.

The sound is exactly the green PR153 candidate:
`e34bcf372637c6e3db239849550d7bd4bb015bdc`, tree
`f43cda115e55bca5e2652b71160bf1685be1c6d9`. Its complete GitHub workflow
38092364185/run681 passed. No firmware core, header, CMake, source articulation,
generator, owner, Room or schema is changed here. Spec0.8, Reference default,
Product candidate and JournalOFF remain. Nature still defaults to zero.

## What to hear

Each new WAV is 27.5 s, stereo PCM16/44100 Hz, 1,212,750 frames / 4,851,044 bytes:

- `AMBIENT_COAST_Nature_AB_27p5s.wav`
- `AMBIENT_WOODLAND_Nature_AB_27p5s.wav`
- `AMBIENT_HIGHLANDS_Nature_AB_27p5s.wav`

| Delivered section | Same actual excerpt with |
|---|---|
| 0–6.5 s | Held ensemble, Nature0 |
| 7–13.5 s | Same held ensemble, Nature0.35 |
| 14–20.5 s | Later real passage, Nature0 |
| 21–27.5 s | Same later passage, Nature0.35 |

Three 0.5 s editorial gaps and 40 ms cut-edge fades. The cuts are not continuous
Nature changes. Held source excerpt is 6.0–12.5 s in every World; later excerpts
are selected from the actual source-state log and unchanged Nature-off music.

| World | Later source excerpt | Actual source-free content |
|---|---|---|
| COAST | 51.69–58.19 s | 0.01 s; real quiet transition, not a full source pause |
| WOODLAND | 40.32–46.82 s | 4.49 s; release into the real source pause |
| HIGHLANDS | 29.38–35.88 s | Entire 6.5 s source pause; Nature-off PCM is exactly zero |

COAST retains its connected outer/common-tone movement. Its longest logged free
interval before Stop is only 58.17–58.18 s. WOODLAND's longest free interval is
42.12–46.61 s, shorter than the comparison cut. Their excerpts therefore use
the quietest actual later 6.5 s musical window instead of inventing a pause.
HIGHLANDS has a complete free interval 29.38–43.23 s; its cut lies inside it.
Source-free does not generally imply an empty Room; exact silence is measured.

Real Product Engine, D-major/Equal, seed1234, Activity/Color/Attack/Release0.5,
Volume0.6, calibrated Room0.5; 1 s truly silent settling with default Nature0.
Nature is enabled at the beginning of Generate, not mixed in afterwards.
Full performance 112 s, GenerateStop80 s, natural source/Nature/Room releases.
No direct-source score, altered hold, substitute noise or artificial raw ending.

The existing positive-Nature candidate contains wind in all three Worlds,
COAST waves and WOODLAND short unpitched drops. HIGHLANDS currently has wind
only. This common wind component is explicitly unaccepted; the musical World
must stand alone before optional location noise is retained. Amount0.35 is one
review value, not a new default, control mapping or final Nature calibration.

## Fair gain and measured output

ONE identical scalar per World across off/on, held/quiet, chosen from the held
Nature-off excerpt towards −23 LUFS subject to −6 dBFS true-peak reserve.
No compression, AGC, independent pair normalization or pause amplification.

| World | Fixed gain | Whole-file LUFS | Whole-file true peak | Held Nature difference / music RMS |
|---|---:|---:|---:|---:|
| COAST | +13.4 dB | −22.8 | −8.3 dBFS | 2.617% |
| WOODLAND | +15.4 dB | −23.5 | −13.4 dBFS | 2.977% |
| HIGHLANDS | +8.9 dB | −23.1 | −13.6 dBFS | 1.384% |

Held sections measure −23 LUFS with and without Nature. The residual is a
quantized, master-filtered PCM difference, not a wet energy fraction or a
perceptual masking test. It remains deliberately quiet. Residual RMS under
held notes is −69.92/−70.71/−71.20 dBFS before audition gain. In the later
sections it is −70.70/−76.24/−73.11 dBFS. HIGHLANDS' on-pause after the same
gain measures −61.8 LUFS / −51.0 dBFS true peak; its off-pause is exact zero,
recorded as such without inventing a finite loudness for silence.

Windowed residual spectral band fractions and mono/DC are diagnostic values
in the metrics, not proof that the noise is pleasant or distinct. Most energy
is 100–1000 Hz; below100 Hz is approximately 12.4–16.0% in these six excerpts.
No numeric gate is claimed for wind homogeneity or subjective rumble.

## Focused complete verification

18 full actual 112 s cases: three Worlds × seeds1234/42 × Nature0/0.35/1.
Every case is rendered at64 and512 blocks on the same10 ms Main grid:
36 full captures, 67.2 min actual musical PCM. Each complete PCM, note-on/off
CSV and source-count/phrase/rejection CSV is byte-identical across the two
partitions. Within each World/seed, all three Nature amounts have exactly the
same musical note and owner/phrase/state histories. No broader control-rate
or arbitrary cross-compiler byte identity is claimed.

| World | Actual starts, both seeds | Maximum sources | Final source end, seeds1234/42 |
|---|---:|---:|---|
| COAST | 13 | 3 | 88.71 / 88.84 s |
| WOODLAND | 11 | 2 | 82.00 / 82.00 s |
| HIGHLANDS | 11 | 3 | 87.07 / 87.18 s |

Stop80 s prevents new plans; final second111–112 s is exactly zero in all18
cases, with no Clear/final fade. NaN/limiter counters stay0. Raw true peaks
are at most −20.0 dBFS; minimum full-performance mono energy ratio0.9307985,
DC below0.0002. These are the stated seeds and normal macros plus maximum
Nature, not an exhaustive cartesian test or a device-output measurement.

Three additional Nature-only actual Engine cases, no phantom voice, at each
Room0/block512, Room0/block64 and Room1/block512: nine complete32 s captures,
4.8 min. PCM and note/state histories match across those conditions. The
optional location output is independent of Room, even at maximum Room.
Positive Nature works without source owners; Init's Nature0 is exactly silent.
The existing real `test_room_nature.c` also passes on this unchanged core:
impulse/decay/mono, Dry revival prevention, seed/block/invalid-input invariance,
cold idle, score independence and actual Clear.

Host driver builds with C11/O2/Wall/Wextra/Werror. It checks stream errors,
flush, sync and close. All full captures are verified in temporary storage;
only final short WAVs, metrics and complete note/state CSVs are exported.
Full byte counts, PCM shape and hashes are checked again before successful
completion. Earlier incomplete scratch captures were discarded; no cause for
their truncation is inferred. Silence/score/headroom assertions were retained.

## Firmware and device evidence

Exact PR153 source/header/tool fingerprints are checked before rendering.
Git diff also contains no firmware compilation inputs. The prior full host
suite and real H743 build are covered by green PR153 CI and its committed
`SD19_ROOM_CALIBRATION_ARM_METRICS.json`. No new ARM build or full host runner
is claimed for this tools/documents-only checkpoint. Firmware remains Flash
195928 B, D1 RAM65664 B, D2 RAM60808 B, reservedDTCM stack16384 B; largest
individual core compiler frame192 B. Real CPU/ISR/nested-stack high-water,
DAC/amp/speaker and storage acceptance still require the device.

Reproduce from firmware-c-next with the exact PR153 object available in Git:

```sh
python3 tools/review_nature_comparison.py build-nature-comparison
```

CI fetches the exact baseline commit and uploads the three short WAVs, JSON
and complete CSVs; full PCM is verified and discarded, not uploaded.
Evidence is saved as [SD28_NATURE_METRICS.json](SD28_NATURE_METRICS.json).
Same TODO24 closed/30 open; SD28 remains unchecked. The final source/Room
hearing decision remains a prerequisite for Nature selection or removal.
Next independent preparation: SD33 Activity min/default/max in actual Worlds.
