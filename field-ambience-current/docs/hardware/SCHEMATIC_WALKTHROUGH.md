# Schematic Walkthrough — A-Z für PCB-Engineers

Dieses Dokument ist die **menschenlesbare Tour durch das gesamte Schaltbild**.
Wenn du das hier von oben nach unten liest, weißt du am Ende:
- welche 7 Sheets es gibt und was jeder Block macht
- welche Bauteile drin sind und wozu jedes da ist
- wie die Signale fließen
- was kaputtgeht wenn ein Bauteil stirbt
- warum genau diese Bauteile gewählt wurden (nicht andere)

> Diese Datei ist **keine** Datasheet-Verifikation — die liegt in
> `field-ambience-current/docs/component_reviews/` (10-Punkte-Reviews gegen
> Hersteller-PDFs). Dieser Walkthrough ist die *Story*, jene Reviews sind die
> *Belege*. Beide Sichten brauchst du beim PCB-Layout.

## Wie liest man das hier?

1. **Das Schaltbild ist generiert.** Quelle: `kicad/generate_kicad_project.py`
   (~7000 Zeilen Python). **Nie die `.kicad_sch` direkt editieren** — alle
   Änderungen über den Generator. Pro Sheet steht im Walkthrough die
   Python-Funktion + Zeilenbereich, damit du beim Lesen springen kannst.
2. **Refdesigns** (`U1`, `J1`, `R12`) entsprechen genau dem, was im Schematic
   + auf der Platine steht. Alle MPNs + LCSC-Nummern in
   [`BOM_MASTER.md`](../../../BOM_MASTER.md).
3. **Aktive-Low-Netze** haben Suffix `_N` (z. B. `AMP_SHDN_N`) — siehe
   [AI-Ready Schematic Standard](AI_READY_SCHEMATIC_STANDARD.md).
4. **UNVERIFIED**-Markierungen sind echt — wo so steht, ist noch ungeprüft.
   Anti-Guess-Regel (CLAUDE.md) gilt für jedes Bauteil ohne komplette
   Datasheet-Auflösung.

## Übersicht der 7 Sheets

| Sheet | Datei | Generator (`generate_kicad_project.py`) | Was es macht |
|---|---|---|---|
| 1 | `power_tree.kicad_sch` | `make_power_tree_sheet()` ~Z. 1887 | USB-C → Polyfuse → Bulk-Caps → +3V3-Rail, Boost-Konverter für Speakers, Akku-Eingang |
| 2 | `stm32h743.kicad_sch` | `make_stm32h743_sheet()` ~Z. 3424 | MCU + HSE-Crystal + Reset + BOOT + Decoupling + VCAP + VDDA |
| 3 | `lcd.kicad_sch` | `make_lcd_sheet()` ~Z. 3533 | ST7789-Modul-Header J3 + Backlight-FET Q2 + lokale Caps |
| 4 | `mcp.kicad_sch` | `make_mcp_sheet()` ~Z. 4560 | MCP23017 (16 I/O over I²C) + PCA9685 (16 PWM für LEDs) + 5 Hall-Sensoren + 10 Buttons + 10 LEDs |
| 5 | `encoder.kicad_sch` | `make_encoder_sheet()` ~Z. 4845 | 4 EC11-Encoder mit Push + RC-Filter |
| 6 | `audio.kicad_sch` | `make_audio_sheet()` ~Z. 4878 | PCM5102A I²S-DAC + PAM8403 Class-D-Amp + Speaker-Header + 3.5-mm-Line-Out + (DNP) MIDI-Out |
| 7 | `battery.kicad_sch` | `make_battery_sheet()` ~Z. 5956+ | MCP73831 Lade-IC + Akku-JST + Power-Path-Switching (USB ↔ Akku) + Bat-Sense-Divider |

Plus das Top-Level `field_ambience.kicad_sch` — verbindet die 7 Sheets über
Hierarchical Labels.

### Block-Diagramm (Signalfluss + Power)

Zeigt wer mit wem direkt redet. Pfeile = Signal- oder Power-Richtung; Doppel-
pfeile = bidirektional (I²C/SPI command + status).

```text
                              ┌──────────────────┐
                              │   USB-C  J1      │
                              │  (Power + Data)  │
                              └────┬─────────────┘
                                   │
                  ┌────────────────┴────────────────┐
                  │ VBUS                            │ D±
                  ▼                                 ▼
        ┌──────────────────┐                ┌────────────────┐
        │  F1 Polyfuse +   │                │ D1 USBLC6 ESD  │
        │  USBLC6 ESD      │                └───────┬────────┘
        └──────┬───────────┘                        │
               │ +5V_USB                            │ USB_DM/DP
               ▼                                    │
        ┌──────────────────┐ ◄──── Akku (LiPo) ──── │
        │  Q1 Power-Path   │      via J_BAT         │
        │  (USB ↔ Akku)    │                        │
        └──────┬───────────┘                        │
               │ +5V_RAIL                           │
               │                                    │
               ├────► PAM8403 Class-D ──► Speakers  │
               │                                    │
               ▼                                    │
        ┌──────────────────┐                        │
        │  U5 AP7361A LDO  │                        │
        │  +5V → +3V3      │                        │
        └──────┬───────────┘                        │
               │ +3V3                               │
               ▼                                    │
        ┌──────────────────────────────────────────┐│
        │             STM32H743 (U1)               ││
        │  ◄── SPI ───► LCD (J3)                   ││
        │  ◄── I²S ────► PCM5102A ─► Line-Out (J8) ││
        │                          └─► PAM8403 ─► SP
        │  ◄── I²C ────► MCP23017 ─► 10× Buttons   ││
        │                          └─► Hall × 5    ││
        │  ◄── I²C ────► PCA9685 ──► 10× LEDs      ││
        │                          └─► Q2 BL-FET   ││
        │  ◄── TIM-QEI ─► 4× EC11 Encoder          ││
        │  ◄── USB-OTG-FS ─────────────────────────┘│
        └──────────────────────────────────────────┘
                       │ ADC1_INP15
                       └──◄── BAT_SENSE-Teiler (LiPo+)

   (Lade-Pfad parallel: USB-VBUS → U7 MCP73831 → LiPo+ — immer aktiv)
   (Boost-Pfad parallel: LiPo+ → U8 TPS61089 + L1 → +5V_RAIL via D3)
```

