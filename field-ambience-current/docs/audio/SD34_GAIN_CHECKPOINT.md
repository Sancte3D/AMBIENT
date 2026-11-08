# SD34 — Quellenpegel, Velocity und erlaubte Summen

Stand 2026-10-13. Die einzelne Softwareprüfung ist abgeschlossen; die finale
gehörte Lautheitsbalance bleibt ein offener SD34-Hörentscheid. Feste Gains und
Spec0.10 bleiben bestehen. Exakte Basis PR160:
a13d01949b425f3cec966573544639bf8ec49b5f, Tree
1a146116b8c966e6fed511fae149e650b8c60f65.
Alle 251 Core-/Header-/Startup-/Linker-/CMakeinputs vor/nach dem Audit bytegleich.
PR160 CI38305233488/run695 frisch completed/success.
Reference Default, Product Kandidat, Journal OFF, SCN7 unverändert.

## Erreichbarer Vertrag

Sourceinput = normierte Velocity ×0,50 COAST/HIGHLANDS oder ×0,22 WOODLAND.
Danach wirken Quellenartikulation/Panning. Gemeinsamer Send0,35, dann Room,
Nature, DC-Tracker, Master und Gate. Hookvelocity ist kein DSP-Peak.
Choir/Guembri sind im Product-Link ausgeschlossen. Keine neue Gainkurve oder AGC.

0/negative/NaN/±Inf starten keinen Ton; Werte über1 werden auf1 begrenzt.
Der Legacyadapter teilt Cell-Amplitude durch CELL_AMP_MAX0,22. Cell-Minimum0,05
entspricht dort ungefähr Velocity0,227 und ist keine untere normierte DSP-Grenze.
Echter Gain-API-Vertrag: drei Ceiling-, sechs Legacyadapter- und sechs
Manual/World-Fälle erhalten ganze vier Sekunden PCM exakt bei64/512.
15 Invalidinputs werden still abgewiesen. Zusätzlich besteht der vorhandene
Productvertrag mit12 Minuten tatsächlicher Musik. Hardware-Hall/ADC separat.

## Register und Anschlagstärke

105 vollständige Einzeltonfälle: jede manuell erlaubte Tonhöhe MIDI50..69
bei Velocity0,5, alle drei Worlds; bei MIDI50/60/69 zusätzlich
0,01/0,25/0,55/0,75/1. Key passend zur Tonhöhe, Major, Attack/Release/Color0,5,
Dry/Nature0/Master0,6. Echtes ungefadetes Stereo-PCM, Körper1,5–2,5s und spätes
Fenster8–9s. Gehalten bis Clear10s; ganze12s-Rohprobes, keine Releasebehauptung.

| World | Körper-RMS bei Velocity0,5 | Registerspanne | Spät/Körper MIDI50 /60 /69 |
|---|---|---:|---|
|COAST|−35,916 bis −35,889dBFS|0,027dB|1,001 /1,000 /1,000|
|WOODLAND|−37,487 bis −34,338dBFS|3,149dB|0,269 /0,206 /0,101|
|HIGHLANDS|−35,203 bis −35,158dBFS|0,045dB|1,001 /1,000 /1,000|

Größter Körperunterschied zwischen Quellen bei derselben Tonhöhe2,329dB.
Alle geprüften Velocities steigen streng an; kleiner Schritt0,50→0,55
ergibt0,828–0,831dB. WOODLANDs höhere Lage verliert mehr späte Energie durch
natürlichen Decay. Kein Hochregeln dieser Fahne; Loop/Envelope/Color/Besitzdauer
bleiben erhalten. RMS ist keine gleich empfundene Lautheit oder Hörfreigabe.

## Maximale Belegung und Referenzpegel

156 echte Summenfälle: alle26 geordneten zulässigen Dreifamilienkombinationen
außer drei WOODLAND-Saiten, jeweils D-Major50/54/57 und62/66/69,
Color0/0,5/1 und Velocity/Room/Nature/Master1. Dazu sechs reine Zwei-Saitenfälle.
Worldwechsel erhalten manuelle Quellen und machen die Mischbelegung erreichbar.
Nach100ms wird eine zusätzliche harmonisch passende Note abgewiesen.
Ursprüngliche Starts bleiben erhalten; global3/lokal2 eingehalten.

