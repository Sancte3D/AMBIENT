# Field Ambience PCB — CHANGELOG

Vollständige Änderungshistorie der PCB-Spec und des KiCad-Schematic.
Die Spec-Body selbst (`field_ambience_pcb_SPEC_v0.6.md`) beschreibt
**immer den aktuellen Stand** — diese Datei trackt wie wir dahin kamen.

Aktuelle Rev: **v0.6.3-r6** (PVDD-Decoupling + Startup-Sequenz-Fix +
SPEC-Konsolidierung).

---

## v0.6.3-r6 (2026-05-15) — Stabilization-Pass (Senior-Review Findings)

Kein neues Feature. Reine Stabilisierung nach Senior-Engineer-Review.

### HIGH-Fixes

**PVDD-Decoupling (PAM8403H pins 4, 13)**
- Vorher: nur C9 10µF + C9b 100nF an VDD (pin 6). PVDD-Pins 4 (links) und
  13 (rechts) hatten KEIN lokales Decoupling.
- Datasheet-Anforderung (PAM8403H.PDF "Application Information"): "1.0µF
  ceramic close to VDD" (HF) + "20µF or greater" (bulk).
- Fix:
  - Links (VDD pin 6 + PVDD-L pin 4): C9 10µF→**22µF** (C45783), C9b 100nF→**1µF**
  - Rechts (PVDD-R pin 13): NEU **C_PVDDR 22µF + C_PVDDR_HF 1µF** lokal
- Verhindert: Class-D-Switching-Transienten (250kHz) modulieren PVDD →
  Distortion bei hoher Lautstärke + EMI.

**Startup-Sequenz (Pop-Suppression-Reihenfolge)**
- Vorher (SPEC §8): /MUTE=HIGH zuerst, dann /SHDN=HIGH 100ms später. FALSCH —
  beide active-low, un-mute VOR chip-wakeup → Pop wenn /SHDN HIGH wird.
- Fix: /SHDN=HIGH zuerst (chip wacht auf, Referenzen settlen), dann ~50ms
  später /MUTE=HIGH (un-mute). Shutdown umgekehrt.

### Cross-Domain-Korrekturen
- SPEC-Body §8 vollständig auf korrekte Startup-Sequenz umgeschrieben (nicht
  nur Errata).
- Errata-Historie aus SPEC-Body extrahiert → diese CHANGELOG.md (single
  source of truth: Body = aktueller Stand, CHANGELOG = Historie).

---

## Errata-Historie

### v0.6.3-r5 (2026-05-15) — N1: XSMT via MCP23017 GPA5 (statt statischem Pull-Up)

Aus User-Wunsch "lieber direkt haben als nicht haben" — Pop-Suppression
für PCM5102A jetzt explizit per GPIO statt nur durch PAM8403-Pull-Downs.

**Hardware-Änderung in audio.kicad_sch (U3 PCM5102A Pin 17 XSMT):**

| Alt (v0.6.3-r4) | Neu (v0.6.3-r5) |
|---|---|
| R_XSMT 10k pull-up zu +3V3 (statisch un-muted) | R_XSMT_PD 10k pull-down zu GND (default LOW = muted) |
| — | Hier-Input `PCM_XSMT` von MCP23017 |

**Hardware-Änderung in mcp.kicad_sch (U2 MCP23017 GPA5 = Pin 26):**

| Alt | Neu |
|---|---|
| GPA5 als NC_GPA5 Reserve-Label | GPA5 → Hier-Output `PCM_XSMT` zu Audio-Sheet |

**Cross-Sheet im Root**: Label-Bridge zwischen MCP-Sheet-Box rechts und
Audio-Sheet-Box links für PCM_XSMT-Net.

**Firmware-Konsequenz** (Pico-Init Reihenfolge):

1. Boot: Alle MCP23017-Pins default Input → R_XSMT_PD zieht XSMT LOW → PCM5102A stumm
2. I²C-Init: MCP23017 GPA5 als Output, default 0 (LOW = stumm bleibt)
3. Power-Sequence: +5V/+3V3-Rails stabilisieren (~50 ms)
4. PAM8403 enable: Pico setzt GP28 (MUTE) HIGH, 100 ms später GP27 (SHDN) HIGH
5. PCM5102A un-mute: MCP23017 GPA5 auf HIGH (PCM_XSMT = +3V3) → DAC liefert Audio
6. → kein Pop am Speaker, kein DAC-Hochfahr-Knack

**Bei Shutdown** umgekehrt: MCP GPA5 LOW → PCM stumm, dann PAM8403 muten/shutdown.

### v0.6.3-r3 (2026-05-14) — Important-Items aus 2nd-Review adressiert

