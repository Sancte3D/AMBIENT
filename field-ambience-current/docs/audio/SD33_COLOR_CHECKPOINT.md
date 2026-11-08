# SD33 — Color im tatsächlich gehaltenen Ensemble

Stand 2026-10-11. Eine einzelne Vorbereitungseinheit: Color0/0,5/1 in
COAST, WOODLAND und HIGHLANDS, jeweils mit dem normalen musikalischen Generator.
Die vorherigen isolierten D4-Proben ersetzen diesen Ensemblevergleich nicht.
Color ist damit prüfbar; SD33, musikalischer Nutzen und Hörwahl bleiben offen.

## Aktueller unveränderter Klang

Spec0.9, exakter grüner PR156:
`c89bc630fec5a9e193f06a4e08d8e6752a16b3b2`, Tree
`b3d1db9e8d9fe6f5c972147fd4df47d331f69a3d`.
Dessen Workflow38145968039/run687 wurde vor dieser Arbeit frisch als
completed/success geprüft.251 getrackte Quellen/Headers/Startup-/Linker-/CMake-
Inputs des Firmwarekerns sind bytegleich gegen diesen Commit geprüft.
Kein Quellen-, Generator-, Hüllkurven-, Parameter-, Scene- oder UI-Umbau.
Reference bleibt Default, Product Kandidat, Journal OFF.

Color bleibt der bestehende Vertrag: Bowed-Cutoff0,8..1,2, Horn0,85..1,15,
WOODLAND-Damp0,025..0,005 (default0,015),80-ms-Zeitkonstante auch live.
Aktivität/Attack/Release/Room0,5, Nature0, Volume0,6, D Major/Equal.

## Echte Endwert- und Liveprüfung

18 feste112-s-Fälle (drei Worlds, zwei Seeds1234/42, drei Werte), plus
ein Live-Fall je World bei Seed1234. Alle21 Fälle jeweils64/512-Frames:
42 komplette Colorperformances/78,4min PCM. Sechs zusätzliche112-s-
Defaultkontrollen mit dem bestehenden normalen Renderer ergeben insgesamt
48 vollständige WAV-Render/89,6min PCM.
DefaultColor0,5 reproduziert dessen komplettes PCM/Ereignisse und gemeinsame
Statusfelder bei jedem World/Seed exakt; das neue Statusformat ergänzt Memory-
Returncount. Nicht nur die Hörschnipsel, die ganzen Aufnahmen stimmen überein.

Jeder Color-Fall ist blockunabhängig in sämtlichen PCMbytes, Note-on/off-
Ereignissen und Statusdaten. Zwischen den drei Colorwerten sind alle kompletten
Ereignisse/Statusdaten identisch: keine geänderten Noten, Hz, Velocity, Owner,
Haltezeiten, Ein-/Aussetzzeitpunkte, Phrasen, Memory-/Zulassungszustände.
Livewerte bei10/20/32/48/64/72s sind0/1/0,5/0/1/0,5, initial0,5.
Alle Liveereignisse/Statusdaten entsprechen dem festen Default exakt.
Setter startet keine sofortige Note, Quellenbudgets bleiben3/2/3 inklusive
Releases. Größter gemessener benachbarter PCM-Sprung in±20ms um Bedienänderungen:
0,02390 normiert. Dies und die80-ms-Glättung sind keine subjektive Klickabnahme.

NaN/Limiter0, Raw True Peak höchstens−20,3dBFS, Mono-Energieverhältnis
mindestens0,93179, DC≤1,27e−8. Stop80s, spätestens88,84s alle Besitzer frei;
finale Sekunde aller112-s-Aufnahmen exakt still, kein künstliches Stop-Fade.
Zusätzlich besteht der echte Product-Sound-Vertragscheck mit zwölf Minuten
gerenderter Musik,64/512-/Invalid-Input-Invarianz, Besitz, Kapazität,
Vorbereitungsstorno und Tuning-Tails. Kein behaupteter neuer voller Hostrunner
oder ARM-Build: diese Tools-/Messänderung behält den grünen PR156-Core und
dessen Host/H743-Beleg. Hardware-/Langzeit-/Familienabnahme bleibt offen.

## Colorwirkung und konkreter Befund

Unverändertes Ensemble bei Seed1234, echte Passage3–15,5s. RMS-Differenz zum
Default misst Sampleänderung, weder Lautheitszuwachs noch Hörbarkeit.
FFT-Leistungszentrum aus6–12,5s misst ebenfalls keine Klangqualität.

| World | Color0 /Color1 RMS-Differenz zum Default | FFT-Leistungszentrum0 /0,5 /1 | Kennlinienbefund |
|---|---|---|---|
|COAST|14,44% /10,63%|310,52 /330,98 /347,93Hz|messbare Filterfarbe, Nutzen ungehört|
|WOODLAND|0,203% /0,204%|181,08 /181,27 /181,46Hz|sehr schwache tatsächliche Colorwirkung|
|HIGHLANDS|11,28% /8,50%|384,01 /389,24 /394,23Hz|messbare Filterfarbe, Nutzen ungehört|

Das Leistungszentrum steigt bei beiden Seeds monoton. Min→Max relativ zum
Default beträgt bei WOODLAND nur0,208%; das zweite Seed bestätigt die geringe
Spanne (180,63 /180,82 /181,00Hz). Die lange Saite bleibt erhalten, ihr bisheriger
sehr enger Dampbereich erzeugt hier kaum Endwertunterschied.
Das ist ein konkreter Kalibrierbefund, keine geeignete fertige Reglerkennlinie.
Nächste einzelne Arbeitseinheit: nur WOODLAND-Color wirksamer konstruieren,
bei erhaltenem langen Ton, Tonhöhe, gehaltenen Besitzern und natürlichem Ausklang.
Keine neue Quellenfamilie, Naturkompensation oder UI vor Soundentscheidung.

## Drei kurze WAVs

Je World eine38,5-s-Datei mit derselben musikalischen Passage3–15,5s,
Seed1234. Positionsfolge0–12,5s Color0,13–25,5s0,5,26–38,5s1.
Ein gemeinsames festes Gain je World hält tatsächliche Pegelunterschiede
erhalten; kein getrenntes Normalisieren, AGC oder künstliche Quellenpause.
Zwei0,5-s-Vergleichslücken und40-ms-Schnittkanten.

| Datei | Gain für alle drei Teile | LUFS /dBTP |
|---|---:|---|
|AMBIENT_COAST_Color_0_50_100_38p5s.wav|+13,0dB|−23,0 /−8,6|
|AMBIENT_WOODLAND_Color_0_50_100_38p5s.wav|+10,4dB|−23,0 /−12,6|
|AMBIENT_HIGHLANDS_Color_0_50_100_38p5s.wav|+7,1dB|−23,1 /−15,2|

Alle vollständigen RAWs entstehen unterOS/tmp. Länge und PCM-/Tracehashes,
Compilationinputs und sämtliche Auditinputs werden am Ende erneut versiegelt.
Exportiert werden nur die drei WAVs, alle vollständigen CSV-Traces und JSON.
CI prüft aktuelle Inputs mit separatem Colorjob und exakter PR156-Baseline;
historische Spec0.8-Hörpakete bleiben wie zuvor auf PR155.
Messwerte und sämtliche Eingabe-/Dateihashes:
[SD33_COLOR_METRICS.json](SD33_COLOR_METRICS.json).