Drei Geschwindigkeits-Tiers im Signal:

1. **Schnell (≥10 MHz):** USB-D± (12 Mbps), SPI-LCD (24–32 MHz), SAI-I²S
   (1,4 MHz BCK aber MHz-Edges). → Diese alle auf Top-Layer mit
   ungebrochener GND-Plane drunter (ADR-0018).
2. **Mittel (100 kHz – 1 MHz):** I²C (100 kHz Fast, beide Slaves auf einem
   Bus), PCA9685-PWM-Träger, ADC-Sampling.
3. **Langsam:** Encoder-Quadratur (≤1 kHz), Button-Polling, Hall-DC.

---

## §1 — Power Tree (Sheet 1)

### Was es macht

Aus dem USB-C-Stecker (5 V von extern) oder dem Akku (3.0–4.2 V LiPo) macht
dieser Block die einzige Logik-Versorgung des Geräts: **+3V3** auf der ganzen
PCB. Zusätzlich erzeugt ein **Boost-Konverter** aus dem Akku +5 V für die
Speaker-Endstufe (PAM8403 mag keinen direkten LiPo-Range). Eingangs-Schutz
gegen Überstrom + ESD ist hier verortet.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `J1` | USB-C Receptacle TYPE-C-31-M-12 | Eingang 5 V + USB-D±-Daten + CC (Power-Negotiation 5 V default) | `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12` |
| `F1` | Polyfuse 1812L300 (3 A hold, 6 A trip) | Hard-Short-Schutz auf VBUS — bei Bass-Peaks (2,45 A worst case) hält, bei Kurzschluss trippt | `Fuse:Fuse_1812_4532Metric` |
| `D1` | USBLC6-2SC6 (TVS) | ESD-Diodenarray für VBUS + D+ + D− (8 kV Kontakt, 15 kV Air) | KiCad-Standard SOT-23-6 |
| `U5` | AP7361A-33ER (LDO) | +5 V → +3V3, 1 A, low-noise (für Analog-Audio kritisch) | KiCad-Standard SOT-223 |
| `C_BULK` | 1000 µF 16 V Alu-Elko | Reservoir-Kondensator für Class-D-Bass-Peaks. *Wichtigster anti-Brumm-Hebel* (ADR-0010 §4) | KiCad-Standard 8 mm-Elko |
| diverse | 4,7 µF + 100 nF X7R | Lokales Decoupling pro IC | 0603 |

> Boost-Konverter `U8` (TPS61089), Speicherdrossel `L1`, USB↔Akku-Power-Path-MOSFET `Q1` und der Lade-IC `U7` (MCP73831) leben im **Battery-Sheet** (§7), nicht hier — sie bilden konzeptionell die untere Hälfte des Power-Trees, sind aber im Generator beim Akku-Block geclustered weil sie alle den Akku-Pfad anfassen.

### Wie es fließt

```
USB-C VBUS ──F1── Power-Path Q1 ──┬── +5V_RAIL ──┬── PAM8403 (Speakers)
                                  │              └── AP7361A LDO ──┬── +3V3_RAIL
LiPo-Akku ────────TPS61089 Boost──┘                                 │
                                                                    └── alle Logik (MCU, MCP, PCA, DAC, LCD)
```

USB-D+/D− gehen durch `D1` zum MCU (USB-OTG-FS für Firmware-Updates per DFU).

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `F1` polyfuse trippt | Gerät komplett tot bis Polyfuse abgekühlt (~30 s) | warten oder echte Ursache (Kurzschluss?) finden |
| `Q1` P-MOSFET | USB lädt, Akku-Betrieb tot — oder umgekehrt | Q1 ersetzen |
| `U8` Boost | Speakers tot, Logik (3V3) lebt | U8 + L1 prüfen |
| `U5` LDO | Gerät komplett tot (kein 3V3) | LDO prüfen — kann thermisch aus, wenn Layout-Ground schlecht |
| `C_BULK` | Brummt bei Bass, klingt schlecht, evtl. Boost-Aussetzer | Elko tauschen, Polarität prüfen |

### Warum gerade diese Wahl?

- **TYPE-C-31-M-12** statt 6-Pin-Variante: Brauchen Daten (USB-DFU), nicht nur
  Power. 16-Pin-Original (r18.19-Revert) garantiert D+/D−/CC am Pad.
- **3 A Polyfuse statt 2 A**: 2,45 A Bass-Peak gemessen → 2 A würde trippen
  bei lauter Wiedergabe. 3 A hat ~25 % Reserve unter 50 °C-Innentemp.
- **TPS61089 + Sunlord-Drossel**: Boost-Konverter mit niedrigem Schaltrauschen
  unter Audio-Band. Wichtig: Bulk-Cap muss **< 5 mm** vom PAM8403-PVDD-Pin
  liegen, sonst koppelt Class-D-Switching auf den DAC-Output (ADR-0010 §4).
- **AP7361A-33ER LDO** statt einfacher Boost-direkt-zu-3,3V: LDO ist *low-noise*,
  Boost ist *switcher*. Audio-Analog hängt am LDO-3V3, nicht am Boost-Output —
  sonst hört man das Switching im Headphone-Out.
- **Power-Path-MOSFET Q1**: hält den Akku frisch, wenn USB angeschlossen ist
  (lädt + speist gleichzeitig).

### Sleep-Architektur (ADR-0016 — geplant)

Geplante Hardware-Erweiterung in diesem Sheet:
- `U9` TPS22918DBVR Load-Switch (SOT-23-6) gated den 3V3-Pfad zu den 5
  Hall-Sensoren (HALL_VDD-Net) — diese ziehen ~17,5 mA und sind im Sleep der
  echte Stromfresser, nicht der MCU. Mit Load-Switch fallen wir auf <100 µA
  Idle. Siehe [ADR-0016](../decisions/ADR-0016-power-sleep-architecture.md).

