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
| 4 | `mcp.kicad_sch` | `make_mcp_sheet()` ~Z. 4560 | MCP23017 (16 I/O over I²C) + PCA9685 (16 PWM für LEDs) + 10 Buttons (5 Cells SW1–5 auf Kailh-Choc-V1 direkt-gelötet + 5 Modifier SW6–10 auf HX-B3F-Tactile, alle digital am Expander) + 10 LEDs |
| 5 | `encoder.kicad_sch` | `make_encoder_sheet()` ~Z. 4845 | 4 EC11-Encoder mit Push + RC-Filter |
| 6 | `audio.kicad_sch` | `audio_sheet()` | PCM5102A I²S-DAC + PAM8406 Class-D-Amp + Speaker-Header + U11 TPA6132A2 HP-Amp (r19.19) + 3.5-mm-PHONES/LINE-OUT + (DNP) MIDI-Out |
| 7 | `battery.kicad_sch` | `battery_sheet()` | BQ24074 Power-Path-Charger (r19.18, ADR-0023) + F2 PTC + Akku-JST + TPS61089 Boost + Bat-Sense-Divider |

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
        │ U7 BQ24074 DPPM  │      via J9 + F2       │
        │ OUT=VSYS→U8 Boost│      (r19.18 ADR-0023) │
        └──────┬───────────┘                        │
               │ +5V_RAIL (Boost via D3)            │
               │                                    │
               ├────► PAM8406 Class-D ──► Speakers  │
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
        │  ◄── I²S ────► PCM5102A ─► TPA6132A2 ─► J8 ││
        │                          └─► PAM8406 ─► SP
        │  ◄── I²C ────► MCP23017 ─► 10× Buttons   ││
        │                  (5 Cells + 5 Modifier)  ││
        │  ◄── I²C ────► PCA9685 ──► 10× LEDs      ││
        │                          └─► Q2 BL-FET   ││
        │  ◄── TIM-QEI ─► 4× EC11 Encoder          ││
        │  ◄── USB-OTG-FS ─────────────────────────┘│
        └──────────────────────────────────────────┘
                       │ ADC1_INP15
                       └──◄── BAT_SENSE-Teiler (LiPo+)

   (Lade-Pfad: VBUS_FUSED → U7 BQ24074 → F2 → LiPo+ — aktiv sobald USB da)
   (Versorgung: BQ-OUT=VSYS → U8 TPS61089 + L1 → +5V_RAIL via D3 — einzige Quelle)
```

Drei Geschwindigkeits-Tiers im Signal:

1. **Schnell (≥10 MHz):** USB-D± (12 Mbps), SPI-LCD (24–32 MHz), SAI-I²S
   (1,4 MHz BCK aber MHz-Edges). → Diese alle auf Top-Layer mit
   ungebrochener GND-Plane drunter (ADR-0018).
2. **Mittel (100 kHz – 1 MHz):** I²C (100 kHz Fast, beide Slaves auf einem
   Bus), PCA9685-PWM-Träger, ADC-Sampling.
3. **Langsam:** Encoder-Quadratur (≤1 kHz), Button-Polling (Cells + Modifier über MCP23017).

---

## §1 — Power Tree (Sheet 1)

### Was es macht

Aus dem USB-C-Stecker (5 V von extern) oder dem Akku (3.0–4.2 V LiPo) macht
dieser Block die einzige Logik-Versorgung des Geräts: **+3V3** auf der ganzen
PCB. Zusätzlich erzeugt ein **Boost-Konverter** aus dem Akku +5 V für die
Speaker-Endstufe (PAM8406 mag keinen direkten LiPo-Range). Eingangs-Schutz
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

> Boost-Konverter `U8` (TPS61089), Speicherdrossel `L1` und der Power-Path-Charger `U7` (BQ24074, r19.18) leben im **Battery-Sheet** (§7), nicht hier — sie bilden konzeptionell die untere Hälfte des Power-Trees, sind aber im Generator beim Akku-Block geclustered weil sie alle den Akku-Pfad anfassen.

### Wie es fließt

```
USB-C VBUS ──F1──► VBUS_FUSED ──► U7 BQ24074 ─┐ (r19.18: Rail haengt NICHT mehr direkt am USB)
LiPo-Akku ──F2──────────────────► (BAT)      OUT=VSYS ──► TPS61089 ──D3──► +5V_RAIL ─┬─ PAM8406 (Speakers)
                                                                                     └─ U_PWR ─► LDO ─► +3V3 (alle Logik)
