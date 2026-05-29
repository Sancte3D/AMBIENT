# Field Ambience Pico Controller PCB — Spec

**Rev:** 0.6.3-r6 (Stabilization-Pass — PVDD-Decoupling + Startup-Sequenz)
**Target:** 4-Layer JLCPCB, partial-PCBA (siehe §4 BOM-Split A/B/C)
**Methodik:** Datasheet-Verifikation + JLCPCB-Stock-Check vor jeder Komponente
**Status:** SCHEMATIC IN REVIEW — noch nicht production-ready. Offene Blocker
siehe §10 + `PCB_TODO.md`. Änderungshistorie in `CHANGELOG.md`.

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
| HIGH | H1: Polyfuse F1 zu eng bei realer Last (Worst-Case 2.45A) | **v0.7: F1 = 3.0A hold / 6.0A trip** (1812 SMD, Littelfuse 1812L300, C18198349) — siehe §3 |
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
USB-C 5V/3A ────► USB-C  ──┬──► F1 (3A/6A) ──► 1000µF Bulk ──► +5V │
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

## 3. Power Tree v0.6.3-r3 (REVIDIERT — siehe Errata-Historie)

### Power Budget (v0.6.3-r3 realistic)

**⚠️ Update v0.6.3-r3**: alte v0.6-Tabelle hatte PAM8403 mit 350 mA peak —
zu optimistisch. Realistisch für 2× 4Ω BTL bei 3W out: ~1.4 A nur für Amp.

| Verbraucher | Idle | Typical Audio | Worst Case (loud, 4Ω) |
|---|---|---|---|
| Pi Zero 2 W (SuperCollider) | 250 mA | 500 mA | 700 mA |
| Pico 2 (RP2350) | 30 mA | 50 mA | 50 mA |
| OLED SSD1322 256×64 | 50 mA | 150 mA | 250 mA |
| MCP23017 + Pull-Ups | 5 mA | 20 mA | 25 mA |
| PCM5102A DAC | 20 mA | 30 mA | 30 mA |
| **PAM8403H @ 4Ω, 6W Out** | 80 mA | 600 mA | **1400 mA** |
| Encoder/LED/Modifier | 5 mA | 15 mA | 25 mA |
| **TOTAL** | **440 mA** | **1365 mA** | **2480 mA** |

### USB-C Power Delivery (Entscheidung v0.7 — final)

**Harte Anforderung: dediziertes 5V/3A-USB-C-Netzteil (15W).** R_CC1 = R_CC2 =
5.1kΩ zu GND signalisieren "Sink". Wir betreiben das Board als reines 5V-Gerät
ohne PD-Negotiation — ein 5V/3A-Wandnetzteil liefert die nötigen 2.45A
Worst-Case problemlos.

**Entscheidung**: Variante A (USB-PD-Controller) wird NICHT verbaut. Begründung:
- Worst-Case 2.45A < 3A Netzteil-Limit → kein Volume-Clamp für das Power-Budget
  nötig. Das Board darf voll aussteuern.
- Ein Firmware-Volume-Clamp ist daher KEINE Power-Schutz-Pflicht mehr; er bleibt
  optional als Akustik-Maßnahme (40mm-Treiber verzerren oberhalb ~1.5W).
- **Benutzer-Manual**: "Nur mit 5V/3A-USB-C-Netzteil betreiben, NICHT an einem
  PC-USB-Port (500mA → Brown-out)."

**Future (Produkt, nicht Prototyp)**: USB-PD-Sink-Controller (TPS25750/CYPD3177)
für echte 3A-Negotiation an beliebigen Quellen. Kosten ~$2 + Passives.

### Polyfuse [Fix H1, revidiert v0.7]

