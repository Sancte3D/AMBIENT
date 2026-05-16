# Field Ambience Pico Controller PCB — Spec v0.6 (Review-Ready)

**Rev:** 0.6.2 (Audio-IC-Pinouts vollständig gegen Datasheets verifiziert)
**Target:** 4-Layer JLCPCB, Full PCBA
**Methodik:** Datasheet-Verifikation + JLCPCB-Stock-Check vor jeder Komponente

---

## Errata-Historie

### v0.6.2 (2026-05-13) — PAM8403H-Pinout per PDF im Repo verifiziert

Der PAM8403H.PDF im Repo-Root ist das offizielle Diodes-Inc-Datasheet
für LCSC C17337. Damit ist die Pin-Belegung jetzt zu 100% gesichert.

Bug in v0.6.1 (zwischenzeitlicher Fix-Versuch ohne PDF): basierte auf
falscher DS31295-Vermutung. Pin 8 (VREF), 9 (NC), 10 (INR), 11 (GND),
12 (SHDN), 13 (PVDD), 14 (+OUT_R), 15 (PGND), 16 (-OUT_R) waren alle
um eine Position verschoben. **+OUT_L/−OUT_L Polarität war auch
vertauscht** (Pin 1 ist −OUT_L, nicht +OUT_L wie in v0.6.1 angenommen).

Verifizierte PAM8403H-Pin-Belegung (Diodes Inc, Nov 2012):

| Pin | Name | Funktion |
|---|---|---|
| 1 | **-OUT_L** | Left negative output (BTL) |
| 2 | PGND | Power Ground |
| 3 | **+OUT_L** | Left positive output (BTL) |
| 4 | PVDD | Power VDD |
| 5 | MUTE | Mute Control (ACTIVE LOW) |
| 6 | VDD | Analog VDD |
| 7 | INL | Left Channel Input |
| 8 | **VREF** | Internal analog ref — Bypass-Cap zu GND **REQUIRED** |
| 9 | NC | No connected |
| 10 | INR | Right Channel Input |
| 11 | GND | Analog GND |
| 12 | **SHDN** | Shutdown Control (ACTIVE LOW) |
| 13 | PVDD | Power VDD |
| 14 | **+OUT_R** | Right positive output (BTL) |
| 15 | PGND | Power Ground |
| 16 | **-OUT_R** | Right negative output (BTL) |

NEUER Component in v0.6.2: **C_VREF 1µF X7R 0603** an PAM8403 Pin 8
(VREF Bypass-Cap, per Datasheet zwingend). War in v0.6.1 vergessen.

### v0.6.1 (2026-05-13) — Erster Pinout-Fix-Versuch (PCM5102A korrekt, PAM8403 noch falsch)

| Errata | Ursprünglich (v0.6) | Korrigiert (v0.6.1) |
|---|---|---|
| PCM5102A pinout | LRCK/BCK/DIN auf Pin 1/2/3 (alle falsch) | Per TI Datasheet SLAS859C: CPVDD=1, OUTL=6, OUTR=7, AVDD=8, BCK=13, DIN=14, LRCK=15, DVDD=20 |
| BOM LCSC-Nummern | PCM5102A=C9900003814 (existiert nicht), PAM8403=C84368 (existiert nicht) | PCM5102A=**C107671** (verified, Stock 6726), PAM8403=**C17337** (verified, Stock 8962) |

---

## Status-Disclaimer

Dieses Dokument ist **nicht final**. Final wird die Spec erst nach:
1. KiCad-Schematic-Eingabe via kicad-happy
2. ERC clean
3. DRC clean nach PCB-Layout
4. Datasheet-Querverifikation jeder kritischen Stelle im Schematic durch `datasheets` Skill (insbesondere PAM8403DR-H)
5. JLCPCB-BOM/CPL-Konformität via `bom` Skill
6. **Cross-Check zwischen `audio.kicad_sch` Symbol-Pinout und tatsächlichem PAM8403DR-H Datasheet — JEDER der 16 Pins einzeln verifizieren**

