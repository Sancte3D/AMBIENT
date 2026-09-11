# SOUND_WORLD.md — die klangliche Verfassung (v2, 2026-09-09)

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

## 3. Rollen und Klangkerne

Drei musikalische Rollen: **Bett**, **Erzählstimme**, **Fundament**.

| Rolle | Umsetzung | Vertrag |
|---|---|---|
| Bett | Pad/PADsynth, Basiston und drei langsame Eno-Loops | trägt Harmonie; kein ständig angeschlagener Akkord |
| Erzählstimme | String, Glass, Bowed, Horn, Choir oder Guembri | VOICE wählt Charakter; kein Stapeln aller Instrumente |
| Fundament | Bass und optionales Tonart-Pedal | Tiefe, keine konkurrierende Basslinie |

Bowed/Horn/Choir behalten je drei Stimmen. Beim Ersetzen blendet die alte
Stimme 8 ms aus; dann übernimmt eine vorbereitete neue Stimme. Gehaltene
Töne gehören ihren Tasten; Loslassen beendet nur die zugehörige Stimme.

CHARACTER wählt alternativ Dusk, Glimmer, Mist, Tide, Horizon oder Dew. Diese
sechs Kerne sind monophon und teilen Master und Effektraum mit Ambient.
Autoplay gehört zu Ambient. Keine neue Klangschicht ohne begründetes Rollen-
und Ressourcenbudget.

Die Namenssprache beschreibt Orte und Atmosphären: WORLD gibt den Ort vor,
CHARACTER dessen klangliche Färbung. SOUND_NAMES.md in docs/audio definiert
Bild und Klangziel je Name. Synthesemethoden gehören in die technische
Dokumentation; ein schöner Name ersetzt keine passende Klanggestaltung.

## 4. Harmonische Sprache

**Tonvorrat → Register → klingender Kontext → Stimmführung → Wahrscheinlichkeit.**

- Live-Spiel und Generator teilen die sechs Modi aus `pitch_modes.h`.
- Das Bett verwendet einen Dur-/Moll-Pentatonikkern. Modalfarben gehören
  sparsam nach oben: Ionian maj7, Dorian 6, Phrygian b2, Lydian #4,
  Mixolydian b7, Aeolian b6.
- Neue automatische Bett-, Eno- und Melodietöne prüfen gehaltene Töne,
  Bass/Pedal und konservativ gespeicherte Ausklänge. Halbton- und
  Tritonusklassen sowie enge Sekunden unter C4 sind ausgeschlossen.
  Weite Nonen werden nicht allein wegen eines tiefen Grundtons blockiert.
- Harmoniewechsel bewahren gemeinsame Töne und bewegen wenige Stimmen.
  Tonart-/Moduswechsel releasen alte generative Stimmen und planen neu;
  ihre Ausklänge bleiben geschützt.
- Ohne passenden Ton folgt eine Pause. Modalfarbe wird nie erzwungen.
  Vom Menschen gespielte Töne werden nicht vom Kollisionsfilter umgeschrieben.

Equal oder tonikabezogene 5-Limit-Just-Intonation: Frequenzen bleiben auch in
V2 als gebrochene MIDI-Tonhöhen erhalten. Just verzichtet auf zufälligen
Pitch-Versatz beim Anschlag. Absichtliches Unisono, Chorus und Vibrato bleiben
Klangmerkmale; reine Grundstimmung bedeutet nicht völlige Schwebungsfreiheit.

## 5. Bewegung

Bewegung soll als langsame Drift wirken. Globale Filterbewegung und Horizon/
Tide-Morphing bleiben unter 0,15 Hz. Kleine Ensemble-Verzögerungsmodulation
und verzögert einsetzendes Instrument-Vibrato sind eigene Charaktermerkmale.
Wind und Noise nutzen unregelmäßige Druck- und Turbulenzverläufe ohne feste
Filtersweeps oder Pfeifresonatoren. Rauschhöhen folgen den Flauten.
Zufallsbewegung bleibt begrenzt und korreliert. Ein gehaltener Ton darf sich
entwickeln, ohne dafür neue Noten auszulösen.

## 6. Phrasen und Composer

- Eine Melodielinie: 4–16 s pro Ton, echte Pausen und Atem zwischen Phrasen.
- 2–5 Töne pro Phrase; Sprünge maximal eine Oktave, auch bei Motiv-Replay.
- Nach zwei gleichen Melodietönen: sicherer anderer Ton oder Pause.
  Eintönige Phrasen werden nicht als wiederholbare Motive archiviert.
