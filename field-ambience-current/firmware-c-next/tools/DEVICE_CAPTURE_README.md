# AMBIENT — lesender Abschluss-Snapshot

Dieses Paket ist eine Beobachtungshilfe für SD48. Es verbindet sich nicht
selbst mit einem Gerät und enthält keinen Flash-/Reset-/Halt-/Continuebefehl.
Es ist kein Gerätepass. Journal bleibt OFF; Live-Save weiterhin synchron.

## Vorbereitung ohne Gerät

Nach normalem Product-ARM-Build und `tools/check_product_link.py <build>`:

```sh
python3 tools/device_capture.py <build>
```

Die Vorbereitung verlangt H743/Product/Journal-OFF, den zum ELF passenden
Linkaudit, ein Bank1-BIN und genau zwei56-B-Profiler an gültigen D1-Adressen.
Objcopy erzeugt separat aus demselben ELF ein Vergleichs-BIN; abweichende
ELF/BIN-Paare werden abgewiesen. Das Paket enthält das echte ELF, BIN, Map,
Audit, DEVICE_CAPTURE_MANIFEST.json und die beiden Pythondateien samt dieser
Anleitung. Keine Firmwareänderung und kein neuer Profiler werden eingebaut.

## Später am bewusst bereits verbundenen Testgerät

Den gewünschten Test vollständig laufen lassen. Erst am Sitzungsende die
CPU mit dem eigenen Debugger in Threadmode außerhalb von IRQs anhalten.
Ein Debuggerhalt unterbricht die Audioverarbeitung; DMA kann weiterlaufen.
Nach einem Halt ist kein ungestörter Fortsetzungslauf bewiesen. Für neue
unbeeinflusste Durchläufe eigene frisch gestartete Sitzungen verwenden.
Dieses Werkzeug startet solche Sitzungen nicht selbst.

In einer GDB-Sitzung mit Pythonunterstützung, vorbereitetem Paket und bereits
gestopptem Ziel nur den Befehl definieren und danach die Beobachtung anfordern:

```gdb
source /pfad/zum/paket/device_capture_gdb.py
ambient-capture /pfad/zum/paket/DEVICE_CAPTURE_MANIFEST.json /pfad/zu/neuem/snapshot.json "world-seed-lastfall-dauer"
```

Das Sourcen definiert nur `ambient-capture`. Jeder Aufruf verlangt bereits
gestoppte Threads und IPSR=0 in xPSR. Er liest das gesamte erwartete Bank1-Image
zum Hashvergleich, danach beide Profiler und DWT-Enable. Kein Firmwaregetter
wird aufgerufen und kein Speicher beschrieben. Noch nicht initialisierte,
laufende, IRQ-unterbrochene, falsch konfigurierte oder abweichende Zielstände
werden abgewiesen. Vorhandene Ergebnisdateien werden nicht überschrieben.
Der echte GDB-/SWD-Transport ist hier noch nicht am Silizium abgenommen.

Auch auffällige Messwerte werden exportiert: Scopeüberschreitungen und
Peak≥0,60 erscheinen als Beobachtungen. `device_accepted` bleibt immer false;
Stack-High-water ist null, physische Clockverifikation false. Ein aktueller SP
ist keine Stackreserve. Renderer- und DMA-C-Werte ersetzen weiterhin keine
volle Exception-/Dispatch-/DMAfehler-/Ausgangsabnahme. Ein Snapshot beweist
keine Sitzungsdauer, früheren Debuggerhalte oder reale CPU-Clock.

## Noch am Gerät ausführen

| Durchlauf | Festhalten |
|---|---|
| Je World mehrere Seeds, lange Releases/Room, höchstens3 reale Quellen | Label, Einstellungen, tatsächliche Dauer, volle IRQ-/Dispatchlatenz und Deadline-/DMAfehler unabhängig messen |
| Encoder-/Display-/LED-/MIDIverkehr,15-min-Displayruhe und Wake | Musikalischer Verlauf, Scopewerte, eventuelle Störungen; spätere Snapshotnahme beendet den ungestörten Lauf |
| Speicher | Isolierten SD41-Bench zuerst real auf ECC/Power-cut/IRQ prüfen; Productaktivierung bleibt gesperrt, kein Live-Save-Dauerlastpass vorweggenommen |
| Stack | Reale High-water-Messung aller relevanten Stacks/Exception-/FPU-/Verschachtelungszustände gesondert; keine Ableitung aus SP oder Compilerframes |
| Ausgang/Hören | Tatsächliche Hardwarekette und Lautstärke/Noise/Pops sowie offene Hörentscheidungen separat protokollieren |

Keine Hauptpunkt-Checkbox aus dem Auslesepaket schließen.
