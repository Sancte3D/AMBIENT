# Nur referenzierte Quellen

Die folgenden Repositories wurden technisch geprüft, aber ihr eigener Code
wird in diesem weitergegebenen ZIP nicht vervielfältigt, weil keine
ausreichende allgemeine Weiterverteilungslizenz gefunden wurde:

## Controllerstech STM32-HAL

- URL: https://github.com/controllerstech/STM32-HAL
- Commit: `f004297a07daa27ccaf8a9d5d0c515c1e735c569`
- Inhalte: UART, ADC, DMA, Timer, Encoder, FreeRTOS, Displays, USB und Netzwerk.
- Lizenzlage: README erlaubt Lernen und private Anpassung, aber es wurde keine
  allgemeine Open-Source- oder kommerzielle Weiterverteilungslizenz gefunden.
- Ersatz: entsprechende offizielle STM32CubeH7 Basic Examples.

## YetAnotherElectronicsChannel

- IIR: https://github.com/YetAnotherElectronicsChannel/STM32_DSP_IIR
- Reverb: https://github.com/YetAnotherElectronicsChannel/STM32_DSP_Reverb
- CMSIS-DSP: https://github.com/YetAnotherElectronicsChannel/STM32_CMSIS_DSP
- Lizenzlage: In den geprüften Snapshots wurde keine Lizenz gefunden.
- Ersatz: STM32CubeH7, offizielles CMSIS-DSP und `clean_room`.

## STM32 Monosynth

- URL: https://github.com/FedericoDiMarzo/stm32-monosynth
- Commit: `5219d3f0775f64438ff31f83294ca2be959ece13`
- Architektur: STM32F407, DMA-Audio, MIDI, vier drückbare Encoder, DPW-Saw,
  exponentielle Envelope und resonantes Vierpol-Filter.
- Lizenzlage: Keine Root-Lizenz gefunden; weitere Abhängigkeiten besitzen
  eigene Bedingungen.
- Ersatz: Architektur mit libDaisy, DaisySP-MIT, CMSIS-DSP und eigenen
  Treibern neu umsetzen.

Eine öffentliche GitHub-Ansicht ist keine automatische Erlaubnis, den Code in
einem anderen Paket oder kommerziellen Produkt weiterzuverteilen.
