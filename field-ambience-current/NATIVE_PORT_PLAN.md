# NATIVE PORT PLAN — Pi-frei auf den RP2350

> **Ziel**: die Audio-Engine + UI nativ in C auf dem Pico 2 (RP2350) laufen
> lassen, sodass der Raspberry Pi Zero 2 W komplett aus dem Gerät rausfällt.
> Eine UF2-Datei per BOOTSEL flashen, fertig.
>
> Status: **Step 1 done** (dieses Dokument + nackte C-Firmware-Hülle).

---

## Was nach dem Port wegfällt

Wenn die Engine auf dem RP2350 läuft, wird die Hardware **echt schlanker**.
Das sind die Teile, die aus der Stückliste verschwinden:

| Ref | Was | Warum weg |
|---|---|---|
| **Pi Zero 2 W** | Linux-Audio-Host | Nicht mehr nötig — RP2350 macht alles |
| **J2** | 40-pin 2×20 GPIO-Header | War nur dafür da, den Pi aufzustecken |
| **R1** | 1 kΩ UART-RX-Serien-R | UART zum Pi entfällt |
| **R_BCK · R_LRCK · R_DOUT** | 3× 33 Ω I²S-Serien-R auf Pi-Side | Neue I²S kommt vom Pico, andere Trace-Längen |

> **Korrektur (Step 6):** D2 (SMAJ5.0A TVS) bleibt drin. Sie sitzt im
> Power-Tree auf der **+5V-Hauptschiene**, nicht am Pi-Header — ihre
> ursprüngliche „Pi-WLAN-Spike"-Begründung entfällt zwar, aber als
> allgemeiner Rail-Surge-Schutz bleibt sie sinnvoll. Real entfernt werden
> **5 Bauteile**: J2, R1, R_BCK, R_LRCK, R_DOUT (+ das Pi-Modul, das auf J2 saß).

Plus **bedeutsame Sekundär-Effekte**:

| Punkt | Heute | Nach dem Port |
|---|---|---|
| Power-Budget (Worst-Case) | 2,45 A | ~1,75 A (700 mA Pi raus) |
| Sicherung F1 | 3 A hold / 6 A trip | überdimensioniert, kann später auf 2 A/4 A runter (optional) |
| C_BULK | 1000 µF (war für Pi-Boot + Bass-Transienten) | nur noch Bass-Transienten → 470 µF würden reichen, aber 1000 µF schadet nicht |
| Boot-Zeit | „Pi booten + SC laden" (10–20 s) | „Pico powerup" (<1 s) |
| Update-Mechanismus | WLAN/SSH zum Pi | USB-C einstecken, BOOTSEL drücken, UF2 droppen |
| Determinismus | Linux-Scheduling, mögliches X-Run | Echtzeit-DMA, sample-genau |
| Stromaufnahme idle | ~440 mA | <100 mA |

---

## Was bleibt (kein Hardware-Redesign jenseits der Pi-Sektion)

Alle anderen 90+ Bauteile bleiben. Insbesondere:
- PCM5102A DAC (jetzt vom Pico per I²S gespeist statt vom Pi)
- PAM8403 Amp + Speaker + Line-Out + Jack-Detect
- OLED + 4× Encoder + 10× Choc-Switches + MCP23017
- USB-C + F1 + ESD + Decoupling
- Alle Pico-seitigen Pull-Ups, Pull-Downs, Debounce-Caps

---

## Pico-Pin-Belegung — die Änderungen

Aktuell (SPEC §5) sind alle 24 GPIOs belegt. Wenn der Pi rausfällt, werden
GP0/GP1 frei (kein UART zum Pi mehr) und GP4 ist eh schon ungenutzt
(OLED_MISO_NC, reserviert). Diese drei werden zu **I²S Out**.

| Pico Pin | GP | Alt (heute) | Neu (Pi-frei) |
|---|---|---|---|
| 1 | GP0 | UART0 TX → Pi RX | **I²S BCK → PCM5102A pin 13** |
| 2 | GP1 | UART0 RX ← Pi TX | **I²S LRCK → PCM5102A pin 15** |
| 6 | GP4 | SPI0 RX (OLED MISO, unused) | **I²S DIN → PCM5102A pin 14** |

Kein GPIO-Engpass. I²S geht über das PIO-Subsystem (RP2350 hat 3 PIO-Blöcke
zu je 4 State-Machines, mehr als genug).

