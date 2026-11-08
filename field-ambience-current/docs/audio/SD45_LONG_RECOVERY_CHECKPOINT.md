# SD45 — durchgehende Product-/Scene-Recoveryprüfung, 2026-10-19

Die technische SD45-Prüfung ist abgeschlossen. Zu den vorhandenen langen
Phrasen-/Stress- und einzelnen Scene-/Worldprüfungen kommt jetzt die fehlende
gemeinsame Sequenz: wiederholte World-/Scenewahl, Savefehler/Retry, beschädigter
SCN7-Store, Recovery und Neustart in **einer durchgehenden Engine-Sitzung**.
Keine Engine-Neuinitialisierung zwischen diesen Aktionen, kein Ersatz-DSP.
SD45 bleibt für die gehörte mehrminütige Form und Wiederholung offen; kein Haken
aus einem fehlerfreien Eventtrace. Technische Blöcke7→6, davon4 Software/
Abschluss und2 Gerät; Hörgruppe22→23.24/54 geschlossen,30 offen.

## Wirklicher Ablauf

`test/test_product_long_recovery.c` führt die unveränderten Productquellen,
Engine, Room, Nature sowie echten Menu-/Params-/Controls-/Scene-Code aus.
Nur der dokumentiert injizierbare native Scene-Backend ist ein512-B-RAMstore.
Ein Aufruf von `engine_init` je vollständiger800-s-Aufnahme, derselbe
10-ms-Main-Tick und fortlaufendes Audio über die gesamte Sitzung.

| Phase je120-s-Zyklus | Tatsächliche Aktion / Prüfung |
|---|---|
|0s|Gespeicherte SCN7-Scene laden, Generate starten; wechselnde Worlds, Keys, Collections, Equal/Just und zwei Seeds. Positive Nature nur explizit in gewählten Zyklen |
|40s|Activity/Color/Attack/Release/Room verändern, bestehende Quellen bleiben physisch budgetiert |
|55/56s|Während Generate native Save fehlschlagen lassen; persistierte Bytes und aktive Scene unverändert. Danach bestätigter neuer Saveversuch |
|65s|Andere reale Scene bei laufendem Audio laden |
|70s|Ein CRC-geschütztes Storebyte verändern, nur Scenes neu laden: alle Slots leer, Recall abgewiesen, kein zusätzlicher Hook. Originalstore zurückgeben, echte Slots/Scene wieder laden; Engine/DSP laufen durch |
|75/95s|GenerateStop mit Naturetarget0; bis95s alle physischen Quellen natürlich frei |
|100/103s|Erneuter GenerateStart und Stop |
|110/110,06s|Mute und Unmute; ab111s ganze Ausgabe exakt Null, keine Quelle/Roomfahne oder Clearpending |

Sechs solcher Zyklen mit wechselnden Scene-/Worldzielen pro Aufnahme. Danach
bei720s eine manuelle lange WOODLAND-Saite220Hz/Velocity0,9, Key-up722s;
bei722,1s noch wirklich aktiv. Worldwahl auf COAST bei725s erhält dieselbe
aktive Saite und ihre tatsächlichen Hz. Bis800s natürliche Retirement;
letzte Sekunde exakt still. Die lange Saite wird nicht auf einen kurzen
Generate-Stop umgedeutet. Zwischen den Aktionen findet kein Engine-Clear als
versteckter Testreset statt; die benannten Mute-Prüfungen bleiben sichtbar.

## Vollständige Kontrollen statt kurzer Ausschnitte

Zwei Seeds1234/42 × Renderobergrenze64/512 ergeben vier Parameterfälle.
Jeder läuft normal und mit Mainclockstart `0xffffe000`, der während Audio
wirklich überläuft. Pro Paar wird die **gesamte** PCM16-Stereoausgabe über eine
geprüfte temporäre Datei byteweise verglichen; komplette Hookrecords ebenso.
Länge, Read/Write/Flush/Seek/EOF/Close werden geprüft. Hashes ergänzen den
Vergleich, ersetzen ihn nicht. Lange PCM werden verworfen, keine lange WAV.

| Je Aufnahme / beide Clockvarianten | Seed1234, beide Obergrenzen | Seed42, beide Obergrenzen |
|---|---:|---:|
|Bestätigte Starts / frische physische Reservierungen|91 /91|91 /91|
|Ganze Hookrecords|188|188|
|Maximale tatsächliche Quellen|3|3|
|Native Savefehler / bestätigte Retries|6 /6|6 /6|
|CRC-Recoveries / echte Recalls|6 /18|6 /18|
|Stop / Restart / Mute-Clearaufträge|12 /6 /6|12 /6 /6|
|Größter beobachteter PCM-Samplebetrag|6477 /32768|6580 /32768|

Jeder bestätigte Start benötigt eine neue zuvor beobachtete physische
Reservierung desselben Owners; Doppelacks zählen nicht als neuer Ton.
Quellenmasken schließen sich zwischen Familien aus, global≤3 und Pluck≤2.
NaN-/Limitercounter bleiben0. Während Generate gilt eine90-s-Lückenobergrenze;
beabsichtigte Stop-/Stillephasen werden nicht als verlorene Zulassung gewertet.
Der gemessene Peak ist ein Samplepeak, kein neuer Intersample-/SPL-Nachweis.

