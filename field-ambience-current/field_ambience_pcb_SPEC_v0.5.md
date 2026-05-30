# Field Ambience Pico Controller PCB — Spec v0.5 (Review-Ready)

**Rev:** 0.5 (review-ready, **NICHT FINAL** — final wird es nach ERC-clean in KiCad)
**Target:** 4-Layer JLCPCB, Full PCBA
**Methodik:** Datasheet-Verifikation + JLCPCB-Stock-Check vor jeder Komponente

---

## Status-Disclaimer

Dieses Dokument ist **nicht final**. Final wird die Spec erst nach:
1. KiCad-Schematic-Eingabe via kicad-happy
2. ERC clean
3. DRC clean nach PCB-Layout
4. Datasheet-Querverifikation jeder kritischen Stelle im Schematic durch `datasheets` Skill
5. JLCPCB-BOM/CPL-Konformität via `bom` Skill

v0.4 hatte 4 Blocker die ich übersehen hatte. v0.5 ist methodisch sauberer, aber **strukturelle Bugs sind erst nach echter KiCad-Eingabe ausgeschlossen**.

---

## Was in v0.5 anders ist als v0.4

| Bug in v0.4 | Fix in v0.5 |
|---|---|
| Pi 4 mit USB-A/HDMI-Steckersalat als Klotz im Gehäuse | **Pi Zero 2 W**: 65×30×5mm, GPIO-Header durchgesteckt, kein externer Stecker |
| Cell-Caps als 22×30mm Custom Silicone (DIY-Ästhetik) | **2u Choc V2 mit Hot-Swap-Sockets**, Custom MX-Stem-Caps designst du selbst |
| 200×100×40mm Form-Faktor | **333×143.3×40mm** Kick75-ähnlich, industriell |
| Speakers nicht klar platziert | **2× PUI AS04008PS down-firing** im Bottom-Case, Bass-Reflex-Port hinten |
| USB-C C165948 als "6-pin THT" falsch beschrieben | **16-pin SMD** TYPE-C-31-M-12 (C165948), 5A-fähig, CC1/CC2 mit 5.1kΩ Pull-down |
| OLED-Maße falsch (80×27 statt 100.5×33.5) | **ER-OLEDM032-1W** 3.2" mit Adapter-Board, BS0/BS1 auf SPI umkonfigurieren |
| Display auf +3V3 (Pico SMPS überlasten) | **Display VBAT auf +5V**, VDDIO auf +3V3 |
| GP8 als SPI0_CSn falsch | **GP5=SPI0_CSn, GP6=SCK, GP7=MOSI, GP4=MISO** (RP2350 Datasheet S.13) |
| Audio nicht spezifiziert | **PCM5102A I²S DAC + PAM8403 Stereo Amp**, beide bei JLCPCB stockbar |
| Pi und Pico kein definiertes Power-Up-Sequencing | **Pi Zero 2 W Boot-Strom 1A peak** → 1000µF Bulk-Cap an +5V |
| JLCPCB DFM (Fiducials, Edge Rails, 2.5mm Spacing) nicht erwähnt | **In Sektion 9 explizit** |

---

## 1. Architecture