---

## §2 — STM32H743VIT6 (Sheet 2)

### Was es macht

Der MCU ist das **Herz**: hostet die komplette Audio-DSP-Engine (Pad / Reverb
/ Bass / Texture / Drone — siehe `firmware-c-next/src/*.c`), liest Cells +
Encoder + Buttons, treibt das Display, spuckt I²S an den DAC raus. ARM
Cortex-M7 480 MHz mit FPU + 1 MB SRAM + 2 MB Flash, LQFP-100-Gehäuse.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `U1` | STM32H743VIT6 | Der MCU selbst | `Package_QFP:LQFP-100_14x14mm_P0.5mm` |
| `Y1` | ABLS-8.000MHZ-B4-T (Crystal) | HSE-Takt für PLL → SYSCLK 480 MHz; Audio-PLL3 für saubere SAI-Clock (ADR-0010 §3) | `field_ambience:Crystal_HC49-US-SMD_ABLS` (Custom) |
| `C_HSE_1/2` | 18 pF C0G | Load-Caps für Y1, auf den ESR + GM=2,97 abgestimmt (Crystal-Review F-4) | 0603 |
| `C_VCAP1/2` | 2× 2,2 µF X5R (ESR < 100 mΩ) | Interner SMPS-Mode-Bypass (Datasheet Table 24) | 0805 |
| `R_NRST` | 10 kΩ Pull-Up | externer NRST-Pull-Up (interner ~30–50 kΩ wäre ausreichend, externer dominiert + EMI) | 0603 |
| `C_NRST` | 100 nF | NRST-Spike-Killer | 0603 |
| `R_BOOT0` | 10 kΩ Pull-Down | BOOT0 default = 0 → Boot aus Flash. Drücken von `SW_BOOT` zieht hoch → Boot in DFU-Bootloader | 0603 |
| `SW_BOOT` | XUNPU TS-1088-AR02016 (Mini-SMD-Tact) | BOOT0-Override für USB-DFU-Flash. **Wird ab ADR-0016 zusätzlich Wake/Sleep-Taster** | `field_ambience:SW_TS1088_SMD` (Custom) |
| `SW11` | gleiches Teil wie `SW_BOOT` | Manueller Reset auf NRST | gleicher FP |
| Decoupling | 5× (4,7 µF + 100 nF) auf VDD-Pins | Lokales Bypassing, je 1 Paar pro VDD-Pin (Pins 11, 27, 50, 75, 100) | 0603 |
| VDDA-Filter | Ferrit + 1 µF + 100 nF | VDD → VDDA Trennung, dämpft Digital-Switching auf den Analog-Pfad | Ferrit 0805, Caps 0603 |

### Pin-Allocation (Stand SPEC v0.7 §5 — verifiziert in U1-Review)

| Funktion | Pin | Net |
|---|---|---|
| Audio I²S out | 4/5/6 | I2S_BCK, I2S_LRCK, I2S_DOUT (SAI1-A AF6) |
| LCD SPI | 29/31/30 + 32/33 | LCD_SCK, LCD_MOSI, LCD_CS, LCD_DC, LCD_RES |
| I²C (MCP+PCA) | 92/93 | I2C_SCL, I2C_SDA (AF4) |
| MIDI TX | 86 | MIDI_TX (UART, DNP-Variante) |
| Encoder TIMs | div. | DRIVE_A/B, BRIGHT_A/B, VOL_A/B, DISPLAY_A/B (Hardware-Quadratur über TIM1–5) |
| USB-OTG-FS | 70/71 | USB_DM, USB_DP (AF10) |
| SWD | 72/76 | SWDIO (PA13), SWCLK (PA14) |
| HSE | 12/13 | OSC_IN, OSC_OUT |
| BAT_SENSE | 25 | PA3 (ADC1_INP15) — Akku-Spannungsteiler |

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `Y1` Crystal | MCU bootet, läuft auf HSI (intern, 64 MHz), Audio-Clock falsch → Pitch + Sample-Rate verstellt | Crystal-Lötstellen prüfen; im Datasheet GM=2,97 ist Indoor-OK aber knapp |
| `C_HSE_1/2` | Crystal startet evtl. nicht (Lastkapazität fehlt) | Caps nachlöten, Wert prüfen |
| Decoupling fehlt | Random Resets unter Last, USB-DFU schlägt fehl, Audio knackt | Phase-5-Profiling-Phase erkennt das; Layout-Constraint: jedes Paar < 3 mm vom VDD-Pin |
| `SW_BOOT` defekt | DFU-Flash nicht möglich (Workaround: BOOT0 mit Drahtbrücke nach 3V3 ziehen) | Taster ersetzen |
| VDDA-Ferrit Kurzschluss | ADC + Audio voll von Digital-Noise → hörbar als Grießeln | Ferrit ersetzen |

### Warum gerade diese Wahl?

- **STM32H743 statt H7B0 / H750**: 2 MB Flash + 1 MB SRAM gibt Reverb-Tails +
  Brain-State problemlos Platz; SAI1 + PLL3 ergeben saubere Audio-Clock (PLL3
  speist SAI direkt → kein SYSCLK-Jitter, ADR-0010 §3). 480 MHz mit VOS0
  reicht für die DSP-Last + UI.
- **LQFP-100 statt BGA**: handlötbar, JLC kann es bestücken, kein X-Ray
  nötig. BGA wurde bewusst verworfen.
- **8 MHz HSE statt MEMS-Oszillator**: ABLS-Crystal mit 80 Ω ESR, GM=2,97
  Worst-Case — Indoor-Use real ~5–6 (Crystal-Review F-4). Bewusst akzeptiert,
  spart 1–2 $.
- **Interner SMPS-Mode**: nutzt VCAP-Caps statt externer DC/DC. Kleiner +
  einfacher. ADR-0010 §3 bestätigt: Audio-Clock kommt aus PLL3 (nicht
  SYSCLK), daher koppelt der SMPS nicht auf BCK.