**F1 = 3.0 A hold / 6.0 A trip, 1812 SMD** (Littelfuse 1812L300, LCSC C18198349,
16V). Rationale v0.7: das alte 2A-hold war zu eng — bei Gehäuse-Innentemp ~50°C
derated jeder PPTC (~0.76×), 2A-hold→~1.5A, was den 1.4A-Typical-Audio + Boot-
Spikes nicht zuverlässig trägt. 3A-hold derated ~2.3A deckt Typical sicher;
Worst-Case-Bass-Peaks (2.45A, sub-ms) reiten den Bulk-Cap. Itrip 6A schützt
weiterhin bei Hard-Short, liegt aber unter der Belastbarkeit der 5V/3A-Quelle.

### Decoupling [Fix H2, M1, M2]

| Cap | Wert | Position | Zweck |
|---|---|---|---|
| C_BULK | 1000 µF Alu 10V Low-ESR SMD (Panasonic EEE-FK1A102P, D10, Footprint CP_Elec_10x10.5) | nahe USB-C | Pi-Boot-Brownout + Bass-Transienten. **v0.7: Alu 10V (nicht Polymer 6.3V — 6.3V zu knapp am 5V-Rail); Inrush-Peak ist R-limitiert, Polyfuse trippt nicht auf <1ms-Spike. Produktion: Soft-Start-Load-Switch.** |
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
| **FB1** | **BLM18AG601 Ferrit-Bead** | **zwischen DVDD und AVDD** | Audio-Rail-Isolation |
| **C9** | **22 µF X5R 0805** | **PAM8403 VDD + PVDD-L (pin 4/6)** | r6: 10µF→22µF per Datasheet ≥20µF |
| C9b | **1 µF X7R 0603** | PAM8403 VDD + PVDD-L | r6: 100nF→1µF per Datasheet HF |
| **C_PVDDR** | **22 µF X5R 0805** | **PAM8403 PVDD-R (pin 13)** | r6 NEU: rechtsseitiges Decoupling |
| C_PVDDR_HF | 1 µF X7R 0603 | PAM8403 PVDD-R | r6 NEU |
| C_VREF | 1 µF X7R 0603 | PAM8403 VREF (pin 8) | Datasheet REQUIRED, Pop-Reduktion |

### LDO für +3V3

Pico 2 SMPS (3V3OUT, 300 mA) speist:
- OLED VDDIO (~30 mA)
- MCP23017 (~10 mA)
- EC11 Pull-Ups + INTA-Pull-Up (~12 mA)
- Pico interner Verbrauch
**Summe ~80 mA. Locker in 300 mA-Budget.**

U5 (AP7361A-33ER) bleibt **DNP** (Reserve-Footprint) für Fall, dass +3V3-Last später steigt.

---

## 4. BOM v0.6.3-r3 — gesplittet JLC-SMT + manuelle Assembly

**⚠️ Update v0.6.3-r3**: BOM ist NICHT "alle SMT/Full-PCBA". Korrekte
Trennung in Section A (JLC-SMT, ~70 SKUs), B (manuelle Assembly, ~15
Items), C (TBD). Siehe Errata-Sektion v0.6.3-r3 oben für vollständige
Aufteilung.

LCSC-Nummern in dieser Sektion wurden v0.6.1/v0.6.2 normalisiert:
PCM5102A = **C107671** (war C9900003814, existiert nicht), PAM8403H =
**C17337** (war C84368, existiert nicht).

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

**Footprint-Hinweis (v0.7)**: Choc-V2-Hotswap-Footprints sind NICHT in der
KiCad-Standard-Library. Benötigt die **kiswitch keyswitch-kicad-library**
(KiCad → Plugin & Content Manager → Libraries → "Keyswitch Kicad Library").
Footprint-Referenz: `Switch_Keyboard_Hotswap_Kailh:SW_Hotswap_Kailh_Choc_V1V2_1.00u`
(1u Modifier) / `_2.00u` (2u Cells + Stabilizer). **Namen verifiziert gegen
kiswitch v2.4** — beide existieren. V1V2 (statt V2-spezifisch) gewählt, weil
es V1+V2 Alignment-Löcher bohrt → Hot-Swap nimmt jede Choc-Generation.

### Line-Out / Kopfhörer (v0.7)

