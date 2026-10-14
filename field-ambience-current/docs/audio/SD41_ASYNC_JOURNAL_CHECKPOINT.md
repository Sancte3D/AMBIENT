# SD41 — Vorbereiteter schrittweiser Main-Journal-Schreiber

Stand2026-10-14. Diese einzelne Arbeitseinheit bereitet den nichtblockierenden
Schreibalgorithmus samt Prüfungen vor. Die neue Komponente ist noch nicht an
Scene-Frontend, HAL oder Firmware-CMake angeschlossen. Bestehendes Live-Save
bleibt synchron, `FAM_SCENE_JOURNAL_CANDIDATE` bleibt OFF. SD41 bleibt offen.
Kein neuer Klangentscheid, keine DSP-/Grammatik-/Gainänderung.

Basis ist [PR164](https://github.com/Sancte3D/AMBIENT/pull/164), Branch
`codex/ambient-boot-recall-2026-10-13`,
HEAD `6e5be3d663a54d3e1e08d77683b23563fae0da09`,
Tree `ff83bb3d6186bbab8c7d76b79d6c864c6609e020`.
Alle251 bisherigen Core-/Header-/Startup-/Linker-/CMakeinputs bleiben bytegleich.
Die zwei neuen Komponentendateien werden separat für Host und Cortex-M7 geprüft.
Der ARM-Objektcheck integriert sie nicht in die vorhandenen Firmwareimages.

## Aufrufvertrag

`scene_journal_async_configure` wird nur bei ruhender Komponente aufgerufen;
es ist keine Abbruchfunktion. Der bestehende Journalcache muss vorher recovered
sein. Während BUSY besitzt der Schreiber ihn exklusiv: keine parallelen
synchronen Reads/Writes/Recovery/Configure. Ein abgewiesenes Begin verändert
weder aktive Transaktion noch Cache. Begin kopiert maximal512 B einschließlich
überlappendem Eingang und berechnet begrenzte CRCs; es macht kein I/O und keinen
Scan. Staging hat544 B, keinen Heap; gesamte Writerstruktur auf Host672 B.

Jeder Step hat höchstens einen Start-, Poll- oder32-B-Readcallback. Ein Callback
muss selbst rasch zurückkehren; diese Softwaregrenze ist keine gemessene
Geräte-WCET. Ein erfolgreicher Start leiht den32-B-Puffer bis zum terminalen
Poll. BUSY bedeutet Hardware läuft; DONE/ERROR und ein abgewiesener Start
müssen ruhende Hardware bedeuten. Der Adapter muss Reads als sicher/Fehler
klassifizieren; ein unbehandelter H743-ECC-Fault erfüllt diesen Vertrag nicht.

Cancel setzt zunächst nur die Absicht. Solange ein Poll BUSY liefert, bleibt
auch die Komponente BUSY und lehnt weitere Begins ab. Kein Timeout gibt den
geliehenen Puffer vorzeitig frei. Erst bei ruhender Hardware wird Cancel als
FAILED gemeldet. Nach versuchter Mutation wird der Cache ungültig; der nächste
Begin wartet auf ausdrückliche Recovery. Ein Cancel vor dem ersten I/O erhält
den gültigen Cache. Ein physisch vollständiger Commit kann nach Cancel oder
späterem Lesefehler durch Recovery gültig gefunden werden: FAILED ist keine
Garantie, dass auf Flash nichts gespeichert wurde.

## Speicherung und Prüfung

Unverändertes JSC1:32-B-Header, maximal512-B-Payload,544-B-Records,
Little-endian-Felder und bestehende CRCs/Padding. SCN7 bleibt368 B.
Ein neuer/fresh Sektor wird einzeln gestartet und gepollt. Jedes Payloadword
wird einmal programmiert und vor Commit zurückgelesen; Headercommit zuletzt.
Danach werden Header und sämtliche programmierten Payloadwords einschließlich
Padding erneut geprüft. Erst der letzte erfolgreiche Read veröffentlicht den
neuen Cache. Jeder BUSY-Schritt erhält den ganzen zuvor bestätigten Cache.
Der erste Journalwrite schützt vorhandene rohe Legacybytes in Sektor1.

## Gerichtete Tests

| Prüfumfang | Ergebnis |
|---|---|
|Modell insgesamt|1.126.722 Checks,5252 gerichtete Fehlerfälle,79 Abbruchzustände,511 abgeschlossene Saves|
|Program-Abbrüche|Jeder Byteprefix0–32 bei jedem Program; acht Payloadlängen1/31/32/33/164/364/368/512, Append und Rollover|
|Erase-Abbrüche|Jeder Prefix0–1088 im Zwei-Record-Sektormodell; alter gültiger Datensatz bleibt recoverbar|
|Cancel|Jeder yielding Zustand einschließlich finalem Commit; Drain erhält Puffer und Busy-Sperre|
|Lange Hardwarewartezeit|100.000 Busy-Ticks plus256 Cancel-Ticks; keine neue Transaktion bis terminalem Poll|
|Fehlermeldung/Rückleseprüfung|Abgewiesener Start, Pollfehler, gelogenes DONE, Readfehler/-korruption; Korruption nach physischem Commit wird vor Cachepublikation erkannt|
|Format und Fortschritt|Ganzes Flash bytegleich zum synchronen Writer für acht Längen; echter Sequenzwrap;128-KiB-Sektoren,240 Records/Sektor,500 Saves und drei async Erases|
|Eingang und Besitz|Keine I/O/Recovery im Begin; ursprünglicher Callerbuffer darf nach Begin verändert werden; geliehenes Programword bleibt stabil|

Host C11/O2/Wall/Wextra/Werror besteht. Das Modell besteht zusätzlich mit
ASan/UBSan; LeakSanitizer ist im Hostlauf ausgeschaltet. Der Writer nutzt keinen
Heap. Modellfaults belegen den Algorithmus, keine physische ECC-/Power-cut-Abnahme.

## Tatsächliches Product-Audio

Die drei Worlds laufen mit realer Product-Engine und Scene/Menu/Params/Controls.
Ein echter SCN7-Snapshot wird vor der Audioaufnahme erzeugt. Der vorbereitete
Journalwriter wird bei2,7s angefordert. Das ist keine Prüfung einer bereits
integrierten asynchronen Scene-Bestätigung.

Pro World: kein Write bei512 gegen async Write bei64 und512, jeweils ganze8s
Audio und sämtliche Hooks per direktem `memcmp`. Neun Render ergeben72s,
3.175.200 Stereo-Frames. D-Major/Equal, Seed1234, Room/Color/Attack/Release1,
Activity0.5, Nature0, Volume0.6. Quellen≤3, keine Nonfinite-/Limiterfälle;
Legacy-Sektor1 bleibt während aller Busy-Schritte erhalten.

Das NOR-Modell liefert20 Busy-Ticks pro Erase/Program bei10-ms-Main-Ticks.
Jede368-B-Transaktion braucht333 Steps und13 Programwords; Abschluss bei6020ms,
also3320ms nach Anforderung. Die vollständigen PCM-/Hookverläufe bleiben exakt.
Diese Dauer ist absichtlich modelliert, keine reale Flashlatenz oder IRQ-Prüfung.

## Reproduzierbarkeit und nächste einzelne Einheit

`test/run_tests.sh` enthält beide neuen Tests. `tools/review_async_journal.py`
prüft zusätzlich die251 alten Firmwareinputs gegen PR164 und erzeugt
[SD41_ASYNC_JOURNAL_METRICS.json](SD41_ASYNC_JOURNAL_METRICS.json).
Die2673-B-Metrikdatei hat SHA256
`2beb4d6bdf026f979103cc07aed1fc5c63eea601865e02602ced67130a9bfb8c`.
CI ergänzt den Audit und einen eigenständigen Cortex-M7-Objekt-/Stackreport;
die bestehenden Host- und Reference/Product/Journal-Firmwarechecks bleiben.
Der tatsächlich beobachtete CI-Run steht im Draft-PR und im Recoverystand.

Nächste einzelne SD41-Einheit: begrenzte, schrittweise Recovery. Der bisherige
synchrone Vollscan darf beim Retry keinen Main-Stall erzeugen. Erst danach
folgen Scene-Frontend mit Pending/Bestätigung und H743-Start/Poll/sicheren Reads,
ECC-/Power-cut-/Geräteprüfung. Keine dieser Arbeiten ist hier als erledigt markiert.
Die Hauptzählung bleibt24/54 geschlossen,30 offen,11 technische Hauptblöcke.