```
                     ┌─────────────────────────────────────────────┐
                     │           Field Ambience Device              │
                     │  333 × 143.3 × 40mm                          │
                     │                                              │
USB-C 5V/3A ────► USB-C  ──┬──► 1000µF Bulk-Cap ──┬──► +5V Rail   │
(Power & Pico              │                       │                 │
 BOOTSEL)                  │                       ├──► Pi Zero 2 W  │
                           │                       │   ─ via 40-pin  │
                           │                       │   ─ GPIO Header │
                           │                       │                 │
                           │                       ├──► PAM8403 Amp  │
                           │                       │                 │
                           │                       └──► AP7361A LDO  │
                           │                            ─► +3V3 Rail │
                           │                                          │
                           ├──► Pi USB-D+/D- via TP2/TP3 (Daten/Boot) │
                           │                                          │
                           ▼                                          │
                          Pico 2 (RP2350)                             │
                              │                                       │
              ┌───────────────┼────────────────────────┐              │
              │               │                        │              │
        UART (115200)     SPI0 (8MHz)              I²C (400kHz)        │
              ▼               ▼                        ▼              │
         Pi Zero 2 W      OLED SSD1322           MCP23017             │
         GPIO 14/15      (256×64 weiß)         (10 Buttons)            │
              │                                                       │
         I²S Audio Out                                                 │
              ▼                                                       │
         PCM5102A DAC ─► PAM8403 Amp ─► 2× PUI AS04008PS              │
                                       (down-firing,                  │
                                        Bass-Reflex hinten)           │
                                                                      │
                     4× EC11 Encoder (Drive/Bright/Display/Volume)    │
                     ─► Pico GP10-GP21 direkt mit RC-Debounce         │
                     └─────────────────────────────────────────────┘
```

---

## 2. Form-Factor & Mechanical

| Parameter | Wert | Quelle |
|---|---|---|
| Outer device | 333 × 143.3 × 40mm | Kick75-Referenz (high-profile-Variante) |
| PCB outline | 320 × 130mm, 4-Layer, 1.6mm thick | 6.5mm Bezel rundherum für Gehäuse |
| PCB Cutouts | 2× Speaker openings (40mm dia each, links/rechts unten) | Down-firing speakers im Bottom-Case |
| OLED window | 80×22mm im Top-Case | Active area ER-OLEDM032-1W |
| USB-C position | Top-Edge zentriert | x=160mm von links |
| Tilt | 0° (Kick75 hat 6°, wir nicht — wäre Gehäuse-Sache) | — |
| Top-Plate Material | PC (Polycarbonat) oder ABS, 2.5mm thick | Industrial-Design-Standard |
| Bottom-Case Material | PC oder Aluminium gegossen | mit Speaker-Grille-Pattern |

### Layout (Top-View)

```
┌────────────────────────────────────────────────────────────┐  ↕
│  ┌──────────────────┐                     ⊙  ⊙  ⊙  ⊙      │  
│  │ OLED 256×64      │  DISPLAY KEY: C    DRV BRT DSP VOL  │  30mm
│  │ 80 × 22mm active │                                      │
│  └──────────────────┘                                      │
├────────────────────────────────────────────────────────────┤
│       ┌────┐  ┌────┐  ┌────┐  ┌────┐  ┌────┐              │  
│       │ I  │  │ II │  │ III│  │ IV │  │ V  │   5 Cells    │  25mm
│       └────┘  └────┘  └────┘  └────┘  └────┘   2u Choc V2 │
│       (mit 2u Choc V2 Stabilizer)                          │
├────────────────────────────────────────────────────────────┤
│       ⊙    ⊙    ⊙    ⊙    ⊙          5 Modifier            │  20mm
│      SHIFT HOLD DRONE GEN CLEAR      1u Choc V2 (no stab)  │
├────────────────────────────────────────────────────────────┤
│       ╱─────╲                          ╱─────╲             │  
│      │SPEAKER│  Bottom-Plate            │SPEAKER│            │  ~40mm
│       ╲─────╱   (down-firing, unsichtbar) ╲─────╱           │  hinten
└────────────────────────────────────────────────────────────┘
                          333mm
                                          USB-C top-edge zentriert
```

### Stack-Up (Top to Bottom)

