# Cell-Entfaltung — Softwarekandidat, 2026-10-26

Basis: PR182, `bea7f5765125b2d55c7bb28832626dce7956b382`.
Dieser einzelne Ausbau ergänzt die bestehende Product-Note-Bedienung.
Keine Übernahme der Tonitor-Engine; keine neue Tonhöhen- oder Akkordauswahl.

## Verhalten

- Nur ein physischer, tatsächlich zugelassener Cell-Druck startet Entfaltung.
- Jede Stimme öffnet ihre Klangfarbe in vier Sekunden bis zu einem begrenzten
  Plateau. Mehrere Cells entwickeln sich unabhängig.
- Physisches Loslassen nimmt die zusätzliche Öffnung innerhalb von höchstens
  1,2 Sekunden zurück. HOLD hält den Ton gemäß bisheriger Quellenart weiter.
- SHIFT beim Loslassen verändert nicht den ursprünglich gedrückten Besitzer.
- Gestenwiedergabe, Generate, Szenenwahl und ein Wake-Druck starten keine
  physische Entfaltung. Ein unter dem Szenenmenü ankommendes Loslassen beendet
  die Öffnung ebenfalls.
- Abgewiesene Noten entwickeln keinen alten Ton. Clear, Mute, Modusverlassen,
  Weltwechsel und erneute Besitzvergabe entfernen ihre alten Ziele.

Das ist zeitgesteuerter Ausdruck mit digitalen Schaltern. Druckstärke und
Fingerbewegung werden nicht gemessen. Es gibt keinen neuen Menüparameter,
kein MIDI-Aftertouch und keine zusätzliche Scene-Persistenz.
Die Erweiterung betrifft den Note-Pfad; archivierte Harmony-/Land-Routings
bekommen dadurch keine neue Expressionszuordnung.

## Klang und Echtzeitpfad

COAST öffnet seinen vorhandenen Körperfilter maximal um Faktor 1,25;
HIGHLANDS seinen vorhandenen nichtresonanten Atemfilter um Faktor 1,40.
WOODLAND ergänzt maximal 0,35 der vorhandenen oberen Ausgangsband-Komponente.
Es entsteht kein neuer Oszillator, kein zusätzlicher Ton, kein Detune und
keine Neu-Anregung der Saite. WOODLANDs Delaylänge, Feedback, Verlust,
Hüllkurve und Ruhestandsentscheidung bleiben unverändert. Seine natürliche
lange Schwingung wird nicht zu wiederholten Zupfern oder endlosem Sustain.

Main veröffentlicht je Quellenfamilie eine atomare Besitzmaske. Audio liest
sie einmal pro Renderaufruf und hält den Fortschritt lokal pro Stimme.
Ganzzahliger Fortschritt mit Smoothstep stellt die Zeitgrenzen unabhängig von
Blockgrößen sicher. Neue Vorbereitungen starten bei null. Keine Allokation,
keine neue Delaylinie, kein Warten, kein IRQ-Masking und keine transzendentale
Berechnung im zusätzlichen Samplepfad. Neutral bleibt hörbares PCM exakt.

## Belege

- `test_cell_unfold.c`: exakte 4 s / 1,2 s, alle drei Welten,
  identische vollständige PCM-Ausgaben bei 37/512 Frames, HOLD/SHIFT,
  Replay-Neutralität, abgewiesene Besitzer, Clear/Mute und frische Noten.
- `test_cell_unfold_sources.c`: echte private Quellenzustände. Ungehaltene
  Nachbarstimme bleibt byte-identisch, Tonhöhe/Phase und Amplitudenhüllkurve
  unverändert; bei WOODLAND auch der komplette zirkulierende Saitenpuffer,
  Feedback und Ruhestand. Zwei unabhängige physische Ziele und Rückkehr.
- Echte H743-Eingabefunktion: sechs neue PCM-Vergleiche für physische
  Note-Druck-/Loslassfolgen, einschließlich offenem Szenenmenü; zusätzlich
  alle bisherigen 126 Wake-/Idle-Vergleiche. Native und ASan/UBSan bestanden.