| Ref | Part | JLCPCB # | Bemerkung |
|---|---|---|---|
| J8 | 3.5mm TRS-Buchse mit Switch (PJ-320) | C2884109 | Insertion-Detect → MCP GPA6 |
| R_LO_L/R | 22Ω 0603 | C22962 | Line-Out Serien/Schutz |

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
| **R_RUN** | **10 kΩ 0603 (Pico RUN pull-up zu +3V3, Reset-Stabilität)** | **1** |
| R_VOL_L/R | 10 kΩ 0603 (PAM8403 input series) | 2 |
| C_BULK | 1000 µF Alu-Elko SMD | 1 |
| C1, C3, C7a, C8a, **C6b**, **C9** | 10 µF X5R 0805 | **6 (war 3)** |
| C2, C4, C5, C6, C7b, C8b, **C6c**, C9b | 100 nF X7R 0603 | **8 (war 6)** |
| C5b | 10 nF X7R 0603 (MCP23017 HF) | 1 |
| **C10-C17** | **100 nF X7R 0603 (Encoder A/B debounce)** | **8 (Wert 10nF → 100nF)** |
| C_in_L/R | 1 µF X7R 0603 (PAM8403 input DC-block) | 2 |
| F1 | **Polyfuse 3.0A hold / 6.0A trip 1812 (Littelfuse 1812L300, C18198349)** | 1 |
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
| **32** | **GP27** | **PAM8403 /SHDN (Pin 12, active LOW; GPIO HIGH = enabled)** | **AMP_nSHDN** |
| **34** | **GP28** | **PAM8403 /MUTE (Pin 5, active LOW; GPIO HIGH = un-muted)** | **AMP_nMUTE** |

**Alle 24 funktionalen Pins jetzt belegt.** ADC0 (GP26) bleibt frei als Status-LED-Pin (ADC-Mux möglich falls später Sensor).

**WICHTIG — Active-Low-Konvention**: PAM8403H /SHDN (Pin 12) und /MUTE (Pin 5)
sind beide ACTIVE LOW. Pico-GPIO HIGH = Funktion AUS (= enabled/un-muted),
GPIO LOW = Funktion AKTIV (= shutdown/muted). Hardware-Pull-Downs
(R_SHDN_PD, R_MUTE_PD 10k) ziehen beide LOW während Pico-Boot → Amp ist
default OFF + MUTED.

**Pop-Suppression-Sequenz (Firmware) — KORREKTE Reihenfolge:**

Beim Power-on muss der Chip ZUERST aufwachen (/SHDN HIGH), DANN un-muted
werden (/MUTE HIGH). Umgekehrt würde der Chip beim /SHDN-HIGH-Übergang
sofort un-muted aufwachen → Pop.

- **Power-on**:
  1. Boot: GP27 (/SHDN) und GP28 (/MUTE) LOW (Pull-Downs) + MCP-GPA5 (PCM XSMT) LOW → alles stumm
  2. Warten ~50 ms bis +5V/+3V3 stabil
  3. GP27 (/SHDN) = HIGH → Chip wacht auf, interne Referenzen settlen (Output bleibt via /MUTE=LOW stumm)
  4. ~50 ms später: GP28 (/MUTE) = HIGH + MCP-GPA5 (PCM XSMT) = HIGH → Audio frei
- **Power-off (umgekehrt)**:
  1. MCP-GPA5 (PCM XSMT) = LOW (DAC stumm)
  2. GP28 (/MUTE) = LOW (Amp-Output muted)
  3. ~50 ms später: GP27 (/SHDN) = LOW (Chip aus)
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

Pin-Verteilung (Update v0.6.3-r5: GPA5 für XSMT-Control):