| mm | Schicht | Bemerkung |
|---|---|---|
| 0-2.5 | Top-Plate (PC) | Custom Cutouts für Switches + OLED |
| 2.5-12 | Keycaps + Switches | Choc V2 = 5.8mm Switch + 4mm Cap = 9.8mm |
| 12-13.6 | PCB | 1.6mm 4-Layer |
| 13.6-18 | SMT Components Bottom | Pico 2 castellated, MCP23017, etc. |
| 18-28 | Pi Zero 2 W mit GPIO Header | 5mm Pi + 8mm Header |
| 28-37 | Speaker drivers + Bass-Reflex-Volumen | PUI 9mm, Volumen ~10mm |
| 37-40 | Bottom-Case + Füße | mit Grille-Pattern |

**Total: 40mm — passt** mit ~1mm Toleranz.

---

## 3. Power Tree (verifiziert)

### Power Budget

| Verbraucher | Typical | Peak | Quelle |
|---|---|---|---|
| Pi Zero 2 W (SuperCollider) | 450 mA | 700 mA | CNX Software measurement |
| Pico 2 (RP2350) | 30 mA | 50 mA | Pico 2 Datasheet |
| OLED SSD1322 weiß | 120 mA | 250 mA | ER-OLEDM032-1W Datasheet |
| MCP23017 + Pull-Ups + LED | 20 mA | 25 mA | MCP23017 Datasheet |
| PCM5102A DAC | 25 mA | 30 mA | TI Datasheet SLAS859A |
| PAM8403 Amp typical | 80 mA | 350 mA | Loudspeaker driven |
| **TOTAL** | **745 mA** | **1.405 A** | |
| **bei 5V** | **3.7 W** | **7.0 W** | |

### USB-C Power Delivery

**USB-C @ 5V/3A (15W) Netzteil minimum.**

Pull-Down R_CC1, R_CC2 = je 5.1kΩ zu GND. Signalisiert dem Source "Sink, 3A capable" laut USB-C Spec.

### Decoupling

- **C_BULK = 1000µF Elko an +5V nahe USB-C** — gegen Brown-out beim Pi-Boot
- C1 = 10µF X5R 0805 + C2 = 100nF X7R 0603 an +5V (HF)
- C3 = 10µF + C4 = 100nF an +3V3 nahe Pico Pin 36
- C5 = 100nF + C5b = 10nF an MCP23017 Pin 9
- C6 = 100nF an OLED VCC (am Header)
- C7 = 10µF + C8 = 100nF an PCM5102A AVDD + DVDD
- C9 = 100nF an PAM8403 VDD

### Polyfuse

F1 = 1.5A hold / 3A trip 1812 SMD. Limitiert Spitzenstrom auf 3A (USB-C-Spec).

### LDO für +3V3

Pico 2 hat eigene SMPS 3V3OUT pin 36, kann 300mA liefern.
**WARNUNG:** Display VDDIO (Logik) zieht max 30mA, MCP23017 ~10mA, EC11 Pull-Ups ~10mA = ~50mA. Passt locker in 300mA.

Display VBAT (analog OLED voltage) zieht **bis 250mA** — geht **direkt auf +5V**, nicht via Pico 3V3. Wichtig.

---

## 4. BOM v0.5 — alle SMT, JLCPCB Full-PCBA

### Controller + MCU

| Ref | Part | Package | JLCPCB # | Status | Hinweis |
|---|---|---|---|---|---|
| U1 | Raspberry Pi Pico 2 (RP2350) | castellated SMD 51×21mm | nicht bei JLC, separat | du lieferst | Drei Optionen: SMD-castellated reflow, oder Pin-Header THT, oder reflow am Test-PCB getestet |
| U2 | MCP23017-E/SS | SSOP-28 | C506653 | Extended, ~$1.62, Stock OK | ✓ verifiziert |
| U3 | PCM5102A | TSSOP-20 | C9900003814 (gelistet "PCM5102") | JLCPCB Stock | I²S DAC, 3-wire ohne MCLK |
| U4 | PAM8403 | SOP-16 | C84368 oder ähnlich | Standard | Stereo Class-D Amp 2×3W |
| U5 | AP7361A-33ER (3V3 LDO, falls Pico-SMPS nicht reicht) | SOT-89 | C156144 | Optional | Reserve, falls 3V3-Last steigt |