```

USB-D+/D− gehen durch `D1` zum MCU (USB-OTG-FS für Firmware-Updates per DFU).

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `F1` polyfuse trippt | Gerät komplett tot bis Polyfuse abgekühlt (~30 s) | warten oder echte Ursache (Kurzschluss?) finden |
| `U7` BQ24074 | Kein VSYS → komplett tot (USB und Akku) | TP_VSYS messen, §7 |
| `U8` Boost | Speakers tot, Logik (3V3) lebt | U8 + L1 prüfen |
| `U5` LDO | Gerät komplett tot (kein 3V3) | LDO prüfen — kann thermisch aus, wenn Layout-Ground schlecht |
| `C_BULK` | Brummt bei Bass, klingt schlecht, evtl. Boost-Aussetzer | Elko tauschen, Polarität prüfen |

### Warum gerade diese Wahl?

- **TYPE-C-31-M-12** statt 6-Pin-Variante: Brauchen Daten (USB-DFU), nicht nur
  Power. 16-Pin-Original (r18.19-Revert) garantiert D+/D−/CC am Pad.
- **3 A Polyfuse statt 2 A**: 2,45 A Bass-Peak gemessen → 2 A würde trippen
  bei lauter Wiedergabe. 3 A hat ~25 % Reserve unter 50 °C-Innentemp.
- **TPS61089 + Sunlord-Drossel**: Boost-Konverter mit niedrigem Schaltrauschen
  unter Audio-Band. Wichtig: Bulk-Cap muss **< 5 mm** vom PAM8406-PVDD-Pin
  liegen, sonst koppelt Class-D-Switching auf den DAC-Output (ADR-0010 §4).
- **AP7361A-33ER LDO** statt einfacher Boost-direkt-zu-3,3V: LDO ist *low-noise*,
  Boost ist *switcher*. Audio-Analog hängt am LDO-3V3, nicht am Boost-Output —
  sonst hört man das Switching im Headphone-Out.
- **BQ24074-Power-Path (r19.18, ADR-0023)**: haelt den Akku frisch bei USB
  (DPPM: Systemlast zuerst, Ladestrom dynamisch) und liefert ILIM, Timer,
  TS und CHG-Status in einem Chip — Details §7.

### Sleep-Architektur (ADR-0016 — geplant)

Geplante Hardware-Erweiterung (ADR-0016, in BOM_MASTER als `U_PWR` TPS22918):
- Der Load-Switch gated den gesamten 3V3-Pfad (`+5V_RAIL → +5V_SW` → LDO →
  +3V3: MCU + MCP + 2× PCA9685 + LCD) hinter dem Power-Schiebeschalter
  `SW_PWR`. Der Lade-Pfad (U7) sitzt davor → lädt weiter im Aus-Zustand.
  (Hinweis r18.73: die früheren 5 Hall-Sensoren — der ehemalige Sleep-
  Stromfresser ~17,5 mA — entfallen, Cells sind jetzt digital am MCP23017.)
  Siehe [ADR-0016](../decisions/ADR-0016-power-sleep-architecture.md).

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
(`U4`) ODER die PHONES/LINE-OUT-Buchse `J8` (via U11 HP-Amp, r19.19).
**Sub-Bass-Layer geht NUR an J8**
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
| `U4` | PAM8406DR | Stereo Class-D-Amp, BTL-Output (r19.37, ADR-0025: ersetzt NRND PAM8406; MODE=+5V→Class-D; RI 174k = Gain +4.3 dB; C_in 10nF = Speaker-HPF ~91 Hz). `AMP_SHDN_N` (active-low Shutdown) vom MCP23017 gated. | SO-16-150mil KiCad-Standard |
| `J7` | Speaker-Header 2×2 Pin (PUI AS04008PS, 8 Ω, 40 mm) | Speaker-Anschluss BTL — 2 Drähte pro Kanal | Pin-Header 2,54 mm |
| `U11` | TPA6132A2RTER (r19.19, ADR-0024) | DirectPath-Kopfhoererverstaerker: DAC → CIN 1µF → U11 (Gain −6 dB, EN=AMP_nSHDN) → 22 Ω → J8. Ladungspumpe intern (C_FLY_HP/C_HPVSS), HPVDD nur an 2,2 µF (NIE an VDD!) | `Package_DFN_QFN:QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm` |
| `J8` | PJ-320D 3,5 mm TRS (mit Insertion-Detect) | **PHONES / LINE OUT** (r19.19): Kopfhörer 16 Ω+ UND Line-Eingänge, niederohmig getrieben von U11. Insertion-Detect → Firmware mutet NUR die Speaker (Auto-Mute beim Einstecken, wieder an beim Ausstecken) | `field_ambience:Jack_3.5mm_PJ-320D_SMT` (Custom EasyEDA-CAD) |
| `J9` | PJ-320D MIDI-OUT — **DNP für 5er-Run** (ADR-0004 r18.30) | 2× 220 Ω Resistor pair + UART-TX. Reaktivierbar durch Bestücken + `midi_tx_init()` | gleicher FP, DNP |
| `FB1` | BLM18AG601 (Ferrit-Bead) | AVDD-Trennung DAC (Digital-Rail → Analog-Rail) | 0603 |
| `C_AVDD` | 10 µF + 100 nF X7R am Ferrit-Output | DAC-AVDD-Decoupling | 0603 |
| `C_AMP` | 1 µF + 100 nF auf PAM8406 PVDD | Amp-lokales Decoupling | 0603 |

### Wie es fließt

```
STM32H743 SAI1
   │ BCK/LRCK/DOUT
   ▼
