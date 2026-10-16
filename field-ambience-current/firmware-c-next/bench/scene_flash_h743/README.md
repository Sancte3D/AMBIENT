# Isolierter H743-Scene-Flashbench (SD41)

Eigener Einstiegspunkt, eigener Linker und echte ST-Startupvektoren. Dieser
Build verbindet den vorbereiteten H743-Adapter mit dem vorhandenen Journal,
schrittweisem Writer und Recovery. Er ersetzt keine Produktfirmware und enthält
keinen Audio-, Display-, LED- oder Scene-UI-Pfad. Der normale Firmware-Sourceplan
und alle bisherigen 263 Firmwareinputs bleiben unverändert.

## Speicher und Voraussetzungen

- STM32H743 mit 2 MiB internem Flash, BankSwap aus, stabile geeignete Versorgung.
  Boot prüft Flashgröße, Vektoren und Adaptervoraussetzungen. Exklusiver
  HAL-Flashbesitz; ECC-Interrupts müssen aus sein, BusFault wird aktiviert.
- Alle Ladebytes und Handler liegen in Bank1, ab `0x08000000`. Mailbox und
  CPU-Zustand liegen in DTCM; mindestens 16 KiB Stack sind reserviert.
- Testjournal: Bank2/Sektor6 und7, `0x081C0000..0x081FFFFF`, je128 KiB.
  Kein ELF-Ladesegment berührt diese Sektoren.
- Reset-HSI, kein Board-PLL-Setup. `cpu_hz` berichtet `SystemCoreClock`;
  I-/D-Cache werden eingeschaltet. Kein 480-MHz- oder Audiolastnachweis.

**Nur auf einem dedizierten Testgerät mit gesicherter bisheriger Firmware und
gesicherten beiden Journalsektoren flashen.** Das Laden ersetzt Code in Bank1.
Der Bootscan liest ausschließlich. Ein ausdrücklich entsperrtes SAVE kann die
beiden Journalsektoren löschen oder beschreiben. Sein 368-B-Payload beginnt mit
`AMBENCH1` und ist absichtlich keine gültige SCN7-Scene. Ein solches Journal vor
der Rückkehr zur Produktfirmware aus der Sicherung wiederherstellen. Weder
Buildskript noch GDB-Datei verbinden sich selbstständig mit einem Gerät.

## Build und automatische Prüfung

Aus `firmware-c-next`, mit CMake, Python3, Host-C-Compiler, ARM-GCC und Newlib:

```sh
python3 tools/review_h743_scene_bench.py build-h743-scene-bench-review --sanitize-model
```

Der unabhängige CMakebuild lässt sich auch direkt ausführen:

```sh
cmake -S bench/scene_flash_h743 -B build-scene-bench -DCMAKE_BUILD_TYPE=Release
cmake --build build-scene-bench -j2
```

`scene_flash_bench` ist das ELF mit typisierten Debugsymbolen; `.bin`, `.hex`,
`.map`, Disassembly, Symbole, Compilerframes und JSON-Nachweis liegen im
Review-Artefakt. Die Prüfung kontrolliert echte ELF-Ladesegmente, Initial-SP,
RESET-/SysTick-/FLASH-/BusFault-Vektoren, starke HAL-Callbacks, BusFault-Tailbranch
und die geschützte `ldr; bx lr`-Instruktion. Der BIN-Hash ist der Vergleich der
Flashbytes; absolute Debugpfade können den ELF-Hash zwischen Hosts ändern.
Newlib-/Vendor-Startupmetadaten können Linkwarnungen zu unbenutzten Syscallstubs
und GNU-stack ausgeben; das ist kein gemessener Stacknachweis.

## SWD-Ablauf: zunächst nur lesen

Mit dem eigenen passenden SWD-Server verbinden und das Testimage bewusst nur
in Bank1 laden; keinen Mass-Erase verwenden. In GDB das ELF als Symbolquelle
wählen, das Ziel resetten, laufen lassen und wieder anhalten. Dann:

```gdb
source bench/scene_flash_h743/commands.gdb
scene_bench_status
```

`source` definiert nur Befehle. Es lädt kein Image, läuft nicht weiter und
veröffentlicht keine Anfrage. `magic=0x424d4146`, `version=1`, `boot_error=0`,
`flash_kib=2048` und eine fortschreitende `heartbeat` prüfen. Nach Boot ist
`state=1` (Recovery), schließlich `state=2` (Ready). `record_valid=0` ist bei
leerem/ungültigem Journal zulässig. `boot_error=1..5` bedeutet HAL-, Vektor-,
Flashgrößen/BankSwap-, Adapter- beziehungsweise Controllerstartfehler.

Jeder Befehl wird bei **angehaltenem** Ziel eingestellt. Danach `continue`,
später mit Ctrl-C anhalten und `scene_bench_status` lesen. Erst fortfahren,
wenn `ack_id==request_id`; keine zweite Anfrage über eine noch unbestätigte
schreiben. GDB veröffentlicht die neue, von null verschiedene `request_id`
zuletzt. Kein Befehl enthält ein automatisches `continue`.

