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
Preset-Pack-Klang · **stationäres Dauerrauschen als „Atmosphäre"**
(r18.97: Wind/Wellen/Regen sind EREIGNISSE mit Flauten und Pausen —
gefiltertes Rauschen, das nie aufhört, ist ein Teppich, kein Wetter).

## 3. Instrumentierung (drei Stimmen, nicht zwanzig)

| Stimme | Modul | Rolle | Register |
|---|---|---|---|
| **Pad-Bett** | pad.c + padsynth.c (Spektraltisch, Nasca-Modell) | der Raum, die Harmonie | ~MIDI 50–78 |
| **Melodie** | pluck.c (KS-Saite) **oder** glass.c (2-op-FM, r18.98) → body.c (Modalkörper pro Welt) | die Erzählstimme über dem Bett — VOICE-Slot wählt String/Glass; bei String/Glass schlägt auch jeder Cell-Press sie an | ~MIDI 64–90 |
| **Bass-Fundament** | bass.c (Sub + Deep) | Boden, folgt der tiefsten Note | −1/−2 Okt. unter Root |

VOICE ist eine WAHL, kein Layer: eine Melodiestimme klingt zur Zeit
(Pad-Default = Referenzklang, Cells rein als Swell). KEY (12 Tonarten im
Menü, r18.98) transponiert im Register MIDI 54–65 — nie Oktavsprünge.

Dazu Nicht-Ton-Schichten: texture.c (Brown+Pink-Atem), ambience.c
(Welt-Atmosphäre), tape.c (Hiss + Sättigung + Vinyl-Crackle), drone.c
(Tonart-Pedal). **Keine neue Stimme ohne Streichung einer alten.**

## 4. Harmonische Sprache — HARMONIC SAFETY CORE (r19.0, bindend)

Der Autoplay-Composer denkt NICHT in Chord Progressions, sondern in einer
**Pitch World** mit einem Sicherheitskern (harmony.c). Reihenfolge ist
Gesetz — **Quality Gate zuerst, Randomness zuletzt**:

1. **PITCH WORLD:** Pentatonik-CORE (Dur `C D E G A` · Moll `D F G A C`) —
   strukturell KEIN Halbton, KEIN Tritonus im Set. + EINE Color-Note
   (maj7 / 9) **nur oberhalb C4**.
2. **REGISTER:** unter C3 nur Root/Quinte/Oktave; Terzen im Mittenband;
   2nds/9ths/Color nur hoch. Bass 38–49 · Stimmen 55–79 · Melodie 62–86.
3. **MUTATION statt Neu-Würfeln:** ein Zustandswechsel behält **≥3
   gemeinsame Pitch-Classes und bewegt ≤2 Stimmen**; gemeinsame Töne
   bleiben auf DERSELBEN Tonhöhe (parsimonious voice leading).
4. **COLLISION-FILTER vor jedem Melodie-Ton:** gegen jede klingende Stimme
   Halbton (1/11) + Tritonus (6) verboten, tiefe 2nds verboten → next-best.
5. Wahrscheinlichkeit ganz zuletzt.

**Verboten:** einen Akkord komplett neu würfeln · einen Halbton oder
Tritonus zwischen gleichzeitig klingenden Stimmen · Color-Note tief · dichte
Cluster unter C3. Messlatte: 0 % Halbtöne / 0 % Tritoni zwischen sounding
voices (r19.0 über 80 000 Intervalle bestätigt). brain.c bleibt für die
Live-Cell-Tonhöhen; **kein Modul erfindet eigene Skalen.**

## 5. Bewegungs-Sprache (Motion)

- Bewegung = **Drift, nie Wobble**: alle LFOs < 0,15 Hz, Raten pro Seite/
  Stimme deliberately inkommensurabel (0.052/0.061/0.087/0.113 Hz …).
- Juno-Prinzip (übertragen, nicht kopiert): Lebendigkeit kommt aus
  **Phasen-Drift zwischen Fast-Unisono-Schichten** (r18.90 Breathing-Detune
  ±1,8 Cent), nicht aus großem statischem Detune.
- Zufall nur als Random-Walk mit Zeitkonstante (Drone-Drift τ=18 s), nie
  als Sample-und-Halt-Springen.

## 6. Melodie-Grammatik (die lange generative Stimme, r19.0)

EINE lange Voice (kein Arpeggiator), gespeist aus dem Safety Core (§4).
Zufall ist IMMER eingesperrt in diese Regeln (engine.c Tick + harmony.c):

| Regel | Wert |
|---|---|
| Tonlänge | 4–16 s pro Note |
| Stille | 1–8 s, + Atem 3–8 s nach jeder Phrase (Stille ist Komposition) |
| Phrasenlänge | 2–5 Noten |
| Tonvorrat | Pitch World §1 im Melodie-Register 62–86 |
| Bewegung | repeat > Schritt > Quart/Quint > Sext/Oktave > Color (Tabelle) |
| Kein Leap | > Oktave (per Register + Oktav-Fold) |
| Collision | jeder Ton gegen alle klingenden Stimmen gefiltert (§4.4) |
| Déjà-vu | 35 % Replay der letzten Phrase, jeder Ton erneut durch World + Filter |
| Onset | zusätzlicher VOICE-Anschlag (String/Glass) vor dem Pad-Swell |

**Blendwave (Liven Ambient Ø, gelernt):** ein gehaltener Ton lebt durch
korrelierten Timbre-Walk (Pad-Brightness-Tilt, kleine Schritte alle 400 ms)
— „same note, evolve timbre", nicht durch neue Events.
| Dynamik | Phrasen-Opener leicht lauter (0.075 vs 0.05–0.065) |
| Timing | 20–60 % der Bar, humanisiert |

**Verboten:** Arpeggiator-Muster, chromatische Töne, Intervalle > Oktave,
mehr als ein neuer Ton pro Bar, Melodie während der User spielt.

### 6b. Composer-Ebene (r18.96 — Atmoscapia/Eno-Prinzip)

Über der Grammatik läuft ein Composer, der über Minuten Zustände wechselt
und dabei **nur Wahrscheinlichkeiten** ändert — nie Noten setzt, nie den
Audio-Pfad berührt. Zyklus CALM → OPEN → DEEP → EMPTY → RETURN, je
40–80 s (humanisiert):

| State | mel_density | rest_add | high_p | bed_amp | bass_depth |
|---|---|---|---|---|---|
| CALM   | 0.70 | +0.10 | 0.04 | 1.00 | 0.50 |
| OPEN   | 1.30 | −0.10 | 0.15 | 1.05 | 0.40 |
| DEEP   | 0.45 | +0.20 | 0.02 | 0.90 | 0.85 |
| EMPTY  | 0.15 | +0.45 | 0.00 | 0.60 | 0.30 |
| RETURN | 1.00 |  0.00 | 0.08 | 1.00 | 0.55 |

Der High-Response (+1 Okt.) ist eine EIGENE antwortende Stimme — die
Melodielinie führt am Basiston weiter (Leap-Regel bleibt hart). EMPTY ist
nicht Stille: das Bett hält bei 0.6×, Texture/Atmos unberührt — der
angehaltene Atem, der RETURN warm macht.

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