---

## §3 — Audio Chain (Sheet 6)

### Was es macht

Die zweite Hälfte der Sound-Pipeline. MCU spuckt **I²S 16-Bit 44,1 kHz** an
einen externen DAC (`U3`), der DAC fährt entweder die Class-D-Speaker-Endstufe
(`U4`) ODER die Line-Out-Buchse `J8`. **Sub-Bass-Layer geht NUR an Line-Out**
(ADR-0010 §6) — die 40-mm-Speaker können keinen Sub-Bass, das wäre nur
Geklapper.

Was bis zum I²S-Stream geschickt wird, ist die fertige DSP-Mischung aus
firmware-c-next/src/: Pad + Texture + **Ambience (Wind / Rain / Waves /
Vinyl — ADR-0017 Phase 2)** + Bass + Drone + Reverb → Master mit DC-Block +
soft-Limiter → Int16. Die *Ambience*-Schicht macht aus Tokyo akustisches
Nacht-Stadt-Atmen, aus Coast Sonnenuntergang-Wellen, etc. — sie sitzt
software-seitig im Engine-Mix-Bus, nicht in der PCB.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `U3` | PCM5102APWR | I²S → Stereo-DAC, 32-Bit-Resolution, interne PLL (synct sich auf BCK ohne MCLK). Eigene AVDD-Versorgung über Ferrit-Bead. | TSSOP-20 KiCad-Standard |
| `U4` | PAM8403DR-H | Stereo Class-D-Amp, 3 W/ch @ 4 Ω, BTL-Output. `AMP_SHDN_N` (active-low Shutdown) vom MCP23017 gated. | SO-16-150mil KiCad-Standard |
| `J7` | Speaker-Header 2×2 Pin (PUI AS04008PS, 8 Ω, 40 mm) | Speaker-Anschluss BTL — 2 Drähte pro Kanal | Pin-Header 2,54 mm |
| `J8` | PJ-320D 3,5 mm TRS (mit Insertion-Detect) | Line-Out / Kopfhörer. Insertion-Detect-Pin gated optional die Speaker-Amp (Auto-Mute beim Einstecken) | `field_ambience:Jack_3.5mm_PJ-320D_SMT` (Custom EasyEDA-CAD) |
| `J9` | PJ-320D MIDI-OUT — **DNP für 5er-Run** (ADR-0004 r18.30) | 2× 220 Ω Resistor pair + UART-TX. Reaktivierbar durch Bestücken + `midi_tx_init()` | gleicher FP, DNP |
| `FB1` | BLM18AG601 (Ferrit-Bead) | AVDD-Trennung DAC (Digital-Rail → Analog-Rail) | 0603 |
| `C_AVDD` | 10 µF + 100 nF X7R am Ferrit-Output | DAC-AVDD-Decoupling | 0603 |
| `C_AMP` | 1 µF + 100 nF auf PAM8403 PVDD | Amp-lokales Decoupling | 0603 |

### Wie es fließt