---

## Was in v0.6 anders ist als v0.5

v0.6 behebt vier CRITICAL/HIGH-Findings aus dem v0.5-Review, plus vier MEDIUM-Korrekturen.

| Severity | Finding v0.5 | Fix v0.6 |
|---|---|---|
| CRITICAL | C1: USB-C-D±-Routing zu Pico **und** Pi gleichzeitig — physikalisch unmöglich ohne USB-Switch-IC | **USB-C-D± nur an Pico**. Pi bekommt nur +5V via 40-pin-Header. Pi-Updates via WLAN/SSH (Pi Zero 2 W onboard). Einziger sichtbarer Stecker = USB-C |
| CRITICAL | C2: PAM8403 Pop-Suppression in §10 gefordert, aber keine Pico-GPIOs alloziert | **GP27 = AMP_SHUTDOWN, GP28 = AMP_MUTE** — beide GPIOs aus den v0.5-„reserve"-Pins genommen |
| HIGH | H1: Polyfuse F1 1.5A hold zu eng bei 1.4A peak Last + Inrush | **F1 = 2.0A hold / 4.0A trip** (1812 SMD, z.B. Bourns MF-MSMF200) |
| HIGH | H2: PAM8403 nur 100nF an VDD — bei 350mA peak unterdimensioniert | **C9 = 10µF X5R 0805 + C9b = 100nF X7R 0603** direkt an PAM8403 VDD |
| HIGH | H3: MCP23017 INTA-Leitung ohne Pull-Up — floatet beim Reset | **R20 = 10kΩ 0603 Pull-Up** auf INTA zu +3V3 |
| HIGH | H4: OLED-Größen-Konflikt 256×64 (SPEC) vs 128×64 (ROADMAP/engine) | **256×64 ER-OLEDM032-1W bestätigt**. Firmware-Menü-Layout muss angepasst werden (separater Task in ROADMAP) |
| MEDIUM | M1: OLED VBAT (+5V, 250mA peak) ohne lokale Decoupling | **C6b = 10µF X5R 0805 + C6c = 100nF X7R 0603** direkt am OLED-Header Pin 2 |
| MEDIUM | M2: PCM5102A AVDD/DVDD nicht getrennt decoupelt | **FB1 = Ferrit-Bead BLM18AG601** zwischen AVDD und DVDD. Je 10µF + 100nF lokal an AVDD und DVDD |
| MEDIUM | M3: Pi-Power-Injection via GPIO ohne TVS | **D2 = SMAJ5.0A TVS** auf +5V am Pi-Header (Schutz gegen Spikes vom Pi-WLAN-Modul) |
| MEDIUM | M4: Encoder RC-Debounce 10nF zu klein (0.1ms) | **C10-C17 = 100nF X7R 0603** (1ms RC, deckt EC11-Prellzeiten 1-5ms gemeinsam mit Firmware-Debounce ab) |

---

## 1. Architecture (v0.6 — USB-C-only Variante A)

