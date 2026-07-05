# SOUND_WORLD.md — die klangliche Verfassung (v1, r18.90)

Bindend für jede Änderung an `firmware-c-next/src/` die Klang erzeugt.
Wie `AI_READY_SCHEMATIC_STANDARD.md` für die Hardware: erst gegen dieses
Dokument prüfen, dann coden. Kurz gehalten — Regeln, keine Prosa.

## 1. Identität (ein Satz)

**Ein warmes, langsames, kinoartiges Ambient-Instrument, das nach einer
Erinnerung klingt — nie nach einem Preset.**

Ruhig · warm · tief · melancholisch (nicht traurig) · cinematisch (nicht
Trailer) · dunkel (nicht Horror) · futuristisch (nicht Sci-Fi-Klischee) ·
organisch (keine Fake-Natur) · imperfekt (nicht kaputt) · generativ (nicht
zufällig).

## 2. Verbotene Ergebnisse (hart)

Meditations-Musik-Generik · AI-Ambient-Brei · Horror-Drones · Random-Noten-
Suppe · Dauer-Shimmer · Supersaw-Billigpads · offensichtliches LFO-Wobble ·
„endless rain + pad" · grelle Digital-Bells · Fake-Booms · matschige
Low-Mids · Sub-Bass-Übermaß · Granular-Artefakte ohne Absicht ·
Preset-Pack-Klang.

## 3. Instrumentierung (drei Stimmen, nicht zwanzig)

| Stimme | Modul | Rolle | Register |
|---|---|---|---|
| **Pad-Bett** | pad.c + padsynth.c (Spektraltisch, Nasca-Modell) | der Raum, die Harmonie | ~MIDI 50–78 |
| **Pluck-Melodie** | pluck.c (Karplus-Strong) | die Erzählstimme über dem Bett | ~MIDI 64–90 |
| **Bass-Fundament** | bass.c (Sub + Deep) | Boden, folgt der tiefsten Note | −1/−2 Okt. unter Root |

Dazu Nicht-Ton-Schichten: texture.c (Brown+Pink-Atem), ambience.c
(Welt-Atmosphäre), tape.c (Hiss + Sättigung + Vinyl-Crackle), drone.c
(Tonart-Pedal). **Keine neue Stimme ohne Streichung einer alten.**

## 4. Harmonische Sprache

- 6 Kirchenmodi, 4 Akkordfamilien (add9/maj7/min11/sus2) — brain.c ist die
  einzige Quelle von Tonhöhen. **Kein Modul erfindet eigene Skalen.**
- Harmoniewechsel sind LANGSAM: 8-s-Bars ±10 %, Markov-gewichtete
  Stufenübergänge (generative.c) — nie mehr als eine Stufe pro Bar.
- Voice-Centering um MIDI 64 — alles bleibt im warmen Mittenband.

## 5. Bewegungs-Sprache (Motion)

- Bewegung = **Drift, nie Wobble**: alle LFOs < 0,15 Hz, Raten pro Seite/
  Stimme deliberately inkommensurabel (0.052/0.061/0.087/0.113 Hz …).
- Juno-Prinzip (übertragen, nicht kopiert): Lebendigkeit kommt aus
  **Phasen-Drift zwischen Fast-Unisono-Schichten** (r18.90 Breathing-Detune
  ±1,8 Cent), nicht aus großem statischem Detune.
- Zufall nur als Random-Walk mit Zeitkonstante (Drone-Drift τ=18 s), nie
  als Sample-und-Halt-Springen.

## 6. Melodie-Grammatik (generative Stimme)

Zufall ist IMMER eingesperrt in diese Regeln (engine.c Tick):

| Regel | Wert |
|---|---|
| Phrasenlänge | 2–4 Bars |
| Pausen-Phrasen | 30 % (Stille ist Komposition) |
| Noten pro Bar | max. 1 (Opener 85 %, sonst 55 %) |
| Tonvorrat | obere Akkordtöne der aktuellen Stufe, +1 Okt. |
| Wiederholung | 35 % (erwünscht!) |
| Schrittweite | nächster Akkordton (85 %), zweitnächster (15 %) |
| Oktav-Antwort | 18 %, gleicher Ton −12, nie neue Tonklasse |
| Déjà-vu | 40 % der Phrasen = Replay der letzten (1 Note variiert 30 %), re-fitted auf aktuelle Harmonie |
| Dynamik | Phrasen-Opener leicht lauter (0.075 vs 0.05–0.065) |
| Timing | 20–60 % der Bar, humanisiert |

**Verboten:** Arpeggiator-Muster, chromatische Töne, Intervalle > Oktave,
mehr als ein neuer Ton pro Bar, Melodie während der User spielt.

## 7. Raum & Imperfektion

- Ein Hall für alles (Sends 0.35–0.55) — der Raum ist Teil des Instruments,
  kein Effekt danach. Plucks blühen mit 0.5 hinein.
- Imperfektionen sind DOSIERT und GEALTERT über das Age-Makro: Hiss
  (−46 dB…), Vinyl-Ticks (2,6-kHz-Resonator, age²), tanh-Sättigung.
  Nie „kaputt", nie Bitcrush.
- Master-Kette fix: Drive → DC-Block → Volume → Tape-Sättigung →
  Soft-Limit. Reihenfolge ist Teil des Klangs — nicht umsortieren.

## 8. Makro-Regeln (Bedienung = Emotion, nicht DSP)

| Control | Emotion | interne Ziele |
|---|---|---|
| DRIVE | „Wärme/Dichte" | Master-Sättiger + Reverb-Input-Drive (geslavt) |
| BRIGHTNESS | „Licht" | Pad-Cutoff + Hall-Dämpfung + Pluck-Dämpfung |
| SPACE | „Raumgröße" | Reverb size/decay/wet (Preset-Kurve) |
| AGE | „Alter" | Hiss + Sättigung + Vinyl-Crackle (age²) |
| MOTION | „Lebendigkeit" | Filter-LFO-Tiefe + Ensemble-Drift-Tiefe |

Regel: Ein Encoder bewegt **mindestens zwei, höchstens vier** Ziele, alle
in dieselbe emotionale Richtung; bei Mittelstellung/0 exakt der
bench-getunte Referenzklang.

## 9. Technische Verfassung (Echtzeit)

Kein Heap im Audio-Pfad · keine Blocking-Ops · transzendente Funktionen
nur at control-rate oder als LUT (dsp_sin) · Parameter geglättet (≥80 ms)
· Feedback geklemmt (rho<1, SVF-Clamps 80–8000 Hz) · Denormals: FTZ auf
dem M7 (FPSCR+FPDSCR Bit 24) · fixe LCG-Seeds → jede Klangentscheidung
ist im Host-Test bit-reproduzierbar · jede neue Klangeinheit kommt mit
Test (Statistik/Autokorrelation, nicht nur „läuft").

## 10. Referenz-Lernregel

Von Legenden **Prinzipien** extrahieren (Juno = Phasen-Drift; OP-1 =
begrenzte, immer musikalische Makros; Eno = Random-Walk statt Zufall;
Lexicon/Dattorro = Hall als Instrument; **Nasca/PADsynth = Partialtöne
als Rauschbänder, nicht als Linien; Gillet/Marbles = Zufall, der sich
erinnern kann**) — nie Schaltungen, Samples oder Markenklänge nachbauen. Jede Übernahme wird im Code-Kommentar als
„Prinzip X, gelernt aus Y, hier neu interpretiert als Z" dokumentiert.