```
STM32H743 SAI1
   │ BCK/LRCK/DOUT
   ▼
PCM5102A (U3)
   │ Analog L/R
   ├──► PAM8403 (U4) ──► J7 Speaker-Header ──► 2× PUI AS04008PS 40 mm
   │           ▲
   │           │ AMP_SHDN_N (MCP23017 GPA4) — Mute aus Firmware
   │
   └─────────► J8 3,5 mm Line-Out (TRS) ──► Kopfhörer / Mixer / Aktiv-Box
                   │
                   └─ Insertion-Detect ──► MCP23017 (optional Auto-Speaker-Mute)
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `U3` PCM5102A | Komplett stumm an allen Outs (Line-Out + Speakers) | DAC oder I²S-Verkabelung prüfen |
| `U4` PAM8403 | Speakers stumm, Line-Out lebt | Amp prüfen — oft thermisch oder Strapping-Pin falsch |
| `FB1` Ferrit | Digital-Switching grießelt im Headphone-Out | Ferrit tauschen |
| `J7` Speaker-Header lose | Speaker brüllt, Brummen, evtl. Amp thermisch | Header neu löten |
| Speaker fällt aus Mesh | Hörbar dünn — und mechanisch oft Folge eines lose gewordenen Mesh-Klebepunkts | Membran + Mesh checken (ADR-0007) |

### Akustik — was die PCB **nicht** löst (mechanische Aufgaben)

Klingt das Gerät am Ende wie Plastik-Spielzeug? Hängt **nicht** an der PCB,
sondern an drei mechanischen Dingen:

1. **Geschlossene Akustikkammer pro Speaker** — der wichtigste Hebel. 40 mm
   Driver in offener Plastikhülle = blechern. 40 mm Driver in versiegelter
   ~5 cm³-Kammer + Dust-Mesh (ADR-0007) = überraschend gut, OP-1-Niveau. **Im
   Gehäuse-CAD vorsehen — kein BOM-Eintrag, aber kritisch.**
2. **EQ-Pre-Filter im Pad-Render-Pfad** — der DSP rollt unter 80 Hz ab fürs
   Speaker-Routing (ADR-0010 §6 + `engine.c`), damit der Treiber nicht
   mechanisch überlastet wird. Sub-Bass landet nur am Line-Out.
3. **Passive Membran** — *optional* on top der geschlossenen Kammer. Zusatz-
   Bauteil (25–30 mm PR-Membran pro Seite, ~1–3 $/Stück), erweitert f3 von
   ~250 Hz auf ~150 Hz. Nicht critical-path; entscheiden nach Hörtest auf
   Prototyp.

**Honest take:** Line-Out (`J8`) ist die *echte* Hörerfahrung. Speakers sind
"convenience ohne Kopfhörer" — das Gerät richtig zu positionieren ist Punkt
des Sound-Designs, nicht ein Versuch, 40 mm zu HiFi zu prügeln.

### Warum gerade diese Wahl?

- **PCM5102A statt billigerer PT8211**: Interne DAC-PLL synct sich auf BCK
  → kein externer MCLK nötig → ein SAI-Pin weniger. Saubere 112 dB SNR.
- **PAM8403 statt MAX98357A I²S-Amp**: PAM8403 ist analoger Class-D — wir
  *wollen* den Analog-Pfad zwischen DAC + Amp, damit der Line-Out gleichzeitig
  möglich ist (MAX98357 hat keinen Analog-Output).
- **8 Ω / 40 mm PUI statt 4 Ω / 28 mm**: 40 mm gibt physikalisch mehr Membran-
  Fläche → mehr Mid-Lautstärke (alles unter 250 Hz ist sowieso nur am
  Line-Out). 8 Ω passt zur PAM8403-Optimierung.
- **PJ-320D-Jack mit Insertion-Detect**: Auto-Mute der Speakers beim
  Einstecken — Standard-User-Erwartung.

---

## §4 — MCP23017 + PCA9685 + Cells + Buttons + LEDs (Sheet 4)

**Generator:** `mcp_sheet()` Zeile 3807 in `generate_kicad_project.py`.

### Was es macht

Das **I/O-Outsourcing**-Sheet. Der MCU hat „nur" 100 Pins (LQFP) und davon
sind ~30 an Audio + Display + USB + Encoder gebunden. Knöpfe, LEDs und Cells
landen daher auf zwei dedizierten I²C-Slave-Chips, die jeweils 16 Kanäle
nachreichen. Der MCU spricht beide über *einen* I²C-Bus (`I2C_SCL`/`I2C_SDA`,
4,7 kΩ Pull-Ups) an.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `U2` | MCP23017-E/SS | I²C-GPIO-Expander, 16 Kanäle: 10 Mini-Knöpfe (Modifier + Drone + Hold + Shift + Generate + Clear) + Speaker-Insertion-Detect (`J8`) + 4× Encoder-Push (gating-Reserve) + `AMP_SHDN_N` + `AMP_MUTE_N` + `PCM_XSMT` | SSOP-28 KiCad-Standard |
| `U6` | PCA9685PW | I²C-PWM-Driver, 16 Kanäle: 5 Cell-LEDs + 5 Modifier-LEDs + Backlight-FET-Gate (Kanal 12, gated `Q2` Low-Side) + Reserve-Kanäle | TSSOP-28 KiCad-Standard |
| `R5` | 4,7 kΩ 0603 | `I2C_SCL` Pull-Up (gemeinsam für beide Chips, 100 kHz I²C-Fast) | 0603 |
| `R4` | 4,7 kΩ 0603 | `I2C_SDA` Pull-Up | 0603 |
| `R6` | 10 kΩ 0603 | MCP23017 `RESET_N` Pull-Up (active-low, default released) | 0603 |
| `R20` | 10 kΩ 0603 | MCP23017 `INTA` Open-Drain Pull-Up (Interrupt-Pin zum MCU) | 0603 |
| `C5` | 100 nF X7R 0603 | MCP23017 VDD-Decoupling | 0603 |
| `C5b` | 10 nF X7R 0603 | MCP23017 VDD-HF-Decoupling (Pärchen-Strategie für saubere I²C-Edges) | 0603 |
| `D_CELL1..5` | Cell-LEDs (5×) — PCA9685-getrieben | Visualisiert Cell-Velocity (Brightness ∝ Anschlag) | LED 0603 / 0805 — *UNVERIFIED: konkrete PN noch zu finalen* |
| `D_MOD1..5` | Modifier-LEDs (gelb, rot, weiß, grün) — PCA9685-getrieben | Status der 5 Modifier-Buttons (Hold/Drone/Shift/Generate/Clear) | LED 0603 — siehe BOM §"Modifier-LEDs" |
| `SW1..10` | XUNPU TS-1088-AR02016 Mini-Tact (10×) | Modifier-Buttons (Top-Reihe) + Reserve | `field_ambience:SW_TS1088_SMD` (Custom, EasyEDA-verifiziert) |
| `HALL1..5` | DRV5056A4 (5×) Linear-Hall (ratiometric) | Cell-Velocity-Pickup — misst Magnetfeld-Verschiebung beim Tap (ADR-0013) | `Package_TO_SOT_SMD:SOT-23` |

### Wie es fließt

```
STM32H743 I²C1 (PB6/PB7 AF4) ──┬──► U2 MCP23017 (Addr 0x20)
                               │       └──► 10× Modifier-Buttons (Pull-Up + Pin)
                               │       └──► AMP_SHDN_N + AMP_MUTE_N (Audio-Mute)
                               │       └──► PCM_XSMT (DAC-Mute, GPA5)
                               │       └──► J8 Insertion-Detect
                               │       └──► INTA ───► MCU GPIO (`MCP_INT`, Pin 7 PC13)
                               │
                               └──► U6 PCA9685 (Addr 0x40)
                                       └──► 10× LED-Kanäle (Konstant-Strom über 0603-Widerstände)
                                       └──► Backlight-FET (Q2-Gate) auf Kanal 12

HALL1..5 (DRV5056A4) ──► Analog-Out ──► STM32H743 ADC (CELL1..5_SENSE)
   ▲
   └── VDD = HALL_VDD (gegated durch U9, ADR-0016) — im Sleep aus
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `U2` MCP23017 | Alle 10 Buttons tot, Speaker bleibt unmuted, Insertion-Detect tot — aber DSP läuft weiter | I²C-Pull-Ups + Power + Adresse prüfen |
| `U6` PCA9685 | Alle LEDs aus, Backlight dunkel (FET-Gate floated) — Gerät spielt aber Sound | wie oben |
| `R4`/`R5` Pull-Ups | I²C-Bus floated → beide Chips antworten nicht → 1× Reset hilft nicht, Layout-Fehler | Pull-Up nachrüsten / Wert prüfen |
| 1× Hall-Sensor | Diese eine Cell reagiert nicht auf Tap; andere 4 funktionieren | Sensor ersetzen; ADC-Eingang gegen Magnet-Drift testen |
| `U9` Load-Switch (ADR-0016) hängt | HALL_VDD nicht da → alle 5 Cells tot | Load-Switch-Enable-Signal vom MCU prüfen, `LSW_EN` Pin |