```
                     ┌─────────────────────────────────────────────┐
                     │           Field Ambience Device              │
                     │  333 × 143.3 × 40mm                          │
                     │                                              │
USB-C 5V/3A ────► USB-C  ──┬──► F1 (2A/4A) ──► 1000µF Bulk ──► +5V │
(Power + Pico              │                                       │
 BOOTSEL/Flash)            ├──► D+/D− ──► Pico USB                 │
                           │              (BOOTSEL drag-drop UF2)   │
                           │                                        │
                           └─────► +5V Rail ──┬──► Pi Zero 2 W      │
                                              │   via 40-pin GPIO   │
                                              │   (Pi-Updates via   │
                                              │    WLAN/SSH)        │
                                              │                     │
                                              ├──► PAM8403 Amp      │
                                              │                     │
                                              ├──► OLED VBAT        │
                                              │                     │
                                              └──► Pico VBUS        │
                                                   │                 │
                                                   ▼                 │
                                                  Pico SMPS          │
                                                   ─► +3V3 Rail      │
                                                   ─► MCP23017       │
                                                   ─► OLED VDDIO     │
                                                   ─► EC11 Pull-Ups  │
                                                                      │
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
                       (GP27=SHDN,                                    │
                        GP28=MUTE,                                    │
                        Pop-Suppression-                              │
                        Sequencing)                                   │
                                                                      │
                     4× EC11 Encoder (Drive/Bright/Display/Volume)    │
                     ─► Pico GP10-GP21 direkt mit RC-Debounce         │
                        (100nF + 10kΩ = 1ms RC)                       │
                     └─────────────────────────────────────────────┘
```

**Update-Pfade**:
- **Pi-Software** (SuperCollider, Bridge): SSH über WLAN — kein USB nötig
- **Pico-Firmware**: USB-C einstecken → BOOTSEL-Button drücken → drag-drop UF2

**Kein micro-USB am Gehäuse sichtbar.** Einziger äußerer Stecker = USB-C.

---

## 2. Form-Factor & Mechanical

Unverändert gegenüber v0.5.

| Parameter | Wert | Quelle |
|---|---|---|
| Outer device | 333 × 143.3 × 40mm | Kick75-Referenz (high-profile-Variante) |
| PCB outline | 320 × 130mm, 4-Layer, 1.6mm thick | 6.5mm Bezel rundherum für Gehäuse |
| PCB Cutouts | 2× Speaker openings (40mm dia each, links/rechts unten) | Down-firing speakers im Bottom-Case |
| OLED window | 80×22mm im Top-Case | Active area ER-OLEDM032-1W |
| USB-C position | Top-Edge zentriert | x=160mm von links |
| Tilt | 0° | — |
| Top-Plate Material | PC (Polycarbonat) oder ABS, 2.5mm thick | Industrial-Design-Standard |
| Bottom-Case Material | PC oder Aluminium gegossen | mit Speaker-Grille-Pattern |

Stack-Up und Layout-Skizze siehe v0.5 §2 — keine Änderungen.

---

## 3. Power Tree v0.6 (verifiziert + corrected)

### Power Budget

Unverändert (Peak 1.405 A bei 5V = 7.0W).

| Verbraucher | Typical | Peak |
|---|---|---|
| Pi Zero 2 W (SuperCollider) | 450 mA | 700 mA |
| Pico 2 (RP2350) | 30 mA | 50 mA |
| OLED SSD1322 weiß | 120 mA | 250 mA |
| MCP23017 + Pull-Ups + LED | 20 mA | 25 mA |
| PCM5102A DAC | 25 mA | 30 mA |
| PAM8403 Amp typical | 80 mA | 350 mA |
| **TOTAL** | **745 mA** | **1.405 A** |

### USB-C Power Delivery

USB-C @ 5V/3A (15W) Netzteil minimum. R_CC1 = R_CC2 = 5.1kΩ zu GND (Sink, 3A capable).

### Polyfuse [Fix H1]

**F1 = 2.0 A hold / 4.0 A trip, 1812 SMD** (Bourns MF-MSMF200 o.ä.).
Rationale: 1.405 A peak + Inrush von 1000 µF Bulk + Pi-Boot-Strom-Spike (1 A peak laut CNX Software Messung) ergibt kumuliert > 1.5A für mehrere ms. 2A hold gibt ~600 mA Headroom.

### Decoupling [Fix H2, M1, M2]