```gdb
scene_bench_read 0
```

Nach Resume/Halt: `reply=4`, `data_length=32` bei Erfolg. Der Offset ist relativ
zu `0x081C0000`, durch32 teilbar und kleiner als262144. `131072` liest das erste
Wort des zweiten Sektors. CORRUPT (`reply=5`) oder ERROR (`reply=6`) liefert
`data_length=0`; alte Datenfelder sind dann ungültig. `scene_bench_recover`
startet einen neuen Scan. Boot/READ/RECOVER starten keinen Erase/Program.

## Bewusstes Schreiben und Abbrechen

Erst nach geprüftem Backup und abgeschlossenem Scan:

1. Anhalten, `scene_bench_arm`, fortsetzen, erneut anhalten. `reply=3` und
   `arm_token!=0` prüfen.
2. `scene_bench_save 7`, fortsetzen, später anhalten. ARM gilt für genau einen
   SAVE-Versuch und weniger als10000 laufende HAL-Millisekunden. Debuggerhalt
   ist keine Echtzeituhr. READ/RECOVER/DISARM/ungültige Befehle entwaffnen.
3. `ack_id` bestätigt nur die Anfrage. Abschluss erst bei passender
   `operation_id` und `state=4` (Done), `error=0`, `record_valid=1`, Länge368.
   Jeder spätere Save braucht einen neuen bestätigten ARM.
4. Reset und vollständigen Bootscan prüfen: der bestätigte Record muss wieder
   dieselbe Sequenz/Länge liefern. `record_sector/slot/sequence/length` nur bei
   `record_valid=1` auswerten. Payload ist ein reproduzierbares Testmuster.

`scene_bench_cancel` fordert Abbruch an. Eine bereits gestartete Hardwareaktion
wird zuerst über ihren tatsächlichen IRQ/Pollabschluss abgewickelt; erst dann
`state=5,error=3`. Kein Timeout oder Debuggerkommando gibt einen noch benutzten
Programmpuffer frei. Vor erneutem Save `scene_bench_recover` abschließen.
`scene_bench_disarm` entfernt nur die Schreibfreigabe und bricht nichts ab.

| Feld | Bedeutung |
|---|---|
| `reply` | 1 angenommen,2 abgelehnt,3 scharf,4 Read OK,5 corrupt,6 Read ERROR |
| `state` | 1 Recovery,2 Ready,3 Write,4 Done,5 Failed |
| `error` | 0 keiner,1 Recovery,2 Write,3 Cancel |
| `operation_id` | Anfrage der aktuellen/langen Operation; fremde Ablehnung überschreibt sie nicht |
| `steps` | Schritte dieser Recovery/Write-Operation, keine Flashzeit |
| `flash_irqs` | Aufrufe des echten FLASH-Vektors, kein alleiniger Erfolgsbeleg |
| `max_step_cycles` | Höchster DWT-Abstand um einen Main-Schritt, inklusive Präemption; kein Start/Poll/Read-Einzelprofil |

Pro Tick höchstens ein IO-Callback, Read höchstens32B. Der echte Flashabschluss
braucht dennoch einen funktionierenden IRQ und zulässigen Mainkontext. Bleibt
eine Operation hängen, zuerst Fault-/HAL-/Flashregister sichern; niemals durch
Reinit oder eine erfundene erfolgreiche Antwort entsperren.

## Noch am echten Gerät zu prüfen

Das gebaute Image und die nativen Modelle sind keine Geräteabnahme:

- Intakte, Single- und Double-ECC-Worte mit warmem/kaltem Flash-D-Cache;
  zugeordneter Double-ECC ergibt CORRUPT ohne Teilpublikation. Nicht einfach
  ein zweites Mal über ein Flashword programmieren, um einen ECC-Fall zu raten:
  eine kontrollierte, gerätespezifisch dokumentierte Fehlereinbringung verwenden.
- Präziser Guardfault kehrt am festen Resume-PC zurück; fremder Fault bleibt
  fatal. Der Bench läuft im Main/MSP ohne FP-Arbeit. PSP-/FPU-Rückkehr benötigt
  einen separaten gezielten Gerätefall; native Framefixtures ersetzen ihn nicht.
- Bestätigten Record A behalten; Save B an Erase/Payload/Commit/Verify gezielt
  abbrechen oder Versorgung unterbrechen. Nach Neustart nur einen vollständig
  gültigen Record wählen; A oder bereits vollständig bestätigtes B, kein Teil-B.
- Start/Poll/Read separat mit DWT erfassen, Mainfortschritt und IRQ-Reaktion
  beobachten. `max_step_cycles` allein reicht dafür nicht. Audio-/Display-/LED-
  Last, echte Stackreserve und spätere Scene-UI-Integration bleiben eigene
  Produktprüfungen nach Soundabnahme.

SD41 bleibt offen; dieser Softwareteil schließt keinen weiteren Hauptpunkt.
