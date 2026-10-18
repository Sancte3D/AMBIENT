# SD47 — doppelte Room-Löschung im Clear-Block vermeiden

Stand2026-10-18, einzelne Softwareeinheit auf PR172/Commit
`8b7044206f7c9d29323f95a03f1c9ef6041818fd`. Dessen vollständige CI ist23/23
grün; Quellen-Prep-Artefakt gegen committed/lokal geprüft. Kein Hör- oder
Gerätehaken. SD47 bleibt für das übrige abschließende Spitzenlastinventar offen.

## Tatsächlicher Befund und Korrektur

Room-Amount-null oder Dry kann dieselbe Audiohälfte wie der40-ms-Clear-/Mute-
Abschluss erreichen. Die erste Room-Löschung setzt bereits Tank, Diffusion,
Zeiger, Filterhistorien und Peak auf null. Der gesamte Engine-Clear löschte
anschließend dieselben Bytes nochmals. Reproduziert am echten Product-Kern:
14 beobachtete C-memset-Aufrufe/121440 Zielbytes innerhalb eines512er Blocks.
Ein bereits kalter expliziter Wiederholungs-Clear löschte erneut60720 Bytes.

`cold` ist bereits ausschließlich vom Audiobesitzer verwaltet und bedeutet,
dass alle durch Clear zu nullenden Felder schon null sind. Der öffentliche
Clear kehrt nun in diesem Zustand sofort zurück. Ein warmer Clear führt
weiter die vollständige Löschung aus. Initialer statischer Zustand ist
`cold=false`; der erste Init leert vollständig. Späterer Init kann nur eine
nachgewiesen bereits geleerte Arena überspringen und setzt seine übrigen
Initialwerte unverändert. Kein neuer Cache, State, Buffer oder IRQ-Maskieren.

## Messung der Arbeit, kein erfundener Zeitgewinn

| Fall | Basis | Korrektur |
|---|---:|---:|
| Room-null/Dry plus Clear/Mute, maximale Zielbytes pro Audioblock |121440|60720|
| Clear eines schon kalten Rooms |60720|0|
|20 weitere Clear-Aufrufe im kalten Zustand |1214400|0|
| notwendiger warmer Clear |60720|60720|

Gemessen werden die Zielgrößen expliziter C-memset-Aufrufe, deren Beobachter
die tatsächlichen libc-Schreiboperationen ausführt. Das sind weder gemessene
physische Busbytes noch eine Halbierung der gesamten Blocklaufzeit. Im ARM-
Produktobjekt existiert kein Beobachter. Cachemisses, Schreibpuffer, Bibliotheks-
code und übrige Audioarbeit müssen weiterhin am Gerät mitgemessen werden.

## Vollständiger Audio- und Regressionstest

36 Fälle: drei Worlds ×64/512 ×Room-null gleichzeitig/bereits kalt/Dry
×Clear/Mute. Reale Quellen und Nature1, volle Gate-/Room-Rampen, jede
Nullausgabe nach Abschluss,20 wiederholte Clears, Unmute/Neustart und erneut
warmer Room. Ganze PCM und echte Note-Hook-Dateien vorher/nachher bytegleich,
484128 Stereo-Frames je Version. Ein warmer Folge-Clear muss weiterhin sieben
beobachtete C-Aufrufe/60720 Zielbytes ausführen; dadurch wird kein bloßes
Überspringen jeder Löschung als Erfolg gewertet.

Die unveränderte alte Roomquelle scheitert mit demselben neuen Test an der
Arbeitsgrenze. Eine ausdrücklich markierte Baseline-Variante akzeptiert die
alten Grenzen und zeichnet deren echte Arbeit und dieselbe gesamte PCM auf.
C11/O2/Wall/Wextra/Werror sowie ASan/UBSan positiv. Der Test beobachtet nur das
separat kompilierte Roomobjekt. Sein Header bindet libc vor dem Makro ein,
damit optimierte/umgeleitete libc-Deklarationen den Beobachter nicht umgehen.

Zusätzlich24 ganze12-s-Product-Paare: drei Worlds ×Manual/Generate ×Nature0/1
×64/512, echte Roomänderungen/Clear/Neustart. PCM und echte Hooks bytegleich,
ganze PCM auch64/512 bytegleich;12700800 Stereo-Frames je Version in getrennten
Fällen, keine behauptete Langzeit- oder Hörabnahme.

## Tatsächlicher H743-Gegenlink

Vollständiger Product-Link plus Gegenlink mit alter Roomquelle, denselben
übrigen Objekten/Flags und echtem Linker. Alter Objektbasename bleibt
`ambient_room.c.obj`, damit die reale D2-Platzierung erhalten bleibt.

| Größe | Basis | Korrektur |
|---|---:|---:|
| Product-BIN |196836B|196844B|
| zusätzlicher State/RAM |0B|0B|
| eigener Room-Process-Compilerframe |160B|160B|
| eigener Clear-Compilerframe |8B|16B|

Flash+8B; `.bss`, `.d2_bss`, `.dtcm_bss` und `.data` unverändert. Die
Instruktionsbytes von `.text.ambient_room_process` sind exakt gleich. Nur
der öffentliche Clear gewinnt den frühen Guard; kein Atomic-Laufzeithelper
oder Testbeobachter im Produktobjekt. Der eigene Clearframe steigt um8B;
daraus keine ganze IRQ-Stackreserve oder transitive Stackänderung ableiten.
Keine gemessene Geräte-WCET, Deadline oder Audio-Lastreserve.

Nur `src/ambient_room.c` ändert sich unter263 bestehenden Firmwareinputs;
262 bleiben bytegleich PR172. Quellen, Grammatiken, kalibrierte Gains,
lange WOODLAND-Holds, SCN7 und Defaults bleiben auf ihrem Stand.

## Reproduktion und CI

Im `firmware-c-next`:
`python3 tools/review_room_clear_work.py build-room-clear-review --sanitize --arm`.
[SD47_ROOM_CLEAR_WORK_METRICS.json](SD47_ROOM_CLEAR_WORK_METRICS.json),27471B,
SHA256 `5f2b2eece9227ca88cc4db04606e6f2c9ef3d0cfc37756987a2896be9705e6a5`.

Eigener aktueller Clear-Job sowie aktuelle Host-/H743-Jobs prüfen den neuen
Stand. Die unverändert strikten Source-Handoff- und Prep-Audits laufen
ausdrücklich am exakten grünen PR172; Room-Publikationsaudit weiter PR170,
die sieben älteren Audits weiter PR169. Keine Byteguards abschwächen und
keine alten Artefakte als aktuelle Geräte- oder Hörabnahme ausgeben.
