# AMBIENT — Arbeit an den 30 offenen Abnahmen, 2026-10-07

## Ergebnis und Umfang

Die verbliebenen 30 Aufgaben sind vollständig einer konkreten nächsten Prüfung
zugeordnet: 22 Hörabnahmen und acht Storage-/Geräte-/Freeze-/UX-/Abschlussgates.
Der Produkt-DSP wurde in dieser Einheit nicht verändert. COAST-Grain,
Sympathieresonatoren und HIGHLANDS-Onset-Air werden als einzelne Gegenmodelle
vergleichbar, bevor eine Klangentscheidung die Firmware verändert.

PR138 ist nach geprüftem finalem Head und sechs erfolgreichen CI-Jobs in
`claude/hall-sensor-bom-pcb-update-a8xj82` gemerged:
`d97824ec0039499969ef91353633b2c934818690`.
Der Journal-Kandidat bleibt OFF; synchrone Saves und ECC-/Gerätegates bleiben offen.

## Reproduzierbarer Hörvergleich

```bash
cd field-ambience-current/firmware-c-next
python3 tools/build_sound_acceptance.py /absolute/fresh/review
```

Benötigt Python-Standardbibliothek, C11-Compiler und ffmpeg. Ein neuer Zielordner
ist erforderlich. Die tatsächlichen zwölf Produkt-C-Dateien werden mit `-O2`
und `-DFAM_SOUND_PRODUCT` kompiliert. Jede exportierte WAV ist genau 27 Sekunden.
Das ZIP besitzt einen offline verwendbaren Player, Original-/Vergleichspegel,
Zeitmarken und exportierbare Hörnotizen; alle 30 Aufgaben sind darin auffindbar.
Die getrennte Antwortdatei gehört nicht in den Blindvergleich.

| Vergleich | Anzahl | Exakte Frage / Bedingung |
|---|---:|---|
| Trockene Quellen | 3 | D3/D4/A4, je 9 s; drei Sekunden halten, Clear bei 8,75 s |
| Color, Attack, Release | 9 | Nur benannter Parameter 0/0,5/1; D4 und gleiche übrige Werte |
| Activity | 6 | Je World 0/1 mit gleichem Seed23891; Ausschnitt nach 90 s |
| Natürliche Tails | 6 | Einsekündiger D4, maximaler Release; Dry oder maximaler Room; kein Clear |
| Natur allein | 6 | Seed1234 ab 0 s und Seed23891 ab 45 s; Amount0,7 |
| Natur im Zusammenhang | 6 | Gleiche World/Seed1234 ab 90 s; Amount0 oder0,7; Ereignis-CSV identisch |
| Gerichtete Worldwechsel | 6 | Alle Richtungen bei 12 s mit maximalem Room/Release |
| Blind: Dry/Room | 18 | Drei Seeds91267/23891/67431 ab45/90/150 s; Buchstaben pro Runde neu zugeordnet |
| Später Verlauf | 3 | Seed38291, ab1080 s; alle vorherigen Samples tatsächlich gerendert |
| Komponenten A/B/A/B | 3 | D4: Grain aus, Sympathie aus oder Onset-Air aus; jeweils nur eine Änderung |
| Gesamt | 66 | 132 WAVs: Original und Vergleich pro Fixture |

Für spätere Ausschnitte läuft der Renderer vom ersten Sample an. Er springt
keine Uhr vor und verändert den Scheduler nicht am Start des Exportfensters.
Hostregressionen vergleichen 24 überlappende Sekunden bytegenau, einschließlich
eines Teilblocks, für alle Worlds und beide Activity-Grenzen. Natur muss das PCM
ändern und gleichzeitig die tatsächlichen musikalischen Ereignisse bewahren.
Ungültige Fixture-Aufrufe werden vor dem Schreiben verworfen.

Vergleichskopien bekommen einen konstanten Gain mit Ziel−26 LUFS und maximal
−6 dBFS True Peak. Mitglieder von Dry/Room-, Activity- und Naturpaaren bekommen
denselben Gain; ihre Pegelrelation bleibt bestehen. Das ergibt keine exakt
gleiche Einzeldatei-Lautheit über alle unbekannten Worlds. Für SD34 die
Originalpegel verwenden. Kein AGC und keine neue DSP-Kalibrierung.

