# SD41 — Scene speichern: läuft, bestätigt oder fehlgeschlagen

Stand2026-10-14. Einzelne Einheit nach der schrittweisen Journal-Recovery:
echte Scene-Save-API mit unveränderlichem SCN7-Snapshot und bestätigtem Abschluss.
Product-only, separat mit `FAM_SCENE_SAVE_ASYNC_PREPARED` vorbereitet. Kein
UI-/H743-/CMake-Anschluss, kein neuer Default; bestehendes Live-Save bleibt
synchron, Journal OFF. SD41 und Geräte-/Hörabnahmen bleiben offen.

Basis [PR166](https://github.com/Sancte3D/AMBIENT/pull/166), HEAD
`6c1316bea9dc5ced80dafcde29872ac4eb2b784e`, Tree
`783a3d57f24a6be388b6428aa4b2d0728b8758c6`. Basisrun38633041563/712 wurde frisch
completed/success mit allen17 erfolgreichen Jobs gelesen. Eigener Branch:
`codex/ambient-scene-save-2026-10-14`; aktuelle CI-Evidenz im Draft-PR/Recovery.

## API und Besitz

`scenes_save_async_configure(writer,recovery)` verbindet ein ruhendes Paar am
selben Journal. Beide Objekte, Medium und IO bleiben gültig bis zum Detach;
Configure/Detach/Init verwerfen keine laufende Operation. `NULL,NULL` löst ein
ruhendes Paar. Auch ein extern gestarteter Writer darf nicht verlassen werden.
Während BUSY besitzt die API Journal, Writer, Recovery und Medium exklusiv.
Keine zusätzlichen synchronen Journal-/Backend-Aufrufe oder Reconfiguration.
Nur Main; keine Audio-/IRQ-Aufrufe. Callbacks müssen rasch zurückkehren und
geliehene Words bis zum terminalen Poll halten.

`scenes_save_begin(slot,now_ms)` bedeutet **angenommen**, niemals **gespeichert**.
Kein I/O: scenes.c kopiert seinen bestätigten Store in einen festen368-B-
Pendingstore, übernimmt aktuelle Menüwerte, Collection, Brightness und Seed,
berechnet dieselbe SCN7-CRC und startet Writer oder Recovery ohne Scanloop.
Spätere Regleränderungen verändern diesen Snapshot nicht. Kein dupliziertes
Wirelayout im Controller, kein Heap oder ganzer Store im automatischen Stack.

`scenes_save_step(now_ms)` führt höchstens einen Start/Poll/Read aus, Read≤32B.
Nach erfolgreicher Recovery beginnt der Writer erst im nächsten Step, ohne
zusätzlichen I/O. Snapshot-/Begin-/Publikationsarbeit ist fest auf≤512B begrenzt;
dies ist keine gesamte32-B-CPU-Arbeitsgrenze oder gemessene Main-/IRQ-WCET.
Erst Writer-DONE nach vollständiger Rückleseprüfung publiziert bestätigten Store,
aktive Scene und UI-Zeitstempel. Kein Engine-/Noten-/Owner-/Room-Aufruf beim Save.

| Rückmeldung | Bedeutung |
|---|---|
|IDLE|Kein Save angenommen; Slot−1|
|BUSY / Speichern läuft|Snapshot geschützt, bestätigte Slots/aktive Scene unverändert|
|DONE / gespeichert|Ganzer Write zurückgelesen und bestätigt, RAM/aktive Scene veröffentlicht|
|FAILED / Speicherfehler oder Abbruch|Kein neuer RAM-Stand bestätigt; Fehlergrund und erforderliche Recovery sichtbar|

`scenes_save_info()` liefert Status, Fehler, Slot, Request-/Endzeit, Cancel und
`storage_uncertain`. DONE/FAILED bleiben bis zu neu angenommenem Begin oder
ruhigem Configure stehen. Abgewiesene Aufträge verändern Ergebnis oder laufende
Arbeit nicht. Während BUSY lehnen die vorhandenen synchronen Scene-Save-/Recall-
APIs ab; Init ist ein No-op. Bestehende UI wird hier nicht umgebaut oder umgeleitet.

Cancel bleibt BUSY, bis geliehener IO quiescent ist; kein Timeout gibt seinen
Puffer frei. Der bestätigte RAM-Stand bleibt bei Fehler/Cancel alt. **Physisch
kann ein vollständiger Commit trotz fehlgeschlagener Bestätigung existieren.**
`storage_uncertain` bedeutet fehlende Cacheverfügbarkeit, keine garantierte
Rücknahme des Flashwrites. Ein neuer Versuch prüft dann schrittweise das Journal
und nimmt einen **neuen** aktuellen Scene-Snapshot. Die erfolgreiche Recovery
schützt ihren tatsächlich gültigen neuesten Record; ein Retry darf danach den
anderen Sektor löschen, auch wenn die alte RAM-Scene noch nicht aktualisiert ist.

## Native Scene- und Fehlerprüfung

C11/O2/Wall/Wextra/Werror und ASan/UBSan-Modell bestehen; LeakSanitizer aus.
337 gerichtete native Scene-Fälle,283.584 Checks; SCN7 exakt368B und JSC1 gleich.

| Gruppe | Tatsächlicher Beleg |
|---|---|
|Snapshot/Bestätigung|Alle fünf Slots, neuer und bereits belegter Slot; Regler-/World-/Seedänderungen nach Begin verändern den gespeicherten Snapshot nicht; exakter Vergleich mit bestehendem synchronem SCN7|
|Writerfehler|Alle51 I/O-Positionen beim Append, alle53 nach frischem Erase; danach echte schrittweise Recovery und Save-Retry|
|Recoveryfehler|Alle20 Reads des Acht-Record-Modells, auch nach gültigem Zwischenfund; kein Teilcache oder RAM-Commit|
|Abbruch|Alle90 Append- und95 Fresh-Writerpositionen sowie21 Recovery-/Startpositionen; Borrowed Word unverändert und gehalten bis Quieszenz|
|Späte Fehler|13 Append-Readfehler nach physisch vollständigem Commit; Flash-Recovery findet neue Sequenz, RAM bleibt vorher bestätigt|
|Besitz/Ergebnis|Konkurrierende Begins, Sync-Save/Recall, Init und Detach während BUSY gesperrt; Terminalstatus sticky, ungültige Slots/Config unverändert; extern gestarteter Writer wird nicht verlassen|
|Neuer Versuch|Tatsächlicher Retry übernimmt neue Key-/Seed-/Brightnesswerte, statt den alten fehlgeschlagenen Snapshot zu wiederholen|

Mit vorbereitetem Flag laufen außerdem bestehender nativer Scene-/Legacytest
und der volle gerichtete Boot-/Recall-/Seedtest: sechs Boots,380 Native-Fallbacks,
zehn Legacy-/20 Off-Recalls,12 Manual-/3 Failed-Save-/9 Seed-Triplets, neun
Timerwrap-Paare. Diese sind zusätzliche Regressionen, keine Hardwareabnahme.

## Ganze tatsächliche Audiovergleiche

Product-Engine/Menu/Scene/Params/Controls, G-Major/Equal, Seed1234,
Room/Color/Attack/Release1, Activity0.5, Nature0, Master0.6. Echte alte/new SCN7-
Stores;128-KiB-Sektoren,20 modellierte Busy-Polls pro Erase/Program und10-ms-
Main-Ticks. Save-Anforderung bei1,5s,12s ganze Ausgabe pro Aufnahme.

| Ablauf über die echte neue Scene-API | Abschluss bei allen Worlds/Blöcken |
|---|---:|
|Direkter Save bei verfügbarem Cache|4,600s|
|Recovery vor Save|9,750s|
|Erstes Programword bei17B abgebrochen→Recovery→Retry|9,970s|
|Recovery-Read5 fehlerhaft→neuer Versuch|9,800s|
|Abbruch bei in-flight Program→drainen→Recovery→Retry|9,970s|

15 Triplets in drei Worlds: No-work512 gegen tatsächliche API-Arbeit bei64/512.
Drei Baselines geteilt,33 Render/396s,17.463.600 Stereo-Frames. Ganze PCM und
Hookrecords per direktem `memcmp` identisch, Nonfinite/Limiter0; bestätigte Slots
bleiben bis Abschluss alt. Der vorhandene Reader decodiert den tatsächlichen
neuen Store exakt. Zwölfsekündige Anfänge, keine ganzen musikalischen Langphrasen;
die Sekunden sind modellierte Ausführung, keine gemessenen Flashzeiten.

## Default, Nachweise und nächste einzelne Einheit

254/255 bisherige Inputs sind bytegleich:250 Laufzeitinputs und vier vorbereitete
Writer-/Recoveryinputs. Nur scenes.c bekommt ausgeschaltete Hooks. Der gemeinsame
Auditguard prüft seine vollständige präprozessierte Reference- und Product-
Übersetzungseinheit ohne neues Flag gegen die genaue jeweilige Altbasis: exakt.
Auch aktuelle World-, Boot-, Writer- und Recoveryaudits behalten ihre genaue
Baselineprüfung mit ehrlichen geänderten Sourcecounts. Keine Klang-/Default-
oder CMake-/Linker-/H743-Änderung. Das neue Flag wird in keinem Firmwareimage gesetzt.

`test/run_tests.sh` enthält die neue Frontendprüfung; `tools/review_scene_save.py`
erzeugt [SD41_SCENE_SAVE_METRICS.json](SD41_SCENE_SAVE_METRICS.json),7893B,
SHA256 `a96dd968689aadb4d01dfe23063a83c3f13924fd45c44e42b3a566f02d9a2e66`.
CI ergänzt separat Cortex-M7-Objekte für Scene-Bridge/Controller/Journal/Writer/
Recovery, partiellen Link und statische Stackreports; kein Flashimage der neuen
API, keine vollständige Geräte-/transitive Stackbehauptung. Bestehende
Reference/Product/experimentelle synchrone Journalimages bleiben.

Nächste einzelne Einheit: sicherer H743-Start/Poll/Read-/ECC-Adapter für diese
vorbereiteten Komponenten. Danach physische Power-cut-/IRQ-/Geräteabnahme;
UI-Anschluss erst nach Soundabnahme.24/54 Hauptpunkte geschlossen,30 offen,11 technische.
Keine neue Natur-/UI-/Klangarbeit in diesem Checkpoint.
