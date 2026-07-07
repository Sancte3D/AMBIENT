# Realtime-Audio-Regeln — Field Ambience (STM32H743)

Bindend für alles, was im **Audio-Hot-Path** läuft (`engine_render()` und der
ganze Aufruf-Graph darunter). Ziel: **kein Knacken, kein Delay, keine
Bounces** — nicht „im Schnitt", sondern **jede einzelne Render-Deadline**.

> Golden Rule: *Never reason about real-time audio performance when you can
> measure the deadline.* Diese Datei + der DWT-Profiler (`audio_profiler.c`)
> + das Lint-Gate (`test/lint_hotpath.sh`) machen genau das durchsetzbar.

Grundlage: die vier Fehlerklassen der Embedded-Audio-Praxis —
**Deadline-Miss · State-Discontinuity · Data/Clock-Failure · Electrical
Coupling**. Fast jeder hässliche Audiofehler landet in einer davon.

---

## Architektur-Fakten (im Tree verifiziert, r19.11)

| Fakt | Stelle | Status |
|---|---|---|
| `engine_render()` läuft in der **SAI-DMA-IRQ** (Prio 5), nicht im Main-Loop | `audio_h743.c` `fill_half` | ✅ Audio kann durch Main-Loop-Stalls **nicht** glitchen |
| Ping-Pong Circular-DMA, Pump startet einmal, stoppt nie | `audio_h743.c` | ✅ |
| Buffer 32-Byte-aligned in RAM_D1, `SCB_CleanDCache_by_Addr` nach jedem Fill | `audio_h743.c:70` | ✅ Cache-Kohärenz korrekt |
| FPU Flush-to-Zero (Thread + Handler) | `main_h743.c:100` | ✅ Denormal-Schutz |
| Sinus-LUT + lineare Interpolation statt `sinf` pro Sample | `dsp.c` | ✅ |
| Live-Parameter-Smoothing (Reverb, Master-Vol, Drive, Wet) | `engine.c:823`, `reverb.c` | ✅ |
| Master-Schutz: Tanh-Sättigung + Soft-Limit ab 0,75, **keine** Gain-Automation | `engine.c:820` | ✅ bounce-frei by design |
| Blocksize **512 Frames = 11,6 ms** | `audio.h:39` | ⚠️ groß für ein gespieltes Instrument — messen (P0.2) |
| `oled_show()` **blocking SPI, ~29 ms** Full-Frame | `lcd_st7789_h743.c` | ⚠️ Control-Latenz, kein Audio-Glitch (P0.3) |
| **CPU-Headroom nie auf Silizium gemessen** | — | ⚠️ P0.1 |

---

## Die harten Regeln

### §1 — Die Deadline ist ein Gesetz, keine „Performance"
Bei 512 Frames muss `engine_render` **immer** unter **11,6 ms** fertig sein
(bei 128 Frames: 2,9 ms). Ein einziger Ausreißer über der Deadline → DMA
wiederholt den letzten Buffer → **hörbarer Klick**. Durchschnitt ist egal.

**Produkt-Gate (unsere Regel, kein Naturgesetz):** `peak_load < 0.60` —
also 40 % Deadline-Reserve. Gemessen per `audio_profiler_t` (`peak_load`,
`deadline_miss_count`). `deadline_miss_count > 0` = Audio-Bug, wird auf echter
Hardware gejagt (BRING_UP Stufe 10).

### §2 — Audio niemals stoppen und neu starten
Preset-, Welt-, Reverb-Wechsel: **nie** SAI/DMA stoppen. Mute = Pipeline
läuft weiter mit Silence/Ramp. Der PCM5102A geht bei Clock-Halt in Standby und
fährt bei Clock-Rückkehr seine Power-up-Sequenz → „mal eben BCK/LRCK aus" ist
**keine** neutrale Operation.

### §3 — Kein unbounded / blocking Code im Hot-Path *(Lint-Gate erzwingt das)*
Verboten in jeder von `engine_render()` erreichbaren Funktion:
`malloc/calloc/realloc/free`, `printf/sprintf/snprintf`, `HAL_Delay`,
blocking `HAL_I2C_*`/`HAL_SPI_*`, Filesystem, `rand/srand`, `exit/abort`,
`while(hardware_bedingung)`, unbeschränkte Retry-Loops.
→ `test/lint_hotpath.sh` scannt den Render-Graph und **hard-failt** die Suite.

### §4 — Parameter nie springen lassen
`gain = new_gain;` von 0,13 auf 0,67 ist eine Sample-Discontinuity = Klick.
Immer rampen: `cur += coef * (target - cur)`. Betrifft **jeden** live
veränderbaren DSP-Wert: gain, freq, cutoff, resonance, delay-time, feedback,
reverb-size, drive, pan, wet/dry. **Delay-Time** braucht interpoliertes Lesen
oder Crossfade; **Algorithmus-/Welt-Wechsel** brauchen 5–20 ms Crossfade.