Doc-side updates (kein KiCad-GUI nötig). Hardware-Schema unverändert
außer N2 (I²S Series-Resistoren am Pi-Header).

#### I1 — Power-Budget realistisch rekalkuliert

Der v0.6-PAM8403-Wert (350 mA peak) war zu optimistisch. Für 2× 4Ω
Speaker bei 3W BTL: ~6W Audio out, ~85% Class-D-Effizienz → ~7W Eingang
→ ~1.4 A nur für Amp.

| Last | Idle | Typical Audio | Worst Case (loud, 4Ω) |
|---|---|---|---|
| Pi Zero 2 W (SuperCollider) | 250 mA | 500 mA | 700 mA |
| Pico 2 (RP2350) | 30 mA | 50 mA | 50 mA |
| OLED SSD1322 256×64 | 50 mA | 150 mA | 250 mA |
| MCP23017 + Pull-Ups | 5 mA | 20 mA | 25 mA |
| PCM5102A DAC | 20 mA | 30 mA | 30 mA |
| **PAM8403H @ 4Ω, 6W Out** | 80 mA | 600 mA | **1400 mA** |
| EC11-Encoder + LED + Modifier | 5 mA | 15 mA | 25 mA |
| **TOTAL** | **440 mA** | **1365 mA** | **2480 mA** |

**Konsequenz:** 2A-Hold-Polyfuse (F1) trippt im Worst-Case.
Optionen:
- (a) **Firmware Volume-Clamp** auf ~50% max → Worst Case ~1.7 A,
  passt unter 2A-Hold knapp. SAFE für USB-C 5V/3A-Source.
- (b) Polyfuse höher dimensionieren auf 3A-Hold (z.B. MF-MSMF300),
  aber dann USB-C-Spec-Verletzung wenn Source nur Default-Current liefert.
- (c) **USB-PD-Sink-Controller** (siehe I2) + 3A-Polyfuse.

**Aktuelle Empfehlung für Prototyp**: (a) Firmware Volume-Clamp,
2A-Polyfuse beibehalten. Final-Produkt-Entscheidung mit (c).

#### I2 — USB-C Power-Negotiation

5.1kΩ CC-Pulldowns signalisieren "Sink", aber NICHT "berechtigt 3A".
USB-C Source-Port kontrolliert via Rp (Pull-Up an CC) ob Default
(~500 mA), 1.5 A oder 3 A erlaubt.

**Decision für v0.6.3**: Variante A (Konservativ) für Prototyp.
- Pi: Firmware-side Volume-Clamp damit Worst-Case Peak < 1.5 A
- BOM-Note: Mit Standard 5V/3A-Netzteil (z.B. Apple 20W) garantiert OK
- Mit altem 5V/0.5A-Hub-Port: Risiko Brown-out bei lauten Passagen

**Future Hardware-Option**: TPS25750 oder CYPD3177 als USB-PD-Sink-Controller.
Kosten: ~$2 IC + ein paar passives. Rechtfertigt sich für Produkt-Stage.

#### I3 — Inrush-Strategie für 1000µF Bulk-Cap

Roher Elko hinter USB-C ohne Strombegrenzung kann USB-C-Source
unhappy machen (Spike beim Hot-Plug). Polyfuse limitiert NICHT
elegant — sie wird träge warm, lässt den Spike aber durch.

**Mitigation v0.6.3**:
- C_BULK MPN-Spec auf **Low-ESR Polymer** geändert: nicht generischer Elko,
  sondern z.B. Nichicon **PCV1A102MCL1GS** (1000µF / 6.3V / Polymer / ESR < 20mΩ).
  Höhere Lebensdauer, definierter ESR begrenzt Charge-Strom selbst etwas.
- ADD-ON für Final-Build (DNP für Prototyp): **NTC-Inrush-Limiter** (CL-130
  Ametherm, 5Ω cold / 0.5Ω hot, in Serie zwischen Polyfuse und +5V-Rail).

#### I4 — 4-Layer Stack-Up überdacht

Alt: Signal / GND / **+3V3** / Signal
Neu: Signal / GND / **+5V** / Signal

**Begründung:** Auf diesem Board ist +5V die Hochstrom-Schiene:
- Pi Zero 2 W (700 mA peak)
- PAM8403H (1.4 A peak)
- OLED VBAT (250 mA peak)
- Summe: bis 2.4 A auf +5V-Plane

+3V3 trägt nur ~80 mA (OLED VDDIO + MCP + Pull-Ups + EC11). Kann lokal
gepoured werden.

**GND-Plane-Regel**: Layer 2 muss **eine ungeteilte Fläche** sein, KEINE
Splits unter USB-C-Bereich, I²S-Strecke (Pi→PCM5102A), oder Audio-Out
(PAM8403→Speakers). Trace-Loops über GND-Splits sind die Hauptquelle
für EMC-Probleme und Audio-Brumm.

