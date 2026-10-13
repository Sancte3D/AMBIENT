# SD35 — Erreichbare Parameterkombinationen

Stand 2026-10-13. Die einzelne Softwareprüfung ist abgeschlossen. Finale
musikalische Endwertwahl, Bandpeak-/Buzz-/Pumpen-Hörurteil und Geräteabnahme
bleiben offen. SD35 bleibt unchecked und zählt jetzt zur Hörgruppe.
Exakte Basis PR161:9bde4de2646bafb027f1e76d6b7b683c32a56798,
Tree9308e26869589f4d4e30a53b433c158780a9ae5c. Dessen Gesamt-CI
38359372020/run698 ist frisch completed/success.
Alle251 Core-/Header-/Startup-/Linker-/CMakeinputs bytegleich zur Basis.
Spec0.10/SCN7/Defaults, feste Gains0,50/0,22/0,50 und Send0,35 gelten.
Reference Default, Product Kandidat, Journal OFF. Keine DSP-Kalibrieränderung.

## Aktueller Umfang der Aufgabe

Der alte SD35-Auftrag enthält inzwischen entfernte Effektregler.
Excitation/Damp, Resonance/Drive, Width/Chorus/Detune, Age/Blur und Shimmer
sind keine erreichbaren Produktkombinationen. Der Raumfeedbackwert folgt
Room mit nominalem T60=1,2+3,6*r²; es gibt keinen freien Feedbackregler.
Geprüft werden Activity×Attack×Release, Color×Room×Nature×Volume,
Dry/Room und zwei tatsächliche musikalische Pitch-Kontexte.

Acht gezielte Profile, keine vollständige kartesische Parameterfläche:

| Profil | Activity | Attack | Release | Color | Room | Nature | Volume |
|---|---:|---:|---:|---:|---:|---:|---:|
|Referenz|0,5|0,5|0,5|0,5|0,5|0|0,6|
|Schnell/kurz/hell/volle Summe|1|0|0|1|1|1|1|
|Dicht/lang/hell/volle Summe|1|1|1|1|1|1|1|
|Langsam/lang/dunkel/volle Summe|0|1|1|0|1|1|1|
|Schneller Einsatz/langer Schluss/dunkel|1|0|1|0|1|1|1|
|Langsamer Einsatz/kurzer Schluss/hell|0|1|0|1|1|0|1|
|Schnell/lang/trocken/dunkel|1|0|1|0|0|0|1|
|Langsam/kurz/trocken/hell|0|1|0|1|0|0|1|

48 tatsächliche192-s-Generatefälle: jedes Profil und jede World, einmal
D-Major/Equal/Seed1234 und einmal A-Minor/Just/Seed42. Stop bei160s,
danach32s natürliche Fahnen. Die zwei Kontexte koppeln Seed/Key/Collection/
Tuning ausdrücklich; dies ist kein isolierter Test jedes dieser Faktoren.
Zusätzlich drei Live-Generatefälle mit denselben realen Quellenautomaten.

20 manuelle112-s-Fälle: fünf Familienbelegungen (COAST3,WOODLAND2,HIGHLANDS3,
WOODLAND/WOODLAND/COAST und COAST/WOODLAND/HIGHLANDS), zwei Lagen und
die Profile dicht/lang/hell sowie schneller Einsatz/lang/dunkel. Velocity1.
Tiefe MIDI50/57/62, hohe62/66/69; die reine hohe Saite62/69, tiefe50/57.
Die Mischbelegung entsteht über reguläre Worldwahl mit erhaltenen manuellen
Quellen. Weitere harmonisch passende Eingabe nach100ms wird abgewiesen.
Key-up und Nature0 bei4s, Quellen enden natürlich; kein Clear/Mute.
Sechs weitere manuelle Livefälle: jede reine Familie in beiden Lagen.

## Ganze Audiodaten und wirklicher Besitz

77 Szenarien bei64/512 plus drei bestehende112-s-Defaultgegenproben ergeben
157 ganze WAV-Render/429,067min PCM. Getrennte Szenarien, keine kontinuierliche
Langzeit-/CPUabnahme. Jede vollständige PCM-Datei und jeder ganze Event-,
Status- und Controltrace sind blockgenau identisch. Pro Referenzworld sind die
ersten80s PCM exakt zum bisherigen Renderer; Events bis80s stimmen bei dessen
Velocity-Ausgabepräzision von fünf Nachkommastellen überein.

1049 tatsächliche DSP-Starts sind mit frischen physischen Quellenreservierungen
verknüpft. Jeder wiederverwendete Generate-Owner ist vorher wirklich retiert;
NoteOff allein gibt keinen Platz frei. Reglerwechsel entfernen keinen
physisch belegten Owner. Alle Quellen und die letzte PCM-Sekunde werden exakt
still. Global maximal3 Quellen, lokal maximal2 Saiten; keine Stimmen werden
gestohlen. Belegungsbeobachtung alle10ms im deterministischen Hostlauf,
kein Nachweis gegen beliebige Geräte-ISR-Verschachtelung.

| World | Bestätigte Generate-Hz in diesen Fällen | Quellenende nach Generate-Stop | Manuelles Ende nach Key-up |
|---|---|---|---|
|COAST|110–587,330|5,82–14,04s|14,76s|
|WOODLAND|110–293,665|0–3,04s|35,68–45,04s|
|HIGHLANDS|110–440|1,28–10,80s|11,47s|