| MCP Pin | Net | Funktion |
|---|---|---|
| GPA0-4 | CELL1-5 | Cell-Switches SW1-5 |
| **GPA5** | **PCM_XSMT** | **PCM5102A Soft-Mute Control (v0.6.3-r5 N1)** |
| **GPA6** | **JACK_DETECT** | **Line-Out-Buchse J8 Insertion-Detect (v0.7)** |
| GPA7 | Reserve (NC) | — |
| GPB0 | MOD_SHIFT | Modifier-Switch SW6 |
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
| GPIO18 (PCM_CLK) | 12 | BCK (pin 13) | Bit clock — v0.6.3-r3 mit R_BCK 33Ω series |
| GPIO19 (PCM_FS)  | 35 | LRCK (pin 15) | Left/Right select — mit R_LRCK 33Ω series |
| GPIO21 (PCM_DOUT)| 40 | DIN (pin 14) | Data in — mit R_DOUT 33Ω series |

PCM5102A im **3-wire mode**: SCK pin → GND. FMT pin → GND (I²S Format).

**Power-Routing PCM5102A** (Pin-Nummern per TI Datasheet SLAS859C):
- DVDD (Pin 20) ← +3V3 → C8a 10 µF + C8b 100 nF lokal
- **FB1** zwischen DVDD-Plane und AVDD-Plane (v0.6 M2 split)
- AVDD (Pin 8) ← +3V3 via FB1 → C7a 10 µF + C7b 100 nF lokal
- CPVDD (Pin 1) ← +3V3 → C_CPVDD_BULK + _HF
- CAPP/CAPM (Pin 2/4) charge-pump fly cap 1µF
- VNEG (Pin 5) 1µF reservoir
- VCOM ist KEIN Pin auf PCM5102A (alte v0.6-Anmerkung war falsch)
- LDOO (Pin 18) optional 100nF Stability-Cap

Output: VOUTL (Pin 6) / VOUTR (Pin 7) ground-centered, ~1.65V DC-Bias, **2.1 Vrms full-scale**.

**Kein externer Output-Filter-Cap**: Der PCM5102A hat einen internen Interpolations-
/Rekonstruktions-Filter; TI-Referenz nutzt keinen Cap-to-GND am Output. Der frühere
BOM-Eintrag `C_audio_filt` (2× 220nF) wurde daher gestrichen (war nie im Schaltplan
platziert). Outputs gehen direkt an PAM8403-IN (via C_in DC-Block) und an den
Line-Out (R_LO Serien-R) — beides ohne Shunt-Filter-Cap.

### Audio-Gain-Strategy (NEU in v0.6.3-r3)

**Problem**: PCM5102A 2.1 Vrms full-scale × PAM8403H 24 dB gain (15.85x) →
33 Vrms gewollt, Amp clipt heftig bei 5V Supply (max ~1.77 Vrms BTL out).

PAM8403H Datasheet (Diodes Inc DS31295) warnt explizit: "Large signal can
cause the clipping of output signal when increasing the volume. This will
damage the device because of the big gain of the PAM8403H."

**v0.6.3-r3 Lösung**: Kombination Hardware + Firmware:
1. **Hardware**: R_VOL_L/R = 20 kΩ (NICHT 10k wie alte v0.6) — das ist RI
   per PAM8403H-Datasheet (RImin=18kΩ). AVD = 2·142k/20k = **23 dB** = 14x
2. **Firmware muss garantieren**: PCM5102A Output ≤ **126 mV Vrms** =
   1.77V/14 (= -24 dB / 6% von full-scale digital)
3. SuperCollider-Master-Volume auf ≤ 0.06 clampen, ALSA mixer-volume
   entsprechend setzen

**Future-Hardware-Option**: External Voltage-Divider zwischen PCM und
PAM-IN: z.B. R_a 18kΩ series + R_b 1kΩ shunt to GND → 18.9x Attenuation.
Erlaubt firmware-side dB-Range +24 dB.

### PAM8403H Connection (per Datasheet DS31295, korrigiert v0.6.2)

PAM8403H Stereo Class-D, 2×3W @ 4Ω.