Die drei Komponenten-Montagen enthalten A aktuell bei0–6/14–20 s, B mit genau
dem bezeichneten Bestandteil entfernt bei7–13/21–27 s; je eine Sekunde Ruhe.
20-ms-Blenden sind Schnittbearbeitung, keine neue Firmware-Envelope.
Die Literaländerungen und beide Quellhashes stehen im Messprotokoll. Temporäre
Gegenmodelle ändern weder den Quellcode noch das Product-/Reference-Profil.

Teiltoninventar: echte trockene Hold-Fenster von einer Sekunde mit Hann-Fenster,
Goertzel-Proben um1f,1,5f und2f–8f. Die relativen Bandwerte sind weder exakte
Harmonischenamplituden noch ein validierter Rauigkeits-/Beruhigungswert.
SD11 benötigt weiter konkrete gehörte Konflikte und danach eine begründete
Regelentscheidung. Es wurde keine Resonanz allein aufgrund einer Zahl entfernt.

Exporte werden zunächst vollständig in einer temporären Arbeitsablage erzeugt.
Größe, Quellenfingerprints sowie alle Raw-/Listen-Prüfsummen werden vor und
nach der Ausgabe geprüft. Das verhindert die Veröffentlichung einer beschädigten
oder während des Exportprozesses veränderten Datei. Die Review-ID hängt an den
tatsächlichen Fixture-Hashes; Notizen verschiedener Blindzuordnungen mischen sich
nicht allein wegen eines identischen Renderers.

## Die 22 Hörgates

| Aufgabe | Nächster konkreter Befund |
|---|---|
| SD02 | COAST D3/D4/A4, Color, Grain-A/B und Sympathie-A/B: konkrete Störstelle oder tragfähige Quelle |
| SD03 | Grain-A/B und mehrstimmige spätere/Blind-COAST: eigenständige Entwicklung statt mechanischer Welle |
| SD04 | HIGHLANDS trocken gegen COAST: eigenständige ruhige Artikulation; Keep/drop entscheiden |
| SD05 | HIGHLANDS-Air-A/B sowie isolierte Attack-/Release-Grenzen: Beginn/Ende runden |
| SD06 | WOODLAND trocken, Color/Attack/Release und ungekappte Tails: Anschlag/Decay auswählen |
| SD11 | Tatsächliche Ereignis-CSV, Quellen-Bandinventar und gemeinsam klingende Abschnitte: relevante Konflikte benennen |
| SD12 | Dry/Room-Tailpaare und Übergänge: Hörrelevanz der Release-/Room-Gedächtnisgrenze kalibrieren |
| SD14 | Drei Blind-Durchgänge und späteres COAST: Übergaben/Zusammenhang hörbar beschreiben |
| SD15 | Dasselbe für WOODLAND: Figur, Antwort, Variation und Rückkehr benennen |
| SD16 | Dasselbe für HIGHLANDS: Fragmente und tragende Pausen; erst nach Quellenwahl |
| SD17 | Mehrere Seeds und spätere Entwicklung: Zusammenhang versus zu kurze Wiederholung; lange Hörsitzung folgt |
| SD19 | Maximaler gemeinsamer Room gegen Dry: Körper/Abstand/Mono und störende Resonanz beurteilen |
| SD28 | Naturpaare bei unveränderter Musik: hörbarer Ortsnutzen oder weglassen |
| SD29 | Natur solo bei zwei Seeds: unregelmäßiger Wind ohne Pfeifen/Rumpeln |
| SD30 | Dieselben Natur- und Kontextclips: Tropfen/Wellen ohne nervöse Wiederholung |
| SD31 | Natur- und Komponentenvergleiche: jedes verbliebene Geräusch einzeln begründen |
| SD33 | Isolierte Color/Attack/Release-Grenzen plus Activity0/1: brauchbare gesamte Makrospanne |
| SD34 | Originale Quellpegel und tatsächliche Übergänge: störende Lautheitssprünge entscheiden |
| SD35 | Diese Grenzen plus vorhandene Stressfälle: konkrete Zeitstelle statt pauschaler Freigabe |
| SD37 | Alle sechs gerichteten Wechsel: musikalischer Zusammenhang bei vollständigen Tails |
| SD44 | Unbekannte Worlds erst trocken beschreiben, dann Room; getrennten Schlüssel erst danach öffnen |
| SD45 | Späte Ausschnitte nach18 min plus echte längere Sitzung: Form und Ermüdung unterscheiden |