**Wichtig — PCM5102A 3-wire-Mode**: SCK (pin 12) → GND, FMT (pin 16) → GND.
Kein MCLK nötig, der DAC erzeugt sich seine interne Clock aus BCK. Steht
schon so in SPEC §8 → keine Änderung nötig.

---

## Die 12 Steps

Jeder Step ist ein eigener kleiner PR, damit du jederzeit abbrechen oder
umsteuern kannst. Keine Mega-Dumps.

- **Step 1** ✅ — Plan-Doku + nackte C-Firmware-Hülle (CMake +
  blinkende LED). Baut zu einer UF2. Keine Audio, kein UI, nur Lebenszeichen.
- **Step 2** ✅ — OLED-Treiber portiert (SSD1322 256×64 via SPI0, identische
  Init-Sequenz wie die MicroPython-Variante). Statischer Banner-Text
  *„FIELD AMBIENCE / V0.9 STEP 2"*. Starter-Font wächst mit jedem nächsten Step.
- **Step 3** ✅ — I²C1 @ 400 kHz, MCP23017 @ 0x20, byte-für-byte
  identische Init zur MicroPython-Variante. GP22 falling-edge IRQ für INTA.
  Live-State-Render auf OLED (5 Zellen + 5 Modifier + Jack-Detect), USB-CDC-
  Trace bei jedem Tap. XSMT-Pin als Output verfügbar für Step 5.
- **Step 4** ✅ — 4× EC11 (DRIVE/BRIGHT/DISPLAY/VOLUME) auf
  GP10–GP21 quadrature-dekodiert per 1 kHz Repeating-Timer + 4-State-Tabelle
  + 4 Sub-Steps pro Detent (identisch zur Python-Logik). Push-Switches
  per 3-of-N-Bounce-Filter. Lock-free Ring-Buffer für Events → kein I/O im
  Timer-Callback. OLED zeigt Position + Push-State pro Encoder.
- **Step 5** ✅ — **Du bist hier. Erster Sound.** PIO0-SM0 mit dem
  pico-extras-`audio_i2s`-Programm (copy mit BSD-Attribution), DMA-
  Ping-Pong (2 × 256 Frames) in den PIO-TX-FIFO bei 44,1 kHz / 16-Bit-Stereo.
  Pins: BCK=GP0, LRCK=GP1, DIN=GP4 (die freigewordenen UART-/MISO-Pins).
  Pop-suppressed Power-Sequenz nach SPEC §8 (Rails → /SHDN → 50 ms →
  /MUTE+XSMT, alles während die I²S schon Stille pumpt). Continuous
  440-Hz-Sinus @ -20 dBFS als „it works". **Ab hier ist der Pi funktional
  redundant** — der Audio-Pfad geht Pico → DAC → Amp ohne Linux.
- **Step 6** ✅ — **Du bist hier.** Schaltplan Pi-frei: `pi.kicad_sch` gelöscht,
  J2/R1/R_BCK/R_LRCK/R_DOUT raus (5 Bauteile, real 97→92), GP0/GP1/GP4 im
  Pico-Sheet auf I²S_BCK/LRCK/DOUT umverdrahtet, Root-Sheet brückt Pico-I²S →
  Audio-Sheet, kicad_pro-Sheetliste bereinigt. Analyzer: 6 VM-001-Blocker —
  **alle die dokumentierte False-Positive-Klasse** (Heuristik taggt den Pico
  wegen VBUS-Pin als 5V-Domain; real sind GP0/1/4 @ 3V3 → PCM5102A @ 3V3,
  kein Level-Shifter nötig). Warnings sogar von 19 → 16 gesunken (Pi-Nets weg).
  GUI-ERC (B3) bleibt die maßgebliche Instanz. **Erstes BOM-Schrumpfen.**
- **Step 7** — Voice-Infrastruktur: ringbasierter Voice-Pool, MIDI-Note-zu-Hz,
  einfache ADSR, klick-freier gate-on/off. Noch kein konkretes Pad — nur
  „Render eine Sinus-Voice pro Cell-Tap".
- **Step 8** — `famSubBass` + `famDeepBass` (zwei Sinus-/Tri-Stimmen, LPF,
  Lag). Eine pro Akkord-Wurzel.
- **Step 9** — `famPadCore` (5-fach polyphon: 3 detuned Saws + Pulse-Crossfade
  pro Stimme, Stereo-LPF mit ADSR + LFO + Brightness-Offset).
- **Step 10** — `famTexture` (Brown + Pink Noise, BPF mit LFO-modulierter
  Mitte, Lag-Smooth-Amp für Ducking).