### Connector + USB

| Ref | Part | JLCPCB # | Bemerkung |
|---|---|---|---|
| J1 | USB-C TYPE-C-31-M-12 | C165948 | 16-pin SMD, 5A. Nutzen: VBUS×4, GND×4, CC1, CC2, D+, D−, SBU1/2 als NC, Shield zu GND |
| J2 | GPIO Header 2×20 für Pi Zero 2 W | Standard 2.54mm | Pi steckt durch oder löst gerichtet |
| J3 | 16-pin OLED-Header 2.54mm | Standard | OLED-Modul steckt drauf |
| J4 | SWD-Header 3-pin 1.27mm | Standard | SWCLK, GND, SWDIO an Pico TP1-TP3 |
| J5 | VSYS bridge 2-pin (default unbestückt) | Standard | Optional 0Ω-Bridge zu +5V |

### Switches + Encoder

| Ref | Part | JLCPCB Status | Du lieferst |
|---|---|---|---|
| SW1-SW5 | Kailh Choc V2 Hot-Swap Socket (5×) für 2u Cells | Nicht im JLC-Stock | Ja, selbst von Keebio/Kailh bestellen |
| SW6-SW10 | Kailh Choc V2 Hot-Swap Socket (5×) für 1u Modifier | Nicht im JLC-Stock | Ja |
| STAB1-5 | Kailh 2u Choc V2 Stabilizer (CPG1353G24D01) | Nicht im JLC-Stock | 5× von Keebio |
| SW11 | Reset Tactile 6mm SMD | Generic SMD | JLC standard |
| EN1-EN4 | EC11 Encoder mit Push (RVE/PEC11R) | Verschiedene bei JLC | SMD-Variante bevorzugt |

### Resistors + Capacitors + Misc

| Ref | Part | Quantity |
|---|---|---|
| R1 | 1kΩ 0603 (UART RX series) | 1 |
| R2, R3 | 5.1kΩ 0603 (USB-C CC1/CC2) | 2 |
| R4, R5 | 4.7kΩ 0603 (I²C SDA/SCL pull-up) | 2 |
| R6 | 10kΩ 0603 (MCP23017 RESET pull-up) | 1 |
| R7-R14 | 10kΩ 0603 (Encoder A/B pull-up) | 8 |
| R15-R18 | 10kΩ 0603 (Encoder SW pull-up) | 4 |
| R19 | 820Ω 0603 (Status LED limit) | 1 |
| R_VOL_L/R | 10kΩ 0603 (PAM8403 volume / gain) | 2 |
| C_BULK | 1000µF aluminum electrolytic SMD | 1 |
| C1, C3, C7 | 10µF X5R 0805 | 3 |
| C2, C4, C5, C6, C8, C9 | 100nF X7R 0603 | 6 |
| C5b | 10nF X7R 0603 (MCP23017 HF) | 1 |
| C_audio_filt | 220nF 0603 (PCM5102A output filter) | 2 |
| C10-C17 | 10nF X7R 0603 (Encoder A/B debounce) | 8 |
| F1 | Polyfuse 1.5A hold / 3A trip 1812 | 1 |
| D1 | USBLC6-2SC6 ESD (USB-C D+/D− Protection) | 1 |
| LED1 | Status LED 0805 warm white | 1 |

**Total: ~50 SMT components** + Pi Zero 2 W, OLED, 10× Hot-Swap-Sockets, 5× Stabilizer (du lieferst).

---

## 5. Pico 2 Pin Allocation (RP2350 Datasheet S.13 verifiziert)