#### I5 — Mechanische Koordinaten

Siehe neue Datei `mechanical_coordinates.md` für die vollständige
Tabelle. Hier nur Außen-Maße:

| Element | Dim |
|---|---|
| Gehäuse | 333 × 143.3 × 40 mm |
| PCB-Outline | 320 × 130 mm |
| Edge-Rails | 5 mm rundherum (entfernen nach Bestückung) |
| Component-to-Edge | min 2.5 mm |

#### I6 — BOM-Split JLC-fitted vs Manual

**SECTION A: JLCPCB SMT-bestückt (Full-PCBA)** — ~70 SKUs:

Discrete passives + Power-ICs + Connectors:
- C_BULK (Polymer 1000µF), C1-C9b, C10-C17, C_VREF, C_VCOM, C_VNEG, C_LDOO, C_FLY,
  C_CPVDD_*, C_in_L/R, C6/C6b/C6c — ALLE 0603/0805 SMD-Caps
- R1-R20, R_VOL_L/R, R_RUN, R_XSMT, R_MUTE_PD, R_SHDN_PD, R_BCK, R_LRCK, R_DOUT — ALLE 0603 SMD
- F1 Polyfuse 1812 (MF-MSMF200)
- FB1 Ferrite-Bead 0603 (BLM18AG601)
- D1 USBLC6-2SC6 ESD (SOT-23-6)
- D2 SMAJ5.0A TVS (SMA)
- LED1 0805 warm white

ICs (alle JLC Extended Stock):
- U2 MCP23017-E/SS (SSOP-28, C506653)
- U3 PCM5102APWR (TSSOP-20, C107671)
- U4 PAM8403H (SOIC-16, C17337)

Connectors (SMD):
- J1 USB-C TYPE-C-31-M-12 (C165948)
- J3 OLED-Header 2.54mm 16-pin
- J4 SWD-Header 1.27mm 3-pin
- J5 VSYS-Bridge 2-pin (DNP default)
- J6, J7 Speaker-Headers 2.54mm 2-pin

Switches:
- SW11 Reset 6mm SMD (TL3342)
- SW12 BOOTSEL 6mm SMD (TL3342) — DNP für THT-Pico-Variante

**SECTION B: Manuell zu bestücken (du lieferst)** — ~15 Items:

Mikrocontroller/Compute-Module:
- U1 Pico 2 (RP2350) als 40-pin THT Pin-Header (Empfehlung Prototyp)
- Pi Zero 2 W mit GPIO-Header J2 2×20 durchgesteckt

Switches (Hot-Swap-Sockets nicht im JLC-Stock):
- SW1-SW5 Kailh Choc V2 Hot-Swap-Socket 2u (Cells) — 5×
- SW6-SW10 Kailh Choc V2 Hot-Swap-Socket 1u (Modifier) — 5×
- 5× Kailh 2u Choc V2 Stabilizer (CPG1353G24D01)

Module:
- OLED ER-OLEDM032-1W 3.2" 256×64 SSD1322 mit 16-pin Header
- 4× EC11-Encoder (wenn nicht SMD-Variante gewählt)

Speaker:
- 2× PUI AS04008PS-4W-WR-R Lautsprecher (mit Drähten an J6/J7)

**SECTION C: TBD (noch zu sourcen oder mechanisch)**:
- Custom MX-Stem Silikon-Cell-Caps (5×, DIY/print)
- BOOTSEL- und Reset-Caps (klein)
- Gehäuse-Schraubdome + M3-Schrauben (4×)
- Bass-Reflex-Ports (Bottom-Case 2× Port 8×25 mm)
- USB-C-Source-Netzteil (5V/3A) — User liefert separat

#### N2 — I²S Series-Resistoren am Pi-Header (DONE)

3× 33Ω 0603 Series-Resistoren am Pi-GPIO-Header in Sheet 7 hinzugefügt:
- **R_BCK** an Pi pin 12 (GPIO18 PCM_CLK)
- **R_LRCK** an Pi pin 35 (GPIO19 PCM_FS)
- **R_DOUT** an Pi pin 40 (GPIO21 PCM_DOUT)

Dämpft Overshoot/Reflexionen auf I²S-Strecke (~100mm über Layer 1/4).
LCSC C23138 (RC0603FR-0733RL).

#### N3 — ERC/DRC Report-Workflow

Convention für die nächsten Iterationen:

1. Nach jedem Major-Pinout-Fix oder Architektur-Änderung:
   ```
   cd kicad/
   kicad-cli sch erc --severity-all --output ../reports/ERC_$(date +%Y-%m-%d).txt field_ambience.kicad_sch
   ```