- **Step 11** — `famReverbMaster` (algorithmischer Reverb statt Convolution —
  8 Feedback-Combs + 4 Allpass à la Freeverb/Schroeder, pre-Reverb tanh-Drive,
  LeakDC + Limiter). v0.8-Reverb-Reconciliation (computeReverb mit
  spaceTailMul/moodSizeMul) übernehmen.
- **Step 12** — Harmonic-Brain-Port: Skalen, Akkord-Familien, Voice-Leading,
  Markov-Übergänge, opt-in Generative-Bed + Drone. v30-Menü (PLAY/SETUP).
  USB-MIDI-Out via TinyUSB. Doku finalisieren.

Nach Step 12 ist die alte MicroPython-Firmware obsolet — wird nach
Sign-off entweder gelöscht oder als `firmware-mpy-legacy/` archiviert.

---

## Performance-Sanity-Check (warum das überhaupt geht)

RP2350: dual Cortex-M33 @ 150 MHz, FPU für single-precision, 520 KB SRAM.

| Layer | Grobe Schätzung CPU/Core |
|---|---|
| 5× Pad-Voice (3 Saws + 2 Pulse + 2-Pol-LPF + ADSR + LFO + Haas-Delay) | ~12 % |
| 1× famSubBass + 1× famDeepBass | ~3 % |
| 1× famTexture (2 BPF + LPF + 2 LFOs) | ~4 % |
| Algorithmischer Reverb (8 Comb + 4 Allpass + DC-Block + Limiter) | ~8 % |
| Master + I²S-DMA + Hilfsmath | ~3 % |
| **Audio total auf Core 0** | **~30 %** |
| OLED + Encoder-Scan + MCP23017-IRQ + USB-MIDI | **~5 % auf Core 1** |

Sample-Buffer: 256 × 2ch × 4 byte = 2 KB pro Block, 2 Blöcke = 4 KB. Reverb-
Delay-Lines (alle 12 Delays bei ≤ 50 ms) = ~100 KB. Insgesamt ~150–200 KB
SRAM-Nutzung. Bleibt **viel** Spielraum.

Realistisch ist sogar **8 Pad-Stimmen** drin, falls man später mehr Polyphonie
will. 5 reichen aber per Sound-Constitution.

---

## Risiken / offene Fragen, die ich beobachte

- **Convolution-Reverb → Freeverb-Klasse**. Die Web-Audio-Version nutzt
  `createConvolver()` mit einer generierten Impulsantwort. Das geht auf
  dem RP2350 nicht (zu wenig RAM, zu wenig Cycles). Stattdessen ein
  Schroeder/Freeverb-Stil-Algorithmic. Klingt nicht *identisch*, aber sehr
  nah — und beide implementieren die Sound Constitution. Tuning-Phase
  in Step 11.
- **PIO-I²S-Reliability**. Standard pico-extras `audio_i2s` lib funktioniert
  zuverlässig, müssen wir aber an unsere Buffer-Größen anpassen.
- **USB-MIDI auf RP2350 mit TinyUSB**. Bewährter Pfad, sollte sauber laufen.
- **Update-Workflow**: BOOTSEL+drag-drop ersetzt SSH zum Pi. Für den User
  Workflow simpler, aber bei einem ausgelieferten Gerät müssen wir den
  BOOTSEL-Button auf der Front zugänglich machen oder einen „Update-Mode"
  per UI bauen (BOOTSEL aus Software triggerbar via `reset_usb_boot()`).

---

## Was Step 1 *jetzt* liefert

Siehe `firmware-c/`:
- `CMakeLists.txt` — Pico SDK 2.x, Board `pico2` (RP2350), USB-Serial an
- `pico_sdk_import.cmake` — Standard-Boilerplate
- `src/main.c` — STATUS-LED auf GP26 blinkt @ 1 Hz, USB-Serial sendet alle 2 s
  „FIELD AMBIENCE v0.9-dev pico2"
- `include/version.h` — Versions-Konstante
- `README.md` — Build-Anleitung
- `.gitignore` — Build-Artefakte raushalten

Auf deinem Rechner mit installiertem Pico SDK:

```bash
cd field-ambience-current/firmware-c
mkdir build && cd build
cmake .. -DPICO_BOARD=pico2
make -j
# field_ambience_native.uf2 entsteht — auf den Pico mit BOOTSEL droppen
```

Das ist der erste Atemzug. Step 2 macht das Display sprechen.