**Je Prüfmodus8 ganze Captures/106min40s Audio**, vier ganze Wrap-Paarvergleiche
mit141120000 tatsächlich verglichenen Stereo-Frames. Native C11/O2/Wall/Wextra/
Werror und ASan/UBSan bestehen mit identischer Metrik: insgesamt16 Captures/
213min20s Audio in den zwei Prüfmodi. Die Host-Testbuffer/Dateien sind keine
Firmwarebuffer und ändern weder RAM noch Audiopfad.

Ganze Gleichheit wird für normal/wrappende Uhr **innerhalb derselben Partition**
geprüft. Gleiche64/512-Diagnosehashes in diesen Fällen sind kein kompletter
zusätzlicher Kreuzvergleich oder pauschales Partitionsversprechen: die inSD12
dokumentierte blockweise Room-Quiet-Zulassungsgrenze bleibt gültig.
Auch eine512-Obergrenze rendert auf dem10-ms-Raster441 Frames am Stück;
dies ist keine echte512er DMA-/IRQ-Abnahme.

## Bestehende Abdeckung und Grenzen

| SD45-Kriterium | Zugeordneter Nachweis |
|---|---|
|Mehrminütige Entwicklung ohne kurzen technischen Dauermotivloop|Aktueller Host `test_phrase_development.c`:9×10min tatsächliche Musik,3 Seeds und vier gehörte Entwicklungsstufen/erinnerte Rückkehr;2304 reine Key/Collection/Seedfälle×16 Phrasen. [Phrasencheckpoint](PHRASE_DEVELOPMENT_CHECKPOINT.md); musikalisch angenehme Form weiterhin hören |
|Retained-Endwerte, Mono/Headroom, Start/Stop, fehlende Zulassung|Aktueller `test_product_stress.c`:36min echte Musik plus Limits/alle sechs Wechsel/40-s-Wrap/Mute; `test_product_sound.c` weitere12min. [Parameterkombinationen](SD35_PARAMETER_CHECKPOINT.md), [Harmony](SD11_PUBLIC_HARMONY_CHECKPOINT.md), [Tailgrenzen](SD12_TAIL_HISTORY_CHECKPOINT.md) behalten ihre genauen Scopes |
|Wiederholte Worlds/Scenes/Recovery im laufenden Prozess|Neue800-s-Sitzungen oben, ganze PCM-/Hook-Wrapkontrollen; keine neue Hook- oder Engineimplementierung |
|Kaputte/alte Scene, sichere Wiederaufnahme|Neue wiederholte native CRC-/Savefehler plus [Boot/Recall](SD41_BOOT_RECALL_CHECKPOINT.md) und [vorbereitete Journal-Recovery](SD41_JOURNAL_RECOVERY_CHECKPOINT.md). Native RAMprüfung ist kein Flash-/ECC-/Power-cut-Nachweis |
|Später musikalischer Ausschnitt / gehörte Langzeitwirkung|Vorhandene kurze Entwicklungs-/World-/Nature-Vergleiche; hier keine neue WAV und keine Hörentscheidung. Eine Montage ersetzt keine tatsächliche mehrminütige Hörabnahme |

Die neue native Recoveryprüfung aktiviert weder den vorbereiteten asynchronen
Writer noch Journal noch H743-I/O. Product-Save bleibt synchron. Echte I/O-
Blockade/Power-cut/ECC und danach Productanschluss bleiben SD41; SD47 hängt
weiter an SD41/SD48. Kein Timing-/CPU-/Stack-/Ausgangs-/Hörpass aus Host-PCM.

## Unveränderter Code und Reproduktion

Exakte Basis PR178 `fe0551c3f0ca1bbf2fc7d1693a2a8d337c5e77c2`, Tree
`00fd0b61e6516d9eb45e1ac6f3bcf5e5fd8fb1ce`. Alle263 bestehenden Firmware-/
Header-/HAL-/Startup-/Linker-/CMakeinputs vor der Prüfung bytegleich.
Keine Runtime-/Gain-/Artikulations-/Schemaänderung. Reference Default,
Product Kandidat, Spec0.10/SCN7, Journal OFF und lange WOODLAND erhalten.
Bestehender Host-/ARM-Bezug bleibt vollständig grüner PR176/Run734 (27/27);
kein neuer lokaler voller Hostrunner oder gepaarter ARM-Build in dieser Einheit.

Im `firmware-c-next`:
`python3 tools/review_product_long_recovery.py build-product-long-recovery --sanitize`.
Der eigene aktuelle SD45-CI-Job prüft diese komplette Sequenz samt263-Input-
Vergleich und lädt nur [SD45_LONG_RECOVERY_METRICS.json](SD45_LONG_RECOVERY_METRICS.json)
hoch. Vorige strikte/historische Audits bleiben unverändert, Workflow nun28 Jobs.
`--fixture-only` ist ohne historische Gitobjekte möglich; dessen Beleg behauptet
ausdrücklich keinen Baselinevergleich. Kein erneuter Gesamtlauf für Archivtests.

Metrik5185B, SHA256
`d99ff46bbbd668296dd6ff010c29c41f76918f3a04b244a151718e07d15aa428`.
Alle23 tatsächlichen Firmware-C-Module sowie Fixture und Tool stehen mit
eigenen SHA256 im JSON; keine Laufzeit-Testhooks im Produkt.

Nächste unabhängige Einheit: SD51, den reproduzierbaren Softwarekandidaten
mit seinen ausdrücklich offenen Hör-/Gerätewerten konsolidieren. Kein endgültiger
Sound-Freeze vor dessen Voraussetzungen. SD41/SD47 bleiben geräteabhängig.
