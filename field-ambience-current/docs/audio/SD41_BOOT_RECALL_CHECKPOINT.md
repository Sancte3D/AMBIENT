# SD41 — Aktueller Boot-, Recall- und Seed-Nachweis

Stand 2026-10-13. Diese einzelne Software-/PCM-Prüfung ist abgeschlossen.
SD41 bleibt offen für Flash-ECC, Power-cuts, nichtblockierendes Speichern
und Geräteabnahme. Der Journal-Kandidat bleibt OFF. Kein neues Klangurteil,
keine DSP-/Grammatik-/Gainänderung, keine Aktivierung eines Storagekandidaten.

Basis ist PR163, Branch codex/ambient-world-transitions-2026-10-13,
HEAD 67b4ac83f42e5af7b5190db812005ed8c4d693ae,
Tree 73cac3ef4682f19dc74d1fe1c29f6311bc2f6d6a.
Alle 251 getrackten Core-/Header-/Startup-/Linker-/CMakeinputs sind bytegleich.
Der Test benutzt die wirkliche Product-Engine und Scene-, Menu-, Params- und
Controls-Module. World-/Key-Callbacks enthalten wie H743 auch
controls_refresh_held_pitches. Persistenz ist ein RAM-Backend mit realen
SCN5/6/7-Bytes; dies simuliert weder Flash noch HAL, DMA, Cache oder ECC.

## Erhaltener Vertrag

Boot-Reihenfolge: engine_init → engine_boot_mute → Hook/Menu → Controls →
Params → scenes_init → erstes reguläres Audio. Der Device-Volume-Target ist30%,
Host-engine_init allein bleibt60%. Das ist keine physische SPL-Abnahme.
Scene-Init liest Slots, wählt keinen aktiven Slot und schreibt nicht beim Boot.
Generate, gehaltene Besitzer, Mute und Volume sind keine Scene-Persistenz.

Recall aus Generate-Off erzeugt keine neue Tonquelle. Ein ausdrücklich
gespeichertes positives Nature-Target kann dabei hörbar werden: Nature ist
eine unabhängige optionale Schicht, kein implizites Generate. Volume0 und
Mute halten auch dieses Target bei exakt null Ausgabe.

Bereits gehörte manuelle Besitzer behalten Quelle/Hz und ursprünglichen
Key-up; Recall verbindet neue World/Key/Collection/Tuning/Seedtargets damit.
Color/Room/Attack/Release sind in den PCM-Erhaltgegenproben absichtlich gleich,
damit kein erlaubtes Klangregler-Update als Besitzerfehler beurteilt wird.
Recall bei bereits laufendem Generate erhält dessen ausdrückliche Absicht.
Seedneustart beendet alte erzeugte Quellen weich100ms und beginnt neue
Entscheidungen; Room und tatsächliche alte Hz-History werden nicht gelöscht.
Autoplay-Gate0 wird durch Recall nicht aufgehoben.

## Tatsächlicher Prüfumfang

