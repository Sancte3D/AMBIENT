# WOODLAND-Körper — gesicherter harmonischer Hörkandidat

Stand 2026-10-26. Basis ist PR184,
`24936b4e19895627d8b4bbdb1ba19a5b05c3db10`, Branch
`codex/ambient-world-character-2026-10-26`.
Neuer Arbeitsbranch: `codex/ambient-woodland-body-2026-10-26`.

## Übernommener letzter Stand

Die jüngsten Dateien des fortgesetzten Chats wurden tatsächlich eingelesen:
`AMBIENT_Entfaltung_AB.wav`, `AMBIENT_Welten_Charakter_V1.wav`,
`AMBIENT_Woodland_Body.wav`, die letzte gespeicherte To-do/Recovery und
`tonitor-main.zip`. Der aktuelle Git-Stand ist neuer als die Übergaben vom
20.10.; deshalb bleibt PR184 die Codebasis.

Der frühere Körper-WAV ist ein erhaltenes Hörartefakt, kein gesicherter
Quellstand: SHA256
`d9356154a3ad8d130d3e6d56feae190665d144296afcbe10337b0ef231581761`.
Die hier explizit neu implementierte und reproduzierbare Fassung erhält
deshalb eigene V2-Hördateien. Sie setzt dieselbe zuletzt vereinbarte kleine
Stufe um: zwei leise Körperresonanzen am zweiten und dritten Oberton.
Es wird keine bytegleiche Wiederherstellung des ungesicherten Codes behauptet.

## Umsetzung und Grenzen

`woodland_body.h`, Modell 1, ist eine eigene Float-Implementierung. Zwei
Bandpässe mit Q=6 und normierter Resonanzspitze werden beim Note-on auf die
tatsächlich zugelassene Frequenz ×2/×3 vorbereitet. Ihre Ausgangsgewichte sind
0,10/0,06; die unveränderte Saite ist die einzige Anregung. Modi oberhalb
0,45×Samplerate entfallen, statt auf eine andere Frequenz geklemmt zu werden.

Die Filter liegen vor der bestehenden Ausgangsfarbe. Trocken- und Raumsignal
verwenden denselben Ausgang und dieselben Attack-/Stoprampen. Die Filter-
Erinnerung gehört ausschließlich zur Stimme; Clear und erneute Vorbereitung
löschen sie. Der Schalter betrifft künftige Noten und verändert keine aktive
Stimme. Ein inaktiver Körper liefert den Eingang exakt zurück.

Saitenpuffer, Rückkopplung, Dämpfung, Frequenz, Notenereignisse und natürliche
Ruhestandsentscheidung bleiben unverändert. Es entstehen keine zusätzlichen
Quellplätze, frei laufenden Oszillatoren oder Delaybanken. Zwei kleine
Filterzustände benötigen 52 B pro Saitenstimme; Gesamt-Linkbudget und echte
Gerätelast sind eigene Nachweise. Nature bleibt separat.

Referenz ist wieder das hochgeladene Tonitor 0.56 mit Archivhash
`9c52dcfebb94058bb0476763aaf4649603cd384015b1d8bf0aeeb1b51e275e1a`.
Der Kopf von `firmware/src/druid.c` beschreibt sympathische und modale
Resonanz als Anregung für diese Stufe. Kein Tonitor-Code, keine Koeffizienten-
tabelle und kein Hilfsmodul wurde kopiert. Die GPL-3.0-only-Dateikennung wird
nicht mit der MIT-Ursprungsangabe für Rings verwechselt.

## Aktivierung

`FAM_WOODLAND_BODY_CANDIDATE` bleibt standardmäßig OFF und ist unabhängig von
`FAM_WORLD_CHARACTER_CANDIDATE`. Für den separaten Hörbuild beides einschalten:

```sh
cmake -S . -B build-h743-woodland-body -DFAM_TARGET=h743 -DFAM_SOUND_PROFILE=product -DFAM_WORLD_CHARACTER_CANDIDATE=ON -DFAM_WOODLAND_BODY_CANDIDATE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-h743-woodland-body
python3 tools/check_product_link.py build-h743-woodland-body
```

`engine_set_woodland_body(bool)` ist Main-only und erfasst nur künftige lange
Product-Saiten. Kein neuer UI-/MIDI-/Scene-Parameter, keine Formatmigration.
Journal bleibt OFF und Live-Save synchron. Kein Merge, Retarget oder Flashen.

