# Auftrag für Claude

Du arbeitest vollständig offline in diesem Paket. Verwende keine erfundenen
APIs, Dateien oder Lizenzen.

## Projektkontext

Ziel ist ein portables Ambient-Instrument auf STM32H743:

- kontinuierliche, glitchfreie Audioausgabe;
- SAI/I²S mit Ping-Pong- oder Circular-DMA;
- mehrere langsame, musikalische Stimmen;
- weiche Hüllkurven und geglättete Parameter;
- Encoder, Buttons und MIDI;
- 320 × 170 ST7789 über SPI-DMA;
- UI und Audio dürfen sich nicht gegenseitig blockieren;
- D-Cache und DMA-Speicherbereiche des STM32H7 müssen korrekt behandelt
  werden.

## Verbindliche Quellenreihenfolge

1. Beginne mit den BSD-3-Clause-HAL- und Basic-Example-Dateien in
   `upstream_sources/STM32CubeH7`.
2. Verwende für Mathematik und Filter bevorzugt
   `upstream_sources/CMSIS-DSP`.
3. Nutze libDaisy und DaisyExamples als Architektur- und Instrumentenreferenz.
4. Verwende nur die MIT-Bestandteile von DaisySP, sofern ein geschlossenes
   kommerzielles Produkt geplant ist.
5. Behandle `upstream_sources/ST7789-STM32-GPL` ausschließlich als
   GPL-Referenz. Kopiere daraus nichts in proprietäre Firmware.
6. Behandle die gemischten Mutable-Instruments-Quellen entsprechend
   `LICENSE_MATRIX.md`.
7. Code aus `reference_only` existiert absichtlich nicht im Paket. Baue diese
   Funktionen aus offiziellen Spezifikationen und zulässigen Quellen neu.

## Regeln für jede Änderung

- Nenne im Ergebnis alle verwendeten Quelldateien und deren Lizenz.
- Prüfe zuerst den nächstgelegenen Lizenztext im Verzeichnisbaum.
- Übernimm keine Datei nur aufgrund der Lizenz des übergeordneten
  Repositories; Submodule können anders lizenziert sein.
- Schreibe DMA-Callbacks kurz und deterministisch.
- Allokiere nicht im Audiocallback.
- Verwende keine Locks, Dateizugriffe, Displaytransfers oder Logs im
  Audiocallback.
- Lege DMA-Puffer in DMA-erreichbares SRAM.
- Richte cachefähige DMA-Puffer und Cache-Operationen an der
  32-Byte-Cacheline aus.
- Dokumentiere Sampleformat, Kanalanordnung, Blockgröße und Latenz.
- Glätte jeden hörbaren Parameterwechsel.
- Wenn eine Lizenz unklar ist: Datei nicht verwenden und eine unabhängige
  Ersatzimplementierung vorschlagen.

## Empfohlener erster Arbeitsschritt

Analysiere:

`upstream_sources/STM32CubeH7/Projects/STM32H743I-EVAL/Examples/SAI/SAI_AudioPlay`

Erstelle daraus zunächst eine minimale technische Beschreibung:

1. Clock- und SAI-Konfiguration.
2. DMA-Modus und Bufferwechsel.
3. Callback-Reihenfolge.
4. DMA-Speicherort und Cachebehandlung.
5. Welche Board-spezifischen BSP-Aufrufe für unsere eigene Platine ersetzt
   werden müssen.

Implementiere erst danach eigene Synthese oder Effekte.
