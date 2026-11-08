# SD47 — alter Quellenstopp darf keinen neuen Ton löschen

Stand2026-10-17, einzelne Softwareeinheit auf dem Room-Stand PR170/Commit
`6990f11d8ddbe0bf6a9676309b6589acd0b94b9b`. SD47 bleibt für die übrigen
Hotpath- und Spitzenlastfälle offen. Reference ist Default, Product Kandidat;
Hörwahl und Geräteabnahme bleiben getrennt.

## Befund am echten Engine-/Quellencode

Main hält die drei Besitzerslots und veröffentlicht Release-/Quick-Bits.
Audio verbraucht die Bits und beendet die tatsächlichen Quellen. Wenn der
Audio-IRQ die alte Quelle kurz vor der Main-Veröffentlichung beendet, kann
der alte Stopp erst nach diesem Retirement im Bitfeld landen. `reap()` gibt
den alten Slot korrekt frei. Ohne Bereinigung wurde der Besitzer neu belegt,
und der nächste IRQ löschte die neue Vorbereitung mit dem alten Befehl.

Die tatsächliche unveränderte Basis scheitert in jeder der drei Worlds:
`source handoff: case=1 line=77: engine_active_voices()==1`.
Die Gegenprobe fügt nur Beobachtungspunkte vor/nach dem Main-`note_off` des
Kontextwechsels hinzu. Sie rendert eine reale alte Quelle bis32 Samples vor
deren100-ms-Ende, unterbricht Main mit einer echten Audiohälfte, lässt den
alten Stopp publizieren und startet denselben Besitzer erneut. Kein erfundenes
DSP-Ende, keine gesetzte Voice-Aktivflagge, kein Ersatzrenderer.

## Änderung und Besitzvertrag

Nach allen gültigen Admission-/Pool-/Harmonieprüfungen steht fest, dass kein
alter Ledger-Slot dieses Besitzers mehr belegt ist. Main entfernt dessen
Release- und Quick-Bit, jeweils mit einem atomaren Wort-RMW, **vor** der neuen
Quellenvorbereitung. Audio kann vor, zwischen oder nach den beiden Operationen
unterbrechen: Die neue Quelle ist dann noch nicht veröffentlicht. Andere
Besitzerbits bleiben erhalten. Abgewiesene Starts erreichen die Bereinigung
nicht. Reguläre aktuelle Stopps und Abbruch vor dem ersten Sample bleiben
wirksam; nach neuer Veröffentlichung schreibt nur Main neue Befehle.

Voraussetzung: genau ein Main-Schreiber und der Audio-Verbraucher, keine
Reentranz von Engine-Steuerungen aus einem Note-Hook. Diese Änderung macht
daraus keinen beliebigen Mehrschreibervertrag. Zwei feste Main-Wort-RMWs,
kein neuer State, Buffer, Heap, I/O oder IRQ-Maskieren. Cortex-M-Exclusive-
Stores können bei Unterbrechung wiederholt werden; keine feste Main-WCET aus
zwei C-Ausdrücken ableiten. Im Audioweg kommt keine Operation hinzu.

## Gezielte Nachweise

`test/test_source_handoff.c` bindet die echte Engine ein und linkt alle echten
Quellen, Room, Natur und DSP. Testhooks existieren nur unter
`FAM_SOURCE_HANDOFF_TEST`, nicht im ARM-Produktobjekt.

- 645 gerichtete Fälle,48809 Assertions und1395 Audiohalb-Unterbrechungen an
  allen zehn markierten Main-Grenzen;64/512 Frames, eine/drei Audiohälften.
- Alle neun alten/neuen Worldpaare; manuelle Besitzer0/4/9/13 und
  Generate-Besitzer6/7/15. Neue Quellen bleiben aktiv und werden genau einmal
  als tatsächlicher Start bestätigt.
- Veraltete reguläre Releases nach natürlichem Ende einer legalen sehr leisen
  WOODLAND-Vorbereitung; kein behaupteter hörbarer alter Ton.
