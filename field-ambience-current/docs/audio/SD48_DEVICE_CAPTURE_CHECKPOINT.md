# SD48 — lesendes Geräte-Auslesepaket

Stand2026-10-20, Basis PR181/0637707d2d878abf3093c4c4a9d5dc9e47d4f11e.
Dessen CI742/run39381951516 ist vollständig30/30 erfolgreich. Kein neuer
Firmwarecode, keine weitere Audioinstrumentierung und kein Gerätepass.

Der bisherige Product-CI-Download enthielt BIN/HEX/Map/Audit, aber kein ELF
oder definiertes Ausleseformat. Das ergänzte Paket enthält nun zusätzlich
ELF, DEVICE_CAPTURE_MANIFEST.json, zwei Pythondateien und die
[Geräteanleitung](../../firmware-c-next/tools/DEVICE_CAPTURE_README.md).
Die bestehende ARM-CI erzeugt das Paket nach dem Product-Linkaudit; keine
zusätzlichen Workflowjobs. Der normale Hostrunner enthält die neuen Tests.

`tools/device_capture.py <build>` verlangt H743/Product/Journal-OFF und einen
zum ELF passenden Product-Linkaudit. Ein unabhängiger Objcopy-Vergleich
verifiziert BIN gegen dasselbe ELF. Bank1-Größe, eindeutige separate56-B-
Profiler in D1 und aktuelle Layoutfelder müssen stimmen. Das reale ARM-Paket
wurde lokal erzeugt: BIN200132B/SHA256
`c07844a1f18ca60b5de84f6d91fbe38ce482875d4cff8fcb65cd2b7a6a7b73a5`,
bytegleich PR181; beide tatsächlichen Profileradressen aus dessen ELF.

`device_capture_gdb.py` definiert beim Sourcen nur `ambient-capture`.
Kein Target-/Flash-/Reset-/Halt-/Continuebefehl, kein Firmwarefunktionsaufruf
und keine Schreiboperation auf dem Gerät. Der Nutzer muss das bereits
verbundene Testgerät am Ende einer Sitzung selbst in Threadmode gestoppt
haben. Alle Threads müssen gestoppt und IPSR=0 sein, auch wenn die Profiler-
In-flight-Flags bereits0 zeigen: die Endbuchhaltung könnte sonst halb fertig
sein. Das Werkzeug liest das erwartete ganze Bank1-Image zum Hashvergleich,
beide Profiler und DWT-Enable; bestehende Ergebnisdateien bleiben erhalten.

Der Export bewahrt auch schlechte Messwerte. Budgetüberschreitungen und
Peak≥0,60 sind Beobachtungen, keine automatische Freigabe. `device_accepted`
bleibt false, Stack-High-water null und physische Clockverifikation false.
Ein Snapshot beweist weder Sitzungsdauer noch frühere Debuggerhalte oder
vollständige Exception-/Dispatch-/DMAfehler-/Stack-/Ausgangsreserve. Debuggerhalt
unterbricht Audio; DMA kann weiterlaufen. Neue ungestörte Messsitzungen dürfen
nicht aus einem Halte-/Resumeverlauf behauptet werden.

## Prüfung und Grenzen

Neun Offline-Tests bestanden: echtes C-Headerlayout/Offsets, tatsächlicher
GDB-Commandshim mit injiziertem GDB-/Speichertransport, Definition ohne
Zugriff, laufendes/IRQ-gestopptes Ziel, falsches Flashimage, falsche Profile/
Layouts/Bereiche, überlappende Daten, ungültige/unfertige Profiler, Erhalt
schlechter Messwerte und exklusives Ergebnisfile. Separate Paketgegenproben
weisen Reference/Journal-ON, abweichenden Audit und falsches ELF/BIN-Paar ab.
Der echte GDB-/SWD-Transport wurde nicht am Gerät ausgeführt. Ein echter
H743-Product-Build samt Linkaudit und Paketerzeugung besteht lokal.

[SD48_DEVICE_CAPTURE_METRICS.json](SD48_DEVICE_CAPTURE_METRICS.json) hält Paket-
manifest, Sourcehashes und Grenzen fest. Alle265 bestehenden Firmware-/Header-/
HAL-/Startup-/Linker-/CMakeinputs bytegleich PR181, auch das tatsächliche BIN.
Reference Default, Sound0.10/SCN7, lange WOODLAND, UIruhe, Parameter, Journal OFF
und synchroner Product-Save bleiben unverändert. Keine erneut gerenderten
langen Hörpakete oder ganze lange Hostsuite für unveränderten Runtimecode.

SD48 bleibt ungecheckt.25/54 Hauptpunkte geschlossen,29 offen, darunter die
vier technischen Hauptpunkte SD41/47/48/49. Der nächste echte Abschluss
benötigt das Gerät: volle IRQ-/Dispatch-/DMAfehler-/Stackmessung und reale
Ausgangskette; Speicheraktivierung erst nach SD41-ECC-/Power-cut-Abnahme.