### Warum gerade diese Wahl?

- **MCP23017 statt MCU-GPIOs direkt**: spart 14 MCU-Pins die wir anderswo
  brauchen (Audio + Display + Encoder + USB). I²C ist langsam (max ~400 kHz),
  aber für Tasten + Mute-Steuerung weit ausreichend.
- **PCA9685 statt PWM aus dem MCU**: 16 echte PWM-Kanäle parallel (12-Bit
  Auflösung, 1500 Hz Refresh) ohne MCU-Last. Der MCU hat zwar TIM-Kanäle
  satt, aber die sind für Encoder-Quadratur reserviert.
- **DRV5056A4 statt Reed-Switches / FSR-Pads**: ratiometric Hall liefert
  *kontinuierliche* Position → echte Anschlagsgeschwindigkeit (Velocity), nicht
  nur On/Off. Macht das musikalische Spiel "anschlagsdynamisch" — ADR-0013.
- **TS-1088-AR02016 Mini-Tact**: kleinster vernünftiger SMD-Taster, JLC-fertig,
  ~3 mm hoch — passt in flaches Gehäuse.

---

## §5 — Encoders (Sheet 5)

**Generator:** `encoder_sheet()` Zeile 4591.

### Was es macht

Vier Drehgeber für die vier Macro-Encoder (Display, Drive, Brightness,
Volume). Jeder Encoder liefert zwei Quadratursignale (`A`, `B`, 90° versetzt)
+ einen Push-Switch. Quadratur wird per Hardware-Timer im STM32 dekodiert
(TIM-QEI-Modus) — kein Polling, kein Jitter, kein verlorener Detent.

### Bauteile (pro Encoder × 4)

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `EN1..4` | ALPS EC11E18244AU (18 Pulse, 36 Detents/U, mit Push) | Der Encoder selbst. Einheitlich gleicher Part für alle vier — ADR-0012 (NRND-Familie ALPS, einheitlicher SKU spart Stock-Risiko) | `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` |
| `R_A`/`R_B` (8×) | 10 kΩ Pull-Up auf jeden A/B-Pin | Quadrature-Eingang ist normalerweise floating zwischen Detents; Pull-Up gibt definierten "released"-State | 0603 |
| `C_A`/`C_B` (8×) | 100 nF | RC-Filter (10 kΩ × 100 nF ⇒ τ = 1 ms) — entprellt mechanisches Klingeln am Detent | 0603 |
| `R_SW` (4×) | 10 kΩ Pull-Up auf Push | Push-Switch normally-open, Pull-Up zieht hoch wenn nicht gedrückt | 0603 |

### Wie es fließt

```
ALPS EC11E   ── A ──┬── (RC) ──► STM32 TIMx_CH1 (Quadrature-Encoder-Modus)
                    └── R-PU zu +3V3
             ── B ──┬── (RC) ──► STM32 TIMx_CH2
                    └── R-PU zu +3V3
             ── SW ──┬── (R-PU) ──► STM32 GPIO (oder MCP23017)
                     └── GND wenn gedrückt
```

Mapping der vier Encoder zu STM32-Timern siehe SPEC §5 Pin-Allocation:
`EN_DRIVE → TIM2`, `EN_BRIGHT → TIM3` (oder TIM8), `EN_DISPLAY → TIM4`,
`EN_VOLUME → TIM1` — alle als QEI konfiguriert.

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `EN_x` mechanisch | "totes Klicken", Drehen passiert nichts | Encoder ersetzen — JLC-Stock prüfen, ALPS-Familie NRND |
| RC-Filter-Cap fehlt | Encoder funktioniert, aber jeder Detent zählt 2× oder 3× wegen mechanischem Prellen | Cap nachlöten |
| Pull-Up fehlt | A/B floated → Timer-QEI sieht Müll | Widerstand nachrüsten |
| Push-Switch defekt | Drehen geht, drücken nicht (oder umgekehrt) | Mech-Lebensdauer ALPS ≈ 30 k Cycles, normal viele Jahre — bei früherem Ausfall RMA |

### Warum gerade diese Wahl?

- **ALPS EC11E18244AU für alle vier statt zwei Varianten (Push+Detent vs.
  Smooth)**: ADR-0012 — die ursprünglich vorgesehene Smooth-Variante
  (EC11E1834403) ist NRND, alle Konkurrenten in der Familie auch. Einheitlich
  Push+Detent erkauft sich UX-mäßig durch Firmware-Velocity-Acceleration
  (langsam = 1 %/Klick, schnell = ×8/Klick) — gleiche „Smooth-Like"-Feel ohne
  zwei BOM-Stock-Linien.
- **Hardware-Quadratur (TIM-QEI) statt GPIO-Interrupt**: keine verlorenen
  Pulse bei schnellem Drehen, kein Software-Debounce nötig. Der STM32 hat
  genug Timer (TIM1/2/3/4/5/8), um alle 4 Encoder hardwareseitig zu lesen.
- **RC-Filter 1 ms statt nur Software-Debounce**: schließt mechanisches
  Klingeln *vor* dem Pin aus → Timer-QEI sieht saubere Edges. Standard-EC11-
  Mechanik prellt 0,1–0,5 ms; 1 ms RC ist sicher drüber.

---

## §6 — LCD (Sheet 3)

**Generator:** `lcd_sheet()` Zeile 3435.

### Was es macht

