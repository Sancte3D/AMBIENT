# Field Ambience Pico Controller PCB — Spec

> ⚠️ **v0.9 Pi-frei-Transition aktiv.** Die Audio-Engine läuft jetzt nativ
> auf dem RP2350 (Pico 2), der Raspberry Pi Zero 2 W ist **entfernt**. Dadurch
> sind im Schaltplan **J2, R1, R_BCK, R_LRCK, R_DOUT** raus (Pi-Sheet gelöscht),
> und GP0/GP1/GP4 sind jetzt der I²S-Master zum PCM5102A (siehe §5).
> Dieses Dokument ist noch in weiten Teilen Pi-zentrisch formuliert
> (§1 Architektur-Diagramm, §3 Power-Budget mit Pi-Zeile); diese Abschnitte
> werden in einer SPEC-v0.9-Überarbeitung nachgezogen. Maßgeblich für den
> Pi-frei-Stand ist `NATIVE_PORT_PLAN.md`. D2 (TVS) **bleibt** als allgemeiner
> +5V-Rail-Surge-Schutz (saß nie am Pi-Header, sondern auf der Hauptschiene).

**Rev:** 0.6.3-r9 (Battery-Add: 5000mAh LiPo + MCP73831-Charger + TPS61089-Boost + P-MOSFET Power-Path. Tragbarer Betrieb)
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
         Pi Zero 2 W      OLED SSD1322       MCP23017 (0x20) +          │
         GPIO 14/15      (256×64 weiß)     PCA9685 (0x40, r7)           │
                                           (10 Buttons + 5 LEDs)        │
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
| PCB Cutouts | (entfällt in r14 — siehe §8 Speakers) | Top-firing macht die unteren PCB-Speaker-Cutouts unnötig |
| OLED window | 80×22mm im Top-Case | Active area ER-OLEDM032-1W |
| USB-C position | Top-Edge zentriert | x=160mm von links |
| Tilt | 0° | — |
| Top-Plate Material | PC (Polycarbonat) oder ABS, 2.5mm thick | Industrial-Design-Standard |
| Bottom-Case Material | PC oder Aluminium gegossen | (Speaker-Grille-Pattern entfällt — siehe §8 r14) |

Stack-Up und Layout-Skizze siehe v0.5 §2 — keine Änderungen.

---

## 2.1. USB-C Premium-Upgrade-Intent (r7.1, 2026-05-31)

**Status quo**: TYPE-C-31-M-12 (C165948, Generic-China) — ~5000 Insertion-Cycles
laut China-Generic-Datasheet, OK für Prototyp.

**Design-Intent**: Upgrade auf Premium-Connector mit ≥10000 Cycles für
Produktions-Build. Daily-Touched-Connector = höchste UX-Priorität für Reliability.

**Kandidaten zum Verifizieren** (r7.1-B4 Blocker, Sourcing-Pass nötig):
- JAE **DX07S016JJ1** (16-pin SMD, Power+USB2, 10000 Cycles)
- Amphenol **12401610E412A** oder Amphenol GCT **USB4055** (gleiche Klasse)
- Acceptance-Kriterium: JLC SMT-Assembly-tauglich, in Stock ≥100 pcs, KiCad-
  Footprint-Standard ODER Drop-in zum C165948-Footprint

Falls in Sourcing-Pass kein Premium-Equivalent JLC-stockable: bleibt C165948
+ user-replaceable Soft-Mount-Reinforcement-Plan (Epoxy am Connector-Body).

---

## 2.2. Battery & Power-Path (NEU r9, 2026-05-31)

**Was**: Tragbarer Betrieb — 5000 mAh LiPo + Charger + Boost + Power-Path-Selector.
USB-C lädt; ohne USB-C läuft das Gerät aus dem Akku. Worst-Case-Runtime
~1.5 h bei voller Lautstärke, ~10 h typical (ambient Hörlautstärke).

### Architektur

```
USB-C (5V/3A) ──► F1 (3A/6A) ──┬──► Q1 (P-MOSFET) ──► +5V-Rail ──► alle Verbraucher
                               │                          ▲
                               └──► U7 MCP73831 ──► BAT1  │
                                    (Charger 500mA)   │   │
                                                       ▼  │
                                                   U8 TPS61089
                                                   (Boost 5V/2A)
                                                       │
                                                       └──► +5V-Rail (wenn USB-C abwesend)

Q1-Logik: USB-C-VBUS HIGH = Q1 leitet (Bypass-Boost), Battery lädt parallel
          USB-C-VBUS LOW  = Q1 sperrt, Boost-Output speist +5V-Rail
```

### Bauteile (Battery-Block)

| Ref | Part | LCSC | JLC | Funktion |
|---|---|---|---|---|
| BAT1 | LiPo 3.7V 5000mAh Pouch 8050120 oder 9050120 (8-9mm × 50mm × 120mm) | nicht JLC | du lieferst | Energiespeicher, JST PH 2.0 2-pin |
| J9 | JST PH 2.0 2-pin Battery-Connector vertical SMD | C2845240-Klasse | JLC Basic | Battery-Anschluss, polarisiert |
| U7 | **MCP73831T-2ACI/OT** (Microchip, SOT-23-5) | C424093 | Basic | LiPo Single-Cell Charger, Ladestrom programmierbar via R_PROG (R21 = 2 kΩ → 500 mA charge) |
| U8 | **TPS61089RNR** (TI, VQFN-11 HotRod 2×2.5mm + Thermal Pad) | C165129 | Extended | Boost-Converter LiPo→5V, bis 2A @ 5V Out. Programmable Fsw via R_FSW (360k → ~1.21 MHz, über Audio-Band). r12-B11: Wechsel von RNSR auf RNR-Variante wegen JLC-Stock-Verfügbarkeit. Benötigt 5 zusätzliche externe Bauteile: C_VCC 1µF (interne LDO-Decoupling), R_FSW 360k (Fsw-Set), R_ILIM 20k (current-limit ~4A peak), C_BOOT 100nF (high-side gate driver bootstrap zwischen BOOT und SW), R_COMP 22k + C_COMP 1nF (Type-II loop-compensation). |
| Q1 | **DMG2305UX** (Diodes, SOT-23, P-MOSFET, -20V, -4.2A, Rds 31mΩ) | C150470 | Basic | Power-Path-Selector USB-C vs Boost-Output |
| L1 | **2.2 µH 5A Shielded Inductor 0630** (Sumida CDR63B-2R2) | C83455 | Extended | TPS61089 Boost-Inductor |
| D3 | **SS34 Schottky 40V 3A** (DO-214AC/SMA) | C8678 | Basic | Boost-Output-Diode-Reverse-Schutz (optional bei TPS61089-Synchronous, aber sicherheitshalber) |
| R21 | 2 kΩ 0603 (MCP73831 R_PROG → 500 mA Ladestrom) | Generic | Basic | I_CHARGE = 1000 / R_PROG |
| R22 | 10 kΩ 0603 (Q1 Gate Pull-Down) | Generic | Basic | Default-OFF wenn USB-C-VBUS unbestimmt |
| R23, R24 | TPS61089 Feedback-Divider (R23=200kΩ, R24=39kΩ → Vout=5.0V) | Generic | Basic | Vout-Set für TPS61089 |
| C_BAT_IN | 22 µF X5R 0805 (Battery-Input bulk) | Generic | Basic | LiPo-Cap-Reservoir für Boost-Inrush |
| C_BAT_HF | 100 nF X7R 0603 (Battery HF) | Generic | Basic | HF-Decoupling am Charger |
| C_BOOST_OUT | 22 µF X5R 0805 (Boost-Output) | Generic | Basic | Output-Filter für TPS61089 |
| C_BOOST_HF | 100 nF X7R 0603 (Boost HF) | Generic | Basic | HF an Boost-Output |
| LED_CHRG | 0603 Amber (Charging-Indikator) | Generic | Basic | direkt vom MCP73831 STAT-Pin (Open-Drain LOW=charging) via 1kΩ |
| LED_FULL | 0603 Green (Charge-Complete-Indikator) | optional | — | gleicher STAT-Pin alternativ überwacht via Pico-GPIO |
| R_CHRG | 1 kΩ 0603 (LED_CHRG Series) | Generic | Basic | — |