2. Datei format `reports/ERC_YYYY-MM-DD.md`:
   ```
   KiCad Version: X.Y.Z
   Date: YYYY-MM-DD
   Schematic Commit: <git-sha>
   Errors: N
   Warnings: M
   Accepted warnings (with reason): list
   ```
3. Committen ins Repo. Vor jedem Merge in `main`: aktueller ERC-Report
   muss vorhanden sein und 0 unaccepted Errors zeigen.

### v0.6.3-r2 (2026-05-14, gleicher Tag) — Hardware-Pull-Downs + UART-Naming

Nach zweiter Review-Iteration ergänzt:

**B4-Fix: PAM8403 /SHDN und /MUTE Hardware-Defaults**

PAM8403H /SHDN (Pin 12) und /MUTE (Pin 5) sind ACTIVE LOW. Während
Pico-Reset/Boot floaten GPIOs unbestimmt — Amp könnte un-defined oder
voll-on starten → Pop/Klick. Fix:

- **R_MUTE_PD = 10 kΩ 0603** Pull-Down von Pin 5 (MUTE) zu GND
- **R_SHDN_PD = 10 kΩ 0603** Pull-Down von Pin 12 (SHDN) zu GND

Default-State (während Boot, Reset, Pico-down): beide LOW → Amp ist
GESHUTDOWN und GEMUTED. Pico-Firmware zieht beide aktiv HIGH erst
NACH Power-Sequencing (per Spec v0.6 §5 GP27/GP28 Pop-Suppression).

**I7-Fix: UART-Net-Naming disambiguiert**

Alt: UART0_TX / UART0_RX — perspektivisch mehrdeutig (TX/RX hängt
ab welches Bauteil schaut).

Neu (eindeutige Direction):
- `PICO_TX_PI_RX` — Pico GP0 (UART0 TX) → Pi GPIO15 (RX) auf pin 10
- `PI_TX_PICO_RX` — Pi GPIO14 (TX) auf pin 8 → R1 1k → Pico GP1 (UART0 RX)

Geändert in: pico_sheet, pi_sheet, root_sheet (sheet-pins + cross-sheet labels).

### v0.6.3 (2026-05-14) — CRITICAL: USB-C VBUS/GND-Pinout korrigiert

Externer Review fand: USB-C-Symbol hatte VBUS auf Pin-Numbers A1/A4/B1/B4
und GND auf A12/A9/B9/B12. Das ist **falsch gegen USB Type-C Spec
Rev 2.1 Table 3-1**. Die korrekte Pin-Belegung ist:

```
GND  = A1, A12, B1, B12  (NICHT was im v0.6 Symbol stand)
VBUS = A4, A9, B4, B9
CC1  = A5
CC2  = B5
D+   = A6 (+ B6 für reversed orientation)
D-   = A7 (+ B7 für reversed orientation)
SBU1 = A8
SBU2 = B8
```

**Kritische Konsequenz** ohne Fix: Beim PCB-Layout hätte das Pad A1
des USB-C-Footprints +5V bekommen — Standard-Pad A1 ist aber GND.
Beim Einstecken eines USB-C-Kabels wäre **VBUS direkt auf GND
kurzgeschlossen** worden (Pad A1 vom Kabel = GND, hier am PCB = +5V).

Fix in `generate_kicad_project.py`: Symbol pin numbers für die 8
Power-Pins korrigiert (Positionen bleiben, nur Numbers geswappt):
- local +10.16: number A1 → **A4** (name VBUS)
- local +7.62:  number A4 → **B4** (name VBUS)
- local +5.08:  number B4 → **A9** (name VBUS)
- local +2.54:  number B1 → **B9** (name VBUS)
- local -2.54:  number A12 → **A1** (name GND)
- local -5.08:  number A9 → **B1** (name GND)
- local -7.62:  number B9 → **A12** (name GND)
- local -10.16: number B12 → **B12** (name GND, unchanged)

Power-Tree-Sheet-Wiring bleibt unverändert da Y-Positionen identisch.
Analyzer `pin_nets`-Dump verifiziert nun alle 17 Pins (16 + Shield S1)
korrekt gegen USB Type-C Spec.

**Verbleibender PCB-Layout-Check**: Footprint des USB-C-Receptacle
(JLCPCB C165948 = TYPE-C-31-M-12) muss in KiCad gegen das Symbol
gegengecheckt werden — Pad-Number-Zuordnung muss zur Pin-Number im
Symbol passen. Standard-KiCad-Footprints `USB_C_Receptacle_HRO_TYPE-
C-31-M-12` haben die korrekte Spec-Belegung; trotzdem zwingend
verifizieren bevor Bestellung.

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