## Geprüfte Belege

- Private echte Saitenstimmen: Nachbarstimme bytegleich, Saitenpuffer und
  Schleifenparameter bytegleich, gleiches natürliches Ende nach 1.359.710
  Frames bei 196 Hz im Test. Grundton fast unverändert; gezielte Verstärkung
  von zweitem/drittem Oberton bei 60/196/470 Hz. Stabile Impulsantwort,
  exakter Bypass, künftige Snapshots, volle Pool-/Fehleingabe-Rejektion,
  ungehörtes Abbrechen und sauberes Wiederverwenden geprüft.
- Echte Product-Engine: 37/512-Frame-Ausgaben bei gleichem 10-ms-Mainraster
  bytegleich, dieselben tatsächlichen Noten-/Owner-/Hz-/Velocity-Traces.
  COAST und HIGHLANDS bleiben mit Körper ON/OFF vollständig bytegleich.
- 12.700.800 Stereo-Frames mit Körper OFF stimmen mit PR184 überein, sowohl
  bei Charakter OFF als auch ON. Der erste 24-s-Abschnitt der neuen WOODLAND-
  A/B-Datei stimmt zudem mit 00:24–00:48 der jüngsten gespeicherten
  `AMBIENT_Welten_Charakter_V1.wav` überein.
- Native Prüfungen und ASan/UBSan: privater Körpertest, vorhandener vollständiger
  Charakter-/Kontroll-/Partitionsfixture mit Körper ON und 48 laute Endpunkt-
  Kombinationen mit Körper ON. Volle Velocity/Master/Room/Nature, Major/Minor,
  ET/JI, Color-Enden und lange Hüllkurven; Clear/Mute enden exakt still.
- Der bestehende Hostrunner enthält diese Prüfungen. CI veröffentlicht den
  aktuellen PCM-/Sanitizerbeleg und baut das separate H743-ON-Image mit
  Linkaudit und passendem Geräte-Auslesepaket im vorhandenen ARM-Job.
  Die Workflowzahl bleibt 30. ARM-Ergebnis am jeweiligen Commit prüfen;
  ein Compiler-/Linkpass wäre kein gemessener Echtzeit-/Stack-/Ausgangspass.

Reproduktion: `python3 tools/review_woodland_body.py build-woodland-body`.
Der oben genannte PR184-Commit muss verfügbar sein. Ausgaben enthalten
`WOODLAND_BODY_METRICS.json` mit Quellhashes und tatsächliche Ereignistraces.

## Neue Hördateien

Beide Dateien sind echte generative C-Firmware, Stereo PCM16/44.100 Hz,
Seed 1234, World-Charakter ON, Room 0,5, Nature 0, Master 0,75. Generate endet
nach 18 s je Abschnitt; die vorhandene Quelle klingt natürlich aus. Kein
Datei-Gain, keine externe Komposition oder künstliche Schlussblende.

| Datei | Inhalt | SHA256 |
|---|---|---|
| `AMBIENT_Woodland_Body_V2_AB.wav` | 48 s: 24 s bisher / 24 s mit Körper | `3af9948c5fc517bdaadd3013a55529f41eca0e02fec7b602386b80215816d399` |
| `AMBIENT_Woodland_Body_V2.wav` | 24 s nur neue Fassung | `9f22a512e7e7674ddcb9f77ef1b6132ca4d3531c020344b59e124ab205c7401d` |

A/B-Samplepeak 0,0990601 (rund −20,08 dBFS); kein Nonfinite-/Limiterereignis.
Das ist eine technische Ausgabebeobachtung, keine subjektive Hörabnahme.

## Nächste einzelne Einheit

Diese Stufe ergänzt den Körper der bestehenden langen Saite. Eine eigene
lang klingende Resonatorquelle als **Ersatz** ist weiterhin der nächste Block;
danach die begrenzte Flächenalternative, Stimmführung/Loops, Raum/Delay und
Grains in der vereinbarten Reihenfolge. Für eine neue eigenständige Quelle
müssen tatsächliche Tonhöhen, Stimmenzulassung und Release-/Tailhistorie
erneut vollständig passen. Die bisherigen SD-Hör-/Gerätegates bleiben offen.