- Gültiger aktueller Stopp vor/nach erster Audioübernahme, vollständiger
  Abbruch ohne Hook/PCM vor dem ersten Sample, anderer Besitzer mit offenem
  Stopp, abgewiesene/ungültige Starts und Epoch-Wrap.
- C11/O2/Wall/Wextra/Werror sowie ASan/UBSan: identisches positives Ergebnis.
  Die drei beobachteten alten Worlds scheitern weiterhin ohne die Korrektur.

Der Test unterbricht Main mit vollständigen Audiohälften. Main läuft niemals
innerhalb eines Audiohalfs weiter; keine Behauptung über beliebige parallele
C11-Threads. Die ganze Host-Suite enthält diesen Regressionstest.

## Unveränderter musikalischer Verlauf

24 vollständige12-s-Paare aus dem tatsächlichen Product-Renderer: drei Worlds
×Manual/Generate ×Nature0/1 ×64/512. Ganze PCM und echte Note-Hook-Dateien
vorher/nachher bytegleich; gesamte PCM auch zwischen64/512 gleich. Dry/Room-
Änderungen, ungültige Roomwerte, Clear und frischer Neustart enthalten.
12700800 Stereo-Frames je Version in getrennten Fällen. Kein Langzeit- oder
Hörurteil aus dieser Summe; lange WOODLAND-Quellen bleiben erhalten.

## Tatsächlicher H743-Gegenlink

Vollständiger neuer Product-Link und Gegenlink mit alter Enginequelle, allen
anderen identischen Objekten, Flags und echtem Linker. Objektbasename bleibt
`engine_product.c.obj`. GCC13.2.1, Cortex-M7/thumb/hard-float, Release.

| Größe | Basis | Korrektur |
|---|---:|---:|
| Product-BIN/Flashladebytes |196700B|196836B|
| D1-BSS |61344B|61344B|
| D2-BSS |60848B|60848B|
| DTCM-BSS |0B|0B|
| `.data` |240B|240B|
| eigener `admit`-Compilerframe |56B|56B|
| eigener `engine_render`-Compilerframe |88B|88B|

Flashdelta+136B, kein zusätzliches RAM. Die1592 Bytes des nicht gelinkten
`.text.engine_render`-Abschnitts sind exakt gleich; SHA256
`6e15883a017bdd296e81d876b6fd84d0f22d0f46ba6639db0f91353df26deb66`.
Kein Atomic-Laufzeithelper oder Testhook im Produktobjekt. Compiler kann
Wrapperframes anders aufteilen; daraus keine ganze Stackersparnis ableiten.
Keine gemessene Deadline, Cache-/IRQ-WCET oder reale Stackreserve; das bleibt
SD48 am Gerät.

262/263 bestehende Firmware-/Header-/HAL-/Startup-/Linker-/CMakeinputs sind
bytegleich PR170. Ausschließlich `src/engine_product.c` ändert sich. Keine
Quellen-/Hüllkurven-/Grammatik-/Gain-/SCN7-/Defaultänderung.

## Reproduktion und CI-Vertrag

Im `firmware-c-next`:
`python3 tools/review_source_handoff.py build-source-handoff-review --sanitize --arm`.
JSON-Proof: [SD47_SOURCE_HANDOFF_METRICS.json](SD47_SOURCE_HANDOFF_METRICS.json),
19226B, SHA256
`57debf15e559ad61d572e01991b24ea72ca683fc64300636512666477a83e1f6`.

Der neue aktuelle SD47-CI-Job prüft den neuen Enginecode, die alten Gegenfälle,
die ganze normale PCM und den ARM-Gegenlink. Der strikte Room-Audit bleibt
unverändert am exakten vollständig grünen PR170 (21/21 Jobs); sieben frühere
Audits behalten ihren exakten PR169-Stand und unveränderte Byteguards.
Aktuelle Host-/H743-Jobs verwenden weiterhin den neuen HEAD. Alte Artefakte
sind keine aktuelle erneute SD37-/SD41-Geräte- oder Hörabnahme.
