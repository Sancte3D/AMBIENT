# STM32 Offline Knowledge Pack

Stand: 23. Juli 2026

Dieses Paket ist als vollständig offline lesbare Arbeitsgrundlage für Claude
gedacht. Es bündelt zulässig weiterverteilbare Quellcode-Snapshots, die
zugehörigen Lizenztexte, eine technische Navigation und eigene
Clean-Room-Beispiele.

## Sofort anfangen

1. `CLAUDE_START_HERE.md` an Claude geben.
2. Danach `LICENSE_MATRIX.md` lesen.
3. Für das STM32H743-Audiogerät mit `SOURCE_MAP.md` arbeiten.
4. Vor einer Veröffentlichung `LEGAL_REIMPLEMENTATION_GUIDE.md` prüfen.

## Enthalten

- Offizielles STM32CubeH7-Paket inklusive ausgecheckter Submodule.
- libDaisy, DaisyExamples und DaisySP.
- ARM CMSIS-DSP.
- Mutable-Instruments-Quellen.
- ST7789-STM32 als deutlich isolierter GPL-3.0-Lernbestand.
- Alle innerhalb dieser Bestände gefundenen Lizenzdateien.
- Eigene MIT-0-Beispiele für Reverb und Parametersmoothing.
- Offline-Zusammenfassungen und Links für Videos und Repositories, deren Code
  nicht rechtssicher weiterverteilt werden darf.

## Bewusst nicht enthalten

- YouTube-Videodateien oder vollständige Transkripte.
- Code aus Repositories ohne ausreichende Weiterverteilungslizenz.
- Git-Historien.
- Generierte Firmware-Binaries, wiederholte SVD-Dateien, Testkorpora sowie
  große STM32-Demovideos.

Die ausgelassenen Dateien sind für das Lesen und Portieren des Quellcodes nicht
erforderlich. Exakte Upstream-URLs und Commitstände stehen in
`SOURCE_MANIFEST.md`.

## Wichtig

Ein gemeinsames ZIP macht die Bestandteile nicht automatisch zu einem
gemeinsamen Werk. Die Ordner bleiben rechtlich und technisch getrennte
Upstream-Snapshots. Beim Übernehmen in die eigene Firmware gelten die Lizenzen
der tatsächlich verwendeten Dateien und Abhängigkeiten.

Diese Dokumentation ist eine technische Lizenzorientierung und keine
individuelle Rechtsberatung.
