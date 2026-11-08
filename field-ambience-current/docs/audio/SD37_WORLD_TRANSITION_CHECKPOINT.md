# SD37 — Sechs gerichtete Worldwechsel

Stand2026-10-13. Diese einzelne Softwareprüfung ist abgeschlossen; musikalische
Übergangs-/Knack-/Naturhörwahl und Geräteabnahme bleiben offen. SD37 bleibt
unchecked und zählt jetzt zur Hörgruppe. Exakte Basis PR162:
4c784f64de8f27686be6539b228be578d147620e,
Treeae4e183c6a88ab5193158e6784cc4e6e677b222e. Gesamt-CI38364246794/run703
frisch completed/success. Alle251 getrackten Core-/Header-/Startup-/Linker-/
CMakeinputs bytegleich. Spec0.10,SCN7,Reference Default,Product Kandidat,
Journal OFF und Gains bestehen. Eine veraltete20-ms-Vertragsstelle wird auf
den vorhandenen100-ms-Kontextstop der langen WOODLAND-Saite korrigiert.

## Erreichbare Übergaberegeln

Worldwahl beendet erzeugte Besitzer6/7/15 weich innerhalb100ms, storniert
unbegonnene Vorbereitungen und setzt den begrenzten Zielscore neu auf.
Manuelle Besitzer0..4/9..13 erhalten tatsächliche Quelle/Hz; Key-up trifft
deren ursprüngliche Familie. Die lange manuelle Saite klingt natürlich aus.
Entry in Generate beendet auch alte manuelle Quellen innerhalb100ms.
Die drei globalen/zwei lokalen Saitenplätze enthalten Releases und Pending.
Ein NoteOff-Hook gibt keinen Quellenplatz frei. Der gemeinsame Room und seine
Pitchhistory bleiben bei Worldwahl erhalten. Nature ist unabhängig und optional.

Die Produkt-Engine ruft für alle drei Familien100ms quiet_source auf. Die alte
Pluck20-ms-Angabe beschreibt die kurze Referenzartikulation, nicht den aktuellen
langen Product-Kontextstop. Der Quellcode wird dafür nicht geändert. Wiederholte
Zielwahl verlängert eine bereits laufende kurze Übergabe nicht.

## Umfang der aktuellen Fälle

Alle sechs Richtungen: C→W,C→H,W→C,W→H,H→C,H→W.
Activity0,5; Attack/Release/Color/Room1; Master0,6; Nature normalerweise0.
Volle erzeugte Belegung vor Worldwahl: COAST/HIGHLANDS3,WOODLAND2.

| Variante je Richtung | Fälle | Tatsächlicher Ablauf |
|---|---:|---|
|Volle Generate-Belegung|12|Wechsel2,7s; D-Major/Equal/Seed1234 und A-Minor/Just/Seed42|
|Pending vor erstem Audio|6|Alte erste Angebote reservieren, sofort Worldwahl; kein alter DSP-Start|
|Bereits im Release|6|Stop2,7s, Worldwahl2,71s, ausdrückliches Generate-On3,3s|
|Mehrfachziel ohne Audio dazwischen|6|Dritte World→Ausgangsworld→Ziel im selben Controltick; gegen direkten Wechsel|
|Schnelle Ziele mit Audio|6|Zehn Worldwahlen alle10ms von2,7–2,79s; letztes Ziel bleibt|
|Volle manuelle Belegung|6|Worldwahl2,7s, neue Eingabe, voller Pool; ursprüngliche Key-ups4s|
|Optional Nature1|6|Derselbe wirkliche Score/Quellenverlauf wie Nature0, Worldfade separat|
|32-Bit-Timerwrap|6|Clockbase0xfffff000; kompletter Verlauf gegen Clock0|