| Gruppe | Anzahl | Vollständige Gegenprobe |
|---|---:|---|
|Boot aus wirklicher Ausgabe|6|Drei Familien, jeweils manuell gehalten oder Generate; Room/Nature1, Master100%; Save beim laufenden Klang, danach gleicher Initpfad,1s PCM exakt0, keine alten Besitzer/Flags|
|Kaputter/fehlender Native-Store|380|368 Einzelbytefehler mit Nicht-Legacy-Magic bei Headerfehlern; neun fehlende/gekürzte Längen, erased/all-zero/unbekannte Version; leere Slots,20ms PCM exakt0|
|SCN5-/SCN6-Recall|10|Je fünf wirkliche Legacy-World-IDs, überhöhte Felder und Seed0; explizite Ersatzmigration, Nature0/Activity50, begrenzte alte Werte, kein Bootwrite/Generate/Volumewechsel|
|Native-Recall aus Off|20|Alle fünf Slotindizes einzeln×Volume0/30/100 oder Volume70+Mute; fünf World/Key/Collection/Tuning/Seed/Nature-Konfigurationen; je8s, keine Tonquelle, nur ausdrücklich positives ungemutetes Nature hörbar|
|Manueller Recall|12 Triplets|Alle sechs Richtungen, gehalten oder bereits ausklingend; voller alter Pool inklusive echter HOLD-Cell; No-recall512 gegen Recall512 und Recall64, ganze8s PCM und Hooks exakt|
|Abgewiesener Save|3 Triplets|Jede Familie; No-save512 gegen fehlgeschlagenen Save512/64 während Manual/Room; ganze8s PCM/Hooks exakt, alter RAM-Slot/CRC/Active/Flashblob erhalten, später ursprünglicher Recall|
|Frischer Seed-Anfang|9 Triplets|Drei Worlds×0/42/0xffffffff; D-Major/Equal, Nature0; explizite frische Targets512 gegen Boot+Recall512/64, ganze8s PCM und vollständige Hookrecords exakt|
|Seed über Timerwrap|9 Paare|Je derselbe frische Recall bei Clock0 und0xfffff000; ganze8s PCM/Hooks exakt|
|Ungehörte Generate-Vorbereitung|3|Je World Seed-Recall vor erstem DSP; alter Pending ohne PCM/Hook/Memory, danach neue bestätigte Starts bei erhaltener Generate-Absicht|
|Laufender Generate-Seed-Recall|3|Je World voller Pool bei2,7s; Seed1234→42 im selben Pitch-/Worldkontext, physisch bei100ms frei, bestehender Room/Hz-History erhalten, neue tatsächliche Starts|
|Autoplay-Gate|3|Recall hebt0 nicht auf;1s bleibt exakt still, explizites Gate-On lässt neue bestätigte Starts zu|

Insgesamt 38.084.760 Stereo-Frames =863,6s beziehungsweise14,3933min
getrenntes echtes PCM. Die38,084760M Frames sind weder exportierte Lang-WAVs
noch ein kontinuierlicher Dauertest. Manuelle und Seed-Triplets vergleichen
ganze8s per memcmp, nicht nur Energie oder kurze Fenster. Die zusätzlichen
Pending-/Live-/Gateprobes laufen bei512, nicht als64/512-CSV-Gleichheitsclaim.
Globale Belegung≤3, Saiten≤2, keine mehrfach aktive Familie pro Besitzer;
Nonfinite/Limiter bleiben in allen Renderpfaden0.

## Seed-Anfänge ohne Restore alter Sitzungen

Bei Seed0 wird0xA6B13E7D verwendet. Eigener Save speichert den normalisierten
Wert; Legacy-Seed0 und der öffentliche Setter werden ebenfalls normalisiert.
Die neun Anfänge enthalten COAST3, WOODLAND2 und HIGHLANDS3 bestätigte
Starts je8s. Die drei getesteten Seeds besitzen je World verschiedene
Ereignis-/PCMfingerprints; das ist keine Garantie für jedes beliebige Seedpaar.
Wiederholter gleicher Seed ist ein ausdrücklich angeforderter neuer Anfang.

Exakte frische PCM-Reproduktion setzt gleiche Targets, Inputzeiten und
vollständig neu initialisierte Quellen/Room voraus. Ein laufender Seed-Recall
behält den alten Raum: sein komplettes PCM muss deshalb nicht dem frisch
gebooteten Anfang entsprechen. Im laufenden Fall werden echte Übergabe,
Hz-History und neue Starts geprüft. Acht Sekunden belegen den Seedstart,
keine ganze Phrase oder spätere musikalische Wiedererkennbarkeit.

## Format- und Diagnosegrenzen

SCN7 bleibt368B, fünf72-B-Slots inklusive CRC32. Native-CRC schützt Header,
Slots und Padding. Die380 Negativfälle sind gerichtete Byte-/Längenfehler.
SCN5/6 haben keine Store-CRC; eine gültige Legacy-Magic folgt ausdrücklich
der Migration. Kein Anspruch, jedes beliebig beschädigte Legacy-Blob oder
eine zu gültiger Legacy-Magic veränderte Native-Magic unterscheiden zu können.
Unabhängig davon startet Scene-Init keine alte Ausgabe und Legacy-Migration
aktiviert weder Nature noch Generate.