Treibt das **Waveshare 1,9″ ST7789V2 IPS-LCD** (320 × 170, RGB565) per SPI.
Das Display ist als *Modul* aufgesteckt (8-Pin-Header `J3`), nicht ein bares
Panel — Waveshare integriert Treiber-IC, Level-Shifter, Backlight-LED in einem
kleinen Trägermodul. Backlight wird vom MCU per **PCA9685-PWM** über einen
N-FET (Low-Side) heller/dunkler.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `J3` | 1×8 Receptacle 2,54 mm Pin-Header | Aufnahme des Waveshare-LCD-Moduls | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` |
| LCD-Modul | Waveshare 1.9" 170×320 ST7789V2 | Anzeige selbst — kein PCB-Footprint (steckt in `J3`). **PROPOSED:** Pivot 2,0″ 240×320, ADR-0015 | (Modul) |
| `Q2` | 2N7002,215 (N-MOSFET) | Backlight-Low-Side-Switch: PCA9685-PWM (Open-Drain) gated Q2-Gate → schaltet Backlight-LED-GND | `Package_TO_SOT_SMD:SOT-23` |
| `C6b` | 10 µF X7R | Lokales LCD-Modul VCC-Bulk | 0603 |
| `C6c` | 100 nF X7R | Lokales LCD-Modul VCC-HF | 0603 |
| `R_BL_GATE` | Gate-R (1–10 kΩ) am Q2 | Slew-Limit + Default-OFF beim Power-Up | 0603 — *UNVERIFIED: Wert noch zu finalen* |

### Wie es fließt

```
STM32H743 SPI1 (AF5)
   ├── SCK   (PA5) ──► J3.SCK
   ├── MOSI  (PA7) ──► J3.SDA
   ├── CS    (PA6, GPIO) ──► J3.CS
   ├── DC    (PC4, GPIO) ──► J3.DC
   └── RES   (PC5, GPIO) ──► J3.RES

PCA9685 ch12 (LCD_BLK_PWM) ──► Q2 Gate ──► Q2 Drain = LCD_BL_LED_K (Kathode)
                                                      ▲
LCD_BL_LED_A (Anode) ◄── +3V3                          GND
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| LCD-Modul tot | Display schwarz — Backlight aus? Strom prüfen. Backlight an, kein Bild? SPI-Pins prüfen | Modul tauschen (steckt im Header) |
| `Q2` Open | Backlight bleibt aus oder permanent an (je nach Fail-Mode) | FET tauschen |
| `J3` Lötstellen | Display flackert / startet nicht | Header neu löten |
| PCA9685 ch12 tot | Backlight dunkel, sonst funktional | PCA9685-Kanal wechseln im Layout *oder* in Firmware Notlauf-Backlight statisch |

### Warum gerade diese Wahl?

- **Waveshare-Modul statt bares ST7789-Glas**: das nackte Glas am
  AliExpress-Modul ist DOA-Lotterie; Waveshare hat QC, integrierten
  Level-Shifter (3V3-MCU spricht mit 3V3-Modul ohne extra ICs), dokumentiertes
  Pinout. ~11–13 $ statt Adafruit 15 $ oder 6 $ AliExpress mit 30 % Ausfall.
- **PCA9685-Backlight statt MCU-PWM**: spart einen TIM-Kanal am MCU (alle TIMs
  sind bei den Encodern). PCA9685 hat noch genug ungenutzte Kanäle.
- **N-FET Low-Side statt P-FET High-Side**: einfacher Gate-Drive (PCA9685 ist
  Open-Drain, kann nach GND ziehen → N-FET gates direkt), kein Spannungspegel-
  Problem.

---

## §7 — Battery, Charger, Boost & Power-Path (Sheet 7)

**Generator:** `battery_sheet()` Zeile 5994.

### Was es macht

Die *zweite* Hälfte des Power-Trees: vom **LiPo-Akku** auf der einen Seite
zum **+5V-Rail** auf der anderen. Lade-Pfad (USB-VBUS via MCP73831 in den
Akku), Boost-Konverter (Akku → 5 V für die Speaker-Amp), Power-Path-Switch
(automatisches USB ↔ Akku) und Akku-Spannungsmessung sitzen alle hier.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `J_BAT` / `J9` | JST-PH 2.0 2-Pin (S2B-PH-SM4-TB) | Akku-Steckverbinder. **Im Inneren erreichbar** — ABR. trennen für echtes Lagern (ADR-0016) | `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal` |
| LiPo-Pouch | 503759 (50×37×9,4 mm, 2000 mAh) | Energiespeicher. Single-Cell 3,0–4,2 V. Bottom-Case-Slot — kein PCB-Footprint | — (Vendor) |
| `U7` | MCP73831T-2ACI/OT | LiPo-Single-Cell-Lader. CV/CC-Modus, programmierbarer Ladestrom über `R21`. 500 mA bei `R_PROG`=2 kΩ. `STAT`-Pin → Lade-LED | `Package_TO_SOT_SMD:SOT-23-5` |
| `R21` | 2 kΩ 0603 | Sets `Icharge = 1000 / R_PROG` → 500 mA (~0,25C bei 2000 mAh = sanftes Laden) | 0603 |
| `D_STAT` | Lade-LED (rot) | Leuchtet während CV-Charging, geht aus bei Full | LED 0603 |
| `U8` | TPS61089RNR Boost | Akku 3,0–4,2 V → 5,0 V für PAM8403. Synchroner Boost, 2 A continuous. Wenn USB anliegt: bypasst über `Q1`-Pfad | `field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A` (Custom) |
| `L1` | 2,2 µH 5 A Shielded SWPA6045 | Boost-Speicherdrossel. Geschirmt (kein EMI-Streufeld in die Audio-Region) | `field_ambience:L_Sunlord_SWPA6045` (Custom) |
| `R23` | 200 kΩ 0603 | TPS61089 FB-Divider Top — setzt Vout=5,0 V (Vout = 0,6 · (R23+R24)/R24) | 0603 |
| `R24` | 39 kΩ 0603 | FB-Divider Bottom | 0603 |
| `D3` | SS34 (40 V 3 A Schottky) | Boost-Output-Reverse-Schutz — verhindert Rück-Strom in den Boost wenn USB-VBUS hochkommt | KiCad-Standard SMA |
| `Q1` | DMG2305UX (P-MOSFET) | Power-Path: VBUS schaltet Q1-Gate (durch `R22` Pull-Down default-OFF, USB-VBUS pulled high via Gate-Drive) → bei USB-Plug verbindet sich der USB-Pfad direkt aufs 5V-Rail; im Akku-Betrieb sperrt Q1 | `Package_TO_SOT_SMD:SOT-23` |
| `R22` | 10 kΩ 0603 | Gate-Pull-Down — Q1 default OFF, schaltet erst wenn VBUS hochkommt | 0603 |
| `R_BAT_SENSE_A`/`B` | Spannungsteiler (z. B. 100 k : 100 k) | Akku-Spannung halbiert auf MCU-ADC-Pin `BAT_SENSE` (PA3 / ADC1_INP15) | 0603 — *UNVERIFIED: konkrete Werte/Refs noch festzulegen* |