54 Szenarien bei64/512 ergeben108 ganze WAV-Render/220,8min PCM. Generate128s,
Stop96s plus32s natürlicher Schluss; manuell80s mit Key-up/Nature0 bei4s.
Getrennte Szenarien, keine kontinuierliche Langzeit- oder Geräte-ISR-Abnahme.
Die zwei musikalischen Kontexte koppeln Key/Collection/Tuning/Seed gezielt;
kein Anspruch auf jeden Key, jede Lage oder beliebiges Wechsel-Timing.

## Echte Ereignisse und physische Quellen

670 bestätigte DSP-Starts an678 frischen physischen Reservierungen. Acht alte
Pending-Reservierungen enden vor DSP und erzeugen weder Hook noch PCM noch
Pitchhistory. Jeder wiederbenutzte Besitzer ist vorher wirklich retiert.
Kein gleichzeitig in mehreren Familien aktiver Owner, kein Steal/Overbudget.
Globale Quellen≤3/Saiten≤2; Kontrolle beendet keine physische Stimme sofort.

| Beobachtung | Ergebnis |
|---|---|
|Gehaltene/ausklingende Generatequellen nach erstem Worldauftrag|100ms bis tatsächlichem Quellenende|
|Alte unbegonnene Vorbereitungen|Spätestens im nächsten10-ms-Beobachtungstick frei; kompletter alter Audioabschnitt exakt0|
|Erster bestätigter Zieleinsatz nach letzter Wahl|260ms in gewöhnlichen/Pending/Batch/Rapidfällen|
|Absichtlich gestoppte Variante|600ms ab Worldwahl; Generate wurde erst590ms danach ausdrücklich wieder eingeschaltet|
|Manuelle alte Quellen nach Worldwahl|Keine Zwangsübergabe; Original-Key-ups bei4s und natürliche Enden|
|Neue manuelle Zielquelle aus WOODLAND|Ein global freier dritter Platz wird zugelassen; weiterer Start abgewiesen|
|Neue manuelle Eingabe bei alter COAST/HIGHLANDS-Dreierbelegung|Abgewiesen, bestehende Besitzer erhalten|

Bestätigungszeiten sind DSP-Acks bei10-ms-Hostticks. Der erste Renderbeginn
liegt im vorausgehenden Tick; dies ist keine exakte psychoakustische Onsetzeit.
Die260ms enthalten das reguläre250-ms-Retry und den folgenden Ack. Physische
100-ms-Freigabe wird gegen dieselben Besitzer/Quellenmasken geprüft.
Nach jeder hörbaren Generateübergabe bleiben mindestens die alten Hz in der
gemeinsamen History vertreten; die alte Room-Ausgabe ist positiv und wird vom
Setter nicht gelöscht. Nach letzter Zielwahl entstehen nur neue Töne der
Ziel-Familie, kein wiederkehrender alter/zwischenzeitlicher Score.
Jede Zielwelt erreicht mindestens eine vollständige Phrase bis Stop.

Manuelle Enden nach Original-Key-up: COAST14,37s,WOODLAND42,91s,
HIGHLANDS11,17s bei diesen Velocity0,75-Voicings. Die Saite bleibt lang,
auch wenn die ausgewählte World längst COAST/HIGHLANDS ist.

## Ganze PCM-Gegenproben und Beobachtungsgrenzen

Alle54 ganzen PCM-Dateien, Events, physischen Besitzer-/Familienzustände,
Zielwahl/Scorezustände und Aktionsziele sind bei64/512 exakt identisch.
Sechs Batchwechsel entsprechen direkten Wechseln im ganzen PCM/Events/
Besitzverlauf; sechs Timerwrapfälle entsprechen Clock0. Nature1 erhält ganze
Noten-/Besitz-/Scoreverläufe gegenüber Nature0.

Zusätzlich27 gepaarte Vier-Sekunden-Fälle: zwölf gehaltene/ausklingende
Manual-Paare, sechs Source-free-Room-Paare, sechs reine Latest-Nature-Paare
und drei idempotente Generate-Worldwerte. Alle ganzen PCMbytes bleiben exakt.
Room- und Hz-/Tailhistory überleben die Worldwahl; manuelle→Generate-Entry
ist bei100ms physisch fertig. Diese In-memory-Captures zählen separat zu
216s PCM und sind keine zusätzlichen exportierten WAVs. Kein Clear/Mute.