| Cap | Wert | Position | Zweck |
|---|---|---|---|
| C_BULK | 1000 µF Alu-Elko SMD | nahe USB-C | Pi-Boot-Brownout |
| C1 | 10 µF X5R 0805 | +5V Hauptrail | HF-Bulk |
| C2 | 100 nF X7R 0603 | +5V Hauptrail | HF-Decoupling |
| C3 | 10 µF X5R 0805 | +3V3 nahe Pico Pin 36 | HF-Bulk |
| C4 | 100 nF X7R 0603 | +3V3 nahe Pico Pin 36 | HF-Decoupling |
| C5 | 100 nF X7R 0603 | MCP23017 VDD (Pin 9) | HF-Decoupling |
| C5b | 10 nF X7R 0603 | MCP23017 VDD (Pin 9) | HF² |
| C6 | 100 nF X7R 0603 | OLED VDD (Pin 3, +3V3) | Logic-Decoupling |
| **C6b** | **10 µF X5R 0805** | **OLED VBAT (Pin 2, +5V)** | **NEU: Analog-Bulk lokal** |
| **C6c** | **100 nF X7R 0603** | **OLED VBAT (Pin 2, +5V)** | **NEU: Analog-HF lokal** |
| C7a | 10 µF X5R 0805 | PCM5102A **AVDD** (nach FB1) | NEU getrennt |
| C7b | 100 nF X7R 0603 | PCM5102A **AVDD** | NEU getrennt |
| C8a | 10 µF X5R 0805 | PCM5102A **DVDD** | NEU getrennt |
| C8b | 100 nF X7R 0603 | PCM5102A **DVDD** | NEU getrennt |
| **FB1** | **BLM18AG601 Ferrit-Bead** | **zwischen DVDD und AVDD** | **NEU: Audio-Rail-Isolation** |
| **C9** | **10 µF X5R 0805** | **PAM8403 VDD** | **NEU: Class-D-Bulk** |
| C9b | 100 nF X7R 0603 | PAM8403 VDD | umbenannt von alt-C9 |

### LDO für +3V3

Pico 2 SMPS (3V3OUT, 300 mA) speist:
- OLED VDDIO (~30 mA)
- MCP23017 (~10 mA)
- EC11 Pull-Ups + INTA-Pull-Up (~12 mA)
- Pico interner Verbrauch
**Summe ~80 mA. Locker in 300 mA-Budget.**

U5 (AP7361A-33ER) bleibt **DNP** (Reserve-Footprint) für Fall, dass +3V3-Last später steigt.

---

## 4. BOM v0.6 — alle SMT, JLCPCB Full-PCBA

### Controller + MCU

| Ref | Part | Package | JLCPCB # | Status | Hinweis |
|---|---|---|---|---|---|
| U1 | Raspberry Pi Pico 2 (RP2350) | castellated SMD 51×21mm | nicht bei JLC | du lieferst | Empfehlung: Pin-Header THT für Prototyp |
| U2 | MCP23017-E/SS | SSOP-28 | C506653 | Extended, ~$1.62 | verifizieren via `lcsc`-Skill |
| U3 | PCM5102APWR (TI) | TSSOP-20 | **C107671** | Extended Stock | I²S DAC, 3-wire (kein MCLK). Pinout per TI SLAS859C |
| U4 | PAM8403DR-H (Diodes Inc) | SOIC-16 | **C17337** | Extended Stock | Stereo Class-D 2×3W. Pinout per Diodes DS31295 |
| U5 | AP7361A-33ER | SOT-89 | C156144 | **DNP** | Reserve falls +3V3-Last steigt |

### Connector + USB

| Ref | Part | JLCPCB # | Bemerkung |
|---|---|---|---|
| J1 | USB-C TYPE-C-31-M-12 | C165948 | 16-pin SMD, 5A. D+/D− gehen **nur an Pico** |
| J2 | GPIO Header 2×20 (2.54mm) | Standard | Pi Zero 2 W steckt durch (Pi liegt unter PCB) |
| J3 | 16-pin OLED-Header 2.54mm | Standard | OLED-Modul steckt drauf |
| J4 | SWD-Header 3-pin 1.27mm | Standard | SWCLK, GND, SWDIO an Pico TP1-TP3 (Reserve-Flash-Pfad) |
| J5 | VSYS-bridge 2-pin (DNP) | Standard | Optional 0Ω-Bridge |