PCM5102A (U3)
   │ Analog L/R
   ├──► PAM8406 (U4) ──► J7 Speaker-Header ──► 2× PUI AS04008PS 40 mm
   │           ▲
   │           │ AMP_SHDN_N (MCP23017 GPA4) — Mute aus Firmware
   │
   └── C_HP_IN 1µF ─► U11 TPA6132A2 (−6 dB) ─► 22 Ω ─► J8 PHONES/LINE OUT ──► Kopfhörer / Mixer / Aktiv-Box
                   │
                   └─ Insertion-Detect ──► MCP23017 (optional Auto-Speaker-Mute)
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `U3` PCM5102A | Komplett stumm an allen Outs (J8 + Speakers) | DAC oder I²S-Verkabelung prüfen |
| `U4` PAM8406 | Speakers stumm, J8 lebt | Amp prüfen — oft thermisch oder Strapping-Pin falsch |
| `U11` TPA6132A2 | J8 stumm (Kopfhörer UND Line), Speakers leben | AMP_nSHDN high? Ladungspumpen-Caps (C_FLY_HP/C_HPVSS) prüfen; HPVDD-Spannung ~VDD-nah messen |
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
   mechanisch überlastet wird. Sub-Bass landet nur am J8-Ausgang.
3. **Passive Membran** — *optional* on top der geschlossenen Kammer. Zusatz-
   Bauteil (25–30 mm PR-Membran pro Seite, ~1–3 $/Stück), erweitert f3 von
   ~250 Hz auf ~150 Hz. Nicht critical-path; entscheiden nach Hörtest auf
   Prototyp.

**Honest take:** J8 (Kopfhörer/Line) ist die *echte* Hörerfahrung. Speakers sind
"convenience ohne Kopfhörer" — das Gerät richtig zu positionieren ist Punkt
des Sound-Designs, nicht ein Versuch, 40 mm zu HiFi zu prügeln.

### Warum gerade diese Wahl?

- **PCM5102A statt billigerer PT8211**: Interne DAC-PLL synct sich auf BCK
  → kein externer MCLK nötig → ein SAI-Pin weniger. Saubere 112 dB SNR.
- **PAM8406 statt MAX98357A I²S-Amp** (r19.37: PAM8406→PAM8406, NRND-Swap): analoger Class-D — wir
  *wollen* den Analog-Pfad zwischen DAC + Amp, damit J8 gleichzeitig
  möglich ist (MAX98357 hat keinen Analog-Output).