| Pin | Funktion | Verbindung |
|---|---|---|
| 1 | -OUT_L | Speaker L negative |
| 2 | PGND | GND |
| 3 | +OUT_L | Speaker L positive |
| 4 | PVDD | +5V **(PVDD-L: lokal C9 22µF + C9b 1µF, gemeinsam mit VDD pin 6)** |
| 5 | /MUTE | Pico GP28 (AMP_nMUTE) **+ R_MUTE_PD 10k pull-down** |
| 6 | VDD | +5V **(C9 22µF + C9b 1µF lokal — PAM8403H Datasheet: 1µF HF + ≥20µF bulk)** |
| 7 | INL | PCM5102A OUTL via C_in_L 1µF + R_VOL_L **20kΩ** (RI, gain 23 dB) |
| 8 | VREF | **C_VREF 1µF X7R zu GND (Datasheet REQUIRED, reduziert Pop)** |
| 9 | NC | NC label |
| 10 | INR | PCM5102A OUTR via C_in_R 1µF + R_VOL_R **20kΩ** |
| 11 | GND | Analog GND |
| 12 | /SHDN | Pico GP27 (AMP_nSHDN) **+ R_SHDN_PD 10k pull-down** |
| 13 | PVDD | +5V **(PVDD-R: lokal C_PVDDR 22µF + C_PVDDR_HF 1µF)** |
| 14 | +OUT_R | Speaker R positive |
| 15 | PGND | GND |
| 16 | -OUT_R | Speaker R negative |

**Power-Sequencing (Firmware bei Pico-Boot) — KORREKTE Reihenfolge:**

Chip ZUERST aufwecken (/SHDN HIGH), DANN un-muten (/MUTE HIGH). NICHT
umgekehrt — beim /SHDN-HIGH-Übergang würde ein bereits un-muteter Chip
sofort Audio durchlassen → Pop.

1. Boot: /SHDN=LOW, /MUTE=LOW (Pull-Downs), PCM XSMT=LOW → alles stumm
2. Warten bis +5V-Rail stabil (~50 ms)
3. **/SHDN=HIGH** (GP27) — Chip wacht auf, Referenzen settlen, Output bleibt via /MUTE=LOW stumm
4. ~50 ms später: **/MUTE=HIGH** (GP28) + **PCM XSMT=HIGH** (MCP GPA5) → Audio frei

**Power-off (umgekehrt):**
1. PCM XSMT=LOW (DAC stumm)
2. /MUTE=LOW (Amp-Output muted)
3. ~50 ms später /SHDN=LOW (Chip aus)

Verhindert „Klick"-Geräusch beim An- und Ausschalten.

### Speakers

2× **PUI AS04008PS-4W-WR-R**, 40×40×9mm, 4Ω, 4W RMS, down-firing.
Mount: PCB-Cutout 41mm dia, Speaker via 4× M2-Schrauben am Bottom-Case.
Bass-Reflex: 2× Port 8mm × 25mm hinten im Bottom-Case (~80 Hz Tuning für AS04008).

**Limitierung**: 40mm-Treiber können den 30-60Hz-SubBass-Layer nicht
abstrahlen (realistisch erst ab ~150-250Hz, Port-Bump ~80Hz). Der tiefe
Charakter ist über die Onboard-Speaker nur angedeutet — für vollen SubBass
ist der Line-Out (J8) gedacht.

### Line-Out / Kopfhörer (J8, v0.7)

Passiver Tap an PCM5102A VOUTL/VOUTR (vor dem PAM8403) → 3.5mm TRS-Buchse,
damit der tiefe Charakter über externe Boxen/Kopfhörer hörbar wird.

| Element | Wert |
|---|---|
| J8 | 3.5mm TRS-Buchse mit Insertion-Detect-Switch (PJ-320-Klasse, LCSC C2884109) |
| R_LO_L / R_LO_R | 22Ω 0603 Serien (Schutz / Kurzschluss-Limit) |
| Tap | PCM5102A VOUTL (pin 6) / VOUTR (pin 7) — ground-centered, keine Koppel-Caps nötig |
| Jack-Detect | J8 DET-Switch → MCP23017 GPA6 (Pull-Up + IRQ). Idle=LOW, Plug=HIGH |

