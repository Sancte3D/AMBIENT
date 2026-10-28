# AMBIENT — vollständiger Sound-Softwarekandidat

Technischer Ausbau vom 28.10.2026: Auf die Parametergrundlage folgt der
[Effekt-/Session-Kandidat](SOUND_SESSION_CHECKPOINT.md) mit Delay-Zeiten und
Feedback, Grain-Länge, Phaser-Tempo sowie großem asynchronem Scene-/Loop-
Speicher. Die Encoderbelegung ist weiterhin ausdrücklich zurückgestellt.

Aktuelle Weiterentwicklung vom 27.10.2026: Michalis fordert nach negativem
V1-Hörurteil extrem deutlich unterscheidbare Worlds bei erhaltenem Ambient-
Charakter. Der bestehende vollständige Kandidat verwendet nun
[Materialmodell 2](WORLD_MATERIALS_V2_CHECKPOINT.md) mit evenreicher Fläche,
langem angeregtem Körper und oddreichem Atemton. Die Beschreibung der ersten
Fassung unten bleibt historische Basis; neue Dateien heißen ausdrücklich V2.
Eine persönliche Klang-/Gerätefreigabe wird daraus nicht abgeleitet.

Auftrag vom 26.10.2026: „okay. dann sound fertig machen“. Dieser Auftrag
umfasst jetzt alle noch benannten Softwareblöcke; nach der Flächenstufe wird
nicht wieder mit einem bloßen „nächster Schritt“ aufgehört. Basis ist PR186,
Commit `c974584ab24dcb51696d85888ce4125deda20d6f`, Tree
`3d8da53ccc6487dadc2e33fc6e99922f3f4c64e9`.
Branch `codex/ambient-sound-completion-2026-10-26`.

Ergänzung vom 27.10.2026: Auf PR187 folgt der ausdrücklich beauftragte Ausbau
der optionalen [leisen Naturkulissen](NATURE_ACCENTS_CHECKPOINT.md).
Nature0 lässt den hier beschriebenen musikalischen Hörstand exakt erhalten.

## Fertig implementierte Softwareblöcke

| Block | Tatsächlich eingebaut |
|---|---|
| Fläche | COAST erhält als Alternative eine weiche siebenfach harmonische Sinusquelle mit langsamerem Attack. Sie ersetzt die vorherige Sägezahn-/Bogenberechnung im selben Besitzer; kein hörbares Zusatzinstrument und kein Noise-Layer. |
| Lange Resonanz | WOODLAND benutzt den vorherigen eigenständigen langen Modalersatz. Entfaltung, World-Charakter, Besitz und die globalen drei Quellplätze bleiben bestehen. |
| Stimmführung | Die drei Grammatiken können dieselbe Tonklasse in einer näheren zugelassenen Oktave vorschlagen. Die echte gemeinsame Harmonie-/Tailzulassung entscheidet weiter; erst ein Audio-Start-Ack merkt das tatsächlich gehörte Voicing. |
| Gestenloop | Eigene Wiedergabebesitzer 8/14, aufgezeichnetes Register und 7-Bit-Velocity, gebundene offene Noten über den Looprand, getrenntes Stoppen/Leeren. Keine Freigabe einer Live-/Shift-Cell durch Playback. |
| Raum | Eigenständige Float-Alternative mit vier Eingangsdiffusoren und zwei gekreuzten Allpass-/Delayzweigen. Dattorro-inspiriertes Prinzip, eigener vereinfachter Entwurf und eigene Längen, kein behaupteter Originalport. Vorheriger FDN-Raum bleibt als Alternative. |
| Weitere Effekte | Ein gemeinsamer Stereo-Delay, zwei überlappende untransponierte Grains, Freeze und optionale weiche Sättigung, Bitfärbung und Phasentextur. Nature wird erst danach zugemischt. |