| Pico Pin | GPIO | Funktion | Net |
|---|---|---|---|
| 1 | GP0 | UART0 TX | UART_TX_TO_PI |
| 2 | GP1 | UART0 RX | UART_RX_FROM_PI (via R1 1kΩ series) |
| 4 | GP2 | I²C1 SDA | I2C_SDA |
| 5 | GP3 | I²C1 SCL | I2C_SCL |
| 6 | GP4 | SPI0 RX (MISO, unused but allocated) | OLED_MISO_NC |
| 7 | GP5 | SPI0 CSn | OLED_CS |
| 9 | GP6 | SPI0 SCK | OLED_SCK |
| 10 | GP7 | SPI0 TX (MOSI) | OLED_MOSI |
| 11 | GP8 | OLED DC | OLED_DC |
| 12 | GP9 | OLED RES | OLED_RES |
| 14 | GP10 | EN1 A (Drive) | DRIVE_A |
| 15 | GP11 | EN1 B | DRIVE_B |
| 16 | GP12 | EN1 SW | DRIVE_SW |
| 17 | GP13 | EN2 A (Brightness) | BRIGHT_A |
| 19 | GP14 | EN2 B | BRIGHT_B |
| 20 | GP15 | EN2 SW | BRIGHT_SW |
| 21 | GP16 | EN3 A (Display) | DISPLAY_A |
| 22 | GP17 | EN3 B | DISPLAY_B |
| 24 | GP18 | EN3 SW | DISPLAY_SW |
| 25 | GP19 | EN4 A (Volume) | VOL_A |
| 26 | GP20 | EN4 B | VOL_B |
| 27 | GP21 | EN4 SW | VOL_SW |
| 29 | GP22 | MCP23017 INTA | MCP_INT |
| 31 | GP26 | STATUS LED (ADC0 free) | STATUS_LED |
| 32 | GP27 | reserve (ADC1) | NC |
| 34 | GP28 | reserve (ADC2) | NC |

**Alle 22 funktionalen Pins verteilt. 2 ADC-Pins frei für Zukunft.**

Power-Pins: VBUS (40)=+5V von USB-C, 3V3OUT (36)=+3V3 für Logik, GND mehrere.

---

## 6. Display SSD1322 Konfiguration (ER-OLEDM032-1W Datasheet)

### Hardware-Vorbereitung am Modul (NICHT auf unserem PCB)

Default ist 8080 Parallel. Für 4-wire SPI **Lötbrücken am Modul ändern**:

- BS0 = LOW (R18 oder ähnlich entfernen, R19 setzen — je nach Modul-Rev)
- BS1 = LOW (R20 oder ähnlich entfernen, R21 setzen)

→ Bei Modul-Empfang Datenblatt der konkreten Charge prüfen, **Reihenfolge variiert**.

### Pinout 16-pin Header

| Header Pin | Signal | Verbindung |
|---|---|---|
| 1 | VSS | GND |
| 2 | VBAT | +5V (NICHT +3V3, kann 250mA ziehen) |
| 3 | VDD | +3V3 (Logic) |
| 4 | NC | leer |
| 5 | BS1 | GND (4-wire SPI mode) |
| 6 | BS0 | GND (4-wire SPI mode) |
| 7 | RES# | OLED_RES (Pico GP9) |
| 8 | CS# | OLED_CS (Pico GP5) |
| 9 | D/C# | OLED_DC (Pico GP8) |
| 10 | E or R/W# | GND (4-wire SPI) |
| 11 | R/W# or E | GND (4-wire SPI) |
| 12 | SCLK | OLED_SCK (Pico GP6) |
| 13 | SDIN | OLED_MOSI (Pico GP7) |
| 14 | NC | leer |
| 15 | NC | leer |
| 16 | NC | leer |

**Decoupling am Header:** C6 = 100nF zwischen VDD und GND.

---

## 7. MCP23017 Konfiguration

Adresse: A0=A1=A2=GND → **0x20**

Interrupt: INTA aktiv low, **MIRROR=1** im IOCON-Register (Firmware) → INTA covers GPB-changes auch. INTB ist NC.