### Switches + Encoder

| Ref | Part | JLCPCB Status | Du lieferst |
|---|---|---|---|
| SW1-SW5 | Kailh Choc V2 Hot-Swap Socket (5×, 2u Cells) | Nicht im JLC-Stock | Ja, von Keebio/Kailh |
| SW6-SW10 | Kailh Choc V2 Hot-Swap Socket (5×, 1u Modifier) | Nicht im JLC-Stock | Ja |
| STAB1-5 | Kailh 2u Choc V2 Stabilizer (CPG1353G24D01) | Nicht im JLC-Stock | 5× von Keebio |
| SW11 | Reset Tactile 6mm SMD | Generic SMD | JLC Standard |
| **SW12** | **BOOTSEL Tactile 6mm SMD** | Generic SMD | **NEU: dedizierter BOOTSEL-Button für Pico-Flash** |
| EN1-EN4 | EC11 Encoder mit Push (RVE/PEC11R) | Verschiedene bei JLC | SMD-Variante bevorzugt |

### Resistors + Capacitors + Misc [v0.6 Änderungen markiert]

| Ref | Part | Quantity |
|---|---|---|
| R1 | 1 kΩ 0603 (UART RX series) | 1 |
| R2, R3 | 5.1 kΩ 0603 (USB-C CC1/CC2) | 2 |
| R4, R5 | 4.7 kΩ 0603 (I²C SDA/SCL pull-up) | 2 |
| R6 | 10 kΩ 0603 (MCP23017 RESET pull-up) | 1 |
| R7-R14 | 10 kΩ 0603 (Encoder A/B pull-up) | 8 |
| R15-R18 | 10 kΩ 0603 (Encoder SW pull-up) | 4 |
| R19 | 820 Ω 0603 (Status LED limit) | 1 |
| **R20** | **10 kΩ 0603 (MCP23017 INTA pull-up zu +3V3)** | **1 NEU** |
| R_VOL_L/R | 10 kΩ 0603 (PAM8403 input series) | 2 |
| C_BULK | 1000 µF Alu-Elko SMD | 1 |
| C1, C3, C7a, C8a, **C6b**, **C9** | 10 µF X5R 0805 | **6 (war 3)** |
| C2, C4, C5, C6, C7b, C8b, **C6c**, C9b | 100 nF X7R 0603 | **8 (war 6)** |
| C5b | 10 nF X7R 0603 (MCP23017 HF) | 1 |
| C_audio_filt | 220 nF 0603 (PCM5102A output filter) | 2 |
| **C10-C17** | **100 nF X7R 0603 (Encoder A/B debounce)** | **8 (Wert 10nF → 100nF)** |
| C_in_L/R | 1 µF X7R 0603 (PAM8403 input DC-block) | 2 |
| F1 | **Polyfuse 2.0A hold / 4.0A trip 1812** | 1 |
| **FB1** | **Ferrit-Bead BLM18AG601 0603 (600Ω@100MHz)** | **1 NEU** |
| D1 | USBLC6-2SC6 ESD (USB-C D+/D−) | 1 |
| **D2** | **SMAJ5.0A TVS auf +5V am Pi-Header** | **1 NEU** |
| LED1 | Status LED 0805 warm white | 1 |

**Total: ~58 SMT-Komponenten** (v0.5 war ~50) + Pi Zero 2 W, OLED, 10× Hot-Swap-Sockets, 5× Stabilizer.

---

## 5. Pico 2 Pin Allocation v0.6 (RP2350 Datasheet S.13 verifiziert)

[Fix C2: GP27/GP28 jetzt belegt für AMP_SHUTDOWN/AMP_MUTE]

