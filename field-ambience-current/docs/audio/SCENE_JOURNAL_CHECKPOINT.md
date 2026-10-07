# Scene-Journal-Kandidat — 2026-10-07

## Stand und Zweck

SD41 bleibt offen. Der bisherige H743-Save löscht vor jedem Schreiben seinen
einzigen Bank-2-Sektor. Ein Stromausfall während des Löschens kann damit auch
den vorherigen gültigen Save vernichten. Der neue Softwarekandidat erhält
mindestens den letzten vollständig validierten Datensatz in einem zweiten
Sektor, bis der nächste Datensatz vollständig geschrieben ist.

Basis: Entwicklungsbranch `claude/hall-sensor-bom-pcb-update-a8xj82`,
Commit `43729f26d9265417c1db610d58809993c2e27255`.
Arbeit: `codex/scenes-journal-2026-10-07`. Sound-DSP und Scene-Wireformate
bleiben gleich; 24 von 54 Aufgaben geschlossen, 30 Gesamtgates offen.
Separater [Draft PR138](https://github.com/Sancte3D/AMBIENT/pull/138).

`FAM_SCENE_JOURNAL_CANDIDATE` ist **standardmäßig OFF**. Der experimentelle
H743-Adapter verwendet noch gewöhnliche memory-mapped Reads. Ein beschädigtes
Flashword kann deshalb nicht sicher als lesbarer/überspringbarer Datensatz
behandelt werden. Die sichere ECC-/Fault-Behandlung und echte Power-cut-Tests
sind Voraussetzung für eine Aktivierung im Produkt. Save ist weiterhin
synchron und kann Main/Generate-Planung blockieren.

## Format und Transaktion

| Bestandteil | Vertrag |
|---|---|
| Ablage | Zwei unabhängig löschbare Bank-2-Sektoren, 6 und 7, je 128 KiB; ab 0x081C0000 |
| Reservierung | Linker reserviert bei Option ON 256 KiB; Default reserviert weiterhin 128 KiB |
| Record | 544 B fester Abstand: 32-B-Header plus bis zu 512 B Payload |
| Kapazität | 240 Records je Sektor; während ununterbrochener Laufzeit werden freie Records angehängt |
| Header | JSC1, Version 1, uint32-Sequenz, Länge, Payload-CRC32, reservierte FF-Felder, Header-CRC32 |
| Wire | Little-endian, CRC32/EDB88320; SCN5/6/7-Blob unverändert |
| Commit | Payload-Flashwords zuerst; separater 32-B-Header zuletzt; jedes Flashword einmal seit Erase |
| Erfolg | Alle tatsächlich programmierten Worte einschließlich Padding werden zurückgelesen und verglichen |
| Recovery | Beide Sektoren begrenzt scannen; neueste gültige Sequenz unter uint32-Wrap wählen |
| Reset/Fehler | Vor dem nächsten Save anderen Sektor frisch löschen; keine scheinbar freien Worte nach unterbrochenem Erase/Program wiederverwenden |
| Legacy | Erster Save beginnt in Sektor 6; rohes SCN-Blob in Sektor 7 bleibt bis zum ersten gültigen Journal erhalten |
| Read-Ausgang | Erst vollständig validiertes Payload kopieren; Längenfehler oder I/O-Ausfall veröffentlicht keine Teilbytes |
| Fehlerarten | Sicher erkanntes korruptes Flash überspringen; nicht verfügbares I/O bricht Recovery/Save ohne Flash-Mutation ab |
| Kontext | Nur Main; feste Staging-Struktur, kein Heap, keine neuen Audio-Puffer oder Audio-IRQ-Aufrufe |

Wenn ein Commit physisch vollständig wird und erst die Erfolgsmeldung oder
Readback scheitert, kann Recovery den neuen gültigen Stand finden, obwohl
der Save-Aufruf `false` zurückgab. Bei Unterbrechung vor vollständigem Commit
liefert das Modell den vorherigen Stand. Ein API-Fehler allein beweist daher
nicht, welche Version nach dem nächsten Boot gültig ist.

## Softwareprüfungen

`test/test_scenes_journal.c` verwendet NOR-AND-Programmierung mit 32-B-Worten,
Alignment-/Bereichsprüfung und Verbot einer zweiten Programmierung ohne Erase.
Die Tests sind Teil der bestehenden `test/run_tests.sh`-Suite.

- Payloads 1/31/32/33/164/364/368/512 B: jede Program-Operation bei jedem
  Bytepräfix 0–32 unterbrechen, sowohl beim Anhängen als beim Sektorwechsel.
- Jeden Erase-Bytepräfix des verkleinerten Zwei-Record-Sektormodells
  unterbrechen; vorherige gültige Version im anderen Sektor erhalten.
- Jedes Header-/Payloadbyte des neuesten 368-B-Records beschädigen; auf den
  vorherigen gültigen Record zurückfallen, auch bei schon gecachtem Head.
- Sichere Korrupt-Klassifizierung und nicht verfügbares I/O unterscheiden;
  fehlgeschlagene Reads verändern Ausgabe und Flash nicht.
- Falsche Program-Erfolgsmeldung per Readback erkennen; alte Legacy-Daten
  bei abgebrochenem ersten Journal-Save erhalten.
- Echte 128-KiB-Geometrie: 500 Saves mit drei Erases in einem Boot;
  Sequenz-Wrap, Reset und begrenzte Recordlängen prüfen.

Host-Journaltest, gesamte vorhandene Firmware-Host-Suite und Address-/
UndefinedBehavior-Sanitizer bestanden. LeakSanitizer ist wegen der
Container-/ptrace-Einschränkung deaktiviert; der Journalcode allokiert keinen Heap.
ARM-Nachweis erfolgt im erweiterten bestehenden H743-CI-Job: Default,
Product sowie experimentelles Product+Journal, jeweils mit echten Linker-Maps.
Der Journal-Build muss zusätzlich `check_product_link.py` bestehen und
`scene_journal_write` sowie `src/scenes_journal.c` tatsächlich enthalten.

## Tatsächlicher CI-/ARM-Nachweis

Geprüfter Code-Head `9768966cc6a3881ea9ea56cb9e8c2eff2b959a8a`, Tree
`e32fba155f0451813504da9256369296706c874d` (identisch zum lokal geprüften Tree).
[CI37602680211](https://github.com/Sancte3D/AMBIENT/actions/runs/37602680211):
**alle sechs Jobs bestanden** — beide Host-Suites, beide Pico-Builds,
kurzes Product-Hörpaket und H743-Crossbuild. Der H743-Job
`112730608025` enthält alle drei tatsächlichen ARM-Links und deren Map-/Audit-Artefakte.
Nachfolgende Ergänzungen dieses Nachweises betreffen ausschließlich Dokumentation.

| H743-Build | Flash B | DTCM B | D1 B | D2 B | reserviertes Scene-Flash |
|---|---:|---:|---:|---:|---:|
| Reference, Default | 256668 | 119440 | 417440 | 258112 | 128 KiB |
| Product, Default | 190724 | 16384 | 65600 | 60808 | 128 KiB |
| Product + Journal-Kandidat | 192748 | 16384 | 65696 | 60808 | 256 KiB |
| Journal-Delta zum Product | +2024 | 0 | +96 | 0 | +128 KiB |

Product-DTCM ist hier die **16-KiB-Stackreservierung**, kein gemessener
High-water. Bank-1-Imageguard und Ausschluss alter Produktpfade bestehen
auch mit Journal. Compilerframes der unveränderten Klangkette:
`engine_render` 88 B, Room 192 B, Nature 136 B, Generate-Tick 176 B;
`scenes_save` 104 B. Diese Einzelwerte ersetzen keine verschachtelte
Main-/Interrupt-Stackmessung am Gerät.

[Experimentelles CI-Artefakt](https://github.com/Sancte3D/AMBIENT/actions/runs/37602680211/artifacts/11473427883)
enthält BIN/HEX/Map und `PRODUCT_BUILD_AUDIT.json` (14 Tage Retention).
Die Software-/Linkprüfung ist abgeschlossen; die nachfolgenden Geräte-Gates
bleiben offen.

## Verbleibende Gates

1. H743-Read-Adapter kann beschädigte ECC-Worte sicher klassifizieren, ohne
   Audio-/Fault-Handler unbelegt umzubauen oder sich auf CRC allein zu verlassen.
2. Physische Power-cuts während Erase, jedem Flashword und finalem Commit;
   Boot/Recall und Altformat-Migration mit realen ECC-/Cachezuständen.
3. Tatsächliche Versorgung und gewählter Flash-VoltageRange, Bank-1-Ausführung,
   IRQ-Zeitreserve, Stack-High-water und Save-Latenz am Gerät.
4. Nichtblockierende Main-Transaktion und definierte Storage-UX. Dieses Journal
   behebt den synchronen Erase-/Program-Stall nicht.
5. Erst nach diesen Nachweisen über Default-Aktivierung entscheiden. Klang-/
   Hörabnahmen und spätere UI-Aufgaben bleiben eigenständige Gates.

## Primärquelle für das Entwurfsprinzip

ST [AN4894](https://www.st.com/resource/en/application_note/dm00311483.pdf),
Kapitel 3.2 und 4.4: zwei Page-Sets, CRC und Wiederherstellung nach Stromausfall;
unterbrochene Flashoperationen können unbekannte Zustände hinterlassen,
auch scheinbar gelöschte Worte. Das erklärt den konservativen frischen Erase
nach Recovery. AN4894 ist ein Entwurfsbezug und kein Nachweis, dass dieser
eigene H743-Adapter oder eine konkrete Hardware bereits sicher ist.