- **8 Ω / 40 mm PUI statt 4 Ω / 28 mm**: 40 mm gibt physikalisch mehr Membran-
  Fläche → mehr Mid-Lautstärke (alles unter 250 Hz ist sowieso nur am
  Line-Out). 8 Ω passt zur PAM8406-Auslegung.
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
| `D_CELL1..5` | Cell-LEDs (5×) — PCA9685-getrieben | Visualisiert Cell-Status (Press/Hold) | LED 0603 / 0805 — *UNVERIFIED: konkrete PN noch zu finalen* |
| `D_MOD1..5` | Modifier-LEDs (gelb, rot, weiß, grün) — PCA9685-getrieben | Status der 5 Modifier-Buttons (Hold/Drone/Shift/Generate/Clear) | LED 0603 — siehe BOM §"Modifier-LEDs" |
| `SW1..5` | Kailh Choc V1 CPG135001D01, direkt gelötet (5×) | **Cell-Trigger digital, echter Keyswitch** auf MCP23017 GPA0–GPA4 (`CELL1..5_BTN`), r18.75 — ADR-0013 abgelöst, r18.73-UX-Fix (nicht mehr dasselbe Teil wie die Modifier), r18.74-Hotswap-Socket vereinfacht zu Direct-Solder | `field_ambience:SW_KailhChoc_CPG1350_THT_2P` (von LCSC/EasyEDA gezogen für C400229) |
| `SW6..10` | HX B3F-4055-Y THT-Tactile (5×) | Modifier-Buttons auf MCP23017 GPB0–GPB4 (bewusst kleineres/simpleres Gefühl als die Cells) | `field_ambience:SW_TC1212-7.3_THT_4P` (Custom) |

### Wie es fließt