### Power-Budget revidiert für Battery-Betrieb

| Szenario | I @ 5V | I @ 3.7V (von Akku, ÷ 0.85 Boost-Effizienz) | Runtime aus 5000 mAh @ 3.7V |
|---|---|---|---|
| Idle (Display an, kein Audio) | 250 mA | ~395 mA | ~12.5 h |
| Typical Ambient | 500 mA | ~795 mA | ~6.3 h |
| Loud (worst-case, 4Ω BTL voll) | 2.5 A | ~3.97 A | ~1.25 h |

**Hinweis**: TPS61089 max Output ist 2A @ 5V — bei Worst-Case (2.5 A) reicht das
NICHT. Im Battery-Betrieb klemmt der Boost-Output bei 2 A → das ist eine
**implizite Volume-Begrenzung im Battery-Mode** (~1.4 A für PAM8403 deckt ~2W
out per Channel = laut genug für Indoor-Hörsession, nicht für PA-Lautstärke).
Über USB-C (3A-Path via Q1) bleibt voller Headroom verfügbar.

### Audio-Cleanliness (warum TI/Microchip statt IP5306)

IP5306 ist Standard für China-Power-Banks, kostet $1.50 statt $3.50 für unsere
Split-Lösung. Aber:
- IP5306-Boost schaltet bei 150-300 kHz mit hörbarem coil-whine
- Switching-Ripple injiziert in +5V-Rail → durch PAM8403 verstärkt = audible
  hum/whine über Lautsprecher
- TPS61089 schaltet bei **1.2 MHz** (weit über Audio-Band) + integrierter
  Synchronous-Rectifier minimiert Ripple → Audio-Rail bleibt sauber
- Unsere AVDD/DVDD-FB1-Trennung + 22µF Decoupling wäre durch laute Rail
  unterminiert

$2 Aufpreis = Versicherung dass alles vorherige Audio-Design auch im
Battery-Betrieb funktioniert.

### Battery-Mode-Detection & Voltage-Sense (NEU r12, 2026-05-31)

Zwei orthogonale Sensoren liefern der Firmware die State-Information für
Battery-Mode-Volume-Clamp (r9-B7) und Battery-Low-Warning:

**1. USB-C-VBUS-Detection (Battery-Mode-Detect, digital)** — MCP23017 GPA7:
```
USB-C VBUS ──[R_VBUS_SENSE 10kΩ]──┬── MCP GPA7 (Input, internal Pull-Up DISABLED)
                                   │
                                   └──[R_VBUS_PD 100kΩ]── GND
```
- VBUS=5V (USB-C verbunden): V_GPA7 = 5 × 100/(10+100) = 4.55 V → MCP liest HIGH
- VBUS=0V (Battery-only): V_GPA7 = 0 V (durch Pull-Down) → MCP liest LOW
- MCP I/O 5.5 V-tolerant per Datasheet (MCP23017 Rev I, S.4) → 4.55 V safe
- Drain: ~45 µA wenn USB-C verbunden (irrelevant, lädt parallel)
- Firmware-Polling: 100 ms-Intervall reicht (Mode-Switch ist nicht time-critical)

**2. Battery-Voltage-Sense (State-of-Charge, analog)** — Pico GP26/ADC0:
```
VBAT ──[R_BAT_DIV_TOP 100kΩ]──┬── Pico GP26/ADC0 ──[C_BAT_FILT 10nF]── GND
                               │
                               └──[R_BAT_DIV_BOT 100kΩ]── GND
```
- VBAT 0..4.2 V → V_ADC 0..2.1 V (2:1 Divider, gut innerhalb 3.3 V ADC-Range)
- ADC-Auflösung 12 bit → 0..4095 counts → Vbat-Resolution ~1.0 mV pro count
- Source-Impedance 50 kΩ ist über RP2350-Empfehlung (≤10 kΩ für volle
  Genauigkeit), aber für State-of-Charge-Sensing irrelevant: ±50 mV Noise =
  ±1.2 % SoC-Error → akzeptabel
- C_BAT_FILT 10 nF glättet S/H-Spike + tiefpasst Switching-Noise vom Boost
- Drain: ~21 µA continuous (4.2 V / 200 kΩ) — 0.4 % der WFE-Quiescent von
  5-8 mA, irrelevant
- Firmware: Battery-Low-Warning bei <3.4 V (≈10 % SoC), Battery-Cutoff bei
  <3.0 V (Auto-Soft-Shutdown via §13 zum LiPo-Schutz)