Tonitor 0.56 bleibt die untersuchte Anregung, mit dem bereits bestätigten
Archivhash `9c52dcfebb94058bb0476763aaf4649603cd384015b1d8bf0aeeb1b51e275e1a`.
HIVE, DRUID, Reverb, Delay, Grains und Ereignislooper liefern Prinzipien.
Es wurden keine Tonitor-Sourcen, Tabellen oder Fonts kopiert. Die neuen
Module sind eigenständiger Float-Code.

## Aktivierung und Reproduzierbarkeit

`FAM_SOUND_FINISH_CANDIDATE=ON`, Modell 1, ist ein ausdrücklicher neuer
Hörbuild. Er aktiviert auch World-Charakter und WOODLAND-Modalersatz. Der
Reference-Default und der bisherige Product-Build ohne den neuen Schalter
bleiben erhalten. Ein neues UI, Encoderbelegung oder Scene-Format wird
dadurch nicht beschlossen. SCN7 speichert die neuen Effektmakros und Loops
noch nicht; das ist kein behauptetes dauerhaftes Loop-/Presetformat.

```sh
python3 tools/review_sound_finish.py build-sound-finish
python3 tools/review_ui_idle.py build-sound-finish/ui --fixture-only --finish-candidate --sanitize
cmake -S . -B build-h743-sound-finish -DFAM_TARGET=h743 -DFAM_SOUND_PROFILE=product -DFAM_SOUND_FINISH_CANDIDATE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-h743-sound-finish
python3 tools/check_product_link.py build-h743-sound-finish
python3 tools/device_capture.py build-h743-sound-finish
```

Der Review vergleicht OFF mit dem exakten PR186-Stand, sowohl mit dessen
vorherigem Character-/Modal-Hörstand als auch neutral. Er prüft vollständiges
ON-PCM und tatsächliche Ereignistraces zwischen 37-/512-Frame-Partitionen.
Quellhashes sind vor und nach der Ausführung gleich, sonst schlägt er fehl.

## Loop- und Harmonievertrag

128 Ereignisse à 8 B. Wiedergabe belegt maximal zwei eigene Besitzer unter
derselben globalen Drei-Stimmen-Grenze. Ein volles Register/Pool oder eine
harmonisch unpassende Überlagerung lehnt einen Start ab; sein späteres
Off-Ereignis kann keine live gespielte Note lösen. Eine bereits gehaltene
Wiedergabenote bekommt beim Looprand keinen weiteren Anschlag.

SHIFT+HOLD im neuen Hörbuild: aufnehmen → spielen → stoppen → wieder spielen.
Clear entfernt die Aufnahme. Ein großer verpasster Zeitabschnitt führt
höchstens zum Abarbeiten eines Endes und der aktuellen Schleife, maximal
256 Ereignisse pro Tick. Es gibt keine unbeschränkte Nachholschleife.
Es bleibt eine Ereignisaufnahme, keine Audioaufnahme oder Overdub-Bank.

## Raum-, Effekt- und Speichervertrag

Die beiden Räume werden am tatsächlichen Sample gewechselt: 40 ms Return
ausblenden, alte Speicher löschen, neuen Return einblenden. Der Zeitpunkt
hängt nicht von der vom Aufrufer gewählten Blockgröße ab. Der bisherige
FDN-Kern bleibt ohne den Kandidatenschalter bytegleich hörbar.

Die Grains laufen mit Lesegeschwindigkeit 1 und ohne Rückkopplung. Der
Delay hat feste Zeiten von 210/267 ms und feste begrenzte Rückkopplung.
Der Standard ist zurückhaltend; Crush und Phasentextur starten bei null.
Alle Makros akzeptieren nur endliche, begrenzte Werte. Hauptparameter werden
im Main-Pfad gesetzt, Audio übernimmt begrenzte Snapshots und Rampen.