- Alle automatischen Schichten teilen mindestens 1,4 s Einsatzabstand.
- Physisches Spiel pausiert neue Einsätze; sanfte Rückkehr nach etwa 8 s.
  Geplante Releases laufen währenddessen weiter.

CALM, OPEN, DEEP, EMPTY und RETURN bilden einen gewichteten Graphen.
Klangbelegung, kürzlich besuchte Zustände und lange nicht besuchte Ziele
beeinflussen die Wahl. Spätestens beim sechsten Wechsel seit dem letzten
Atem kommt EMPTY; EMPTY führt über RETURN zurück.

| Zustand | Dichte | zusätzliche Pause | Farbtendenz | Bettpegel | Basstiefe |
|---|---:|---:|---:|---:|---:|
| CALM | 0,70 | +0,10 | 0,04 | 1,00 | 0,50 |
| OPEN | 1,30 | −0,10 | 0,15 | 1,05 | 0,40 |
| DEEP | 0,45 | +0,20 | 0,02 | 0,90 | 0,85 |
| EMPTY | 0,15 | +0,45 | 0,00 | 0,60 | 0,30 |
| RETURN | 1,00 | 0,00 | 0,08 | 1,00 | 0,55 |

Ziele gleiten etwa vier Sekunden. Der Bettpegel verändert auch gehaltene
Noten. EMPTY lässt Eno-Stimmen los und setzt sie aus; nachfolgende Einsätze
bleiben gestaffelt. Raum und Restklang dürfen weiter atmen.

## 7. Gemeinsamer Raum

Ein zentraler Effektpfad mit getrennten Dry-/Send-Bussen verbindet die Stimmen.
Sends bestimmen die Entfernung; Master und Mute greifen nach dem Raum.
**Klangkerne → Drive → FX → DC-/Hochpass → Master → Soft-Limit**.
Alle fünf Weltcharaktere haben eigene Raumparameter; Desert ist enger und näher.
BLUR verwischt zeitlich bei unveränderter Leserate; es transponiert keine
zusätzlichen Tonhöhen in die Harmonie. Wave-Morphs erhalten den Grundton
durch angeglichene Phasen.
Hiss, Shimmer und Sättigung bleiben dosierte Färbungen. Kein dauernder
Shimmer-Teppich oder übermäßiger Subbass.

## 8. Regler

BRIGHTNESS verändert den vorhandenen Klangfilter, RESONANCE dessen Betonung,
SWEEP eine langsame Öffnung und ENVMOD die Öffnung durch die Klanghüllkurve.
Diese Makros funktionieren in Ambient und allen sechs V2-Kernen. V2-Makros
werden 80 ms geglättet und nutzen vorhandene Filter; keine zusätzlichen
Filterketten. Die sechs eigenen Kernparameter bleiben erhalten.
Attack/Release skalieren die natürliche Hüllkurve neuer Noten: Dew bleibt
LPG-Schlag, Mist bleibt Pad. SPACE/ECHO/MOTION/AGE/BLUR/SHIMMER gestalten den
Raum. Reglerrichtung verständlich halten, Instrumentcharakter erhalten.

## 9. Technik und Abnahme

Kein Heap/Blocking im Audiopfad; feste Stimmen-/Speicherbudgets. Live-Werte
glätten, Transzendentale nur bei Initialisierung/Kontrollrate, kein heißer
Puffer in PSRAM. `REALTIME_AUDIO_RULES.md` bleibt bindend. Reproduzierbare
Seeds und hörbare Verhaltensregressionen gehören zur Entwicklung.

Pitch-Memory schätzt Ausklänge zeitlich, analysiert den Hall nicht spektral.
Host-Tests und Firmware-Render beweisen keine endgültige Klanggüte.
DWT-Spitzenlast <60 %, Ausgänge und Lautsprecher/Kopfhörer müssen auf dem
H743 gemessen und gehört werden. Hörentscheidungen bleiben Teil der Abnahme.

## 10. Referenz-Lernregel

Prinzipien übernehmen: Eno — asynchrone Wiederkehr; Marbles — begrenzte
Variation mit Gedächtnis; Juno — Ensemble durch kleine Unterschiede;
OP-1 — wenige sinnvolle Regler; PADsynth — spektrale Fläche. Keine fremden
Samples oder Markenklänge kopieren.
