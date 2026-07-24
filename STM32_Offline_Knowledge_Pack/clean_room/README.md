# Clean-Room DSP

Diese Dateien wurden für dieses Paket neu geschrieben und basieren nicht auf
dem Quelltext der ausgeschlossenen Tutorial-Repositories oder dem
GPL-ST7789-Treiber.

Lizenz: MIT-0. Verwendung, Änderung und Verkauf sind ohne
Namensnennungspflicht erlaubt.

## Enthalten

- `parameter_smoother.h`: exponentielles Glätten hörbarer Parameter.
- `schroeder_reverb.h/.c`: kleiner Mono-Reverb mit vier parallelen
  Feedback-Comb-Filtern und zwei seriellen Allpass-Stufen.
- `test_reverb.c`: Hosttest für Stabilität und endliche Ausgabe.

## Bauen

Auf einem Host mit C11-Compiler:

```sh
make
./test_reverb
```

## STM32-Hinweise

- Reverb-Zustand statisch oder global anlegen, nicht auf dem Stack.
- Keine Initialisierung im Audio-DMA-Callback.
- Für Stereo zwei Instanzen mit leicht unterschiedlichen `spread`-Werten
  initialisieren.
- Wet/Dry und Feedback über den Smoother ändern.
- Vor Produktion Pegel, Denormals, CPU-Zeit und Worst-Case-Stabilität messen.

Die Implementierung ist eine saubere Lern- und Ausgangsbasis, kein fertig
gemasterter Produktionsreverb.