ambient_room_peak ist das Maximum des letzten Renderblocks.64/512 lesen
verschiedene Zeitfenster, deshalb werden diese Floatpeaks einzeln exportiert
und nicht fälschlich als gleicher State verglichen. Die darauf beruhende
Quiet-History-Freigabe unterscheidet sich in78 einzelnen Beobachtungen um
genau einen10-ms-Tick, jeweils ohne aktive Quellen. Ganze Audio-/Ereignis-/
Besitz-/Zielverläufe bleiben gleich. Kein vollständiger CSV-Bytegleichheitsclaim
für diese beiden Diagnosefelder. Werte/Zeiten sind im Audit enthalten;
SD12-Kalibrierung der Hörrelevanz und Geräte-ISR bleiben offen.

Alle Fälle: Nonfinite/Limiter0, letzte PCM-Sekunde exakt0, Monoenergie≥0,882484,
mittlerer Kanal-DC≤3,355e−8, TruePeak≤−15,3dBFS bei diesem Master0,6.
Dies sind gerichtete Übergangsfälle, keine neue globale Pegelreserve.
Inputs, ganze Captures,162 Event/State/Action-CSVs,54 physische Ledger und
Hör-WAV abschließend gegen ihre Fingerprints geprüft. Lange Rohdateien bleiben
nur OS/tmp und werden nach dem Audit entfernt; vollständige Traces im CI-Artefakt.

## Kurzer Übergangsvergleich

AMBIENT_Worldwechsel_50p5s.wav: sechs echte Generatepassagen0,7–8,7s,
je8s; Actual Worldauftrag jeweils2s nach Abschnittsbeginn. Keine nachträgliche
Überblendung am Wechsel. Ein gemeinsamer Gain+4dB,40-ms-Ausschnittkanten,
fünf0,5-s-Lücken. Alle sechs verwenden D-Major/Equal1234, Activity0,5,
Attack/Release/Color/Room1,Master0,6,Nature0. Keine neue Komposition.

| Richtung | Abschnitt | Worldauftrag im WAV |
|---|---|---|
|COAST→WOODLAND|0–8s|2s|
|COAST→HIGHLANDS|8,5–16,5s|10,5s|
|WOODLAND→COAST|17–25s|19s|
|WOODLAND→HIGHLANDS|25,5–33,5s|27,5s|
|HIGHLANDS→COAST|34–42s|36s|
|HIGHLANDS→WOODLAND|42,5–50,5s|44,5s|

44,1kHz/Stereo/16bit,8.908.244B,−24LUFS/−15dBTP.
SHA256d4db7935f073c6bfae5c0e687cd98d6a2ffb27239cf0b26c0f91eb0b3b928c03.
Daten:[SD37_WORLD_TRANSITION_METRICS.json](SD37_WORLD_TRANSITION_METRICS.json),
[SD37_WORLD_TRANSITION_CASES.csv](SD37_WORLD_TRANSITION_CASES.csv).
Reproduktion:`python3 tools/review_world_transitions.py build-world-transition-review`.

Musikalische Abnahme bleibt: passt die tatsächliche100-ms-Übergabe mit neuem
Attack und bestehendem Room, gibt es hörbaren Knacks/Loch oder störende Nature?
Zeitmarke im kurzen WAV nennen. Softwarebeleg ersetzt diese Entscheidung nicht.
SD37 bleibt unchecked;24/54 geschlossen,30 offen. Planung jetzt11 technische
Hauptpunkte (neun Software-/Prüfblöcke,zwei Gerätegates),18 Hörentscheidungen
plus Gesamtcheck. Nächste einzelne Einheit: SD41 Boot/Recall/Seed. Hier nicht starten.