**Jack-Detect-Verhalten**: Plug einstecken → Firmware mutet NUR den PAM8403
(Speaker), PCM5102A-DAC + Line-Out bleiben live. Speaker und Line-Out sind
also gegenseitig ausschließend (Plug = Speaker aus).

**Kopfhörer-Hinweis**: Direkter PCM5102A-Tap treibt Line-In (hochohmig) und
hochohmige Kopfhörer (>32Ω) sauber. Für niederohmige Kopfhörer wäre ein
dedizierter Kopfhörer-Amp (TPA6132 o.ä.) besser — v0.8-Option.

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
| Layer count | 4 (Signal / GND / **+5V** / Signal) — +5V ist Hochstrom-Schiene, siehe §3 |
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
| Polyfuse-Dimensionierung (war H1) | v0.7: 3A/6A (Littelfuse 1812L300) — 2A war zu klein für 2.45A Worst-Case |
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

### Was v0.6.3-r3 ZUSÄTZLICH adressiert hat (siehe Errata-Historie oben)

- **CRITICAL** USB-C-Symbol-Pinout (war A1=VBUS, sollte GND sein) → v0.6.3
- **PCM5102A**-Symbol-Pinout (war erfunden) → per TI Datasheet → v0.6.1
- **PAM8403H**-Symbol-Pinout (war Vermutung) → per PDF im Repo → v0.6.2
- **B4**: PAM8403 /SHDN /MUTE Hardware-Pull-Downs (Boot-safe Default) → v0.6.3-r2
- **I7**: UART rename (PICO_TX_PI_RX, PI_TX_PICO_RX) → v0.6.3-r2
- **I1-I6, N2, N3**: Power-Budget, USB-C-Decision, Inrush-Strategy, Stack-Up,
  Mech-Coords, BOM-Split, I²S-R-Series, ERC-Workflow → v0.6.3-r3

### Was IMMER NOCH NICHT verifiziert ist (BLOCKER für PCB-Layout)

Siehe `PCB_TODO.md` für vollständige Liste. Drei harte Blocker brauchen
KiCad GUI lokal:

- **B1**: PAM8403H SOIC-16-Footprint-Pad-Mapping gegen Symbol
- **B2**: USB-C TYPE-C-31-M-12-Footprint-Pad-Mapping gegen Symbol
- **B3**: Echter KiCad GUI ERC-Lauf + Report in `reports/`-Verzeichnis

---

## 11. Nächste Schritte (revidiert v0.6.3-r3)

### Stand: Schematic-Eingabe komplett ✅

Alle 7 Sheets implementiert via `kicad/generate_kicad_project.py`. PR #1.
Pinouts gegen Datasheets verifiziert (PCM5102A, PAM8403H, USB-C).
0 funktionale Analyzer-Errors, 19 Warnings (alle non-blocking), 3 VM-001
false-positives (Pico-5V/3V3-Domain Heuristik-Bug).

### Vor PCB-Layout (lokal in KiCad GUI):

1. **Footprint-Verification** (Blocker B1, B2)
2. **KiCad GUI ERC** + Report ins Repo (Blocker B3)
3. **Mechanische Koordinaten** finalisieren — siehe `mechanical_coordinates.md`
4. **Footprint-Library-Sync** sicherstellen (alle benutzten Library-Refs vorhanden)

### Vor JLCPCB-Order:

5. **PCB-Layout** mit Industrial-Design-Floorplan (333×143.3mm)
6. **DRC clean** + EMC-Analyse via `emc`-Skill
7. **Netclasses** definieren: +5V main rail (3-4 mm wide), USB VBUS, PAM PVDD,
   Speaker-Outputs, USB D+/D-, I²S signals
8. **BOM/CPL** Section-A export für JLCPCB via `bom`-Skill
9. **Manual-Assembly-Liste** (Section B) für eigene Bestellung erstellen
10. **Order**

Erst nach Schritt 6 (DRC clean + EMC OK) wird Spec → v1.0 FINAL.

---

**Ende v0.6.3-r3 Errata-Stand.** Siehe `PCB_TODO.md` für Item-Tracking.