Pin-Verteilung:

| MCP Pin | Net | Switch |
|---|---|---|
| GPA0 | CELL1 | SW1 |
| GPA1 | CELL2 | SW2 |
| GPA2 | CELL3 | SW3 |
| GPA3 | CELL4 | SW4 |
| GPA4 | CELL5 | SW5 |
| GPA5-7 | NC | reserve |
| GPB0 | MOD_SHIFT | SW6 |
| GPB1 | MOD_HOLD | SW7 |
| GPB2 | MOD_DRONE | SW8 |
| GPB3 | MOD_GENERATE | SW9 |
| GPB4 | MOD_CLEAR | SW10 |
| GPB5-7 | NC | reserve |

Alle 10 Switches: ein Pin zu MCP-GPIO, anderer Pin zu GND. **Interne Pull-Ups via GPPU-Register aktiviert** (Firmware).

---

## 8. Audio Path

### PCM5102A I²S Connection zu Pi Zero 2 W

| Pi Zero 2 W GPIO | PCM5102A Pin | Funktion |
|---|---|---|
| GPIO18 (PCM_CLK) | BCK (pin 13) | Bit clock |
| GPIO19 (PCM_FS) | LRCK (pin 15) | Left/Right select |
| GPIO21 (PCM_DOUT) | DIN (pin 14) | Data in |

PCM5102A im **3-wire mode** (kein MCLK): SCK pin auf GND → interne PLL generiert MCLK aus BCK.

FMT pin = GND → I²S Format.

Output: VOUTL/VOUTR ground-centered, 2.1Vrms.

### PAM8403 Connection

PAM8403 Stereo Class-D, **2×3W an 4Ω**.

| Pin | Verbindung |
|---|---|
| VDD | +5V |
| GND | GND |
| LIN+ | PCM5102A VOUTL via C_in 1µF + R_in 10kΩ |
| LIN− | virtueller Mittelpunkt 2.5V (oder GND mit C-coupling) |
| RIN+ | PCM5102A VOUTR via C_in 1µF + R_in 10kΩ |
| RIN− | virtueller Mittelpunkt |
| LOUT+ / LOUT− | Speaker L (BTL) |
| ROUT+ / ROUT− | Speaker R (BTL) |
| SHUTDOWN | high (immer an, optional an Pico GPIO) |
| MUTE | high (immer aus, optional an Pico GPIO) |

### Speakers

2× **PUI AS04008PS-4W-WR-R**, 40×40×9mm, 4Ω, 4W RMS, down-firing.

Mount: PCB-Cutout 41mm dia, Speaker via 4× M2-Schrauben am Bottom-Case.
Bass-Reflex: 2× Port 8mm × 25mm hinten im Bottom-Case (Bass-Reflex-Tuning ~80Hz für AS04008).

---

## 9. JLCPCB DFM-Konformität (NEU in v0.5)

| Requirement | Wert | Status |
|---|---|---|
| **Fiducial Markers** | 3× 1mm runde Cu-Pads, 2mm Solder-Mask-Opening | Pflicht für SMT-PCBA, eingeplant |
| **Edge Rails** | ≥5mm rundherum (entfernen nach Bestückung) | Eingeplant |
| **Component-to-Edge** | ≥2.5mm Abstand SMT-Bauteile zu PCB-Rand | Layout-Pflicht |
| **Min Trace Width** | 0.15mm (6 mil) | Standard JLCPCB Econ |
| **Min Trace Spacing** | 0.15mm | Standard |
| **Min Via Drill** | 0.3mm | Standard |
| **Min Annular Ring** | 0.075mm | Standard |
| **Layer count** | 4 (Signal/GND/3V3/Signal) | Standard |
| **Board thickness** | 1.6mm | Standard |
| **Surface finish** | ENIG (für Pico SMD-Reflow besser als HASL) | Spec |
| **Color** | Schwarz Solder-Mask + weiße Silkscreen | Industrial-Design-Look |

