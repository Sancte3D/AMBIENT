# COAST — trockener Grundton, 2026-10-06

## KERNURTEIL

Die verstimmte Begleitsäge erzeugte eine periodische Grundtonbewegung, obwohl
die Hauptstimme selbst stabil war. Eine einzelne bandbegrenzte Säge erhält den
Toncharakter und senkt diese Bewegung über 24 reale PCM-Probes von maximal
3,102 dB auf 0,049 dB. Das ist eine konkrete technische Verbesserung, keine
pauschale Hörfreigabe der Quelle oder der späteren COAST-Grammatik.

## FUNDAMENTAL FALSCH

Eine zweite leicht verstimmte Säge sollte Lebendigkeit erzeugen, fügte aber
einen wiederholbaren Grundton-Pegelzyklus hinzu. Eigenständige musikalische
Entwicklung braucht diesen dauernden Taktgeber nicht. Die leise Zwischen-
resonanz bei 1,5f ist dagegen im isolierten Vergleich keine gleich starke
Ursache: Energie etwa −57 bis −62 dB relativ zum Grundtonband. Daraus keinen
erfundenen dominanten Fremdton ableiten.

## NOCH NICHT SELBSTVERSTÄNDLICH

Der Source-Kandidat bleibt eine gestrichene synthetische Klangfarbe. Ob Körper,
Grain und höhere Obertöne als angenehm wahrgenommen werden, bleibt Hörfrage.
Die verbleibende langsame Körperfarbe ist individuell pro Stimme; ihre Stärke
allein hat in diesen Probes nur geringe Wirkung auf den Grundtonpegel. Body,
Masterraum, neue Makros und World-Grammatik sind in diesem Vergleich aus.

## LOCKED

Strikte Quellenzulassung, drei lokale Slots, Besitzer, weiche Releases und
Dry/Send-Verhältnis bleiben erhalten. Die kleine Quellresonanz wird nicht
ohne einen konkret belegten Schaden entfernt. Ein neuer Layer oder globaler
Lautheitsausgleich ist für diese Korrektur nicht nötig.

## REMOVE / MERGE / REDESIGN

- Zweite Säge und ihre Phasen-/Incrementfelder entfernt.
- Gewicht der verbleibenden Säge 0,721, entsprechend der früheren mittleren
  Leistung `sqrt(0,71² + 0,125²)`. Keine laufende Normalisierung.
- Bestehende tatsächliche Grundton-/Oktav-Regression verlangt nun höchstens
  0,20 dB Root-Schwankung statt 6 dB; acht Register-/Farb-Probes bestehen.
- Kein gleichzeitiger Umbau von Resonanzen, Body, Grammatik oder Effekten.

## BESTE VERSION

Ein kontinuierlicher tragfähiger Ton, dessen Entwicklung aus Einsatz,
individueller Artikulation und Beziehungen zu anderen Stimmen entsteht.
Einzelstimme und Mono-Summe besitzen denselben Tonkörper. COAST muss später
Zusammenhänge weitergeben, statt als ständig moduliertes Flächenpreset zu laufen.

## TEST AM GERÄT

Über die reale Ausgabekette: D3/A3/D4/A4, beide Farben, verschiedene Pegel,
Mono und drei überlappende Releases. Auf Buzz/Hohlklang und Ermüdung hören;
die messbare Beating-Korrektur alleine beantwortet diese Fragen nicht.
Release-/Onset-Spitzenlast und Stack bleiben physisch ungemessen.

## Reproduzierbarer Nachweis

Referenz ist der gemergte Stand `bf5fe2c8a89a91b61c2c11462520a8acdf5ab2e3`.
`tools/review_coast_dry.py` rendert echte Source-DSP-Blöcke mit 256 Frames:
2 Farben × MIDI 50/57/62/69 × Amplitude 0,18/0,42/0,58, je 12 s intern.
24/24 Probes bestehen; rohe Root-Gain-Differenz ist begrenzt und protokolliert.
Die bestehende Source-Regression besteht zusätzlich bei MIDI 50/62/71/81.
Gemessene H743-Bankdeltas werden erst nach dem Cross-build nachgetragen.

Hördatei: `COAST_Dry_AB_27s.wav`; 0–6 s Farbe 0 vorher, 7–13 s danach,
14–20 s Farbe 1 vorher, 21–27 s danach; jeweils 1 s Pause. Gehaltener D4-Ton
bis 4 s, dann Release. Keine Body-/Raum-/Naturbearbeitung. Feste Gains je
Excerpt, −26,1 LUFS gesamt, −18,8 dBFS True Peak. Keine Klang-Diagnosetöne.
Rohdaten/Fingerprints: [COAST_DRY_METRICS.json](COAST_DRY_METRICS.json).

SD02 ist **softwareseitig korrigiert, Hörabnahme offen**. SD03 bleibt eine
eigene Bewegungs-/Hörentscheidung. Vollständiger Auftrag:
[AMBIENT_SOUND_DESIGN_TODO.md](AMBIENT_SOUND_DESIGN_TODO.md).