FNV-1a64 im JSON ist nur eine Diagnose auf Hostbytes. Die Gleichheitsprüfung
verwendet direkte vollständige PCM-/Event-memcmp; Eventpadding ist definiert
auf0 initialisiert. Keine portable ARM-ABI-/Crosscompiler-Fingerprintbehauptung.
Last-block-Roompeaks sind unterschiedlich breite Beobachtungen; hier wird
Room-Erhalt am Setter und im512-Livefall geprüft. Das eigene SD12-Hörgate
und die SD37-History-Diagnosegrenze werden nicht numerisch geschlossen.

## Reproduktion und Buildnachweis

Im Firmwareordner:
python3 tools/review_boot_recall.py build-boot-recall-review

Test: test/test_product_boot_recall.c, C11/O2/Wall/Wextra/Werror,
FAM_SOUND_PRODUCT; durch test/run_tests.sh Teil der normalen Host-Suite.
Der zusätzliche CI-Job erzeugt denselben vollständigen gemessenen
[SD41_BOOT_RECALL_METRICS.json](SD41_BOOT_RECALL_METRICS.json).
Artefakt: ambient-boot-recall-seed-proof,30Tage.
Die PR-CI enthält außerdem die vollständige Host-Suite und die bestehenden
H743-Links Reference, Product und experimentelles Product+Journal.
Ein aktueller ARM-Link ersetzt keine Hardware-/Save-Latenzprüfung.

## Konkrete verbleibende SD41-Abnahme

1. Am Product-Gerät dieselben sechs vorher lauten Manual-/Generatefälle
   mit Room/Nature1, gespeichertem Seed und Volume100 vorbereiten. Power-cycle:
   Boot-World/defaults, Volume-Target30, kein aktiver Scene-Slot, kein Generate,
   kein HOLD-Besitzer und keine Nature-Ausgabe ohne ausdrücklichen Recall prüfen.
   DAC/Line-out vom Einschalten bis zur ersten Eingabe aufnehmen; analoger
   Einschaltimpuls und tatsächliche Startlautheit sind hier nicht hostbelegt.
2. Jeden der fünf Scene-Slots bei Volume0/30/100 und bei Mute abrufen.
   Generate-Off bleibt Off; positives gespeichertes Nature ist nur beim
   ausdrücklichen Recall zulässig. Gehaltene Originalquelle und Key-up sowie
   laufender Generate-Seed-Recall mit bestehendem Raum am Gerät prüfen.
3. ECC-sicherer Read-Adapter und echte Power-cuts bleiben Voraussetzungen
   der Journal-Aktivierung. Aktueller Default hat keine atomare Sicherung
   der vorherigen Flashversion. Die vorhandenen NOR-Abbruchtests ersetzen
   keine H743-ECC-/Cache-/Versorgungszustände.
4. Der nächste einzelne Softwareteil bleibt SD41: nichtblockierende
   Main-Speichertransaktion vorbereiten. Der bestehende Erase/Program-Pfad
   ist synchron; UI/Generate können warten. Danach echte Save-Latenz,
   DMA-Misses/WCET und Bedienrückmeldung gegen11,61-ms-Audioblocks messen.
   Keine Flasharbeit in engine_render, kein Audio-Stop als Save-Workaround.

Kein neuer Hauptpunkt-Haken:24/54 geschlossen,30 offen, weiterhin11 technische
Hauptblöcke. Diese Einheit schließt die aktuelle Boot-/Recall-/Seed-PCM-Lücke;
die bestehende Storage-/Geräteprüfung von SD41 bleibt bestehen. Kein neuer
Hörvergleich nötig, da Klangkern und musikalischer Vertrag unverändert sind.
