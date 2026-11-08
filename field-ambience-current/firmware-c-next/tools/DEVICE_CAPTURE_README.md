# AMBIENT — tatsächlichen Kandidaten lesend beobachten

Stand28.10.2026. Das Paket verbindet/flasht/resetet/stoppt/startet kein Gerät
und ruft keine Firmwarefunktion auf. Ein Snapshot ist keine Gerätefreigabe.
Ohne Sessionflag bleibt Save synchron; der explizite SCN8/JSS1-Kandidat
enthält jetzt wirklichen asynchronen Speicher.

## Paket und Messbuild

Nach H743/Product-Build und check_product_link.py <build>:

```sh
python3 tools/device_capture.py <build>
```

ELF/BIN/Map/Audit/Manifest gehören zusammen. Unabhängiger Objcopyvergleich
prüft ELF/BIN; das Ziel muss denselben Imagehash haben. Altes experimentelles
JSC1 ausgeschlossen. Das Manifest unterscheidet Session und Messbuild.

```sh
cmake -S . -B build-h743-device-bench -DFAM_TARGET=h743 -DFAM_SOUND_PROFILE=product -DFAM_SOUND_FINISH_CANDIDATE=ON -DFAM_SOUND_SESSION_CANDIDATE=ON -DFAM_DEVICE_BENCH_CANDIDATE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-h743-device-bench
python3 tools/check_product_link.py build-h743-device-bench
python3 tools/device_capture.py build-h743-device-bench
```

Vor HAL/Audio nur unbenutzte reservierte16-KiB-MSP-Region markieren. Aktiver
Main-/Initstack und256B Abstand bleiben unberührt; Compilerframe dagegen
geprüft. Linker schließt Überschneidung mit DTCM-DSPdaten aus. Kein Paint/Scan
im Audio-IRQ, unveränderte Soundziele.64B Beobachtungen: Start-/letzter Maintick,
sticky SAI-Fehlerzahl mit letztem SAI-/DMA-Code, bestätigte/fehlgeschlagene Saves.
Fehlerzähler sättigen.

## Bereits verbundenes Gerät am Sitzungsende

Test zuerst ungestört ausführen. Erst am Ende selbst in privilegiertem
Threadmode außerhalb von IRQs anhalten, alle Threads bereits gestoppt.
Im Messbuild muss der Thread den markierten MSP verwenden; PSP/RTOS wird
abgewiesen. Halt unterbricht Audio, DMA kann weiterlaufen. Fortsetzen
beweist keinen ungestörten neuen Lauf. Dieses Werkzeug erzeugt keine Starts.

```gdb
source /pfad/zum/paket/device_capture_gdb.py
ambient-capture /pfad/zum/paket/DEVICE_CAPTURE_MANIFEST.json /pfad/zu/neuem/snapshot.json "World-Seed-Last-Dauer"
```

Sourcen definiert nur den Befehl. Er liest Bank1-Image, beide Profiler und
DWT-Enable; im Messbuild64B Zustand und höchstens16KiB Paint in validierten
Bereichen. Kein Targetschreiben/Getters/Halt/Resume und kein Überschreiben
von Ergebnisfiles. Fremde, unfertige oder abweichende Ziele scheitern.

stack_high_water_bytes ist im Messbuild der tiefste beobachtete
**Schreibzugriff** ab Stacktop, einschließlich konservativ mitgezählter
aktiver unmarkierter Topregion. Kein kleinster jemals erreichter SP oder
vollständiger per-Kontext-/FPU-/Exception-/Verschachtelungsnachweis.
Zerstörte Paintregion wird als ausgeschöpft exportiert. Fehler, Last>=0,60
und Misses bleiben erhalten. Maintickspanne beweist keine ungestörte Dauer.
Physische Clockverifikation und device_accepted bleiben false.

## Tatsächlich auszuführende Geräteserie

| Gate | Durchführung und Belege |
|---|---|
| SD41 | Erst isolierten ECC-/Power-cut-Bench, danach Session-Save/Cancel/Neustart/alle Slots unter Audio. Abbrüche vor/nach Header, Rollover, beschädigtes Wort, gültige alte Version und Retry. Datensatz je echtem Versuch |
| SD48 Last | Alle Worlds/Seeds,3 Quellen, lange Releases, Room/Nature/FX-Maxima, Freeze, schnelle Zeit-/Längen-/Modellwechsel und Clear/Mute; UI/LED/MIDI und wiederholte Saves. C-Maxima/Misses/Fehler/Stackpaint aufbewahren |
| SD48 Grenzen | Volle Exception-Entry/Exit-/Dispatchlatenz und DMA-Unterläufe unabhängig messen. Ziel peak<0,60, null Misses und Stackreserve je Kontext zusätzlich bestätigen |
| SD49 Entwurf | BOM_CURRENT dokumentiert PCM5102A -> PAM8406/Lautsprecher sowie TPA6132A2 -> J8 Phones/Line. J10 ist MIDI, J6/J7 Lautsprecher. Reale PCB-/Bestückungsrevision und tatsächlich verbaute Teile/Lasten bestätigen |
| SD49 Zustände | Stereo/Mono, gebaute Lasten, USB/Batterie, Jackwechsel, Boot/Mute/Power: Pegel/DC/Clock/Noise/Hum/Pops und reale Lautstärke messen. BOMwerte sind keine Messwerte |
| SD44/50 | Neues unbekanntes A/B/C-Paket, später längere echte Nutzung; Quelle/Parameter/Zeitpunkt und konkrete störende Alarm-/Tube-/Buzz-/Beep-/Ermüdungsstelle protokollieren |

Die Fehlererfassung zählt HAL-SAI-Fehler einschließlich gemeldeter
DMAfehler; kein Beweis für Abwesenheit aller physikalischen I2S-/Clockfehler.
Keine Hauptpunkt-Checkbox allein aus dem Auslesepaket schließen.
