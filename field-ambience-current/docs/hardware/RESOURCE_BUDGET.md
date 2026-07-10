# Resource Budget — STM32H743VIT6

Wie viel Flash / RAM / CPU der Sound frisst und **was noch reinpasst**.
Zahlen aus dem realen h743-Cross-Build (Stand r19.16 — V2-SYNTH-Modus gelinkt). Bei jeder neuen
Sound-Idee hier zuerst schauen.

> Regenerieren: `cmake --build <build> ` liest das Size-Report am Ende aus;
> RAM-Attribution per `arm-none-eabi-nm --print-size --size-sort <elf>`.

## Regionen (gemessen)
| Region | Belegt | Größe | % | Verdikt |
|---|---|---|---|---|
| FLASH | 227 KB | 2 MB | 11,1 % | 🟢 massig |
| DTCMRAM | 119 KB | 128 KB | 90,9 % | 🟡 knapp |
| RAM_D1 (AXI) | 510 KB | 512 KB | **99,7 %** | 🔴 VOLL (r19.16: V2-Synth-Busse +16,5 KB — D1 ausgereizt; jede neue Puffer-Idee braucht den Rückgewinn-Pfad oder das PSRAM) |
| RAM_D2 | 283 KB | 288 KB | 95,9 % | 🔴 fast voll |
| RAM_D3 | 0 KB | 64 KB | 0 % | ⚪ **brach** |
| ITCMRAM | 0 KB | 64 KB | 0 % | ⚪ **brach** |

## Wer frisst den RAM (größte Symbole, gemessen)
| Symbol | Größe | Domäne | Was |
|---|---|---|---|
| `H` | 152 KB | D1 | **Reverb** (Dattorro-Tank, alle Delay-Lines) |
| `bufL`+`bufR` | 207 KB | D2 | **Echo**-Delay-Puffer |
| `s_re`+`s_im` | **128 KB** | D1 | **PADsynth-FFT-Scratch — nur beim Weltwechsel** |
| `voices` | 100 KB | DTCM | Pad-Stimmen (Voice-Pool) |
| `ringL`+`ringR` | 69 KB | D2 | **Blur**-Granular-Ringe |
| `s_table` | 64 KB | D1 | PADsynth-Spektraltisch (Dauer-Nutzung) |
| `rbL`+`rbR` | 32 KB | D1 | Shimmer-Ring |
| `wow_rbL`+`wow_rbR` | 32 KB | D1 | Tape-Wow-Ring |
| `fb` | 27 KB | D1 | OLED-Framebuffer |

Kurz: **D1** = Reverb + PADsynth (Tisch **+ 128 KB Build-Scratch**) + kleine
Ringe + Display. **D2** = Echo + Blur. **DTCM** = Pad-Voices.

## CPU
- **480 MHz Cortex-M7, FPU, I/D-Cache an.** Design-Disziplin: Filter auf
  Control-Rate (÷16), kein `powf` pro Sample, LUT-Sinus, Block-Processing,
  **FTZ** gegen Denormal-Spikes in leisen Tails.
- Vergleichsklasse: **Daisy Seed** (gleiche H7-Familie, 480 MHz) fährt weit
  schwerere Polyphonie problemlos.
- **⚠ Ungemessen auf echtem Silizium** — es ist nie auf Hardware gelaufen.
  Die echte CPU-Last zeigt erst der Prototyp (im Bring-up-Runbook Stufe 10:
  Dauerlauf + Spike-Check in Stille).

## Fazit
- **Flash & CPU:** für den aktuellen Sound reichlich (CPU: begründete
  Schätzung, nicht gemessen).
- **RAM ist die Wand** für „maximalen Sound" — nicht CPU, nicht Flash. D1/D2
  sind bei 96 %. Deshalb ging der echte Blendwave-Zweittisch (64 KB) nicht
  und wurde als Filter-Morph gebaut (r19.5).

## Latente Reserve (Rückgewinn-Pfad)
Wenn ein Feature RAM braucht, gibt es effektiv **~180–250 KB** zu holen:

1. **128 KB FFT-Build-Scratch** (`s_re`+`s_im`) — nur bei `padsynth_build`
   (Weltwechsel) gebraucht, aber permanent in D1 reserviert. Größter
   Einzelbrocken. **Caveat:** der Weltwechsel läuft mit **live Audio**, also
   kann der Scratch nicht naiv die aktiven Audio-Puffer (Echo/Reverb)
   aliasen. Saubere Optionen: eigener overlaybarer Bereich, kleineres
   In-Place-FFT, oder PADSYNTH_N reduzieren (Timbre-Retune nötig).
2. **128 KB brach** (RAM_D3 64 KB + ITCM 64 KB) — komplett ungenutzt,
   aber Domänen-/Cache-Vorsicht (D3 ist eine eigene Power-/Bus-Domäne;
   ITCM ist eigentlich für Code). Belegung will auf **echter Hardware**
   validiert werden — Host-Tests fangen Cache-/Domänen-Bugs NICHT.

Damit wäre Platz für z. B. einen echten Zweittisch-Blendwave, ein größeres
Clouds-Granular oder mehr Stimmen.

### Externe Erweiterung: QSPI-PSRAM (die große Tür)
Für **Megabyte** statt Kilobyte gibt es die echte Lösung: **8 MB QSPI-PSRAM**
(APS6404L, LCSC C3028887, JLC-Economic, ~$2,94, 6 freie Pins), memory-mapped
→ CPU liest wie internen RAM. Ermöglicht Sample-Playback, Convolution-Reverb
mit echten IRs, großes Granular. **Board-Änderung** → muss vor Arons Layout
rein. Voll verifizierte Integration: **`ADR-0022`**.

## Regel
- RAM-Umbauten (`.bss` zwischen Domänen schieben, D3/ITCM belegen, Scratch
  überlagern) sind **cache-heikel und host-untestbar** → **erst wenn ein
  echtes Board da ist** oder ein konkretes Feature es erzwingt. Nicht
  spekulativ.
- Vor jeder neuen großen Puffer-Idee: obige Tabelle + die Reserve prüfen.