### Wie es fließt

```
USB-C VBUS (5 V) ──┬── MCP73831 VIN ──► CHARGE ──► LiPo+ ──► J_BAT
                   │
                   └──► Q1 P-MOS Source     LiPo+ ──► TPS61089 VIN ──► L1 ──► +5V_BOOST
                            │ Gate-Pull-Down (R22)             │
                            ▼ Drain                            └──► D3 Schottky ──┐
                          +5V_RAIL ◄───────────────────────────────────────────────┘
                            │
                            ├──► AP7361A LDO ──► +3V3 (Sheet 1)
                            └──► PAM8403 (Audio)

LiPo+ ──► R_BAT_SENSE-Divider ──► STM32 ADC (`BAT_SENSE`)
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `U7` MCP73831 | Akku lädt nicht (USB liegt an, aber Spannung steigt nicht) | Lader prüfen, R21 (kein Kurzschluss?), Lade-LED-Pfad |
| `R21` falscher Wert | Ladestrom zu hoch (Akku zu heiß) oder zu klein (lädt ewig) | Auf 2 kΩ ±1 % prüfen |
| `U8` TPS61089 | Im reinen Akku-Betrieb: kein +5 V → Speaker tot, +3V3 tot (LDO speist sich aus +5V) — USB-Betrieb funktioniert noch | Boost-Layout prüfen (Bulk-Caps, Drossel-Lötstellen) |
| `L1` Drossel-Lötstelle | Boost macht Mucken (Aussetzer, Pfeifen) | Drossel neu löten, magnetische Sättigung ausgeschlossen? |
| `Q1` defekt offen | Im USB-Betrieb läuft der Akku-Pfad mit, lädt + entlädt gleichzeitig — Akku-Stress | Q1 ersetzen |
| `D3` Schottky kurz | Boost speist *in* die LiPo zurück bei USB-Betrieb → MCP73831 verwirrt | D3 prüfen — Polarität extrem wichtig (Anode = TPS61089-Output) |
| `J_BAT` Lötstelle | Gerät startet nicht aus Akku, USB funktioniert noch | JST neu löten |

### Warum gerade diese Wahl?

- **MCP73831 statt smarter Lader (BQ24074 etc.)**: Single-Cell-LiPo,
  USB-VBUS-Eingang, kein I²C nötig — die simple analoge Lade-Maschine reicht.
  Spart 2–3 $ + 4–5 Bauteile.
- **TPS61089 statt einfacherem Boost**: programmierbare Schaltfrequenz +
  synchroner Boost = sauber unter dem Audio-Band; bei naiven Boost-Konvertern
  pfeift bei lauter Wiedergabe Class-D-Carrier-Mischprodukte (ADR-0010 §4).
- **Schottky `D3` statt MOSFET-OR**: einfacher, 1 Bauteil; der ~300 mV
  Vorwärts-Drop ist verschmerzbar (5,3 V Boost-Setting kompensiert).
- **JST-PH-Innen statt Slide-Switch außen**: gewollte Friction-Schwelle für
  echtes Lagern + spart Panel-Bohrung (ADR-0016).
- **R_BAT_SENSE 100 k : 100 k Divider**: 1:2 Reduktion (4,2 V max → 2,1 V am
  ADC-Eingang, sicher unter 3,3 V VDDA-Ref). Hoher Wert = niedriger
  permanenter Drain (4,2 V / 200 kΩ = 21 µA — vernachlässigbar gegen Sleep-
  Budget).

### Sleep-Architektur (ADR-0016)

Im Sleep-Mode:
- `U7` Lader bleibt aktiv — wenn USB anliegt soll das Gerät laden (auch im Sleep)
- `U8` Boost bleibt aktiv — er treibt aber nichts (Audio-Stages sind dann muted)
- `Q1` schaltet wie immer (USB-Plug-Wake nutzt diesen Pfad)
- Bat-Sense-Divider zieht 21 µA — bleibt an, sonst kann der MCU nicht
  Akku-Stand vor dem nächsten Wake lesen

---

## Maschinenlesbare Anker

Damit ein Tool (oder ein anderer Engineer) das hier durchsuchen kann, gilt:
- Jeder Refdes erscheint **genau wie auf der Platine** (Großbuchstaben + Zahl,
  keine Variation).
- Aktive-Low-Netze haben `_N`-Suffix; Differentialpaare `_P`/`_N`.
- Footprints sind als `<Library>:<Name>` notiert wie in KiCad. Custom-Lib =
  `field_ambience:*`, sonst KiCad-Standard.
- Cross-Refs zu BOM, ADRs und Component-Reviews als relative Markdown-Links.

## Erweiterung dieses Dokuments

- **Neuer Block?** Sektion mit "Was es macht / Bauteile / Wie es fließt / Was
  kaputt geht / Warum" einfügen. Format ist absichtlich rigide.
- **Bauteil-Wechsel?** Hier korrigieren UND
  [`BOM_MASTER.md`](../../../BOM_MASTER.md) UND den Generator. Anti-Drift-
  Regel: drei Stellen, alle gleich.
- **UNVERIFIED → verified:** Wenn du Datasheet-Belege hast, im
  [`component_reviews/`](../component_reviews/) ein 10-Punkte-Review
  anlegen + hier die UNVERIFIED-Markierung entfernen.
