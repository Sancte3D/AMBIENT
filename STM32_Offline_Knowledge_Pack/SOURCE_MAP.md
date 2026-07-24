# Technische Source Map

## 1. STM32H743: verlässliche Hardwarebasis

Startpunkt:

`upstream_sources/STM32CubeH7/Projects/STM32H743I-EVAL/Examples/SAI/SAI_AudioPlay`

Lernziele:

- SAI-Konfiguration;
- Circular DMA;
- Halb-/Vollbuffer-Logik;
- Audio-BSP-Aufrufe;
- Clocktree für Audio;
- Trennung zwischen Board Support und eigener Signalverarbeitung.

Weitere relevante Bereiche:

- `Projects/NUCLEO-H743ZI/Examples`
- `Drivers/STM32H7xx_HAL_Driver`
- `Drivers/CMSIS/Device/ST/STM32H7xx`
- `Projects/*/Examples/SPI`
- `Projects/*/Examples/TIM`
- `Projects/*/Examples/DMA`

Bei H7-Portierungen immer D-Cache, MPU, SRAM-Domains und DMA-Erreichbarkeit
prüfen.

## 2. Audioarchitektur auf STM32H750

libDaisy:

- `upstream_sources/libDaisy/src/sys`
- `upstream_sources/libDaisy/src/per`
- `upstream_sources/libDaisy/src/hid`
- `upstream_sources/libDaisy/src/daisy_seed.*`
- `upstream_sources/libDaisy/examples`

DaisyExamples:

- `upstream_sources/DaisyExamples/seed/Osc`
- `upstream_sources/DaisyExamples/seed/Drum`
- `upstream_sources/DaisyExamples/seed/WavPlayer`
- `upstream_sources/DaisyExamples/seed/USB_MIDI`
- `upstream_sources/DaisyExamples/pod`
- `upstream_sources/DaisyExamples/field`

Diese Quellen zeigen das nützliche Muster:

1. Hardware initialisieren.
2. Sample Rate und Blockgröße festlegen.
3. Controls außerhalb oder kurz vor der Audioverarbeitung aktualisieren.
4. Audio ausschließlich im Callback rendern.

## 3. DSP-Bausteine

CMSIS-DSP:

- `upstream_sources/CMSIS-DSP/Source/FilteringFunctions`
- `upstream_sources/CMSIS-DSP/Source/TransformFunctions`
- `upstream_sources/CMSIS-DSP/Examples`
- `upstream_sources/CMSIS-DSP/Documentation`

DaisySP MIT:

- `upstream_sources/DaisySP/Source/Control`
- `upstream_sources/DaisySP/Source/Effects`
- `upstream_sources/DaisySP/Source/Filters`
- `upstream_sources/DaisySP/Source/Synthesis`
- `upstream_sources/DaisySP/Source/Utility`

LGPL-Isolationsgrenze:

`upstream_sources/DaisySP/DaisySP-LGPL`

## 4. Produktionsnahe Instrumenten-Firmware

Mutable Instruments:

- `clouds`: Granular-/Texture-Verarbeitung;
- `rings`: Resonator;
- `elements`: Physical Modeling;
- `plaits`: verschiedene Syntheseverfahren;
- `marbles`: musikalisch kontrollierter Zufall;
- `tides` und `tides2`: Modulation und Hüllkurven.

Nicht blind portieren. Erst Lizenz und konkrete MCU-/Speicherannahmen der Datei
prüfen.

## 5. Display

GPL-Lernreferenz:

`upstream_sources/ST7789-STM32-GPL`

Für proprietäre Firmware nur Verhalten und Datenblattanforderungen analysieren.
Den eigenen Treiber anhand `LEGAL_REIMPLEMENTATION_GUIDE.md` neu schreiben.

## 6. Eigene lizenzarme Ausgangsbasis

`clean_room`

Enthält:

- einen neu geschriebenen Mono-Reverb;
- Parametersmoothing;
- Hosttest;
- MIT-0-Lizenz.