- `review_cell_unfold.py`: ASan/UBSan durch echte Engine/Controls und exakt
  3.175.200 Stereo-Frames unverändertes neutrales PCM gegen PR182 bestanden.
  Leak-Erkennung ist für diese statischen Firmware-Fixtures abgeschaltet;
  Adress- und Undefined-Behavior-Prüfung bleiben aktiv.
- Alle Host-Testblöcke bestanden. Im ersten Gesamtlauf fehlte lokal der durch
  Sparse-Checkout ausgelassene, unveränderte Effects-Test. Nach Bereitstellung
  seiner originalen Git-Version bestand auch dieser letzte Block separat.
- H743 Product Release mit ARM GCC 13.2.1 gebaut; Link-/Placement-/Frameaudit
  und neu erzeugtes, zum Kandidaten passendes Geräte-Auslesepaket bestanden.
  Journal bleibt OFF. Der Reference-Pfad bekommt keine Entfaltungs-DSP-Änderung.

| ARM Release | PR182 | Entfaltung |
|---|---:|---:|
| text | 199.884 B | 198.924 B |
| data | 240 B | 240 B |
| bss | 142.768 B | 142.864 B |
| Bank-1-Loadimage | 200.132 B | 199.172 B |

Die zusätzlichen 96 B statischen Zustands bedeuten keine gemessene
Echtzeitreserve. Compiler-Frames: `engine_render` 88 B, Bowed/Horn je 120 B,
Pluck 96 B; kein gemessener Stack-High-Water. SD48-Hardwaremessung bleibt offen.

Der strenge historische IRQ-Inventarvergleich bleibt unverändert bei PR182
archiviert. Aktuelle IRQ-/UI-Fixtures samt Sanitizern und Entfaltungsvergleich
laufen im bestehenden Host-CI-Job. Keine zusätzlichen Workflow-Jobs, keine
abgeschwächten historischen Quellengleichheitsprüfungen.

## Direkter Hörvergleich

`AMBIENT_Entfaltung_AB.wav`: 72 s, 44.100 Hz, Stereo, PCM16, 12.700.844 B.
Je Welt identische Noten/Zeiten, Anschlag, Raum und Lautstärke; Natur aus.
Kein nachträglicher Gain, keine Lautheitsautomatik.

| Zeit | Welt | Variante |
|---|---|---|
| 00:00–00:12 | COAST | ohne Entfaltung |
| 00:12–00:24 | COAST | mit physischer Entfaltung |
| 00:24–00:36 | WOODLAND | ohne |
| 00:36–00:48 | WOODLAND | mit |
| 00:48–01:00 | HIGHLANDS | ohne |
| 01:00–01:12 | HIGHLANDS | mit |

Pro Abschnitt: Cell 1 bei 0 s, Cell 3 bei 1 s; physisch loslassen bei 6/7 s,
HOLD bleibt zunächst bestehen; Clear bei 9 s. Dadurch ist sowohl die Öffnung
als auch die Rückkehr bei weiter gehaltenem Ton vergleichbar.
WAV-SHA256: `d76c8646c90da5f841273d45e57cac657c80641c9061eff8f253b5f9fa716c50`.

Reproduktion im Firmwareordner:
`python3 tools/review_cell_unfold.py build-cell-unfold`.
Dafür muss der oben genannte PR182-Commit lokal verfügbar sein.
CI speichert WAV und Metriken im Artefakt
`ambient-cell-unfold-current-proof-and-wav`.

## Noch abzunehmen

Hören: Klingt die Öffnung in allen drei Welten musikalisch, ruhig und dennoch
wahrnehmbar? Passt die Stärke bei gleichzeitig gehaltenen Cells und extremer
bestehender Klangfarbenstellung? Danach gegebenenfalls nur die Grenzen anpassen.
Am Gerät: gleichzeitige Knöpfe, HOLD/SHIFT, Szene öffnen und loslassen,
Clear/Mute, schnelle Wiederholungen sowie IRQ-/Stack-/Audiomessung.
Keine dieser Hör- oder Geräteentscheidungen wird durch diesen Softwarebeleg
als erledigt markiert. Keine Programmierung oder Zusammenführung ausgeführt.