```
STM32H743 I²C1 (PB6/PB7 AF4) ──┬──► U2 MCP23017 (Addr 0x20)
                               │       └──► 5× Cell-Switches SW1–5 (GPA0–4, Pull-Up + Pin → GND)
                               │       └──► 5× Modifier-Buttons SW6–10 (GPB0–4, Pull-Up + Pin)
                               │       └──► AMP_SHDN_N + AMP_MUTE_N (Audio-Mute)
                               │       └──► PCM_XSMT (DAC-Mute, GPA5)
                               │       └──► J8 Insertion-Detect
                               │       └──► INTA ───► MCU GPIO (`MCP_INT`, Pin 7 PC13)
                               │
                               └──► U6 PCA9685 (Addr 0x40)
                                       └──► 10× LED-Kanäle (Konstant-Strom über 0603-Widerstände)
                                       └──► Backlight-FET (Q2-Gate) auf Kanal 12

Cells (r18.75, ADR-0013 abgelöst): SW1..5 (Kailh-Choc-V1, direkt gelötet) →
MCP23017 GPA0..4 → I²C → MCU. Rein digital (on/off), MCP-interner Pull-Up +
IRQ-on-change über INTA. Keine Hall-Sensoren, kein STM32-ADC-Pfad mehr
(PC0/PC1/PA4/PB0/PB1 frei). Echter Keyswitch (~3mm Hub), nicht das kleine
Modifier-Tactile — bewusster UX-Unterschied (r18.73-Fix). Kein Hotswap-Socket
mehr (r18.74 → r18.75): Switch fest verlötet, gleiche THT-Löttechnik wie
jeder andere Button hier.
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `U2` MCP23017 | Alle 10 Buttons (5 Cells + 5 Modifier) tot, Speaker bleibt unmuted, Insertion-Detect tot — aber DSP läuft weiter | I²C-Pull-Ups + Power + Adresse prüfen |
| `U6` PCA9685 | Alle LEDs aus, Backlight dunkel (FET-Gate floated) — Gerät spielt aber Sound | wie oben |
| `R4`/`R5` Pull-Ups | I²C-Bus floated → beide Chips antworten nicht → 1× Reset hilft nicht, Layout-Fehler | Pull-Up nachrüsten / Wert prüfen |
| 1× Cell-Switch SW1–5 | Diese eine Cell triggert nicht; andere 4 funktionieren | Lötstelle des Switches prüfen (fest verlötet, ggf. nachlöten/tauschen); MCP-GPA-Pin gegen GND messen |

### Warum gerade diese Wahl?

- **MCP23017 statt MCU-GPIOs direkt**: spart 14 MCU-Pins die wir anderswo
  brauchen (Audio + Display + Encoder + USB). I²C ist langsam (max ~400 kHz),
  aber für Tasten + Mute-Steuerung weit ausreichend.
- **PCA9685 statt PWM aus dem MCU**: 16 echte PWM-Kanäle parallel (12-Bit
  Auflösung, 1500 Hz Refresh) ohne MCU-Last. Der MCU hat zwar TIM-Kanäle
  satt, aber die sind für Encoder-Quadratur reserviert.
- **Digitale Cell-Switches statt DRV5056A4-Hall (r18.73, ADR-0013 abgelöst)**:
  HiChord-Batch-4+-Weg — Switch → I²C-Expander → MCU. On/Off reicht zum Triggern;
  spart Magnet-Z-Abgleich, ADC-Kalibrierung. Analoge Velocity/Aftertouch wäre nur
  mit Hall nötig — bleibt als Option dokumentiert (ADR-0013).
- **Kailh-Choc statt HX-B3F-Tactile für die Cells (r18.74, User-UX-Fix)**:
  r18.73 hatte die Cells auf denselben kleinen Taster wie die Modifier gesetzt —
  fühlte sich dann identisch zu einem simplen Modifier-Knopf an und zerstörte das
  "Tastatur"-Spielgefühl. Fix: echter Kailh-Choc-Keyswitch (~3mm Hub), elektrisch
  unverändert digital. Bewusster Unterschied: Cells = Keyboard-Feel, Modifier =
  simples Tactile.
- **Direkt-Lötung statt Hotswap-Socket (r18.75, User-Nachfrage)**: der r18.74-
  Hotswap-Socket hatte keine saubere Hersteller-/LCSC-Teilenummer und brauchte
  eine nicht offensichtliche Klein-SMD-Handlöttechnik. Fix: Kailh-Choc-V1-Switch
  (CPG135001D01, LCSC C400229) direkt auf die Platine gelötet — 2 THT-Beinchen,
  gleiche Technik wie jeder andere Button hier. Kein Socket mehr, dafür Switch
  fest verlötet statt tauschbar. Footprint + STEP direkt von LCSC/EasyEDA für
  dieses Teil gezogen (`easyeda2kicad --full --lcsc_id=C400229`).
- **HX B3F-4055-Y THT-Tactile**: nur noch die Modifier-Taster, Square-Head für
  Clip-on-3D-Druck-Caps, 100 k Zyklen, JLC-gelistet (C36498965).

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

PCA9685 ch15 (LCD_BLK_PWM) ──► Q2 Gate ──► Q2 Drain = LCD_BL_LED_K (Kathode)
                                                      ▲
LCD_BL_LED_A (Anode) ◄── +3V3                          GND
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| LCD-Modul tot | Display schwarz — Backlight aus? Strom prüfen. Backlight an, kein Bild? SPI-Pins prüfen | Modul tauschen (steckt im Header) |
| `Q2` Open | Backlight bleibt aus oder permanent an (je nach Fail-Mode) | FET tauschen |
| `J3` Lötstellen | Display flackert / startet nicht | Header neu löten |
| PCA9685 ch15 tot | Backlight dunkel, sonst funktional | PCA9685-Kanal wechseln im Layout *oder* in Firmware Notlauf-Backlight statisch |

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

**Generator:** `battery_sheet()`. **r19.18 komplett neu (ADR-0023)** — das
externe Hardware-Audit (P0-1…P0-7) hat die alte MCP73831+Dioden-OR-Topologie
als fabrikationsblockierend eingestuft; sie ist durch einen echten
Power-Path-Charger ersetzt.

### Was es macht

Das Herz der Stromversorgung: **BQ24074** verwaltet USB-Eingang, Akku-Ladung
und Systemversorgung in einem Chip (DPPM = Dynamic Power Path Management —
Systemlast hat Vorrang, der Ladestrom wird dynamisch gedrosselt, bei
Ueberlast supplementiert der Akku). Sein OUT-Knoten `VSYS` speist den Boost;
die +5V-Rail hat damit genau **eine** Quelle.

### Bauteile

| Ref | Teil | Wozu | Footprint |
|---|---|---|---|
| `J9` | JST-PH 2.0 2-Pin (S2B-PH-SM4-TB, C295747) | Akku-Steckverbinder. Pad 1 = BAT_PLUS (Polung vor erstem Stecken messen!) | `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal` |
| LiPo-Pouch | 2000 mAh mit Schutz-PCB (Pflicht!) | Energiespeicher. Single-Cell 3,0–4,2 V. Bottom-Case-Slot | — (Vendor) |
| `F2` | SMD1812P260TF/16 PTC (C438899) | 2,6 A hold / 5 A trip — Hard-Short-Backup im BAT+-Pfad (r19.18) | `Fuse:Fuse_1812_4532Metric` |
| `U7` | **BQ24074RGTR** (C54313) | 1,5-A-Power-Path-Charger. IN←`VBUS_FUSED`, OUT=`VSYS` (4,4 V @USB), BAT←F2←J9. ICHG 0,89 A (`R_ISET` 1k), IIN-MAX 1,34 A (`R_ILIM_IN` 1,2k, C114605), ITERM/TMR = NC-Default (10 % / 5 h), `R_TS` 10k fest (kein Pack-NTC), CE_N+EN1=GND, EN2=VSYS. CHG (open-drain) → LED_CHRG | `Package_DFN_QFN:QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm` |
| `C_CHG_IN` / `C_BAT` / `C_SYS1`+`C_SYS_HF` | 4,7 µF / 22 µF / 22 µF+100 nF | DS-Bypass: IN (1–10 µF), BAT (4,7–47 µF), OUT=VSYS (4,7–47 µF, zugleich Boost-Input-Bulk) | 0603/0805 |
| `U8` | TPS61089RNR Boost | `VSYS` 3,0–4,4 V → 4,97 V. **EN = `PWR_ON`** (r19.18: Schiebeschalter toetet den Boost, Shutdown-Iq <3 µA) | `field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A` (Custom) |
| `L1` | 2,2 µH 5 A Shielded SWPA6045 (C36500) | Boost-Speicherdrossel, geschirmt | `field_ambience:L_Sunlord_SWPA6045` (Custom) |
| `R23`/`R24` | 121 k / 39 k | FB-Divider → 4,97 V (VREF 1,212 V, r18.79) | 0603 |
| `R_COMP`/`C_COMP`, `R_FSW`, `R_ILIM`, `C_BOOT`, `C_VCC`, 3×`C_BOOST_OUT` | s. BOM_MASTER | Boost-Peripherie (Kompensation r18.80, Fsw ~440 kHz, ILIM 5,9 A) | 0603/0805 |
| `D3` | SS34 Schottky | Trennt den Boost-Regelknoten vom Rail-Bulk (470 µF sieht der Regler nicht) + Reverse-Schutz | SMA |
| `LED_CHRG` + `R_CHRG` | amber 0603 + 1 k | `VBUS_FUSED` → LED → 1 k → CHG: leuchtet nur bei USB **und** laufender Ladung | 0603 |
| `R_BAT_DIV_TOP/BOT` + `C_BAT_FILT` | 100 k : 100 k + 10 nF | `BAT_PLUS` halbiert auf MCU-ADC `BAT_SENSE` (PA3); 21 µA Dauer-Drain | 0603 |
| `TP_VSYS` / `TP_BAT` | Testpunkte | Bring-Up: VSYS ist der erste Messpunkt wenn "nichts geht" | DNP |

### Wie es fließt

```
USB-C VBUS ──F1──► VBUS_FUSED ──► U7 BQ24074 IN     [D2 TVS klemmt VBUS_FUSED]
                                   │  (DPPM)