---

## 10. Bekannte Risiken & offene Verifikationen

### Was nach Datasheet stimmig ist, aber im Schematic erst verifiziert werden muss

- [ ] PCM5102A 3-wire mode mit Pi Zero 2 W I²S: Reference-Schaltungen existieren, aber timing-getreues Connection erst im echten Build verifizierbar
- [ ] OLED BS0/BS1-Jumper-Position: Modul-Versionen variieren, muss bei Empfang gecheckt werden
- [ ] PAM8403 mit Pop-Suppression: Standard SHUTDOWN-Sequenz nötig sonst "Klick" beim Power-on. Pico GPIO für Mute-Sequence empfohlen
- [ ] Pi Zero 2 W Boot-Strom 1A peak: 1000µF Bulk-Cap reicht in Simulation, real-world verifizieren
- [ ] EC11 Hot-Swap-Sockets bei JLCPCB nicht im Stock — alternative: SMD-EC11 direkt verlötet (z.B. Alps EC12 SMD)

### Bekannte Fragezeichen

- **Pico 2 SMD-Reflow:** Pico 2 hat USB-Connector onboard der nicht schöner SMD-reflowt. Drei Optionen:
  - (a) Pico mit USB-Header durchsteckt — wir routen ihn nicht raus
  - (b) Pico SMD-castellated von Hand reflow (oben/Heißluft)
  - (c) Pico als Pin-Header THT (40-pin Standard) — keine USB-Konflikt
  → **Empfehlung: (c)** für Prototyp, später (a)
- **Choc V2 Stabilizer Mounting:** Plate-Mount (über Top-Plate gefasst) oder PCB-Mount? → Plate-Mount Standard, Top-Plate braucht entsprechende Cutouts
- **Pi Zero 2 W Connection:** 40-pin GPIO Header durchgesteckt → Pi liegt auf der Unterseite unseres PCB

### Methodische Verbesserung gegenüber v0.4

Diesmal habe ich:
- Datasheet-Referenzen explizit gemacht (RP2350 S.13, ER-OLEDM032 Manual, MCP23017 Datasheet, PCM5102A SLAS859A)
- JLCPCB-Stock vor BOM-Aufnahme verifiziert (MCP23017, PCM5102, USB-C)
- Echte Power-Measurements aus Community-Tests benutzt (CNX Software, picockpit) statt Schätzungen
- Pi-Boot-Brownout-Risk addressed (1000µF Bulk-Cap)
- Display VBAT vs VDDIO sauber getrennt
- JLCPCB DFM (Fiducials, Edge Rails, Spacing) ergänzt

### Was NICHT verifiziert ist

- KiCad-Schematic-Eingabe — kommt im Claude-Code-Schritt mit kicad-happy
- ERC clean
- Footprint-Korrektheit
- Trace-Routing (USB-C 5A braucht breite Traces)
- Audio-Layout (PCM5102A AGND/DGND-Trennung)

---

## 11. Nächste Schritte

1. **User-Review dieses Dokuments** — Bugs/Lücken finden bevor wir KiCad anfangen
2. **In Claude Code wechseln** mit kicad-happy installiert
3. **`datasheets` Skill** für jedes IC: Pin-Bestätigung, Application Circuit
4. **`jlcpcb` Skill** für jedes Bauteil: aktueller Stock, Extended Part Fee, MSL
5. **KiCad-Schematic-Eingabe** Sheet-by-Sheet
6. **ERC clean** via `kicad` Skill
7. **PCB-Layout** mit Industrial-Design-Floorplan
8. **DRC + EMC** via `emc` Skill
9. **BOM/CPL** via `bom` Skill für JLCPCB
10. **Order**

Erst **nach Schritt 6** wird die Spec v0.5 zu v1.0 FINAL.

---

**Ende v0.5 Review-Ready.**