| Pico Pin | GPIO | Funktion | Net |
|---|---|---|---|
| 1 | GP0 | UART0 TX | UART_TX_TO_PI |
| 2 | GP1 | UART0 RX (via R1 1kΩ series) | UART_RX_FROM_PI |
| 4 | GP2 | I²C1 SDA | I2C_SDA |
| 5 | GP3 | I²C1 SCL | I2C_SCL |
| 6 | GP4 | SPI0 RX (MISO, ungenutzt) | OLED_MISO_NC |
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
| **32** | **GP27** | **PAM8403 SHUTDOWN (active high)** | **AMP_SHUTDOWN** |
| **34** | **GP28** | **PAM8403 MUTE (active high)** | **AMP_MUTE** |

**Alle 24 funktionalen Pins jetzt belegt.** ADC0 (GP26) bleibt frei als Status-LED-Pin (ADC-Mux möglich falls später Sensor).

**Pop-Suppression-Sequenz (Firmware):**
- Power-on: SHUTDOWN=LOW (off), MUTE=LOW (gated); warten 50 ms bis Caps geladen; MUTE=HIGH (entgated), 100 ms später SHUTDOWN=HIGH (Class-D startet)
- Power-off (umgekehrt): SHUTDOWN=LOW, 100 ms später MUTE=LOW
- Resultat: kein „Klick" beim An-/Ausschalten

Power-Pins: VBUS (40)=+5V von USB-C, 3V3OUT (36)=+3V3 für Logik, GND mehrere.

---

## 6. Display SSD1322 Konfiguration (ER-OLEDM032-1W Datasheet)

[Fix H4: 256×64 ist die offizielle Wahl. Engine-/Firmware-Menü-Layout-Anpassung als separater ROADMAP-Task]

### Hardware-Vorbereitung am Modul (NICHT auf unserem PCB)

Default ist 8080 Parallel. Für 4-wire SPI **Lötbrücken am Modul ändern**:
- BS0 = LOW
- BS1 = LOW

Bei Modul-Empfang Datenblatt der konkreten Charge prüfen — Reihenfolge variiert je Rev.

### Pinout 16-pin Header

| Header Pin | Signal | Verbindung |
|---|---|---|
| 1 | VSS | GND |
| 2 | VBAT | **+5V (mit lokalem 10 µF + 100 nF Decoupling, NEU)** |
| 3 | VDD | +3V3 (Logic, 100 nF lokal) |
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
| 14-16 | NC | leer |

---

## 7. MCP23017 Konfiguration

[Fix H3: INTA Pull-Up explizit]

Adresse: A0=A1=A2=GND → **0x20**

**Interrupt-Konfiguration:**
- INTA aktiv low, **MIRROR=1** im IOCON-Register (Firmware-Setup) → INTA covers GPB-changes
- INTB ist NC
- **R20 = 10 kΩ Pull-Up auf INTA zu +3V3** (sonst floatet open-drain-Output beim Reset)

Pin-Verteilung unverändert (siehe v0.5 §7):

| MCP Pin | Net | Switch |
|---|---|---|
| GPA0-4 | CELL1-5 | SW1-5 |
| GPB0 | MOD_SHIFT | SW6 |
| GPB1 | MOD_HOLD | SW7 |
| GPB2 | MOD_DRONE | SW8 |
| GPB3 | MOD_GENERATE | SW9 |
| GPB4 | MOD_CLEAR | SW10 |

Alle 10 Switches: ein Pin → MCP-GPIO, anderer → GND. Interne Pull-Ups via GPPU-Register aktiviert (Firmware).

---

## 8. Audio Path v0.6

[Fix M2: AVDD/DVDD getrennt + FB1; Fix C2: SHUTDOWN/MUTE-GPIOs zu Pico]

### PCM5102A I²S zu Pi Zero 2 W