J9 LiPo+ ──F2──► BAT_PLUS ◄── BAT ─┤
                                   └─ OUT = VSYS (4,4V @USB / VBAT @Akku)
                                          │
        SW_PWR Throw-A ◄──────────────────┤   (Pull-Quelle: immer versorgt)
                                          ▼
                       U8 TPS61089 (EN=PWR_ON) ──L1──► 4,97 V ──D3──► +5V_RAIL
                                                                        │
                                                    ├──► PAM8406 (Audio, ungeschaltet, R_SHDN_PD)
                                                    └──► U_PWR (ON=PWR_ON) ──► +5V_SW ──► LDO ──► +3V3

BAT_PLUS ──► 100k:100k ──► STM32 ADC (`BAT_SENSE`)
```

### Was kaputt geht wenn …

| Bauteil stirbt | Symptom | Fix |
|---|---|---|
| `U7` BQ24074 | Kein VSYS (weder USB noch Akku) → Geraet komplett tot | TP_VSYS messen: 0 V? → U7-Loetstellen (QFN-EP!), VBUS_FUSED 5 V da? |
| `R_ISET` offen | Laden komplett deaktiviert (DS: ISET unconnected = charging disabled) | R_ISET 1 k pruefen |
| `R_ILIM_IN` offen | Laden deaktiviert (DS: ILIM unconnected = all charging disabled) | R_ILIM_IN 1,2 k pruefen |
| `R_TS` fehlt/falsch | Lader verweigert (TS ausserhalb Fenster) | 10 k gegen GND pruefen |
| `F2` getript | Akku-Pfad tot, USB-Betrieb geht noch | Ursache suchen (Short?), PTC kuehlt selbst zurueck |
| `U8` TPS61089 | VSYS ok, aber +5V_RAIL nur ~3,5 V unreguliert → Speaker leise/tot, 3V3 bricht unter Last | Boost-Layout (Bulk, L1), PWR_ON high? |
| `D3` kurz | Rail-Bulk haengt am Regelknoten → Boost-Loop traege/instabil | D3 pruefen — Anode = Boost-Output |
| `J9`/JST | Geraet startet nicht aus Akku, USB geht | JST + F2 nachloeten |

### Warum gerade diese Wahl?

- **BQ24074 statt MCP73831 + Dioden-OR** (r19.18, ADR-0023): das Audit hat
  die Einfachloesung zerlegt (Lader pre-fuse, LED-Rueckspeisung, Boost
  immer an, kein Eingangsstrom-Management, Hot-Plug-Inrush). Der BQ24074
  loest alle fuenf Punkte in einem Chip: DPPM-Power-Path, ILIM, echter
  Lade-Timer, TS, CHG-Status — ohne I²C, rein analog konfiguriert.
- **TPS61089 bleibt**: programmierbare Schaltfrequenz + synchroner Boost =
  sauber unter dem Audio-Band (ADR-0010 §4); Kompensation r18.80 bleibt
  gueltig, weil D3 den Regelknoten weiterhin vom 470-µF-Rail-Bulk trennt.
- **Boost-EN an PWR_ON statt always-on**: Audit P0-3. Im Aus: <3 µA Boost-Iq;
  die Rail liegt dann unreguliert auf ~VSYS−0,7 V (Body-Diode — TPS61089 hat
  kein Output-Disconnect), alle Lasten sind hochohmig (Amp via R_SHDN_PD).
- **SW_PWR-Pull an VSYS**: die Rail existiert erst NACH dem Einschalten —
  ein Pull von der Rail waere ein Henne-Ei-Deadlock. VSYS ist immer da.
- **F2 PTC als Backup, nicht als Ersatz**: Overcharge/Overdischarge-Schutz
  liefert die Zellen-Schutzplatine (Pflicht-Vorgabe), der PTC faengt nur den
  Hard-Short.
- **R_BAT_SENSE 100 k : 100 k Divider**: 1:2 Reduktion (4,2 V max → 2,1 V am
  ADC-Eingang, sicher unter 3,3 V VDDA-Ref). 21 µA Dauer-Drain.

### Aus-Zustand (ADR-0016 + ADR-0023)

SW_PWR OFF (`PWR_ON` low via R_PWR_PD):
- `U7` Lader bleibt aktiv — Geraet laedt im Aus ("dunkel, aber laedt"),
  LED_CHRG zeigt es an (haengt an VBUS_FUSED, nicht an der Rail)
- `U8` Boost aus (<3 µA) + `U_PWR` trennt die 3V3-Domaene
- Rest-Drain am Akku: µA-Bereich (Bat-Sense 21 µA ist der groesste Posten)

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