Stärkster TruePeak−5,1dBFS bei Master1: WOODLAND/WOODLAND/COAST,
tiefe Lage, Color/Room/Nature1. Kein Limiter greift ein.
Referenzmaster ist der unveränderte Product-Default0,6. Die stärkste gemischte
Summe und der stärkste Fall jeder reinen Familie werden vollständig erneut
bei0,6 gerendert. Alle Events/Status entsprechen den Vollpegelfällen exakt.

| Gegenprobe bei Master0,6 | True Peak | Digitale Reserve |
|---|---:|---:|
|Stärkste gemischte Summe|−9,5dBFS|9,5dB|
|COAST, drei Quellen|−13,8dBFS|13,8dB|
|WOODLAND, zwei Saiten|−11,7dBFS|11,7dB|
|HIGHLANDS, drei Quellen|−15,4dBFS|15,4dB|

SD42-Ziel≥6dB beim definierten Referenzpegel besteht auch im neu gefundenen
stärksten Summenfall. 5,1dB bei Master1 ist ein anderer Messpunkt.
Kein physischer SPL/DAC/Amp/Speakerbeleg. Die Prüfung betrifft diese Voicings
und Parameter, keinen vollständigen Sweep aller Keys/Tunings/SHAPEs.

## Vollständige tatsächliche Engine

24 vollständige112-s-Generatefälle bei64/512: pro World D-Major Seeds1234/42,
Key8/9 jeweils Major/Minor und zwei Maxfälle Color/Room/Nature/Master1.
Bestätigte reale Frequenzen: COAST110–830,609Hz, WOODLAND110–415,305Hz,
HIGHLANDS110–622,254Hz. Rollen6/7/15 und Velocities aus DSP-acknowledged Events.
Sechs volle D-Major-Defaultkontrollen entsprechen bisherigem Renderer in ganzem
PCM/Events/gemeinsamen Statusfeldern exakt. Stop80s und genau stille Enden.

267 manuelle Fälle, vier Referenzfälle und24 Generatefälle bei64/512 plus
sechs Defaultkontrollen ergeben596 ganze WAV-Render/209,2min PCM.
Getrennte Szenarien, keine kontinuierliche Langzeitabnahme.
Alle ganzen PCM/Events/Status je Fall blockgenau. NaN/Limiter0, Mono≥0,79865,
DC≤9,54e−7, letzte Sekunden exakt still. Inputs/ganze Captures/Traces/Exporte
abschließend versiegelt. RAW nur OS/tmp, danach entfernt; vollständige Rezepte,
Event-/Status-CSVs und Messdateien exportiert. Neue CI reproduziert den Audit;
historische Releaseprüfung exakt auf PR160 gepinnt.
Keine neue ganze lokale Host-/ARM-Suite behauptet: Firmware251 Inputs exakt
zum grünen PR160. Keine CPU-/ISR-/Geräte-/Hörabnahme daraus.

## Musikalischer Pegelvergleich

AMBIENT_World_Pegelvergleich_40s.wav: COAST0–13s, WOODLAND13,5–26,5s,
HIGHLANDS27–40s. Je echtes autonomes PCM1–14s, Seed1234, Defaults/Nature0.
Unterschiedliche Worldalgorithmen und Ereignisse, keine identischen Notenpläne.
Alle Teile erhalten ein gemeinsames Gain+6,9dB; Pegelunterschiede bleiben.
40-ms-Ausschnittkanten, zwei0,5-s-Lücken, keine AGC.
−24,7LUFS/−15,4dBTP,44,1kHz/stereo/16bit,7.056.044Bytes.
SHA256 1932901e4f572cd134eee82ea2da723aa8ca81ed8fe2ba1fd4fc71bdfb4d0431.

Daten: [SD34_GAIN_METRICS.json](SD34_GAIN_METRICS.json),
[SD34_GAIN_SUM_CASES.csv](SD34_GAIN_SUM_CASES.csv),
[SD34_REFERENCE_HEADROOM.json](SD34_REFERENCE_HEADROOM.json).
SD34 bleibt für finale gehörte Lautheitsbalance unchecked. In der Planung
jetzt13 technische Hauptblöcke: elf Software-/Prüfblöcke plus zwei Gerätegates.
SD34 wechselt ausschließlich zur Hörgruppe; weiterhin24/54 erledigt,30 offen.
Nächste einzelne Arbeitseinheit: SD35 kritische Parameterkombinationen.