### §5 — Keine Transzendentalen blind pro Sample
`sinf/cosf/powf/expf/logf/tanhf/sqrtf` im Sample-Loop nur **bewusst +
profiliert**. LUT-Aufbau (`dsp_init`), PADsynth-Spektralbau und
Control-Rate-Koeffizienten (÷16, `pad.c`) sind ok. Der aktuelle aktive Hot-Loop
hat als per-Sample-Transzendentale nur `tape.c` (2× `tanhf`, gewollt) und
`reverb.c:60` — die `sinf` in `reverb.c:212/230` sind **LIQUID (Mode 1) = totes
Kompilat**, Default ist HALL (Mode 2). Das Lint-Gate listet alle Transzendenten
zur Review (nicht-fatal); neue per-Sample-Aufrufe brauchen einen
Begründungs-Kommentar.

### §6 — Denormals killen stille Engines → FTZ bleibt an
Reverb-/Delay-/Pluck-Tails laufen gegen 1e-38 → FPU-Spike → Crackle im leisen
Tail. FTZ ist gesetzt (`main_h743.c:100`) — **nicht entfernen**. Regression:
max Reverb/Feedback, Ton triggern, in die Stille laufen lassen, 60 s
Render-Zyklen profilen.

### §7 — DMA + D-Cache ist eigene Disziplin
CPU schreibt durch den Write-Back-Cache; DMA liest AXI-SRAM → ohne Clean liest
DMA alte Daten. `SCB_CleanDCache_by_Addr` nach jedem Fill ist da ✅. Regel:
**keine globale `.bss`-Änderung darf die DMA-Kohärenz implizit kippen** — der
Audio-Buffer bleibt aligned + in D1, DMA nie in DTCM (D1-DMA erreicht DTCM
nicht).

### §8 — „Glitcht nicht" ≠ „fühlt sich direkt an"
Audio läuft in der IRQ → **kein Dropout** bei blockiertem Main-Loop. **Aber**
Eingaben werden im Main-Loop verarbeitet, hinter dem 29-ms-Display-Redraw →
Worst-Case-Control-Latenz addiert sich. Fix: Display asynchron (SPI-DMA +
Dirty-Rects), Inputs in eine Event-Queue, **UI zeichnet den State, besitzt ihn
nicht**.

### §9 — LED/Display/I²C erzeugen keine periodische Störung
LED-Frame = **ein** Auto-Increment-I²C-Burst (`pca_set_all_pwm`, r19.11), nicht
16 Transaktionen. Weniger Bus-Overhead, weniger periodische Flanken, die in den
Audiopfad koppeln könnten.

### §10 — Der PSRAM-Vertrag (r19.10, ADR-0022): heiß intern, kalt extern
**Heiße Puffer NIEMALS ins QSPI-PSRAM:** Audio-DMA-Buffer, Reverb-Tank,
Echo-Delaylines, Framebuffer → interner SRAM. PSRAM ist **read-mostly**
(seriell, hinter dem Cache): nur kaltes Material (Samples, IRs, Wavetables), das
durch den Cache gestreamt wird. Das Falsche dort = genau die Delays/Dropouts,
die dieses Dokument verhindert.

---

## PCB-Audio-Review-Checkliste (für Arons Layout)
- **Boost TPS61089 ist der Aggressor:** SW-Node + Boost-Strompfad kurz &
  flächenarm, Datasheet-Topologie kopieren — hier ist „kreatives Layout" falsch.
- Vom PCM5102A OUTL/OUTR fernhalten: SW-Node, Boost-Induktor, Class-D-Output,
  Speaker-Traces, Display-SPI-Clock, LED-Return-Pfade.
- Gemeinsamer Ground (TI-Empfehlung), aber **physische Partitionierung**:
  digitale Clock-/Interface-Traces weg von den Analog-Outputs.
- PAM8403: 1 µF Low-ESR dicht an VDD + ≥20 µF nahe dem Amp; C_BYP kritisch für
  Noise/THD. Turn-on-Pop hängt am Eingangs-Koppel-C + Mute-Sequenz.
- Mute-Sequenz auf Hardware mit Oszi über **jeden** State-Übergang messen
  (Power on/off, USB rein/raus, Jack rein/raus, Reset, Brownout).

---

## Offene Messungen (brauchen echtes Silizium — BRING_UP Stufe 10)
1. **P0.1 — DWT-Profiler ist gebaut** (`audio_profiler.c`, host-getestet).
   Auf Hardware: `peak_load`, `max_cycles`, `deadline_miss_count`,
   `clip_count` im Worst-Case-Soak auslesen.
2. **P0.2 — Block-Sweep 512→256→128→64**, Worst-Case-Szene (alle Voices +
   Bass + Drone + Echo/Blur/Hall/Shimmer/Tape max + Cell-/Encoder-Storm + LED +
   Full-Redraw), 60 min Soak. Hypothese: H743 schafft 128 Frames. **Erst
   messen, dann `AUDIO_BUFFER_FRAMES` ändern.**
3. **P0.3 — Display asynchron** (SPI-DMA + Dirty-Rects): löst Control-Latenz
   *und* macht Animationen erst möglich (der blockierende 29-ms-Transfer ist der
   Flaschenhals, nicht die Rechenlast).
4. **Product-Build:** `CMAKE_BUILD_TYPE` für h743 explizit auf Release zwingen;
   finale Optimization-Flags im Build-Log prüfen. Host-`-O2` ≠ H743-Last.