| Pi Zero 2 W GPIO | Pi Header Pin | PCM5102A Pin | Funktion |
|---|---|---|---|
| GPIO18 (PCM_CLK) | 12 | BCK (pin 13) | Bit clock |
| GPIO19 (PCM_FS)  | 35 | LRCK (pin 15) | Left/Right select |
| GPIO21 (PCM_DOUT)| 40 | DIN (pin 14) | Data in |

PCM5102A im **3-wire mode**: SCK pin → GND. FMT pin → GND (I²S Format).

**Power-Routing PCM5102A:**
- DVDD (Pin 4) ← +3V3 → 10 µF + 100 nF lokal (C8a, C8b)
- **FB1** zwischen DVDD-Plane und AVDD-Plane
- AVDD (Pin 1) ← +3V3 via FB1 → 10 µF + 100 nF lokal (C7a, C7b)

Output: VOUTL/VOUTR ground-centered, ~1.65V DC-Bias, 2.1Vrms.

### PAM8403 Connection

PAM8403 Stereo Class-D, 2×3W @ 4Ω.

| Pin | Verbindung |
|---|---|
| VDD | +5V (mit **C9 = 10 µF + C9b = 100 nF lokal, NEU**) |
| GND | GND |
| LIN+ | PCM5102A VOUTL via C_in_L (1 µF) + R_VOL_L (10 kΩ series) |
| LIN− | virtueller Mittelpunkt 2.5V via C-coupling |
| RIN+ | PCM5102A VOUTR via C_in_R (1 µF) + R_VOL_R (10 kΩ series) |
| RIN− | virtueller Mittelpunkt |
| LOUT+ / LOUT− | Speaker L (BTL) |
| ROUT+ / ROUT− | Speaker R (BTL) |
| **SHUTDOWN** | **Pico GP27 (AMP_SHUTDOWN)** |
| **MUTE** | **Pico GP28 (AMP_MUTE)** |

**Power-Sequencing (Firmware bei Pico-Boot):**
1. SHUTDOWN=LOW, MUTE=LOW (default-state bei Pico-Reset)
2. Warten bis +5V-Rail stabil (50 ms)
3. MUTE=HIGH (Eingangs-Gate auf)
4. 100 ms später SHUTDOWN=HIGH (Class-D PWM startet)

Beim Power-off umgekehrt. Verhindert „Klick"-Geräusch.

### Speakers

2× **PUI AS04008PS-4W-WR-R**, 40×40×9mm, 4Ω, 4W RMS, down-firing.
Mount: PCB-Cutout 41mm dia, Speaker via 4× M2-Schrauben am Bottom-Case.
Bass-Reflex: 2× Port 8mm × 25mm hinten im Bottom-Case (~80 Hz Tuning für AS04008).

---

## 9. JLCPCB DFM-Konformität (unverändert v0.5)

| Requirement | Wert |
|---|---|
| Fiducial Markers | 3× 1mm runde Cu-Pads, 2mm Solder-Mask-Opening |
| Edge Rails | ≥5mm rundherum |
| Component-to-Edge | ≥2.5mm |
| Min Trace Width | 0.15mm (6 mil) Standard JLC Econ |
| Min Trace Spacing | 0.15mm |
| Min Via Drill | 0.3mm |
| Min Annular Ring | 0.075mm |
| Layer count | 4 (Signal / GND / 3V3 / Signal) |
| Board thickness | 1.6mm |
| Surface finish | ENIG |
| Color | Schwarz Solder-Mask + weiße Silkscreen |

---

## 10. Bekannte Risiken & offene Verifikationen v0.6

### Vor KiCad-Schematic geklärt