Frozen-Grain-Inhalte halten ihre tatsächlichen alten Tonhöhen in der
Harmoniehistorie. Ein normaler 7,2-/14-s-Timer darf sie nicht einfach
vergessen. Nach Freigabe des Freeze wird deren Deadline konservativ neu
gesetzt. Für den erweiterten Bus gilt ein 14-s-Horizont ab wirklichem
Quellenende beziehungsweise Freeze-Ende, zusätzlich zur gemessenen Ruhe.
Clear/Mute und fertig ausgeblendetes Dry löschen alle Effektinhalte;
erneutes Aktivieren kann keine alte Grain-/Delay-/Plateaufnahme wiederholen.

Bestehender Raum plus neuer Bus meldet im tatsächlichen C-Zustand
268.924 B Speicher. Auf H743 gehört die gesamte `ambient_room.c`-BSS in D2
mit 288 KiB; ARM-Link und Adress-/Stackaudit entscheiden über den realen
Build. Das ist keine gemessene Echtzeit- oder Stack-High-Water-Freigabe.

Transponierte Grains und Chorus mit Pitchbewegung gehören nicht zu diesem
harmonisch begrenzten Hörstand. Die vorhandenen Filter/Color, neue Phasen-
textur und optionale Sättigung decken die vorgesehenen Klangfarben ab.

## Verifikation und Hördateien

Die neuen Fixtures prüfen native und mit ASan/UBSan:

- Echte 45-s-Engineausgabe in allen Welten mit Freeze-, Raum-, Dry- und
  Clear-Änderungen; vollständiges PCM zwischen 37/512 Frames exakt.
- Beide Raumvarianten und maximale Effektmakros, hörbares tatsächliches
  Signal, begrenzte Ausgänge und abgeklungene 14-s-Reste.
- Samplegenaue Modellwechsel auch bei Änderungen auf ungeraden Frames,
  Freeze über die übliche Taildauer, Dry ohne Wiederbelebung und NaN-Rejektion.
- Tatsächliche Loopbesitzer, Register/Velocity, Ties, Live-Isolation,
  Ablehnung unzulässiger Überlagerungen, Zeit-Wrap und begrenztes Nachholen.
- 48 laute Gesamtfälle und zusätzliche 18 Minuten echte generative Musik
  über zwölf World-/Key-/Collection-/ET-JI-Fälle; gehörte Hz bleiben zugelassen.
- Die echte extrahierte H743-Bedienlogik mit neuer Loopinitialisierung:
  Aufwachen, physische Entfaltung und Scene-Overlay-Freigaben.

`AMBIENT_Sound_Complete_Candidate.wav` ist 96 s lang: COAST 00:00,
WOODLAND 00:32, HIGHLANDS 01:04. Je 32-s-Passage läuft der wirkliche
Generator 24 s und klingt dann aus. Seed1234, Master0,75, Room0,5, Nature0.
Kein externes Arrangement, Dateigain, AGC oder künstlicher Schlussfade.
`AMBIENT_Sound_Complete_AB.wav`: 192 s, je Welt 32 s vorher / 32 s Kandidat.
Hashes und aktuelle Messergebnisse stehen in `SOUND_FINISH_METRICS.json`.
Eine zusätzliche MP3 ist nur eine kleine Hörkopie derselben PCM-Ausgabe.

## Was noch zur endgültigen Abnahme gehört

Die benannten Ausbau-Softwareblöcke haben damit eine zusammenhängende
Implementierung. Ab hier folgt keine weitere offene DSP-Bauliste als
Voraussetzung für die nächste Hörprobe. Offen bleiben die Entscheidung über
den Gesamtklang durch Michalis und die echten H743-Zeit-/Stack-/Ausgangs-
und Lautsprecher-/Kopfhörertests. Alte SD-Hör-/Gerätegates werden nicht aus
Softwaremessungen pauschal geschlossen. Scene-Journal bleibt OFF, Live-Save
synchron. Kein Merge, Retarget oder physisches Flashen.
