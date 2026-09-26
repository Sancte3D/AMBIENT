# Bowed — stabiler Ton, zurückhaltende Textur, 2026-09-26

## KERNURTEIL

Die dominante Hauptsaite trägt bereits zuverlässig. Darüber lagen ein
regelmäßiges gemeinsames 5,1-Hz-Vibrato und permanentes Bogenrauschen.
Dieses Paket reduziert beide Mechanismen, ohne Grundton oder Hüllkurve
abzusenken. Gemessen, noch nicht subjektiv hörend freigegeben.

## FUNDAMENTAL FALSCH

Kein neues grundsätzliches Konzeptproblem nachgewiesen. Die Herkunft aus
Streichinstrumenten rechtfertigt für sich kein Vibrato in jeder Naturwelt.
Ob der verbleibende Sägezahn-/Resonanzcharakter die passende Identität trägt,
ist mit dieser Korrektur noch nicht entschieden.

## NOCH NICHT SELBSTVERSTÄNDLICH

Die zweite Saite bleibt um rund sieben Cent verstimmt; das erzeugt weiterhin
leichte Schwebung. Die Körperbewegung bei 0,13 Hz und die beiden Resonatoren
bleiben erhalten. Ihre Gesamtwirkung mit Pad, Raum und Natur ist offen.
Open Sea/Fjords werden hier als zwei trockene Source-Farben verglichen,
nicht als komplette Generate-Worlds.

## LOCKED

Bisherige 85/15-Saitenbalance, gehaltene Noten und weiche Stimmenübergaben.
Bestehender Grundton-Stabilitätstest bleibt grün. Kein neuer UI-Modus,
keine Änderung von Voice-IDs, Attack, Release oder Effektparametern.

## REMOVE / MERGE / REDESIGN

- Gemeinsames 5,1-Hz-Pitch-LFO samt Einblendung/Verzögerung entfernt.
- Bogenrauschgain `(0.06 + 0.20*bow)` → `(0.015 + 0.05*bow)`.
  12,04 dB weniger Rauschamplitude, ohne die Klangquelle stummzuschalten.
- Ein Sinus-LUT-Aufruf und Pitchmodulationsarbeit pro aktiver Stimme/Sample
  weniger; keine neuen Puffer. Host-Binary: Text -272 Byte, BSS -64 Byte,
  Data unverändert. Keine Aussage über H743-Deadline oder finales RAM-Mapping.

### Nachweis

`tools/review_bowed_motion.py --before baseline.so --after candidate.so
--output /tmp/bowed-calm` untersucht real gerenderte DSP-Ausgaben:
beide Farben × MIDI 50/55/62/69 × Amplitude 0,18/0,42/0,58 = 24 Probes.
Baseline: Remote `5ec053ff023ac4d42f4ff5be66127699fa183126`.

Grundtonbandpass und Hilbert-Phase schätzen die Tonhöhenbewegung in Sekunde
2..5. Der Fit trennt 5,1 Hz von der bekannten Verstimmungsschwebung und ihren
ersten Obertönen. Rauschen wird zwischen 2,25 und 2,75 × Grundfrequenz gemessen,
mit Abstand zu harmonischen Teiltönen und den Resonatoren. Das ist eine
Bandmessung, keine vollständige Wahrnehmungsbewertung.

| Messung | Vorher | Nachher |
|---|---:|---:|
| Geschätzte 5,1-Hz-Modulation | 6,047..6,064 Cent | 0,0017..0,0047 Cent |
| Rauschband relativ zum Grundton | -37,40..-32,34 dB | -49,44..-44,38 dB |

Grundtonänderung -0,0020..+0,0059 dB. Alle 24 Vergleichsgrenzen bestanden:
weniger als 0,7 Cent 5,1-Hz-Anteil, Rauschband mindestens 10 dB niedriger,
Grundton innerhalb 1 dB sowie Endlichkeits-/Mindestpegelkontrolle.
Die verbleibenden Millicent sind Auswertefehler/Restbewegung, kein LFO.
Volle `bash test/run_tests.sh` grün, einschließlich acht Bowed-Balance-Probes,
Voice-Ownership/Releases, Hotpath und 489.609 Effekt-Checks.

## BESTE VERSION

Eine tragfähige, leicht bewegte Fläche mit feiner Textur. Bewegung entsteht
bereits durch die zweite Saite und Klangfarbe; ein weiteres regelmäßiges
Wackeln auf der gesamten Tonhöhe ist dafür nicht erforderlich.

## TEST AM GERÄT

27,5-s-Datei: Open Sea alt/neu ab 0/7 s, Fjords alt/neu ab 14/21 s.
Je D4, Amplitude 0,42, Note-off 4,5 s nach Einsatz. Reale trockene Source,
kein Engine-Master oder Raum. Feste Export-Gains 0/0/+0,1/+0,1 dB;
-25 LUFS, -17,7 dBTP, keine Dynamikbearbeitung. Kurze Randfades/Pausen.

Auf mechanisches Zittern, verbleibendes Summen/Sägen und Verlust von
Lebendigkeit achten. Kopfhörer, Gerätespeaker und Mono vergleichen; danach
Pad/Raum gemeinsam prüfen. Scheitert die verbleibende Textur, folgt ein
Timbre-Redesign statt weiterer Hallkaschierung. H743-DWT/Map bleiben offen.

Nächstes Quellenpaket: Choir/Forest, anschließend Guembri/Desert.