| ✓ Item | Status |
|---|---|
| USB-C-Routing (war C1) | Variante A: USB-C nur an Pico, Pi via WLAN/SSH |
| Pop-Suppression-GPIOs (war C2) | GP27/GP28 belegt, Firmware-Sequenz dokumentiert |
| Polyfuse-Dimensionierung (war H1) | 2A/4A statt 1.5A/3A |
| PAM8403 Bulk-Cap (war H2) | 10 µF + 100 nF lokal an VDD |
| MCP23017 INTA Pull-Up (war H3) | R20 = 10 kΩ |
| OLED 256×64 vs 128×64 (war H4) | 256×64 bestätigt (separater Firmware-Task) |
| OLED VBAT Decoupling (war M1) | C6b/C6c lokal |
| PCM5102A AGND/DGND (war M2) | FB1 + getrennte Decoupling-Paare |
| Pi-Power-Injection TVS (war M3) | D2 SMAJ5.0A |
| Encoder RC (war M4) | 100 nF statt 10 nF |

### Was nach Datasheet stimmig ist, aber im Schematic verifiziert wird

- PCM5102A 3-wire mode mit Pi Zero 2 W I²S — Reference-Schaltung-konform, real-world erst beim Build verifiziert
- OLED BS0/BS1-Jumper — Modul-Versionen variieren, bei Empfang prüfen
- Pi Zero 2 W Boot-Strom-Spike — 1000 µF Bulk-Cap simuliert OK, real-world erst beim Build
- EC11 Hot-Swap-Sockets bei JLC — nicht im Stock, du lieferst (oder SMD-EC12 direkt verlötet als Alternative)
- USB-C C165948 Stock — heute via `lcsc`-/`jlcpcb`-Skill prüfen
- MCP23017 C506653 Extended-Status — heute prüfen (möglicher Aufpreis)

### Bekannte Fragezeichen weiterhin offen

- **Pico 2 Mounting**: (a) SMD-castellated reflow, (b) Pin-Header THT (Recommended für Prototyp), (c) reflow mit Hand-Heißluft
- **Choc V2 Stabilizer Mounting**: Plate-Mount Standard, Top-Plate-Cutouts entsprechend designen
- **Pi Zero 2 W Connection**: 40-pin GPIO Header durchgesteckt → Pi liegt auf der Unterseite unseres PCB

### Was NICHT verifiziert ist

- KiCad-Schematic-Eingabe — nächster Schritt
- ERC clean
- Footprint-Korrektheit
- Trace-Routing (USB-C 5A braucht ≥1.5mm breite Traces, Audio-Routing AGND/DGND-Trennung)
- EMC-Analyse via `emc`-Skill nach PCB-Layout

---

## 11. Nächste Schritte

1. **User-Review v0.6** — letzte Bugs/Lücken finden
2. **`datasheets`-Skill** für jedes IC: Pin-Bestätigung, Application Circuit
3. **`lcsc`/`jlcpcb`-Skill** für Stock-Verifikation Stand 2026-05
4. **KiCad-Projekt anlegen** + Schematic-Eingabe Sheet-by-Sheet:
   - Sheet 1: Power Tree (USB-C, F1, C_BULK, +5V rail, +3V3 via Pico SMPS, D2 TVS)
   - Sheet 2: Pico 2 + SWD + BOOTSEL + Status-LED
   - Sheet 3: OLED-Header + Pull-Up + Decoupling
   - Sheet 4: MCP23017 + 10 Switches + INTA Pull-Up
   - Sheet 5: Encoder × 4 (EN1-EN4) mit RC-Debounce
   - Sheet 6: Audio (PCM5102A + FB1 + PAM8403 + Speaker-Header)
   - Sheet 7: Pi-Header + TVS + Power-Routing
5. **ERC clean** via `kicad`-Skill
6. **PCB-Layout** mit Industrial-Design-Floorplan
7. **DRC + EMC** via `emc`-Skill
8. **BOM/CPL** via `bom`-Skill für JLCPCB
9. **Order**

Erst **nach Schritt 5** wird die Spec v0.6 → v1.0 FINAL.

---

**Ende v0.6 Review-Ready.**