**r9-B7 Volume-Clamp-Algorithmus** (jetzt fully spec'd):
```
if (read_MCP_GPA7() == HIGH):  # USB-C verbunden
    volume_max = 100  # voller Headroom (3 A via Q1-Path)
else:                            # Battery-Mode
    volume_max = 70   # TPS61089-2A-Limit Schutz
    if (read_ADC0_voltage() < 3.4):
        warn_low_battery()  # OLED-Notify + LED10 1Hz-Pulse
    if (read_ADC0_voltage() < 3.0):
        trigger_soft_shutdown()  # §13 Sequenz
```

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
| Encoder + Status-LED | 5 mA | 15 mA | 25 mA |
| **PCA9685 + 10× LEDs** (5× Modifier + 5× Cell-HOLD, je 8 mA peak @ 100% PWM) | 5 mA | 35 mA | **85 mA** |
| **TOTAL** | **455 mA** | **1400 mA** | **2565 mA** |

**Anmerkung r14 (Akustik-v2, Impedanz-Korrektur 2026-06-02)**: Die obige Tabelle
geht von 4 Ω-Lautsprechern aus. PUI AS04008PS-Datenblatt sagt **8 Ω** — der
PAM8403-Worst-Case-Verbrauch halbiert sich entsprechend (~700 mA statt 1400 mA
bei beidseitig Volllast, da P = V²/R den Strom für gegebene Spannung halbiert).
Resultierende Effekte:
- F1 (3 A hold): noch mehr Margin, bleibt unverändert
- TPS61089-Boost (2 A @ 5 V im Battery-Mode): Worst-Case nun ~1.9 A (statt
  2.6 A), Battery-Mode-Volume-Clamp wird damit *physikalisch* unnötig — bleibt
  trotzdem in Firmware aktiv als Akustik-Schutz (Treiber-Verzerrung > 1.5 W)
- Max akustische Lautstärke onboard niedriger (8Ω = ~1.5 W RMS/Kanal statt
  3 W) — für das „clear midrange"-Profil nach r14 nicht relevant, da der
  Treiber eh schon vor seiner thermischen Grenze verzerrt

Die Tabelle bleibt vorerst unverändert (Pi-Reihe wurde in NATIVE_PORT_PLAN
Step 6 entfernt — separate Reconciliation TODO, nicht Teil von r14).

**Anmerkung r10**: Aus r7 (5 Modifier-LEDs, +45 mA Worst-Case) wurde mit r10
(5 Modifier + 5 Cell-HOLD = 10 LEDs) +85 mA Worst-Case. Differenz +40 mA. Das
ändert die Polyfuse-Dimensionierung NICHT (F1 = 3 A hold, ~2.3 A derated bei
50 °C; 2.565 A Worst-Case bleibt unter Auslöseschwelle). LEDs werden typisch
bei <40 % PWM-Duty betrieben → Typical ≈ +20 mA, nicht +40 mA. Battery-Mode
TPS61089-2 A-Limit bleibt der relevantere Cap (siehe nächste Anmerkung).

**Anmerkung r9 (Battery-Mode)**: TPS61089-Boost (siehe §2.2) liefert max 2 A
@ 5 V. Worst-Case 2.565 A (r10-updated) überschreitet das → **Volume-
Begrenzung im Battery-Mode notwendig** (Firmware-clamp PAM8403-Volume auf
~70 % bei Battery-Betrieb-Detect). Detect-Pfad **ab r12 fix verdrahtet**:
USB-C-VBUS via 10k+100k-Spannungsteiler → MCP23017 GPA7 (digital HIGH = USB-C
verbunden, LOW = Battery-Mode). Plus VBAT-Voltage via 100k+100k-Teiler → Pico
GP26/ADC0 für State-of-Charge + Low-Battery-Cutoff. Siehe §2.2 Sub-Section
„Battery-Mode-Detection & Voltage-Sense" für volle Schaltung + Algorithmus.

### USB-C Power Delivery (Entscheidung v0.7 — final)

**Harte Anforderung: dediziertes 5V/3A-USB-C-Netzteil (15W).** R_CC1 = R_CC2 =
5.1kΩ zu GND signalisieren "Sink". Wir betreiben das Board als reines 5V-Gerät
ohne PD-Negotiation — ein 5V/3A-Wandnetzteil liefert die nötigen 2.45A
Worst-Case problemlos.

**Entscheidung**: Variante A (USB-PD-Controller) wird NICHT verbaut. Begründung:
- Worst-Case 2.45A < 3A Netzteil-Limit → kein Volume-Clamp für das Power-Budget
  nötig. Das Board darf voll aussteuern. (r14: Mit 8Ω-Treibern ist Worst-Case
  noch deutlich kleiner — siehe Anmerkung r14 oben.)
- Ein Firmware-Volume-Clamp ist daher KEINE Power-Schutz-Pflicht mehr; er bleibt
  optional als Akustik-Maßnahme (40mm-Treiber verzerren oberhalb ~1.5W
  Eingangs-Leistung, unabhängig von Impedanz — Datenblatt: 2 W rated, 4 W max).
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
| C_BULK | 1000 µF Alu 16V SMD (JVJ16V1000M10x10, LCSC **C46550395**, D10, Footprint CP_Elec_10x10.5) | nahe USB-C | Pi-Boot-Brownout + Bass-Transienten. **v0.8: realer lagernder JLC-Teil statt Platzhalter; 16V (war 10V) = mehr Reserve am 5V-Rail. Inrush-Peak ist R-limitiert, Polyfuse trippt nicht auf <1ms-Spike. Produktion: Soft-Start-Load-Switch.** |
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
| **U6** | **PCA9685PW,118 (NXP)** | **TSSOP-28** | **C2678753** | **Extended, ~$1.96 @100, ~1605 pcs Stock** | **NEU r7: 16-Kanal PWM-LED-Driver für 5 Modifier-Button-LEDs (11 Kanäle Reserve). I²C-Adresse 0x40. Symbol `Driver_LED:PCA9685PW`, Footprint `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm`. Datasheet: nxp.com/docs/en/data-sheet/PCA9685.pdf** |

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
| **SW6-SW10** (r10) | **12×12×7.3 mm momentary tactile, plain (KEINE integrierte LED), SMD-4P. Top-Kandidat: HX 12x12x7.3TPFT-B (Mfr: HX). Pad-Pattern: 11.8×11.8 mm Body, 4 Pins gulwing.** | **C36498966 (LCSC) — JLC Extended, 29.840 pcs Stock, $0.029-0.048 je nach Qty** | **Nein (JLC-assembled). r10-B8 SOURCING-PASS noch offen: HX-Datasheet nicht bei LCSC verfügbar → entweder Sample-Vermessung oder Cross-Verify gegen Standard-12×12-SMD-4P-Footprint. Industrie-Standard für 12×12 SMD-Tactile ist 6.5 × 4.5 mm Pad-Raster mit 1.0×1.5 mm Pads. WICHTIG r10: LEDs sind separat (LED6-LED10, SMD 0603, siehe unten + §7.2).** |
| STAB1-5 | Kailh 2u Choc V2 Stabilizer (CPG1353G24D01) | Nicht im JLC-Stock | 5× von Keebio |
| SW11 | Reset Tactile 6mm SMD | Generic SMD | JLC Standard |
| **SW12** | **BOOTSEL Tactile 6mm SMD** | Generic SMD | **NEU: dedizierter BOOTSEL-Button für Pico-Flash** |
| **EN1-EN4** | **ALPSALPINE EC11J1525402** (16-detent, push-switch, SMD) | **C209762** | **JLC Extended, premium-Detent-Feel. r8: MPN konkretisiert (war: generisch „EC11"). Lifecycle 30k Cycles, Detent-Force ~6 mNm typisch.** |

**Footprint-Hinweis (v0.7)**: Choc-V2-Hotswap-Footprints (SW1-5 Cells) sind
NICHT in der KiCad-Standard-Library. Benötigt die **kiswitch keyswitch-kicad-library**
(KiCad → Plugin & Content Manager → Libraries → "Keyswitch Kicad Library").
Footprint-Referenz: `Switch_Keyboard_Hotswap_Kailh:SW_Hotswap_Kailh_Choc_V1V2_2.00u`
(2u Cells + Stabilizer). **Name verifiziert gegen kiswitch v2.4** — existiert.
V1V2 (statt V2-spezifisch) gewählt, weil es V1+V2 Alignment-Löcher bohrt →
Hot-Swap nimmt jede Choc-Generation.

**Footprint-Hinweis r10 (SW6-10)**: SW6-10 sind ab r10 **plain 4-pin
SMD-Tactile** (HX 12x12x7.3TPFT-B, C36498966). Footprint:
`Button_Switch_SMD:SW_SPST_TL3342` (KiCad-Standard für 12×12 SMD-4P-Familie)
oder Custom-Footprint mit 6.5×4.5 mm Pad-Raster, 1.0×1.5 mm Pads — beides ist
JLC-Standard und KEIN Custom-Footprint-Blocker mehr (r7-B1 → downgraded zu
r10-B8 Sourcing-/Pad-Verify ohne Schalter-Spezial-Mess-Pflicht).

LEDs SW6-10 sind ab r10 SEPARATE SMD-0603-LEDs **über** jedem Switch
(LED6-LED10 bei Y=60, X = SW-X). Cell-HOLD-Status nutzt **5 weitere
SMD-0603-LEDs** über jeder Cell (LED11-LED15 bei Y=88, X = Cell-X). Alle
10 LEDs hängen an PCA9685 LED0-LED9 (siehe §7.2 Kanal-Tabelle).

Symbol: `Switch:SW_Push` für SW6-10 (Switch-Teil) + 10× `Device:LED` (für
LED6-LED15) als separate Symbole im Schematic. Vorteile gegenüber r7:
- JLC-Standard-Footprint statt Custom → r7-B1 Custom-Footprint-Blocker fällt weg
- Full-JLC-Assembly für SW + LEDs möglich → kein User-Supplied-Item mehr für Switches
- LED-Helligkeit/Farbe pro Kanal in BOM frei wählbar (SMD-0603 in beliebigen Farben verfügbar)
- Bessere visuelle Hierarchie: LED neben Switch ist klarer ablesbar als „LED-im-Switch-Body"

### Line-Out / Kopfhörer (v0.7)

| Ref | Part | JLCPCB # | Bemerkung |
|---|---|---|---|
| J8 | 3.5mm TRS-Buchse mit Switch (PJ-320D, SHOU HAN) | C431535 | Insertion-Detect → MCP GPA6. **v0.8: war C2884109 → PJ-320D (lagernd, 21k+ pcs); r8: verifiziert gold-plated + 5000 cycles + SPST-NC-Detect = bereits premium-tauglich. AltMPN für 2nd-Source: PJ-320D-B-SMT (C2884940, XKB Connectivity, drop-in)** |
| R_LO_L/R | 22Ω 0603 | C23345 | Line-Out Serien/Schutz. **v0.8: war C22962 (= 220Ω, falsch) → C23345 (echte 22Ω, Basic)** |

### Resistors + Capacitors + Misc [v0.6 Änderungen markiert]

| Ref | Part | Quantity |
|---|---|---|
| R1 | 1 kΩ 0603 (UART RX series) | 1 |
| R2, R3 | 5.1 kΩ 0603 (USB-C CC1/CC2) | 2 |
| R4, R5 | 4.7 kΩ 0603 (I²C SDA/SCL pull-up) | 2 |
| R6 | 10 kΩ 0603 (MCP23017 RESET pull-up) | 1 |
| R7-R14 | 10 kΩ 0603 (Encoder A/B pull-up) | 8 |
| R15-R18 | 10 kΩ 0603 (Encoder SW pull-up) | 4 |
| ~~R19~~ (r12 entfällt) | ~~820 Ω 0603 (Status LED limit)~~ ersetzt durch R_LED_STATUS (PCA9685-getrieben) | ~~1~~ → 0 |
| **R_LED_STATUS** (r12) | **390 Ω 0603 (STATUS_LED1 Series zu PCA9685 LED10, dimensioniert wie R_LED6-15)** | **1 NEU r12** |
| **R20** | **10 kΩ 0603 (MCP23017 INTA pull-up zu +3V3)** | **1 NEU** |
| **R_RUN** | **10 kΩ 0603 (Pico RUN pull-up zu +3V3, Reset-Stabilität)** | **1** |
| **R_LED6-R_LED10** | **390 Ω 0603 (Modifier-LED-Series, je 1× pro LED, dimensioniert für Vf≈2.1 V @ 5 mA @ +5 V Rail: (5-2.1)/5mA = 580 Ω, aber PCA9685-Output sinkt nach +5V → wir nutzen den IC als open-drain Sink mit 5 V Pull-Up am LED-Anoden-Bein; 390 Ω für ~7.5 mA Peak)** | **5 NEU r7** |
| **R_LED11-R_LED15** | **390 Ω 0603 (Cell-HOLD-LED-Series, je 1× pro Cell-LED, identische Dimensionierung wie R_LED6-10)** | **5 NEU r10** |
| **R_OE** | **10 kΩ 0603 (PCA9685 /OE pull-up zu +3V3, default-disabled bis Firmware enabled)** | **1 NEU r7** |
| **R_BAT_DIV_TOP, R_BAT_DIV_BOT** (r12) | **2× 100 kΩ 0603 (Battery-Voltage-Spannungsteiler 2:1 für GP26/ADC0). VBAT 0..4.2 V → 0..2.1 V am ADC. Drain ~21 µA continuous — irrelevant vs. 5000 mAh.** | **2 NEU r12** |
| **C_BAT_FILT** (r12) | **10 nF X7R 0603 (ADC-Filter am GP26, S/H-Spike-Glättung)** | **1 NEU r12** |
| **R_VBUS_SENSE** (r12) | **10 kΩ 0603 (Series VBUS → MCP-GPA7, ESD-Limit)** | **1 NEU r12** |
| **R_VBUS_PD** (r12) | **100 kΩ 0603 (Pull-Down GPA7 → GND, sichert LOW bei Battery-Mode)** | **1 NEU r12** |
| R_VOL_L/R | 10 kΩ 0603 (PAM8403 input series) | 2 |
| C_BULK | 1000 µF Alu-Elko SMD | 1 |
| C1, C3, C7a, C8a, **C6b**, **C9** | 10 µF X5R 0805 | **6 (war 3)** |
| C2, C4, C5, C6, C7b, C8b, **C6c**, C9b | 100 nF X7R 0603 | **8 (war 6)** |
| C5b | 10 nF X7R 0603 (MCP23017 HF) | 1 |
| **C_PCA_VDD** | **10 µF X5R 0805 (PCA9685 VDD, Pin 28, +3V3)** | **1 NEU r7** |
| **C_PCA_VDD_HF** | **100 nF X7R 0603 (PCA9685 VDD HF)** | **1 NEU r7** |
| **C10-C17** | **100 nF X7R 0603 (Encoder A/B debounce)** | **8 (Wert 10nF → 100nF)** |
| C_in_L/R | 1 µF X7R 0603 (PAM8403 input DC-block) | 2 |
| F1 | **Polyfuse 3.0A hold / 6.0A trip 1812 (Littelfuse 1812L300, C18198349)** | 1 |
| **FB1** | **Ferrit-Bead BLM18AG601 0603 (600Ω@100MHz)** | **1 NEU** |
| D1 | USBLC6-2SC6 ESD (USB-C D+/D−) | 1 |
| **D2** | **SMAJ5.0A TVS auf +5V am Pi-Header** | **1 NEU** |
| LED1 | Status LED 0805 warm white | 1 |
| **LED6-LED10** (r10) | **SMD 0603 Modifier-Status-LEDs, warm-weiß XL-1608UWC-04 (C965808 Extended). Vf≈3.0 V @ 5 mA. Position: über jedem Modifier-Switch (Y=60). PCA9685 LED0-LED4** | **5 NEU r10** |
| **LED11-LED15** (r10) | **SMD 0603 Cell-HOLD-Status-LEDs, identisch zu LED6-LED10 (XL-1608UWC-04 C965808). Position: über jeder Cell (Y=88) zwischen Cell-Cap-Top und OLED-Bottom. PCA9685 LED5-LED9** | **5 NEU r10** |

**Total: ~95 SMT-Komponenten** (r7: +9, r9-Battery: +14, r10: +15 (10 LEDs + 5 R), r12: +5 (1 R_LED_STATUS + 2 R_BAT_DIV + 1 C_BAT_FILT + 1 R_VBUS_SENSE + 1 R_VBUS_PD − 1 R19) net +5) + OLED, 5× Choc V2 Hot-Swap-Sockets (Cells), 5× Stabilizer, 5× 12×12×7.3 plain SMD-Tactile (HX 12x12x7.3TPFT-B, JLC-assembled), **+ BAT1 LiPo 5000 mAh user-supplied**. **r12: GP26 ADC0 frei für BAT_SENSE, STATUS_LED auf PCA9685 LED10, USB-VBUS-Detect via MCP GPA7 — Battery-Mode-Logik vollständig hardware-instrumentiert.**

---

## 5. Pico 2 Pin Allocation v0.6 (RP2350 Datasheet S.13 verifiziert)

[Fix C2: GP27/GP28 jetzt belegt für AMP_SHUTDOWN/AMP_MUTE]

| Pico Pin | GPIO | Funktion | Net |
|---|---|---|---|
| 1 | GP0 | **v0.9: PIO I²S BCK → PCM5102A pin 13** (war UART0 TX) | I2S_BCK |
| 2 | GP1 | **v0.9: PIO I²S LRCK → PCM5102A pin 15** (war UART0 RX) | I2S_LRCK |
| 4 | GP2 | I²C1 SDA | I2C_SDA |
| 5 | GP3 | I²C1 SCL | I2C_SCL |
| 6 | GP4 | **v0.9: PIO I²S DIN → PCM5102A pin 14** (war SPI0 MISO, ungenutzt) | I2S_DOUT |
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
| 31 | GP26 | **BAT_SENSE (ADC0) — Battery-Voltage via 100k/100k Spannungsteiler (NEU r12)** | **BAT_SENSE** |
| **32** | **GP27** | **PAM8403 /SHDN (Pin 12, active LOW; GPIO HIGH = enabled)** | **AMP_nSHDN** |
| **34** | **GP28** | **PAM8403 /MUTE (Pin 5, active LOW; GPIO HIGH = un-muted)** | **AMP_nMUTE** |

**Alle 24 funktionalen Pins jetzt belegt.** ADC0 (GP26) ab r12 für BAT_SENSE (war STATUS_LED in r3-r11; STATUS_LED1 wandert auf PCA9685 LED10, siehe §7.2). ADC1/2/3 nicht verfügbar (GP27/28 = Amp-Control, GP29 = interner Vsys/3).

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
| **GPA7** | **USB_VBUS_SENSE** | **USB-C VBUS-Detect (NEU r12, war Reserve). HIGH wenn USB-C verbunden, LOW wenn battery-only. Firmware nutzt für Battery-Mode-Volume-Clamp (r9-B7). Schaltung: 10 kΩ Series von VBUS → GPA7 + 100 kΩ Pull-Down → GND. MCP-I/O 5.5 V-tolerant per Datasheet → V_GPA7 ≈ 4.55 V bei VBUS=5V, 0 V bei Battery-Mode. ~45 µA Drain wenn USB-C verbunden (irrelevant, lädt parallel).** |
| GPB0 | MOD_SHIFT | Modifier-Switch SW6 (momentary, Press-Event) |
| GPB1 | MOD_HOLD | SW7 (momentary, Press = Toggle-Event) |
| GPB2 | MOD_DRONE | SW8 (momentary, Press = Toggle-Event) |
| GPB3 | MOD_GENERATE | SW9 (momentary, Press = Toggle-Event) |
| GPB4 | MOD_CLEAR | SW10 (momentary, One-Shot-Event) |

Alle 10 Switches: ein Pin → MCP-GPIO, anderer → GND. Interne Pull-Ups via
GPPU-Register aktiviert (Firmware).

**WICHTIG r7**: SW6-SW10 sind **momentary** (federn zurück), nicht latching.
Der UI-Zustand HOLD/DRONE/GENERATE/SHIFT-aktiv/CLEAR-confirmation lebt **in
der Firmware**, sichtbar via die zugehörigen LEDs (PCA9685, siehe §7.2). Das
ermöglicht Preset-State-Recall (Snapshot setzt Firmware-Var → PCA9685-Kanal
geht passend an/aus) ohne physisch widersprechenden Switch.

**Update r10**: SW6-SW10 sind ab r10 **plain 4-pin SMD-Tactile** (KEINE
integrierte LED mehr). Status-Anzeige läuft über separate SMD-0603-LEDs
über jedem Switch (siehe §7.2 + `mechanical_coordinates.md` §5). Vorteile:
JLC-Standard-Footprint statt Custom (r7-B1 BLOCKER → downgraded zu r10-B8
Sourcing-Pass), full-JLC-Assembly möglich, kleinere/cleanere LED-Optik.

---

## 7.2. PCA9685 LED-Driver Konfiguration (NEU r7)

I²C-Adresse: A0..A5 = GND → **0x40** (Default). Liegt am selben I²C1-Bus wie
MCP23017 (0x20) und OLED — Konflikt-frei.

**Decoupling**: C_PCA_VDD (10 µF) + C_PCA_VDD_HF (100 nF) lokal an Pin 28
(VDD, +3V3).

**/OE (Output Enable, Pin 23, active LOW)**: über R_OE = 10 kΩ Pull-Up an
+3V3. Default = HIGH = LEDs disabled. Firmware zieht /OE LOW erst nachdem
PWM-Register initialisiert sind (kein Aufblitzen beim Boot).

**EXTCLK (Pin 25)**: **MUSS auf GND gezogen werden** (NXP Datasheet Rev 4 S.7
Footnote [2]: „This pin must be grounded when this feature is not used"). Wir
nutzen den internen 25 MHz Oszillator → Pin 25 fix an GND. **Korrektur 2026-05-31**:
frühere Spec-Fassungen sagten irrtümlich „NC" — wäre ein latenter Bug
(undefined HF-Pickup, kann Oszillator destabilisieren). Im KiCad-Schematic
muss Pin 25 explizit eine GND-Verbindung haben, nicht nur ein NC-Label.

**LED-Kanal-Belegung (erweitert r10 — 5 Cell-HOLD-Status-LEDs hinzu):**

| PCA9685 LEDn | Funktion | LED-Ref | Switch-Ref | LED-Anzeige-Logik |
|---|---|---|---|---|
| LED0 | Modifier SHIFT | LED6 | SW6 | An solange Taster gedrückt (Modifier-Anzeige) |
| LED1 | Modifier HOLD | LED7 | SW7 | An = HOLD-Mode aktiv (Firmware-Toggle) |
| LED2 | Modifier DRONE | LED8 | SW8 | An = Drone spielt (Firmware-Toggle), sanfter Fade |
| LED3 | Modifier GENERATE | LED9 | SW9 | An = Generative-Mode aktiv (Firmware-Toggle) |
| LED4 | Modifier CLEAR | LED10 | SW10 | Short-Press: 200 ms Flash. Long-Press-Hold: 0.5 Hz Pulse (§12.1) |
| **LED5** | **Cell-HOLD CELL1** | **LED11** | **SW1** | **An = Cell1 ist HOLD-aktiv (Drone sustaining)** |
| **LED6** | **Cell-HOLD CELL2** | **LED12** | **SW2** | **An = Cell2 HOLD-aktiv** |
| **LED7** | **Cell-HOLD CELL3** | **LED13** | **SW3** | **An = Cell3 HOLD-aktiv** |
| **LED8** | **Cell-HOLD CELL4** | **LED14** | **SW4** | **An = Cell4 HOLD-aktiv** |
| **LED9** | **Cell-HOLD CELL5** | **LED15** | **SW5** | **An = Cell5 HOLD-aktiv** |
| **LED10** (r12) | **System-Status (heartbeat / battery-low / error)** | **LED1** | — | **Übernimmt die Rolle der bisherigen GP26-STATUS_LED. GP26 wird in r12 frei für BAT_SENSE (ADC0). PWM-Heartbeat-Pattern, Battery-Low-Pulse, Error-Code-Flashes.** |
| LED11-LED15 | Reserve | — | — | 5 Kanäle frei für Future |

**LED-Bauform-Änderung (r10)**: alle 10 Anzeige-LEDs sind **SMD 0603**, NICHT
mehr THT 3 mm und NICHT mehr im Switch-Body integriert. Position: über jedem
Switch-Body. Modifier-LEDs (LED6-LED10) bei Y=60 (über jedem Modifier bei Y=50),
Cell-HOLD-LEDs (LED11-LED15) bei Y=88 (in der 9 mm-Lücke zwischen Cell-Cap-Top
Y=84 und OLED-Modul-Bottom Y=93). Siehe `mechanical_coordinates.md` §4a + §5.

**Schaltung pro LED-Kanal** (open-drain Sink-Konfiguration):
- LED-Anode → R_LEDn (390 Ω 0603) → **+5 V**
- LED-Kathode → PCA9685 LEDn-Pin
- PCA9685 zieht Kathode LOW (~0.5 V) → I_LED = (5 - 2.1 - 0.5) / 390 = ~6.2 mA
- Max-Rating PCA9685: 25 mA pro Pin sink → großzügige Reserve

**Begründung +5V statt +3V3 als LED-Versorgung**: bei Vf ≈ 2.1 V (warm-weiß /
amber) würde +3V3 nur ~1.2 V Headroom lassen → Helligkeit empfindlich
gegenüber Rail-Sag. +5 V gibt ~2.4 V Headroom, viel stabiler. PCA9685-Outputs
sind 5.5 V-tolerant (Datasheet S. 8, V_OL bei externer Pull-Up-Last).

**PWM-Frequenz**: 1.0 kHz (Firmware) — flackerfrei für Auge, kein hörbares
Coil-Whistle bei den 0603-Widerständen.

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

### Speakers (r14 Acoustic-v2 — Sealed + Top-Firing, 2026-06-02)

2× **PUI AS04008PS-4W-WR-R**, 40×40×9mm, **8 Ω** (Datenblatt — korrigiert
gegenüber früheren Spec-Fassungen die fälschlich 4 Ω angaben), 2 W RMS / 4 W max,
**F0 = 380 Hz ± 20 %**, Frequenzbereich 200 Hz – 20 kHz, 84 dB @ 1 W/50 cm.
Quelle: [PUI Audio Datenblatt](https://puiaudio.com/file/specs-AS04008PS-4W-WR-R.pdf).

**Akustik-Konzept: Sealed Box + Top-Firing (kein Bass-Reflex, kein Passivradiator)**

| Element | Wert | Begründung |
|---|---|---|
| Kammer | **Geschlossen pro Kanal**, Trennsteg L/R im Bottom-Case-Inlay | Einzige sinnvolle Kammerform für einen Treiber mit F0=380 Hz. Reflex-Systeme (Port oder PR) lassen sich physikalisch nicht unter F0 abstimmen — ein PR mit Fb≈330 Hz würde nur eine Resonanzspitze in den unteren Mitten machen, schlimmster Fehlerfall für Drone/Sustain-Audio (One-Note-Boom). Sealed = saubere monotonische Roll-Off, kein Dröhnen. |
| Treiber-Ausrichtung | **Top-Firing in der Top-Plate**, nicht down-firing | Down-firing nutzt nur Boundary-Coupled-Bass — den dieser Treiber nicht erzeugt (F0=380 Hz). Top-firing maximiert die *einzige* echte Stärke (Mitten/Höhen-Klarheit) durch direkten Schallweg zum Ohr, ohne Tisch-Reflexion und Kammfilter. |
| Mount | Speaker-Rahmen von unten gegen die Top-Plate, 4× M2 | PCB-Speaker-Cutouts (alt: 41 mm dia bei Y=30) **entfallen** — Treiber sitzen nicht mehr im PCB. Akustik-Kammer wird durch Top-Plate + Bottom-Case + Trennsteg gebildet. |
| Top-Plate-Grille | 2× Cutout 38 mm dia bei (50, 30) und (270, 30), mit Schutzgitter oder Lochmuster | Schallaustritt direkt nach oben. Cutout < Treiber-Außenmaß (40×40 mm) damit die Membran abgedeckt bleibt. |

**Realistische akustische Erwartung**: 

- Onboard ehrlich nutzbar etwa **200 Hz – 20 kHz** (vom Datenblatt).
- Was klingt: klare Mitten, präsente Höhen, Pad-Saws fett und transparent,
  Reverb-Fahnen sauber.
- Was *nicht* klingt: alles unter ~200 Hz. famSubBass (LP90) und der untere
  Teil von famDeepBass (HP50/LP350) sind **onboard schlicht nicht hörbar** —
  sie sind direkter Beitrag null. Indirekt sind sie über den Reverb-Send
  (`verbSend 0.03` bzw. `0.08`) als Wärme der Fahne präsent, aber das ist
  alles. Voller Tiefgang ausschließlich über **Line-Out J8** → externe Boxen.

Ein sanfter DSP-Low-Shelf bei ~400 Hz kann später optional die wahrgenommene
Wärme im Treiber-Eigenbereich anheben (Firmware-seitig, Engine-Step 11/Master
oder Engine-Step 8/Bass). Echten Bass *erzeugt* DSP nicht — der Treiber hat
schlicht keinen Hub unter 200 Hz.

**Mechanik-Konsequenzen** (siehe `mechanical_coordinates.md` §7 r14):
- PCB-Speaker-Cutouts entfallen (mehr nutzbare PCB-Fläche im unteren Streifen)
- Top-Plate bekommt 2× 38 mm Grille-Cutout bei (50, 30) / (270, 30) —
  identische X/Y wie die alten Speaker-Positionen, daher keine Re-Verifikation
  gegen Front-Plate-Bezel/OLED/Encoder-Cluster nötig (in r13 schon gemacht)
- Bottom-Case wird zur reinen Kammer-Rückwand, kein Speaker-Mount mehr,
  keine Port-Ausschnitte, kein Grille-Pattern

**Verlauf der akustischen Entscheidungen**:
- *v0.6 (initial)*: 2× Bass-Reflex-Port 8×25 mm hinten, ~80 Hz Tuning,
  down-firing. Falsch weil Port-Länge nicht ins Gehäuse passt UND Port-
  Chuffing bei Drone/Sustain.
- *r13 (2026-06-01)*: Sealed-Box + Top-Plate-Passivradiator als Verbesserung.
  Falsch weil PR ein Reflex-System ist, das genauso wenig unter F0=380 Hz
  abgestimmt werden kann wie ein Port — der Grundirrtum (man könne diesen
  Treiber zum Bass-Treiber machen) war beim Wechsel von Port → PR mit
  übergegangen.
- *r14 (2026-06-02)*: Sealed + Top-Firing. Ehrliche Akustik: der Treiber
  spielt das, was sein Datenblatt erlaubt (Mitten + Höhen), und maximiert
  diesen Bereich. Bass-Layer ist offiziell Line-Out-Zone.

### Bass-Reflex/Passivradiator — beide ENTFERNT
F0=380 Hz schließt jede Reflex-Lösung (Port oder PR) aus. Geschlossene Kammer
ist die einzige korrekte Lösung für diesen Treiber.

### Line-Out / Kopfhörer (J8, v0.7)

Passiver Tap an PCM5102A VOUTL/VOUTR (vor dem PAM8403) → 3.5mm TRS-Buchse,
damit der tiefe Charakter über externe Boxen/Kopfhörer hörbar wird.

| Element | Wert |
|---|---|
| J8 | 3.5mm TRS-Buchse mit Insertion-Detect-Switch (PJ-320D, LCSC C431535) |
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
- **r7-B1 BLOCKER → r10 RESOLVED**: r7 forderte Vermessung des AliExpress-Generic-Custom-Footprints. Mit r10 (plain 4-pin SMD-Tactile, HX 12x12x7.3TPFT-B / C36498966 als Top-Kandidat aus JLC Extended) ist der Custom-Footprint weg → r10-B8 ist nur noch Sourcing-Pad-Standard-Verify, kein BLOCKER mehr.
- **r10-B8 IMPORTANT**: HX 12x12x7.3TPFT-B Pad-Geometrie gegen Industrie-Standard-12×12-SMD-4P (6.5×4.5 mm Raster, 1.0×1.5 mm Pads) cross-checken. Datasheet bei LCSC nicht verfügbar → entweder Sample-Vermessung (1 Stk @ $0.05) oder Pad-Pattern aus jlcsearch-Footprint-Library ziehen wenn dort hinterlegt. KEIN Custom-Footprint-Erstellungsaufwand mehr nötig.
- **r7 OFFEN: PCA9685 Symbol-Pin-Map** — `Driver_LED:PCA9685PW` aus KiCad-Standard-Lib gegen NXP-Datasheet (PCA9685 Rev. 4, S.6) verifizieren (28 Pins, alle 16 LED-Outputs + I²C + /OE + EXTCLK + VDD/GND + A0..A5)

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

## 12. UX-Specification — Firmware-Contract (NEU r10, 2026-05-31)

Diese Sektion bindet die Firmware an die Hardware-Affordances aus §7 (MCP23017
Switches), §7.2 (PCA9685 LEDs) und §5 (Pico-Pins). Sie ist **kein** PCB-
Layout-Blocker, **ist** aber Vertrag für die Firmware vor erstem Audio-Build.

### 12.1 CLEAR-Switch-Semantik (SW10, MCP-GPB4)

Zwei Druckdauern, zwei Semantiken:

| Dauer | Aktion | Audio | LED10 |
|---|---|---|---|
| **Short-Press** (<700 ms) | **Strong Panic** | 50 ms ramp-down → alle Voices killen + alle Modi (HOLD/DRONE/GENERATE) OFF + alle Cells release → 100 ms silence → Engine bleibt aktiv | 200 ms voller Flash, dann aus |
| **Long-Press** (≥3000 ms) | **Soft Shutdown** (siehe §13) | Save-State → 500 ms fade-out → AMP /MUTE LOW → /SHDN LOW → MCU-WFE-Stop | Während Hold pulsiert 0.5 Hz, dann aus |

**Warum „Strong Panic" statt nur „Mode-Reset"**: bei generativen Modi kann eine
fehlerhaft hängende Voice einen Dauerton produzieren. CLEAR muss garantiert
audio-stille Sicherheit liefern, sonst ist Live-Use riskant. Mode-Reset alleine
würde z.B. eine via HOLD getriggerte Cell nicht freigeben.

**Race-Condition-Garantie**: Short-Press wird **erst beim Release** ausgewertet
(nicht beim Press), damit Long-Press eindeutig erkennbar ist. Während des 3 s
Hold-Fensters läuft Audio normal weiter, LED10 pulsiert sichtbar als
Confirm-Indikator. Bei Release vor 3 s → Short-Press-Aktion. Bei Erreichen
3 s → sofortige Soft-Shutdown-Sequenz, weiteres Halten ignoriert.

### 12.2 State-Persistence über Power-Cycle

Persistiert wird via Pico-2 Internal Flash (XIP, eigener Slot am Ende des
2 MB-Flash, ~4 kB reserviert). Schreib-Trigger:
- Save-Snapshot (Long-press EN3, siehe §12.4)
- Soft-Shutdown (§13)
- **Nicht** bei jedem Encoder-Tick (Flash-Wear-Schutz)

| Field | Boot-Default | Persistiert? | Rationale |
|---|---|---|---|
| Volume (EN4) | **30 %** (clamped) | Last-Saved-Snapshot zurückgesetzt auf 30 % wenn höher | Schutz vor versehentlichem Loud-Boot bei Headphones |
| Drive (EN1) | Last-Saved | ✓ | Klang-Charakter ist gewollt persistent |
| Brightness (EN2) | Last-Saved | ✓ | Display-Helligkeit ist Raum-spezifisch |
| Display-Page (EN3) | Page 0 (Home) | ✗ | Boot-State soll vorhersehbar sein |
| **HOLD** (Mode) | **OFF** | ✗ | Sicherheit: kein Dauerton beim Einschalten |
| **DRONE** (Mode) | **OFF** | ✗ | Sicherheit: kein Audio bevor User bewusst startet |
| **GENERATE** (Mode) | **OFF** | ✗ | Wie oben |
| Preset-Slot | Last-Loaded | ✓ | User erwartet seinen letzten Patch |
| Cell-Toggle-State | alle OFF | ✗ | Cells sind transient, nicht persistent |

**Volume-Clamp-Rationale**: bei Battery-Mode (r9) ist Volume sowieso auf 70 %
gecappt. Der 30 %-Boot-Default ist orthogonal zum Battery-Cap und greift
**immer** (auch bei USB-C-Mode), um „Boot mit Kopfhörer auf 100 %"-Unfälle zu
vermeiden. EN4-Drehung erhöht sofort hörbar — der User weiß instant, dass
das Gerät an ist.

### 12.3 Mode-Interaktionen (HOLD × GENERATE × SHIFT)

Modi sind nicht-exklusiv, aber haben definierte Vorrangsregeln:

| Kombination | Verhalten |
|---|---|
| HOLD aktiv + Cell-Press | Cell wird zur sustained Drone (Voice ohne Release) — Standard |
| HOLD aktiv + GENERATE-Trigger | GENERATE-Trigger wird **ignoriert** (verhindert dass Generator HOLD-Drones überschreibt) |
| GENERATE aktiv + Cell-Press | Cell wird zum Generator-Seed (Tonhöhe/Timbre-Quelle für nächsten Generator-Voice) |
| SHIFT (gehalten) + EN1-Drehung | EN1 wird zu Secondary-Funktion (z.B. Filter-Cutoff statt Drive) — Per-Encoder-Mapping in Firmware |
| SHIFT (gehalten) + HOLD-Press | **Degree-Freeze**: aktuelle Skalenstufe wird gelocked, weitere Cell-Presses bleiben in derselben Tonart |
| SHIFT (gehalten) + DRONE-Press | DRONE-Engine-Variant-Cycle (z.B. Sub-Drone vs. Pad-Drone) |
| SHIFT (gehalten) + GENERATE-Press | GENERATE-Algorithm-Cycle |
| SHIFT (gehalten) + CLEAR-Short-Press | **Soft-Panic**: nur Voices killen, Modi bleiben (nicht-destruktive Variante des Strong-Panic) |
| CLEAR (Short-Press, kein SHIFT) | Strong-Panic (siehe §12.1) — überschreibt alles |

**LED-Sichtbarkeit der Modi**: LED7/8/9 (HOLD/DRONE/GENERATE) zeigen die
Firmware-State binär. LED6 (SHIFT) ist nur an solange Switch gedrückt
(reine Modifier-Indikator-Affordance). LED10 (CLEAR) flasht / pulsiert
contextuell (§12.1).

### 12.4 Save-Snapshot via EN3 (Display-Encoder)

EN3 hat dreifache Funktion abhängig von Press-Pattern:

| Aktion | Effekt |
|---|---|
| Drehen | Display-Page wechseln (Home / Engine-Detail / FX / Preset-Browser / Diagnostics) |
| Short-Press (<400 ms) | Page-Confirm / Sub-Menü-Enter |
| Long-Press (≥1500 ms) | **Save Current State as Preset** — überschreibt aktuell geladenen Preset-Slot. Display-Confirmation: 1 s Toast „SAVED → Slot N" |

**Save schreibt**: Drive/Brightness/Volume/Preset-Engine-Parameter (alle
Per-Engine-State), aber **nicht** transiente Modi (HOLD/DRONE/GENERATE).
Snapshot ist Klang-Snapshot, kein Performance-State-Snapshot.

**Wear-Schutz**: Flash-Slot ist Ring-Buffer mit 32 Save-Slots; jeder Save
schreibt in den nächsten Slot, Header markiert „Current". Worst-Case 32k
Save-Operationen vor Wear-Limit (Pico-2 XIP Flash spec ≥10k cycles/sector,
also ~320k Saves total). Bei typ. 5 Saves/Tag → 175 Jahre Lifetime.

### 12.5 Initial Boot State (T+0 bis T+1 s)

Definierte Power-On-Sequenz, von Pico-Reset bis Audio-Ready:

| t (ms) | Aktion |
|---|---|
| 0 | Pico-Boot, Pull-Downs halten AMP /SHDN + /MUTE LOW (silent) |
| 0..50 | Firmware-Init: GPIO-Direction, I²C-Bus, MCP23017 + PCA9685 + OLED Init |
| 50 | PCA9685 /OE bleibt HIGH (LEDs aus), OLED zeigt „Field Ambience v…" Splash |
| 50..200 | Flash-Read: last Preset + last Drive/Brightness laden, Volume clamp auf 30 % |
| 200 | Pop-Suppression-Sequenz §5 starten: /SHDN HIGH (Amp wakeup) |
| 250 | /MUTE HIGH + PCM XSMT HIGH (Audio frei) — **aber Engine produziert noch silence** |
| 250..750 | Engine startet mit Volume=0, fade-in auf User-Volume (30 %) über 500 ms linear |
| 750 | OLED wechselt zu Home-Page, LED-Indikatoren spiegeln Mode-State (alle OFF) |
| 750+ | Bereit für User-Interaktion |

**Warum 500 ms Fade-in statt sofort hörbar**: vermeidet hörbaren Boot-Click
auch wenn Hardware-Pop-Suppression perfekt ist (Engine-Initialisierung kann
DC-Bias-Drift erzeugen während Voices allokiert werden).

### 12.6 USB-Configuration-Mode (Optional, später)

Aus §12 herausgehalten: USB-MIDI / WebUSB-Config / Serial-CLI sind Firmware-
Aufgaben die nicht im Hardware-Vertrag stehen. Vermerk hier nur damit klar
ist: alle USB-Funktionen laufen über den Pico-USB-Port (J7); kein zweiter
USB-Connector geplant.

---

## 13. Soft-Shutdown-Sequenz (NEU r11, 2026-05-31)

Trigger: Long-Press CLEAR (SW10) ≥3 s (siehe §12.1). Im Battery-Mode wichtig
damit die Engine nicht durch Battery-Cutoff abrupt sterben muss
(Audio-Click + Risk-of-Data-Loss).

### 13.1 Sequenz (Pico-Firmware)

| t (ms ab Trigger) | Aktion |
|---|---|
| 0 | LED10 startet 2 Hz Fast-Pulse (visuelle Shutdown-Confirmation) |
| 0..500 | Engine: Voice-Fade-Out (linear gain → 0) über 500 ms |
| 500 | PCM5102A XSMT (MCP-GPA5) LOW — DAC stumm |
| 550 | PAM8403 /MUTE (GP28) LOW — Amp-Output muted |
| 600 | PAM8403 /SHDN (GP27) LOW — Amp aus |
| 650 | Flash-Save: aktueller Volume/Drive/Brightness/Preset-Slot-State |
| 750 | OLED zeigt „SHUTDOWN" → Display-Power-Down-Command (SSD1322 Sleep) |
| 800 | PCA9685 /OE HIGH (LEDs aus), aber LED10 noch 1 letzten Flash |
| 900 | Pico geht in `__wfe()` Deep-Sleep — nur Wake via CLEAR-Re-Press-IRQ |

### 13.2 Wake-Up-Pfad

CLEAR-Re-Press triggert MCP23017-INTA → Pico-IRQ auf GP22 → Wake aus WFE.
Firmware re-bootet komplett über `watchdog_reboot(0,0,0)` → §12.5 Initial-
Boot-Sequenz greift. Begründung: vollständiger Re-Boot ist sicherer als
inkrementelles Resume (Audio-Stack-State kann während Sleep degradiert sein).

### 13.3 Hardware-Implikation

**Keine Hardware-Änderung** zu r9. Soft-Shutdown nutzt ausschließlich
existierende GPIO-Pfade (AMP /SHDN, /MUTE, PCM XSMT, PCA9685 /OE). Boost-
Converter TPS61089 (r9) bleibt aktiv — wir schalten den nicht ab, weil
sein EN-Pin nicht mit dem Pico verdrahtet ist (r9-Schaltung). Resultierender
Sleep-Drain: ~5-8 mA (Pico WFE + TPS61089 quiescent + MCP23017 input-mode).
Bei 5000 mAh ≈ 25-40 Tage Sleep-Lifetime — akzeptabel für Performance-Gerät.

**Optional r13 (Future)**: TPS61089-EN-Pin auf Pico-GPIO oder MCP-GPIO legen
für echten Zero-Drain-Sleep (<100 µA). Erfordert Re-Spin des Battery-Sheets,
deshalb hier nicht im Scope.

---

**Ende v0.6.3-r3 Errata-Stand.** Siehe `PCB_TODO.md` für Item-Tracking.