## Acht weitere Gates mit konkretem Abnahmeablauf

| Aufgabe | Arbeit / zu protokollierender Nachweis |
|---|---|
| SD41 | H743-ECC-Readadapter zuerst sicher klassifizierbar machen. Dann Power-cut während Erase/Payload/Commit, Reset/Recall, Altformat und Versorgung testen. OFF beibehalten, bis diese Befunde und nichtblockierende Save-Transaktion vorliegen |
| SD47 | Main-Save-Stall bleibt: Start→Commit-Zeit, maximaler Generate-Tick-Abstand und reale Onset-Verspätung separat von IRQ-Renderzeit erfassen. Danach nichtblockierenden Storage-Handoff auf dieselben Szenentests prüfen |
| SD48 | Product auf echtem H743: alle Worlds, maximale Quellen/Tails/Room/Nature, schnelle Eingaben/MIDI/Display und Saveverkehr. `audio_profiler_state()` liefert `peak_load`, `max_cycles`, `deadline_miss_count`, `clip_count`; Gate `peak_load <0,60`, null Misses. Stack-High-water separat mit verschachtelten Main-/IRQ-Pfaden messen |
| SD49 | Gebauten Ausgang aufnehmen: still/typisch/dicht, Boot/Stop/Clear/Mute/Recall/Powerwechsel und erlaubte Lasten. Pegel/DC/Noise/Pops, Clock/Cache und Stromzustand mit Boardrevision, Firmware-SHA und Ausgangskette protokollieren |
| SD50 | Echte längere Sitzungen leise/typisch, mit Hörerkontext und konkreten Zeitstellen: Alarm/Tube/Buzz/Beep/Ermüdung bearbeiten. Dateien und Messwerte ergeben allein kein Nutzerurteil |
| SD51 | Nach Quellen-/Hör-/Gerätebefunden exakte Parametergrenzen, Pegel, Tails, Naturwahl und Quellenpalette versioniert einfrieren |
| SD52 | Nach dem Soundstand physische Tasten/Encoder, Display/LED und15-min-Ruhe an dieselben Audiooperationen anbinden. Wake mit Audioaufnahme auf zusätzliches Ereignis/Tankreset prüfen |
| SD53 | Pro zutreffendem Gate Befund, SHA, Fixture/Board und Ergebnis einsammeln; erst nach vollständiger Abnahme Sounddesign fertig nennen |

SD48-Diagnose existiert bereits in `diag_h743.c` (CELL1 beim Einschalten halten).
Ihre Prozentanzeige rundet; die0,60-Grenze gegen den tatsächlichen Float prüfen.
16 KiB reservierter Stack und Compilerframes sind kein High-water. Ein niedriger
IRQ-Renderwert beweist zudem keine rechtzeitige Main-/Generate-Planung während
Save. Keine Hardwaremessung wurde aus dem Hostlauf abgeleitet.

## Validierung und aktueller Status

Volle Firmware-Host-Suite bestanden; nach Ergänzung der Activity-Fixtures die
gezielte Exportregression erneut bestanden. Die CI erzeugt zusätzlich zum
bestehenden kurzen Paket dieses vollständige Archiv und den getrennten
Blindschlüssel. Der H743-Job prüft weiter Default, Product und den deaktivierten
Journal-Kandidaten; die finalen CI-/Linkdaten werden nach diesem Lauf ergänzt.

Die Hör-Checkboxen sind bewusst weiter offen, bis ein konkreter Hörbefund
vorliegt. Die Liste bleibt **24 geschlossen /30 offene Gesamtgates**.
