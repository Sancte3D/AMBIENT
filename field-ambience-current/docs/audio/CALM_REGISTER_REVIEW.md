# AMBIENT — calming register, 2026-09-22

## KERNURTEIL

AMBIENT already has long notes, rests and a collision-aware harmonic system.
Its autonomous lead nevertheless occupied MIDI 62..86 (D4..D6) and opened
near the highest accompaniment voice. That is a poor default for the user's
explicitly calming instrument. This package moves the foreground into the
landscape; it does not certify every sound as pleasant.

## FUNDAMENTAL FALSCH

For this product, an attention-grabbing alarm, siren or exposed electronic
beep is a failed sound, even when perfectly in tune. This is a product choice,
not a claim that all ambient music must be low or cannot contain bright tones.
Do not fix it solely by lowering playback volume or hiding it under reverb.

The source of the measured register problem is `src/harmony.c`, not a World
label or export transpose. Its automatic melody now uses **MIDI 50..69
(D3..A4)**. Openings start near the World's tonic around A3 instead of the
top accompaniment voice. Candidate generation, modal colour and anti-repeat
fallback share the same band; collisions are checked at the actual new pitch.
No post-filter octave shift that could introduce low-register clashes.

## NOCH NICHT SELBSTVERSTÄNDLICH

- This changes the autonomous melodic foreground only. Accompaniment,
  manually played Characters, resonances and effect-generated upper partials
  can still be higher. A lead-note ceiling is not a spectral low-pass.
- Bowed vibrato is approximately ±6 cents at 5.1 Hz; Choir approximately
  ±4 cents at 4.6 Hz. These are code facts, not proof of a siren. Their overlap
  with tape, chorus and moving filters still needs controlled comparison.
- Chorus moves two delay heads with 5.5/6.5 ms maximum modulation depth;
  Motion also changes other systems. Review perceived pitch stability and
  dry-signal cancellation across default/max values, not just a preset number.
- Horn formants, sympathetic resonators, native FM/Orbit upper partials and
  transient attacks still need individual review. Lower pitch can expose low
  midrange masking; it cannot substitute for balancing these systems.
- Shimmer's tiny measured default contribution remains unexplained. The new
  calming requirement rules out blindly increasing bright octave feedback.

## LOCKED

As principles: calm before spectacle; soft onset and release; stable pitch
centre; space between events; distinct Worlds within one restrained dynamic
range. Preserve harmonic collision checks and prefer a rest to an unsafe tone.
No specific timbre, effect amount or claim of relaxation is locked by tests.

## REMOVE / MERGE / REDESIGN

Remove the high lead register and its top-voice opening bias. Keep existing
source envelopes, World presets and effects for this isolated comparison.
No new UI, modes, audio buffers or state. Host `.data`/`.bss` unchanged;
host text is 216 bytes smaller. Actual H743 map, stack and DWT remain open.

Verification:

- Extended real harmony regression across 12 keys × 6 modes × 240 decisions,
  alternating full/no modal-colour probability, with voiced accompaniment.
  Bounds, collision safety, leap size, anti-repeat movement and lack of
  starvation are checked. A fully blocked register must return silence.
- Regression actually run against old source: 17,512 failures; new source:
  71,831 checks, zero failures. These are behavior checks, not constants only.
- Existing long scheduler simulation now checks the tighter 50..69 band;
  repeats, rests, phrase replay and bounded leaps retain their tests.
- Full `bash test/run_tests.sh` passed: device 15,183 checks and FX 478,860
  checks, zero failures; existing battery-test printf warnings remain.

`tools/review_calm_register.py --before baseline.so --after candidate.so
--output /tmp/calm-register` runs real Generate for 24 seconds plus four
seconds after stop, fresh process per World/version, same seed `0x5EEDBA55`.
No scripted melody, no extra synthesis. First observed melody at 21.408 s:

| World | Previous MIDI | New MIDI |
|---|---:|---:|
| Alps | 76 (E5) | 55 (G3) |
| Open Sea | 71 (B4) | 62 (D4) |
| Fjords | 76 (E5) | 54 (F#3) |
| Moss Fields | 70 (Bb4) | 60 (C4) |
| Desert | No melody in this window | No melody in this window |

Desert's silence is valid phrasing, not register coverage. Full World phrase
tests and the all-mode harmony regression cover behavior beyond this short
audio window. The audio is technically analyzed, not subjectively approved.
Raw metrics/events: `CALM_REGISTER_METRICS.json`.

## BESTE VERSION

A warm body that stays present without demanding attention. Gentle upper
detail adds distance, not a separate lead. Motion changes texture before it
sounds like a repeating pitch sweep; the room supports the note rather than
being required to make it tolerable. Natural layers have irregular lulls.

Next small packages, in order:
1. Open Sea Chorus/Blur and combined pitch movement; compare removing stages.
2. Horn and Bowed resonances/vibrato, then Choir, at the actual new registers.
3. Native Character upper partials/attacks and min/default/max parameter ranges.
4. Natural-layer whistles/periodicity, then full World balance/mono/transitions.

## TEST AM GERÄT

27.5-second comparison: Alps old at 0 s, new at 7 s; Open Sea old at 14 s,
new at 21 s. Each excerpt is 6.5 seconds from the same 20.5..27 s performance
window; 0.5-second gaps. Fixed loudness matching per excerpt, no compressor,
limiter or moving gain, 150 ms edge fades. Fixed gains +3.5 / +5.0 / +8.3 /
+8.4 dB respectively; each excerpt targets −25 LUFS. Combined export −25.1
LUFS and −12.0 dBTP. All ten raw renders have zero clipped PCM samples.

At equal comfortable playback level on headphones and device speakers:
does the foreground sit inside the bed, is its body audible without boom,
does an onset attract attention like a notification, and does the pitch
periodically sweep like a siren? Reject any such instance and isolate the
responsible source/FX. Compare mono too. Several separate <=30 s excerpts
are needed for later phases; one short sample cannot prove “never alarming”.