WOODLANDs manuelle NoteOff-Regel erhält ausdrücklich ihre lange natürliche
Schwingung. Generate verwendet den gespeicherten weichen musikalischen Stop.
Ein Quellenende schon vor Stop erklärt WOODLAND-Fälle mit0s Restbelegung.
Alle bestätigten Onset-Hz erfüllen aktuelle Collection/Tuning und die
jeweiligen öffentlichen oder privaten Register; keine stille Transposition.
FFT-Bandanteile des Körpers sind protokolliert, kein Alarm-/Klangurteil daraus.

## Kombinierte Bedienänderungen

18 gleichzeitige Activity/Attack/Release-Änderungen an vorbereiteten, gehaltenen
und ausklingenden Tönen erhalten jeweils sechs Sekunden PCM exakt.
Sechs all-control Endwertgegenproben bei64/512, sechs Clampfälle und sechs
Invalidfloat/retired-API-Invarianzfälle bestehen. NaN/±Inf halten letzte Ziele;
entfernte API-Regler erzeugen keinen Klangpfad. Setter starten/stoppen keine Note.

Die neun Livefälle enthalten100 Zieländerungen pro Sekunde für2s, danach
beziehungsweise zusätzlich eine0,5-/1-s-Rampe und echte Volume0/1-Schritte.
Manuelle schnelle Änderungen liegen vor Key-up bei1–3s und3–3,5s;
Generate bei32–34s und64–65s.2469 tatsächliche Controlfenster sind gemessen.
Attack/Release gelten weiterhin erst für zukünftige Starts; geplante Activity-
Waits und laufende Holds bleiben erhalten. Color/Room/Nature/Volume glätten live.

Zwölf direkte Gegenproben vergleichen je denselben vollen tiefen/hohen Akkord
mit unveränderten Zielen gegen gleichzeitiges0→1/1→0 samt Dry/Room.
Die zusätzliche Änderung des ersten Stereo-Samples beträgt höchstens
3/32768≈0,0000916; Prüfgrenze33/32768≈0,00101. Tatsächliche Hz, Quellenplätze
und Hooks bleiben erhalten. Das ist eine kontrollierte Diskontinuitätsprobe,
keine Hörfreigabe einer gesamten Reglerfahrt.

Ein fixer Grenzwert für den gesamten natürlichen Samplegradienten würde hier
die unveränderte helle hohe COAST-Dreierwellenform beanstanden: maximal
0,106781 bei2,057s, ohne irgendeine Zielbewegung. Gradient und Controlfenster
bleiben als Diagnostik erhalten; die direkte gepaarte Probe misst den
tatsächlichen zusätzlichen Setteranteil. Helligkeit/Buzz bleiben Hörfragen.

## Pegel und Dateien

Alle77 Fälle: Nonfinite-/Limitercounter0, Monoenergie≥0,795763,
mittlerer Kanal-DC≤1,694e−8 und letzte Sekunde exakt0.
Stärkster TruePeak dieser Auswahl−6,8dBFS bei Volume1:
tiefer COAST/WOODLAND/HIGHLANDS-Akkord, dicht/lang/hell/Room/Nature1.
Die breitere SD34-Summenprüfung enthält weiterhin das globale bisher gemessene
Maximum−5,1dBTP bei Volume1 und dessen definierte Referenzgegenprobe
−9,5dBTP bei0,6. Die beiden Audits verwenden unterschiedliche Voicings/SHAPEs.

AMBIENT_Parameter_Grenzen_40s.wav enthält echte Generateausschnitte2–8,25s:

| World | Referenz | Dicht/lang/hell/volle Summe |
|---|---|---|
|COAST|0–6,25s|6,75–13s|
|WOODLAND|13,5–19,75s|20,25–26,5s|
|HIGHLANDS|27–33,25s|33,75–40s|

Ein gemeinsamer Gain+1,9dB,40-ms-Ausschnittkanten und fünf0,5-s-Lücken.
Referenzmaster0,6 und Stressmaster1 bleiben bewusst verschiedene echte
Pegelzustände. Es sind tatsächliche unterschiedliche Parameter-/Zeitverläufe;
keine identische Partitur oder Lautheitsangleichung. Keine neue Komposition.
44,1kHz/Stereo/16bit,7.056.044B,−24LUFS/−12,5dBTP.
SHA256629f1b904f2e59171a3d8f0d77fa61ceacef62375f4d61dc1b270752bd25f24d.

Daten:[SD35_PARAMETER_METRICS.json](SD35_PARAMETER_METRICS.json),
[SD35_PARAMETER_CASES.csv](SD35_PARAMETER_CASES.csv). Ganze Traces und kurzer
WAV im neuen CI-Artefakt; lange Rohdateien nur OS/tmp und anschließend entfernt.
Alle Inputs, ganze Captures, Traces und Hördatei abschließend geprüft.
Reproduktion:`python3 tools/review_parameter_combinations.py build-parameter-combination-review`.

Hörabnahme bleibt: extreme Helligkeit/Room/Nature/Release zusammen ruhig und
musikalisch sinnvoll? Zeitmarke einer störenden Bandspitze, eines Buzz,
Pumpens oder einer unerklärten Auslöschung nennen. Messwerte entscheiden diese
Fragen nicht. SD35 bleibt unchecked;24/54 erledigt,30 offen. Planung jetzt
12 technische Hauptblöcke (zehn Software-/Prüfblöcke und zwei Gerätegates),
17 Hörentscheidungen plus ein Gesamtcheck. Nächste einzelne Einheit: SD37
sechs gerichtete Worldwechsel. Diese Einheit startet hier noch nicht.
