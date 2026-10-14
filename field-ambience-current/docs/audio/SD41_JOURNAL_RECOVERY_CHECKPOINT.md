# SD41 — Vorbereitete schrittweise Journal-Recovery und Save-Retry

Stand2026-10-14. Diese einzelne Arbeitseinheit ergänzt den vorbereiteten
Journal-Schreiber um Recovery ohne synchronen Vollscan in einem Main-Step.
Die neue Komponente ist separat, noch ohne Scene-/H743-/CMake-Anschluss.
Bestehendes Scene-Read/Live-Save bleibt synchron; Journal bleibt OFF und SD41
offen. Kein neuer Klangentscheid oder DSP-/Grammatik-/Gainwechsel.

Basis: [PR165](https://github.com/Sancte3D/AMBIENT/pull/165), Branch
`codex/ambient-async-journal-2026-10-14`,
HEAD `6c380a62be0a3fd54e1725995ad619de9ab51396`,
Tree `dcd0de756eaded02658b22eb975fc4df02139919`.
Dieser Basisrun38522794416/710 wurde inzwischen frisch als completed/success
mit allen16 erfolgreichen Jobs gelesen. Alle253 bisherigen Core-/Header-/
Startup-/Linker-/CMakeinputs sind bytegleich, darunter251 Laufzeitinputs und
die zwei noch unverbundenen Writerinputs. Neue Recoverydateien sind ebenfalls
nicht in die Firmwareimages integriert. Keine Änderung an JSC1 oder SCN7.

## Aufruf- und Besitzvertrag

`scene_journal_recovery_configure` ist nur bei ruhender Komponente erlaubt.
Begin verlangt dieselbe gültige Basiskonfiguration wie die synchrone Recovery,
macht kein I/O und sperrt `initialized`. Während BUSY besitzt die Recovery
Journal, I/O-Konfiguration und Medium exklusiv: keine anderen Reads/Writes,
Recovery oder Reconfigure; keine Veränderung des Mediums durch einen zweiten
Writer. Ein abgewiesenes Begin verändert die beiden Objekte nicht.

Der bestätigte Metadatenstand und das ganze Basisstaging bleiben während BUSY
erhalten. Der gefundene Kandidat liegt nur im128-B-Hostzustand des Scanners;
ein privater32-B-Puffer, kein Heap und kein512-B-Autostack. Writer-Begin lehnt
den unverfügbaren Cache ohne versteckte Recovery ab. Erst nach dem ganzen
erfolgreichen Scan werden Gewinner oder gültiges leeres Journal veröffentlicht.
Bei leerem Journal bleiben wie im bisherigen Scanner alte unbenutzte
Metadatenfelder bedeutungslos; `has_record=false` ist maßgeblich.

Nach jedem erfolgreichen Scan gilt `fresh_sector_required=true`, `next_slot=0`.
Ein neuer Write löscht daher zuerst den anderen Sektor; mit keinem Journal
zuerst Sektor0, damit rohes Legacy in Sektor1 erhalten bleibt. Ein fehlgeschlagener
oder abgebrochener Scan lässt den Cache unverfügbar und veröffentlicht keinen
Teilfund. Cancel wird vom nächsten Step ohne Read beendet; hier bleibt nach
Rückkehr aus einem Read keine geliehene Hardwareoperation in flight.

## Begrenzte Prüfung

Ein Step hat höchstens einen Read von maximal32 B. Header-CRC prüft28 B;
Payload-CRC arbeitet inkrementell mit höchstens32 B, letzter Teil nur seine
tatsächliche Länge. Keine Record-/Sektor-/Waitschleife. CORRUPT überspringt den
Record, ERROR beendet den Scan. Der Callback muss selbst rasch zurückkehren
und ECC/unverfügbare Reads sicher klassifizieren; keine Geräte-WCET-/IRQ-
oder CPU-Fault-Garantie aus dieser Softwaregrenze.

Unveränderte JSC1-Gültigkeit: Version1, Header/CRC/reservierte Felder,
Payloadlänge1–512 und CRC, Scanreihenfolge Sektor0→1. Serialnumber-Rollover,
duplizierte Sequenzen und exakte Halbintervallgrenze wie bisher.544-B-Records,
240 pro128-KiB-Sektor;512 B Rest am Sektorende werden nicht als Record gelesen.
Voll belegte512-B-Records benötigen maximal8160 einzelne Reads; die im Modell
nach500 Saves verbleibenden260 Records benötigen4640 Reads einschließlich
aller480 Header. Das ist ein Arbeitsumfang, keine reale Flashdauer.

## Modell- und Protokolltests

| Gruppe | Beleg |
|---|---|
|Insgesamt|2.239.290 Checks,7288 vollständige Gegenproben zum vorhandenen synchronen Scanner|
|Power-cut und Retry|4983 Fälle:3894 Program-Byteprefixfälle bei acht Längen/Append/Rollover plus1089 Eraseprefixfälle; danach neue Recovery und tatsächlicher async Writer-Retry ohne Reprogrammierung unsicherer Words|
|Korruption|Jeder Byteflip in vier544-B-Records einschließlich unbenutztem Padding; gleiche erfolgreiche Auswahl/ganze Nutzbytes wie alter Scanner|
|Authentifizierte ungültige Header|Neue gültige Header-CRC bei kaputter Magic/Version, Länge0/513/UINT32_MAX oder reservierten Feldern; sicher verworfen, kein Read außerhalb des Records|
|Klassifizierte Reads|ERROR und CORRUPT an allen35 Readpositionen eines gemischten Vier-Record-Scans; nach ERROR Writer-Begin gesperrt, ausdrücklicher neuer Scan erfolgreich|
|Cancel|Alle35 yielding Positionen desselben gemischten Scans; kein Read nach Cancel, kein Teilcache, neuer ausdrücklicher Scan möglich|
|Grenzen und Besitz|Begin/abgewiesenes Begin ohne I/O; Cache/gesamtes Basisstaging während BUSY exakt, Recovery ohne Flashmutation; leeres/zero/Legacy-Medium und unvollständige Geometrie|
|Format und echte Geometrie|Acht Längen1/31/32/33/164/364/368/512,128-KiB-Sektoren/500 Saves, Sequenzwrap/-ties/Halbintervall, ganze decodierte Payloads gegen den bisherigen Reader|

Host C11/O2/Wall/Wextra/Werror besteht. Dasselbe Modell besteht zusätzlich mit
ASan/UBSan; LeakSanitizer im Hostlauf ausgeschaltet. Die128 B sind gemessene
Hoststrukturgröße, keine Laufzeitmessung eines angeschlossenen Geräts.

## Tatsächliches Product-Audio durch die komplette Folge

Zwei verschiedene echte368-B-SCN7-Snapshots entstehen vor der Aufnahme durch
Scene-Save0/1. Das erste Blob liegt in einem echten modellierten JSC1-Record.
Die Aufnahmen benutzen die reale Product-Engine und Scene/Menu/Params/Controls;
die nachfolgende Speicherfolge ist ausschließlich im Harness verbunden.
Keine bereits integrierte asynchrone Scene- oder Displaybestätigung.

Anforderung bei2,7s,12s komplette Aufnahme pro Fall. Drei Worlds, D-Major/Equal,
Seed1234, Room/Color/Attack/Release1, Activity0.5, Nature0 und Master0.6.
Main-Ticks10ms, modellierte Erase/Programoperationen jeweils20 Busy-Polls.
Pro Tick höchstens ein I/O, der alte bestätigte Record bleibt bytegleich.

| Folge | Recovery-Steps | Writer-Steps | Recovery fertig | Save fertig |
|---|---:|---:|---:|---:|
|Direkte Recovery→Save|492|333|7,610s|10,940s|
|Erster Programword bei17 B abgebrochen→Recovery→Retry|492|355 einschließlich22 fehlgeschlagener Erstwrite-Steps|7,830s|11,160s|
|Read5 scheitert→neuer Scan→Save|497 einschließlich fünf Fehlversuch-Steps|333|7,660s|10,990s|

Jede Folge vergleicht No-work512 gegen Work64/512, in jeder World: neun
matched Triplets mit drei geteilten Baselines.21 vollständige Render/252s,
11.113.200 Stereo-Frames und ganze Hookrecords per direktem `memcmp` exakt.
Quellen≤3, Nonfinite/Limiter0, hörbare echte Starts. Nach Abschluss decodiert
der vorhandene Reader das neue SCN7 ganz korrekt, neue Sequenz1 in Sektor1.
Das sind getrennte12-s-Anfänge, keine vollständigen musikalischen Langphrasen.
Die aufgeführten Sekunden sind absichtlich modelliert, keine Geräteflashzeiten.

## Reproduzierbarkeit und nächste einzelne Einheit

`test/run_tests.sh` enthält Modell und ganze PCM-Folgen. Der neue Audit
`tools/review_journal_recovery.py` prüft alle253 alten Inputs gegen PR165 und
erzeugt [SD41_JOURNAL_RECOVERY_METRICS.json](SD41_JOURNAL_RECOVERY_METRICS.json).
7244 B, SHA256
`ac24bb9091e01d80bf321406b174319523b38e8d7e5b20fe2f1614130ac43cd5`.
CI ergänzt einen separaten Cortex-M7-Objekt-/Stackreport. Bestehende Host- und
Reference/Product/Journal-Links bleiben; das neue Objekt ist noch unverbunden.
Tatsächlich beobachteter eigener CI-Stand wird im Draft-PR/Recovery dokumentiert.

Nächste einzelne SD41-Einheit: Scene-Save-API mit ausstehender Transaktion,
RAM-Snapshot und erst nach Rückleseprüfung bestätigtem Abschluss. Danach sicherer
H743-Start/Poll/Read-/ECC-Anschluss und physische Power-cut-/Geräteabnahme.
Diese Folgeeinheiten wurden hier nicht begonnen. Kein Scene-/UI-/H743- oder
Storage-Default aktiviert;24/54 Hauptpunkte geschlossen,30 offen,11 technische.
