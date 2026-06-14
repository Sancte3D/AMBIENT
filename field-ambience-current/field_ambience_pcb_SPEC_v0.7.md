# Field Ambience Controller PCB — Spec

> ⚠️ **v0.7 — MCU-Migration aktiv: Pico 2 (RP2350) → STM32H743VIT6.**
> Während des UX-Reviews der Display-Sim wurde die Pico-2-Wahl noch einmal
> sauber re-evaluiert (Op-Counting vs WCET, Audio-Headroom für ein Produkt
> statt Prototyp). Ergebnis: **STM32H743VIT6 in LQFP100 als Bare-Chip** —
> 3-4× CPU-Headroom (M7 @ 480 MHz + DTCM/ITCM), 1 MB internes RAM, native
> SAI/QEI/USART-Peripherie ersetzt die Pico-PIO-Tricks. **Alle peripheren
> Komponenten bleiben 1:1** (PCM5102A I²S-DAC, PAM8403 Amp, ST7789 LCD,
> MCP23017 + PCA9685, EC11-Encoder, Choc-Cells, USB-C, Battery-Path). Nur
> der MCU + sein direkter Power/Clock-Tree wird umdesigned. Pico-Stand
> bleibt im Repo als `kicad/legacy_pico2/` archiviert für Notfall-Fallback.
> Maßgebliche Begleit-Doku: `NATIVE_PORT_PLAN.md` Step 13.

**Rev:** 0.7-r18.4 (H7-Migration — SPEC-Major-Bump weil Architektur-Bruch; r18.4: Y1 Crystal final = ABLS-8.000MHZ-B4-T, F-4 RESOLVED)
**Target:** 4-Layer JLCPCB, partial-PCBA (siehe §4 BOM-Split A/B/C)
**Methodik:** Datasheet-Verifikation + JLCPCB-Stock-Check vor jeder Komponente
**Status:** SCHEMATIC IN MIGRATION — Phase 1 (Doku) ✓, Phase 2-5 ausstehend.
**PCB-Layout erst NACH Profiling auf echter H743-Hardware** (Acceptance-Gate
in `NATIVE_PORT_PLAN.md` Step 13.5). Änderungshistorie in `CHANGELOG.md`.

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

## Was in v0.7 anders ist als v0.6 (MCU-Migration)

v0.7 ist ein **Architektur-Major-Bump**: der MCU wird vom **Raspberry Pi Pico 2
(RP2350-Modul)** auf **STMicroelectronics STM32H743VIT6 (LQFP100, Bare-Chip)**
gewechselt. Auslöser war ein UX-Review der Display-Sim, in dem die ursprüngliche
„Pico 2 reicht"-Begründung als methodisch unzureichend entlarvt wurde:

- **Op-Counting statt WCET-Messung** war eine Plausibilitätsstudie, kein Profiling.
- **Beide RP2350-Cortex-M33-Cores als DSP-Ressource** rechnen ist unrealistisch
  (Core 1 wird für UI gebraucht; effektiv ~150 MMACS statt 300).
- **Single-Sample-Miss = Audio-Dropout**, bei ~25 % geschätztem Headroom keine
  Reserve für ein Produkt (vs Prototyp).
- **DTCM/ITCM auf dem H7** löst exakt das XIP-Flash-Cache-Miss-Risiko, das auf
  dem RP2350 die WCET unvorhersehbar macht.

Konsequenz für die Hardware: **nur** der MCU + sein direkter Power/Clock-Tree
ändert sich. Alle peripheren Komponenten (DAC, Amp, LCD, I²C-Expander, Encoder,
Speaker, Battery) bleiben elektrisch identisch — sie hängen an Standard-Bussen
(SPI/I²C/I²S/GPIO), die der H743 nativ kann.

| Severity | Finding v0.6 | Fix v0.7 |
|---|---|---|
| CRITICAL | C1: Pico 2 CPU-Headroom für ein Audio-Produkt nicht messbar (kein Cycle-Profiling), Op-Counting-Schätzung methodisch unzureichend | **MCU-Wechsel auf STM32H743VIT6**, LQFP100, hand-lötbar. 480 MHz M7 + DTCM/ITCM + Double-Precision FPU = 3-4× CPU-Headroom |
| CRITICAL | C2: RP2350-Modul (SC1631) nicht bei JLC bestückbar — externe Beschaffung + manuelle Pre-Bestückung notwendig | **STM32H743VIT6 ist JLC-stockable** (LCSC C114409, ~$6.62, Mouser/DigiKey lagernd) |
| HIGH | H1: Pico-internes SMPS + interne PLL nicht extern messbar (Audio-Clock-Jitter bei stagniertem Pico-Boot unklar) | **HSE 8 MHz Crystal + 2× 22 pF Load-Caps** als Audio-Sync-Quelle (jitter-arm; H743-internes SMPS für VCAP/VCORE) |
| HIGH | H2: Pico-Pin-Reserve = 0 (alle 24 GPIOs belegt) — kein Headroom für SDRAM, Touch, USB-MIDI-IN | **H743 hat ~80 GPIOs in LQFP100** + 4× SPI + 3× I²C + 2× SAI + 12× TIM (4× QEI-fähig) + 4× USART → massive Reserve für Rev-B-Erweiterungen |
| MEDIUM | M1: Encoder-Lesung Software-Polling (Pico Timer-IRQ 1 kHz) — Verlust bei schneller Drehung | **TIM-QEI-Hardware-Modus** auf 4 TIM2/3/4/5 — kein Polling, kein Jitter |
| MEDIUM | M2: MIDI-TX via PIO-UART (Workaround) | **Hardware-USART2 @ 31250 baud** |
| MEDIUM | M3: SPEC §1/§3 noch Pi-zentrisch (historische Reste aus v0.9-Transition) | **§1 Architektur-Diagramm + §3 Power-Budget vollständig auf H743 umgeschrieben** (Pi-Reihe in Power-Budget entfernt) |

**Bewusst NICHT in v0.7:**
- PCB-Layout (`.kicad_pcb`) — kommt nach Profiling-Acceptance-Gate (Step 13.5).
- SDRAM-Bestückung — Footprint vorsehen, Bestückung in Rev-B.
- Convolution-Reverb — Freeverb bleibt (sound-bewährt, getestet).
- Polyphony-Aufstockung über 5 Cells — Sound-Constitution unverändert.

---

## Was in v0.6 anders war als v0.5 (historisch — bleibt zur Nachvollziehbarkeit)

v0.6 behob vier CRITICAL/HIGH-Findings aus dem v0.5-Review, plus vier MEDIUM-Korrekturen.

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

## 1. Architecture (v0.7 — STM32H743 Bare-Chip)

```
                     ┌─────────────────────────────────────────────┐
                     │           Field Ambience Device              │
                     │  333 × 143.3 × 40mm                          │
                     │                                              │
USB-C 5V/3A ────► USB-C  ──┬──► F1 (3A/6A) ──► 1000µF Bulk ──► +5V │
(Power + USB-Data          │                                       │
 + DFU-Flash, optional     ├──► D+/D− ──► USBLC6 ESD ──► STM32 USB │
 SWD via J4)               │                  (USB-CDC / DFU)       │
                           │                                        │
                           └─────► +5V Rail ──┬──► PAM8403 Amp      │
                                              │                     │
                                              ├──► PCM5102A AVDD    │
                                              │                     │
                                              ├──► LCD VLED         │
                                              │                     │
                                              └──► LDO (NCP163)      │
                                                   ─► +3V3 Rail      │
                                                   ─► STM32 VDD/VDDA │
                                                   ─► MCP23017       │
                                                   ─► PCM5102A DVDD  │
                                                   ─► EC11 Pull-Ups  │
                                                                      │
                          STM32H743VIT6 (LQFP100)                     │
                          M7 @ 480 MHz, 1 MB RAM, 2 MB Flash          │
                          HSE: 8 MHz Crystal + PLL → 480 MHz          │
                              │                                       │
              ┌───────┬───────┼────────┬──────────────┐              │
              │       │       │        │              │              │
            SAI1     SPI1   I²C1   USART2         TIM2/3/4/5         │
              ▼       ▼       ▼       ▼            (QEI x4)          │
         PCM5102A    LCD   MCP+PCA  MIDI-TX     EN1/2/3/4            │
         I²S Slave  ST7789   I²C    31250 baud  Drive/Brightness/    │
         44.1k/16   320x170  @0x20  TRS Type A  Display/Volume        │
              │       │      0x40                                     │
              │                                                       │
         Audio Out                                                    │
              ▼                                                       │
         PAM8403 Amp ─► 2× CMS-402811-28SP (8 Ω, sealed enclosure)    │
         (GPIO PB14 = nSHDN, PB15 = nMUTE — Pop-Suppression-          │
          Sequenz wie §8.3, identisch zu Pico-Variante)               │
                                                                      │
         Plus: ADC1_INP15 = BAT_SENSE (PA3), MCP_INT = PC13           │
              └─────────────────────────────────────────────┘
```

**Update-Pfade**:
- **Firmware**: USB-C einstecken → STM32 in DFU-Mode via USB-CDC oder
  externes ST-Link via SWD-Header J4 (PA13=SWDIO, PA14=SWCLK, PB3=SWO).
- **Boot-Mode**: BOOT0-Pull-Down 10 kΩ → System-Memory-Bootloader nur via
  USB-DFU oder externem Reset-Knopf erreichbar (kein Daily-User-Risk).

**Kein micro-USB am Gehäuse sichtbar.** Einziger äußerer Stecker = USB-C.

**Was sich gegenüber v0.6 NICHT geändert hat:** Gehäuse-Form, USB-C-Stecker,
PCM5102A DAC, PAM8403 Amp, ST7789 LCD, MCP23017 + PCA9685, alle EC11-Encoder,
alle Choc-V2-Cells, Battery-Path (MCP73831 + TPS61089 + P-MOSFET), USB-ESD-
Schutz (USBLC6), Polyfuse F1, Bulk-Cap 1000 µF.

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
| BAT1 | LiPo 3.7V **2000mAh** Pouch 503759 (9.4×37×50mm) — **r18.21 rightsize von 5000mAh** (Overkill; 2000mAh ~6.6h @ 300mA) | nicht JLC | du lieferst | Energiespeicher, JST PH 2.0 2-pin |
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

| Verbraucher | Idle | Typical Audio | Worst Case (loud, 8Ω r14) |
|---|---|---|---|
| **STM32H743 @ 480 MHz** (PLL aktiv, DSP-Load, DMA, 8 GPIO-Banks aktiv) | 90 mA | 180 mA | 240 mA |
| ST7789 LCD + Backlight (PCA9685-PWM) | 60 mA | 90 mA | 120 mA |
| MCP23017 + Pull-Ups | 5 mA | 20 mA | 25 mA |
| PCM5102A DAC (AVDD+DVDD) | 20 mA | 30 mA | 30 mA |
| **PAM8403H @ 8Ω, 3W Out (r14)** | 60 mA | 400 mA | **700 mA** |
| Encoder + Status-LED | 5 mA | 15 mA | 25 mA |
| **PCA9685 + 10× LEDs** (5× Modifier + 5× Cell-HOLD, je 8 mA peak @ 100% PWM) | 5 mA | 35 mA | **85 mA** |
| LDO Quiescent (NCP163 oder vgl.) | 1 mA | 1 mA | 2 mA |
| **TOTAL** | **246 mA** | **771 mA** | **1227 mA** |

**Anmerkung v0.7 (MCU-Wechsel)**: H743-Stromverbrauch laut **ST DS12110 Rev 5
§6.3.4 (Table 33, Typical and maximum current consumption in Run mode)**:
- 71 mA typical @ 400 MHz CPU, alle Peripherie aktiviert
- ~120 mA gemessen auf Nucleo-H743ZI @ 480 MHz mit Cache/DMA aktiv (ST-Community-Report,
  IDD-Jumper-Messung am Eval-Board)
- **180 mA Worst-Case-Schätzung** in dieser Tabelle = konservativer Aufschlag
  für unsere spezifische Last (alle GPIOs aktiv, SAI-DMA continuous, SPI-DMA
  Burst für LCD-Blits, ADC continuous, 4× TIM-QEI). Final wird Phase 5
  (Profiling auf echter Hardware) den exakten Wert messen.

Pico 2 lag bei ~50 mA — die H743-Mehrkosten am Strom-Budget sind ~70-130 mA
typical (mid-range Annahme). Im Worst-Case bleibt das Gesamt-Budget mit
**~1.2 A** sehr deutlich unter F1 (3 A hold) und unter TPS61089 (2 A @ 5 V) im
Battery-Mode → der Battery-Mode-Volume-Clamp aus r9 kann gelockert oder
ganz entfernt werden (separater Firmware-PR in Phase 4 nach Profiling-Messung).

**Anmerkung r14 (Akustik-v2, Impedanz-Korrektur 2026-06-02; r18.18 Treiber-
Wechsel)**: Beide Treiber (CMS-402811-28SP primär, AS04008PS-4W-WR-R
sekundär) sind laut Datenblatt **8 Ω** — Worst-Case-PAM8403-Strom halbiert
sich entsprechend gegenüber der alten 4-Ω-Annahme (~700 mA statt 1400 mA). F1 (3 A hold) hat
noch mehr Margin, TPS61089-Boost (2 A @ 5 V) deckt jetzt auch Worst-Case ab
(nominal 1.2 A vs 2 A Limit). Battery-Mode-Volume-Clamp bleibt als Akustik-
Schutz (Treiber-Verzerrung > 1.5 W) optional in Firmware.

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

U5 (AP7361A-33) ist seit r18 **aktiv** (+3V3-Quelle; der Pico-interne SMPS entfällt mit dem Modul, siehe §4 U5-Zeile). _r18.5-Korrektur: hier stand noch 'bleibt DNP' aus der Pico-Ära._

---

## 4. BOM v0.6.3-r3 — gesplittet JLC-SMT + manuelle Assembly

**⚠️ Update v0.6.3-r3**: BOM ist NICHT "alle SMT/Full-PCBA". Korrekte
Trennung in Section A (JLC-SMT, ~70 SKUs), B (manuelle Assembly, ~15
Items), C (TBD). Siehe Errata-Sektion v0.6.3-r3 oben für vollständige
Aufteilung.

LCSC-Nummern in dieser Sektion wurden v0.6.1/v0.6.2 normalisiert:
PCM5102A = **C107671** (war C9900003814, existiert nicht), PAM8403H =
**C17337** (war C84368, existiert nicht).

### Controller + MCU (v0.7-r18: STM32H743 Bare-Chip ersetzt Pico 2)

| Ref | Part | Package | JLCPCB # | Status | Hinweis |
|---|---|---|---|---|---|
| **U1** | **STM32H743VIT6 (STMicroelectronics)** | **LQFP-100** | **C114409** | **Extended, ~$6.62 @1, Mouser/DigiKey lagernd, JLC SMT-bestückbar** | **NEU v0.7: 480 MHz Cortex-M7 + Double-Precision FPU + DTCM/ITCM, 1 MB internes RAM, 2 MB Flash. Symbol `MCU_ST_STM32H7:STM32H743VIT6`, Footprint `Package_QFP:LQFP-100_14x14mm_P0.5mm`. Datasheet: ST DS12110. Pin-Allocation siehe §5.** |
| **Y1** | **ABRACON ABLS-8.000MHZ-B4-T** (HC-49/US SMD, 11.4×4.7×4.2 mm) | **HC-49/US SMD** | **C596838** | **Extended, ~$0.24, LCSC lagernd** | **GEWÄHLT r18.4 (ersetzt ABM3, siehe F-4).** 8 MHz Fundamental, ESR max **80 Ω** (Datasheet Drawing 450669 Rev AD Table 1), C₀ max 7 pF, CL 18 pF, Op-Temp -20…+70 °C (B), Freq-Tol ±30 ppm (4). **Gain Margin = 2.97** (Worst-Case bei ESR_max=80 Ω über vollen Temp-Bereich; AN2867-Lehrbuch-Min ist 5). **Akzeptiert für dieses Produkt:** Indoor-Audio-Gerät, real auftretende Temperatur 15–30 °C → ESR_typ ~40–50 Ω → realer GM ≈ 5–6. AN2867-Min 5 ist konservatives Industrial/Automotive-Kriterium über -40…+85 °C, hier nicht zutreffend. **Phase-5-Verifikation:** Crystal-Start am realen PCB testen, ggf. R_EXT/Load-Caps tunen. Symbol `Device:Crystal` (2-Pin), Footprint-Kandidat `Crystal:Crystal_SMD_HC49-SD` (KiCad-Standard für HC-49 SMD; **nicht** `Crystal_HC49-U_Vertical`, das ist THT) — Pads in Phase 3 gegen Datasheet-Land-Pattern Page 3 (5.6×2.1 mm Pads, 9.5 mm Spacing) verifizieren, ggf. eigenen Footprint zeichnen. Volle Analyse: `docs/component_reviews/Y1_alternatives.md` |
| **C_HSE1, C_HSE2** | **22 pF C0G 0603** | **0603** | Generic | **Basic** | **NEU v0.7: HSE Load-Caps** |
| **R_BOOT0** | **10 kΩ 0603** | **0603** | Generic | **Basic** | **NEU v0.7: BOOT0 Pull-Down → System-Loader nur via DFU/Reset** |
| **R_NRST** | **10 kΩ 0603** | **0603** | Generic | **Basic** | **NEU v0.7: NRST Pull-Up zu +3V3** |
| **C_NRST** | **100 nF X7R 0603** | **0603** | Generic | **Basic** | **NEU v0.7: NRST Debounce-Cap** |
| **C_VDD\*** | **7× (4.7 µF X5R 0805 + 100 nF X7R 0603), 1 Set pro VDD-Pin** | **0805 + 0603** | Generic | **Basic** | **NEU v0.7: H743 Decoupling. STM AN3318 §6 Empfehlung. Direkt am Pin platzieren.** |
| **C_VCAP1, C_VCAP2** | **2× 2.2 µF X5R 0603** | **0603** | Generic | **Basic** | **NEU v0.7: Interner SMPS Bulk-Caps (LDO-Bypass-Mode, kein externer Core-LDO nötig)** |
| **C_VDDA, FB_VDDA** | **1 µF X5R 0603 + 100 nF X7R 0603 + Ferrit BLM18AG601** | **0603** | Generic | **Basic** | **NEU v0.7: VDDA Analog-Filter** |
| U2 | MCP23017-E/SS | SSOP-28 | C506653 | Extended, ~$1.62 | verifizieren via `lcsc`-Skill |
| U3 | PCM5102APWR (TI) | TSSOP-20 | **C107671** | Extended Stock | I²S DAC, 3-wire (kein MCLK). Pinout per TI SLAS859C. **Anschluss bleibt unverändert — nur die Master-Seite ändert sich (Pico-PIO → STM32-SAI1).** |
| U4 | PAM8403DR-H (Diodes Inc) | SOIC-16 | **C17337** | Extended Stock | Stereo Class-D 2×3W. Pinout per Diodes DS31295 |
| U5 | **AP7361C-33Y5-13** (✅ r18.6, AP7361 ist NRND) | **SOT-89-5** | **C460397** (LCSC) | **Aktiv (war DNP)** | r18.6: Symbol/Footprint/Verdrahtung auf AP7361C; Pinout 1=EN,2=GND,3=ADJ/NC,4=IN,5=OUT (Diodes-DS via mouser.de/datasheet/3/175/1/AP7361.pdf — User-verifiziert). Frühere Kandidatur C150719 verworfen (= SOT-223, falsches Package). Cin/Cout 4.7µF X5R 0603 (DS empfohlen). | **NEU v0.7: +3V3-Rail-LDO wird jetzt verbaut (war im Pico-Stand DNP, weil Pico SMPS intern hatte). 1 A LDO genügt für H743 (~180 mA typ) + LCD + I²C + PCM5102A-DVDD.** |
| **U6** | **PCA9685PW,118 (NXP)** | **TSSOP-28** | **C2678753** | **Extended, ~$1.96 @100, ~1605 pcs Stock** | **NEU r7: 16-Kanal PWM-LED-Driver für 5 Modifier-Button-LEDs (11 Kanäle Reserve). I²C-Adresse 0x40. Symbol `Driver_LED:PCA9685PW`, Footprint `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm`. Datasheet: nxp.com/docs/en/data-sheet/PCA9685.pdf** |
| **SDRAM (Footprint vorsehen, nicht bestücken)** | **Reserve-Footprint für IS42S16400J-7TL (4 MByte) auf SDRAM-Pads** | **TSOP-50** | C81878 | **DNP** | **NEU v0.7: Footprint vorsehen für Rev-B, falls Engine später Granular/Samples bekommt. 1 MB internes RAM reicht aktuell für DSP + Reserve.** |

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
| ~~SW1-SW5~~ ✂ r18.9 | ~~Kailh Choc V2 Hot-Swap Socket~~ — r18.9: FSR-Pads; **r18.14 (ADR-0013): Cells sind jetzt Gateron-LP-MAGNETIC-Switches (pin-los, plate-mounted) + Hall-Sensor auf PCB** | — | **Ja** (Switches + LP-Stabilizer: Keyboard-Markt, z.B. Gateron direkt) |
| **J_CELL1-5** (r18.14) | **TI DRV5056A4QDBZR** (SOT-23, 3.3V ratiometrisch). Pinout DS-verifiziert: 1=VCC / 2=OUT / 3=GND. Bridge-Site bleibt als 1×3-Header bis Phase 6 (dann SOT-23-FP), Pin-Mapping ist 1:1. | **C2152902** | JLC Extended — JLC-bestückbar |
| **R_CELL1-5 / C_CELL1-5** (r18.14) | 5× **1 kΩ** 0603 (Hall-OUT-Serien-R) + 5× 10 nF X7R 0603 → RC fc≈16 kHz vor ADC | C21190 / C57112 | JLC-bestückt |
| **STAB_CELL** (r18.14, ADR-0013) | **LP-Stabilizer** für Cell-Caps ≥ 2u (Spacebar-Prinzip: Switch mittig, Stabilizer links/rechts). Gateron-LP-Klasse | — (Keyboard-Markt) | **Ja**, mit Switches zusammen beschaffen |
| **SW6-SW10** (r10) | **12×12×7.3 mm momentary tactile, plain (Modifier-Buttons Shift/Hold/Drone/Generate/Clear)**. HX 12x12x7.3TPFT-B. **Alle 5 identisch, momentary — Latch-Zustand zeigen die LEDs (§7.2), kein Rast-Schalter** | **C36498966** (JLC Extended) | JLC-bestückt mit Custom-FP `field_ambience:SW_HX_12x12x7.3_SMD-4P` (ADR r18.6) |
| SW11 | Reset Tactile SMD (XUNPU TS-1088-AR02016, FP `field_ambience:SW_TS1088_SMD` EasyEDA-verifiziert r18.14) | C720477 | JLC-bestückt |
| ~~SW12~~ ✂ r18 | ~~BOOTSEL Tactile~~ — **entfernt:** war Pico-spezifisch | — | — |
| **SW_BOOT** (r18.10) | **Mini-SMD-Tactile für BOOT0 (USB-DFU-Flash, ADR-0009)**. **r18.14: MPN korrigiert — C720477 ist XUNPU TS-1088-AR02016** (nicht TS-1185A-C-A); FP auf EasyEDA-verifiziertes `field_ambience:SW_TS1088_SMD` | **C720477** | **NEU r18.10: ohne SW_BOOT wäre USB-DFU-Flash nicht möglich** |
| **R_BOOT_SW** (r18.10) | 1 kΩ 0603 (BOOT0-SW-Strombegrenzung) | C21190 | JLC Basic |
| **EN3** (r18.14, ADR-0012) | **ALPS EC11E18244AU** — 18 Puls / 36 Detent, mit Push-Switch, Flat-Shaft 20 mm — Display-Encoder, 1 Rastung = 1 Menü-Schritt | **C202365** | JLC Extended (ALPS Original, in stock) — THT-Handbestückung im Prototyp |
| **EN1/EN2/EN4** (r18.14, ADR-0012) | **ALPS EC11E183440C** — 18 Puls, **OHNE Detent**, mit Push (Firmware nutzt nur A/B), Shaft 20 mm — selbe Bauform wie EN3 → **alle 4 gleich hoch** | **C370986** | JLC Extended (ALPS Original, in stock) — THT-Handbestückung im Prototyp |
| ~~EN1-EN4 alt~~ ✂ r18.14 | ~~EC11J1525402 SMD (C209762)~~ — **retired:** NRND + 3D-verifiziert 24.5 mm hoch (zu hoch, Kick75-Ziel) + Half-Step-Detent-Mismatch (ADR-0012) | — | — |

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

## 5. STM32H743 Pin Allocation v0.7-r18 (LQFP100, Datasheet DS12110 Rev 5 — Tabelle 8 verifiziert)

**Verifikations-Stand:** Alle Pin-Nummern und Alternate Functions sind gegen
die offizielle ST-Pin-Definitionstabelle (DS12110 Rev 5, Table 8) gegengeprüft.
Eine erste Version dieser Sektion enthielt Off-by-N-Fehler (SAI1-Pins, SPI1-
Pins, USB-OTG-Pins, PA15-Position) und einen Pin-76-Doppelbelegungs-Konflikt
(PA14 vs. fälschliches „PD5 = Pin 76") — alle in r18.1 korrigiert. Die finale
Verifikation gegen die Datasheet-Alternate-Function-Matrix (DS12110 Table 12)
für jede AF-Nummer passiert in Phase 3 (KiCad-Symbol-Eingabe).

**Pin-Reserve:** LQFP100 hat **82 GPIOs** (Datasheet S. 18) plus dedicated
NRST/BOOT0/HSE-Pins. Wir belegen **30 GPIOs** für die Engine — also ~50 GPIO-Pins
ungenutzt für Rev-B-Erweiterungen (Touch-Display, USB-MIDI-IN, weitere Encoder,
DFU-Button etc.).

### 5.1 Audio (SAI1 Block A — Master Mode, I²S Format)

| Pin | Port | AF | Funktion | Net |
|---|---|---|---|---|
| 3 | PE4 | SAI1_FS_A (AF6) | I²S Frame-Sync = LRCK → PCM5102A pin 15 | I2S_LRCK |
| 4 | PE5 | SAI1_SCK_A (AF6) | I²S Bit-Clock = BCK → PCM5102A pin 13 | I2S_BCK |
| 5 | PE6 | SAI1_SD_A (AF6) | I²S Serial-Data DOUT → PCM5102A pin 14 | I2S_DOUT |
| (1) | (PE2) | SAI1_MCLK_A (AF6) | nicht verbunden — PCM5102A braucht kein externes MCLK | — |

SAI1-DMA: SAI1_A nutzt DMA1 oder DMA2 (in Phase 4 finalisiert), Memory→Peripheral,
32-bit-Word, circular Mode für Ping-Pong-Buffering. Halb-Block-IRQ liefert je
**512 Frames** (r18.11, war 256); Engine rendert in den jeweils anderen Halb-Buffer.
PE2 (Pin 1) wird nicht beschaltet, da PCM5102A im 3-wire-Mode arbeitet
(BCK, LRCK, DIN — kein MCLK).

**SAI-Clock-Architektur (r18.11, ADR-0010):** SAI1 wird von **PLL2-Q oder PLL3-P**
gespeist, **nicht** von SYSCLK. Begründung: SYSCLK-PLL trägt CPU-Last-Variation
(Cache-Refills, IRQ-Stalls) als Phase-Noise; gekoppelt auf den BCK-Clock zum
DAC = audible Jitter im Hochton. STM32H7 hat zwei dedicated Audio-PLLs, eine
davon (Vorschlag: PLL3-P) auf 11.2896 MHz = 44100 × 256 → Master-Clock-Quelle
für SAI; SAI-Divider erzeugt daraus BCK (2.8224 MHz) und LRCK (44.1 kHz).
Wird in Phase 4 (Firmware-Migration) implementiert; PLL3-Konfig im
NATIVE_PORT_PLAN dokumentiert.

**Audio-Layout-Constraints (r18.11, vor Phase 6 Layout):**
1. **PCM5102A AVDD/DVDD-Decoupling** (FB1 + C7a/b + C8a/b) muss **< 5 mm vom IC**
   sitzen, vorzugsweise im selben GND-Plane-Polygon
2. **Single-Star-GND-Punkt** zwischen Audio-Block und Digital-Block: PCM5102A-
   GND, PAM8403-PGND, Speaker-Return alle auf einen Mid-Layer-Polygon-Anker
   verbunden, kein direkter Pfad über Digital-GND
3. **Class-D-Output-Filter** (PAM8403 hat keinen on-chip-Filter, BTL-Output ist
   ~500 kHz PWM): kurze Leitung zum Speaker (max 50 mm) plus optional
   LC-Bead-Filter (BLM18AG601 in jeder Output-Leitung, AVX TPS-Klasse) → fängt
   die Trägerfrequenz vor dem Speaker ab und reduziert Quasi-Peak-EMI
4. **Bulk-Cap-Platzierung:** C_BULK (1000 µF) so platzieren, dass der Strompfad
   zum PAM8403-PVDD-Pin den kürzesten Polygon-Loop bildet (Class-D zieht
   ~2 A-Spitzen bei Bass-Transienten; das ist der Hauptkratzigkeits-Pfad,
   wenn die Bulk-Cap weit weg liegt)
5. **PCM5102A-LDOO-Cap** (interner 1.8-V-LDO-Output, Pin 18): 100 nF X7R 0603
   direkt am Pin (Cstability)

### 5.2 LCD (SPI1 + GPIO-Control)

| Pin | Port | AF | Funktion | Net |
|---|---|---|---|---|
| 29 | PA5 | SPI1_SCK (AF5) | SPI Clock → LCD pin 4 (SCL) | LCD_SCK |
| 31 | PA7 | SPI1_MOSI (AF5) | SPI Data → LCD pin 5 (SDA) | LCD_MOSI |
| 30 | PA6 | GPIO | LCD CSn (active LOW) | LCD_CS |
| 32 | PC4 | GPIO | LCD Data/Command-Select | LCD_DC |
| 33 | PC5 | GPIO | LCD Reset (active LOW) | LCD_RES |

SPI1-DMA: SPI1_TX nutzt DMA für 320×170-Framebuffer-Blits (4-bit-Grey =
27 KB pro Blit). Backlight-PWM kommt nicht direkt vom MCU, sondern via PCA9685
(siehe §7.2 LED12).

### 5.3 I²C-Bus (MCP23017 + PCA9685)

| Pin | Port | AF | Funktion | Net |
|---|---|---|---|---|
| 92 | PB6 | I2C1_SCL (AF4) | I²C Clock @ 400 kHz | I2C_SCL |
| 93 | PB7 | I2C1_SDA (AF4) | I²C Data | I2C_SDA |
| 7 | PC13 | GPIO + EXTI | MCP23017 INTA (Wake-on-Switch) | MCP_INT |

I²C-Pullups: 2× 4.7 kΩ extern zum +3V3 (PB6/PB7 sind in LQFP100 mit AF I2C1
verfügbar — verifiziert in DS12110 Table 8 Zeile Pin 92/93).

### 5.4 Encoders (4× ALPS EC11 — Quadrature via TIM_QEI)

| Pin | Port | AF | Funktion | Net |
|---|---|---|---|---|
| 22 | PA0 | TIM2_CH1 (AF1) | EN1 Drive A | DRIVE_A |
| 23 | PA1 | TIM2_CH2 (AF1) | EN1 Drive B | DRIVE_B |
| 97 | PE0 | GPIO + EXTI | EN1 Drive Switch | DRIVE_SW |
| 63 | PC6 | TIM3_CH1 (AF2) | EN2 Brightness A | BRIGHT_A |
| 64 | PC7 | TIM3_CH2 (AF2) | EN2 Brightness B | BRIGHT_B |
| 98 | PE1 | GPIO + EXTI | EN2 Brightness Switch | BRIGHT_SW |
| 59 | PD12 | TIM4_CH1 (AF2) | EN3 Display A | DISPLAY_A |
| 60 | PD13 | TIM4_CH2 (AF2) | EN3 Display B | DISPLAY_B |
| 2 | PE3 | GPIO + EXTI | EN3 Display Switch | DISPLAY_SW |
| 67 | PA8 | TIM1_CH1 (AF1) | EN4 Volume A | VOL_A |
| 68 | PA9 | TIM1_CH2 (AF1) | EN4 Volume B | VOL_B |
| — | — | — | EN4 Volume Switch (bleibt auf MCP-GPB5 wie r15) | VOL_SW |

**Hardware-QEI** statt Software-Polling: TIM1/2/3/4 zählen A/B-Quadratur in
Hardware, Firmware liest nur den 32-bit-Counter — kein Jitter, kein Verlust
bei schneller Drehung, halbiert die HAL-LOC. PC6/PC7 statt PA6/PA7 für TIM3,
um den Konflikt mit SPI1 (PA5/PA7 → LCD) zu vermeiden. EN4 (TIM1) nutzt PA8/PA9
— PA9 hat zusätzlich OTG_FS_VBUS als „additional function", aber Bus-Powered-
USB ohne VBUS-Sensing ist Standard (Datasheet S. 39 fußnote).

VOL_SW liegt auf MCP-GPB5 (_r18.5-Anm.: die r15-Behauptung stimmte mit dem
Schaltplan nicht — bis r17 lag VOL_SW real am Pico-GPIO und GPB5 war NC;
seit dem r18.5-Generator ist GPB5 tatsächlich VOL_SW_) — am H7 wäre ein
direkter MCU-Pin frei, aber der MCP-Sheet bleibt damit unverändert.

### 5.5 MIDI Out (USART2 — Hardware-UART statt PIO-Workaround)

| Pin | Port | AF | Funktion | Net |
|---|---|---|---|---|
| 86 | PD5 | USART2_TX (AF7) | MIDI-TX @ 31250 Baud → TRS Type A | MIDI_TX |

USART2_TX hat zwei Pin-Optionen im LQFP100: PA2 (Pin 24) ODER PD5 (Pin 86).
Wir wählen **PD5**, da PA2 frei bleiben soll als Reserve (zukünftig MIDI-RX
auf USART2_RX = PA3 oder PD6 möglich). 8N1 @ 31250 Baud, Standard-HAL-UART,
kein PIO-Workaround.

### 5.6 Amp-Control + Battery-Sense + Status

| Pin | Port | AF | Funktion | Net |
|---|---|---|---|---|
| 25 | PA3 | ADC1_INP15 (analog) | BAT_SENSE via 100k/100k-Teiler | BAT_SENSE |
| 53 | PB14 | GPIO | PAM8403 /SHDN (active LOW) | AMP_nSHDN |
| 54 | PB15 | GPIO | PAM8403 /MUTE (active LOW) | AMP_nMUTE |
| 55 | PD8 | GPIO | Status-LED (boot/heartbeat) | STATUS_LED |

PA3-Hinweis: zusätzliche function `ADC12_INP15` (nicht INP3 wie in r18.0
fälschlich) — Datasheet S. 60 Zeile Pin 25.

PB14/PB15-Hinweis: diese Pins haben als „additional function" auch OTG_HS_DM/DP
(High-Speed-USB mit externem ULPI-PHY). Wir nutzen NUR OTG-FS (PA11/PA12),
keinen externen ULPI-PHY — also sind PB14/PB15 frei als GPIO verwendbar
(Datasheet S. 67-68).

### 5.6a Cell-Velocity-Sense (NEU r18.9, ADR-0006)

| Pin | Port | ADC-Channel (DS12110-bestätigt) | Net |
|---|---|---|---|
| 15 | PC0 | ADC123_INP10 | CELL1_SENSE |
| 16 | PC1 | ADC123_INP11 | CELL2_SENSE |
| 28 | PA4 | ADC12_INP18 | CELL3_SENSE |
| 34 | PB0 | ADC12_INP9 | CELL4_SENSE |
| 35 | PB1 | ADC12_INP5 | CELL5_SENSE |

**ADC-Channel-Hinweis (r18.24 — DS-bestätigt):** Alle 5 Channels liegen auf
**ADC1** (INP5, INP9, INP10, INP11, INP18) → ein einzelner ADC1 kann alle fünf
Cells sequenziell (Scan-Mode) abtasten, kein Multi-ADC nötig. PC0/PC1 sind
zusätzlich auf ADC3 erreichbar (für künftiges Dual-Simultaneous-Sampling).
**Verifikation r18.24:** PA4=ADC12_INP18, PB0=ADC12_INP9, PB1=ADC12_INP5 sind
**verbatim aus DS12110 (STM32H743xI, Additional-Functions-Tabelle)** bestätigt;
PC0=ADC123_INP10 via ST-Community + DS; PC1=ADC123_INP11 folgt der PCx-Sequenz.
Audit-Punkt 4 geschlossen.

Beschaltung pro Cell (**r18.14, ADR-0013** — ersetzt FSR-Teiler aus r18.9):
linearer Hall-Sensor (J_CELLn 1×3-Site: +3V3/OUT/GND; DRV5056A4-Kandidat,
SS49E-Klasse für Prototyp) unter dem Magnet-Stem des Gateron-LP-Magnetic-
Switch. OUT → 1 kΩ Serien-R → Knoten (10 nF → GND, RC fc≈16 kHz) → ADC-Pin.
Stem-Position = analoge Spannung; **Velocity = dPos/dt** beim Durchgang der
Trigger-Zone (Firmware), Aftertouch/Trigger-Punkt später als reine
Firmware-Features möglich. Pins + Netze sind identisch zum FSR-Design.
ADC-INP-Kanal-Nummern werden in Phase 4 beim ADC-Init gegen DS12110 Table 8
(ANA-Spalte) verifiziert. Die Cells hängen damit NICHT mehr am MCP23017 —
GPA0-4 sind frei (Rev-B-Reserve).

**Velocity-Modell (Firmware, ADR-0013 — implementiert r18.15):** Der ADC-Wert
wird auf eine normierte Stem-Position `pos` ∈ [0,1] skaliert (0 = Ruhe, 1 =
Bottom-Out). Die Velocity entsteht aus der **Zeit, in der der Stem das
Velocity-Band durchquert** — exakt wie bei einer echten Hall-Keybed:

| Konstante | Wert | Bedeutung |
|---|---|---|
| `CELL_VEL_BAND_LO` | 0.15 | Start der Velocity-Zeitmessung |
| `CELL_VEL_BAND_HI` | 0.55 | Trigger-Punkt — Note-On feuert hier |
| `CELL_POS_RELEASE` | 0.30 | Note-Off bei Rückzug darunter (Hysterese) |
| `CELL_T_FAST_MS` | 6 ms | Banddurchlauf ≤ → Velocity 1.0 |
| `CELL_T_SLOW_MS` | 70 ms | Banddurchlauf ≥ → Velocity 0.0 |
| `CELL_AMP_MIN/MAX` | 0.05 … 0.22 | Velocity → Voice-Peak-Amplitude |
| `CELL_AMP_GAMMA` | 0.8 | Kurve (<1 spreizt das leise Ende) |

Quelle der Wahrheit: `firmware-c-next/include/cells.h` + `src/cells.c`
(host-getestet, `test/test_cells.c`), in die Engine eingebunden über
`engine_cell_sample(cell, pos, now_ms)`. Dieselben Konstanten sind im
Web-Sim (`tools/display_sim.html`) gespiegelt (sichtbare Velocity-Anzeige).
Aftertouch und ein einstellbarer Trigger-Punkt sind später reine
Firmware-Erweiterungen (analoge Kurve liegt vor).

### 5.7 USB-OTG-FS (built-in, kein externer PHY)

| Pin | Port | AF | Funktion | Net |
|---|---|---|---|---|
| 70 | PA11 | OTG_FS_DM (AF10) | USB Data Minus | USB_DM |
| 71 | PA12 | OTG_FS_DP (AF10) | USB Data Plus | USB_DP |

Externe Beschaltung: USB-C-Buchse → USBLC6-2 ESD-Schutz (bleibt aus v0.6) →
direkt an PA11/PA12. Keine externen Serien-Widerstände nötig (H743 hat
interne USB-PHY mit eingebauten Pull-Ups/Pull-Downs für Speed-Negotiation).

### 5.8 Debug + Boot

| Pin | Port/Signal | Funktion | Net |
|---|---|---|---|
| 72 | PA13 (JTMS/SWDIO) | SWD Debug Data — Standard STM32 Belegung | SWDIO |
| 76 | PA14 (JTCK/SWCLK) | SWD Debug Clock | SWCLK |
| 89 | PB3 (JTDO/TRACESWO) | SWO Trace-Output (optional) | SWO |
| 94 | BOOT0 | dedicated, Pull-Down 10 kΩ → System-Boot-Loader nur via DFU/Reset-Hold | BOOT0 |
| 14 | NRST | dedicated, Pull-Up 10 kΩ + 100 nF Debounce-Cap | NRST |

SWD-Header J4 (3-Pin 1.27 mm, bestehend aus v0.6) wird auf PA13/PA14/GND
umverdrahtet (war bei Pico ein anderer Footprint). SWO auf PB3 ist optional —
PB3 ist in LQFP100 verfügbar (Pin 89) und hat keinen Konflikt mit anderen
Funktionen, da wir TIM2_CH2 auf PA1 statt PB3 mappen.

### 5.9 Clock-Source (HSE)

| Pin | Funktion | Wert |
|---|---|---|
| 12 | PH0-OSC_IN (HSE) | 8 MHz Crystal-Eingang |
| 13 | PH1-OSC_OUT (HSE) | 8 MHz Crystal-Ausgang |

HSE-Crystal: **ABRACON ABLS-8.000MHZ-B4-T** (HC-49/US SMD), 8 MHz, CL 18 pF,
ESR max **80 Ω** (Datasheet Drawing 450669 Rev AD). Load-Caps 2× 22 pF C0G/NP0
0603 als Startwert. PLL-Konfig: PLL_M=4 → 2 MHz PFD → PLL_N=480 → 960 MHz VCO
→ PLL_P=2 → SYSCLK 480 MHz.

**Gain-Margin-Hinweis (AN2867):** Mit ESR_max=80 Ω und C₀+CL=25 pF ist
gm_crit = 4 × 80 × (2π·8e6)² × (25e-12)² = 0.506 mA/V → Gain Margin =
1.5/0.506 = **2.97** im Worst-Case (ESR_max über vollen Temp-Bereich). Das
AN2867-Lehrbuch-Minimum von 5 gilt für Industrial/Automotive über -40…+85 °C;
für dieses Indoor-Audio-Gerät (15–30 °C, ESR_typ ~40–50 Ω) liegt der reale
Margin bei ~5–6. **Bewusst akzeptiert.** In Phase 5 ist der Crystal-Start am
realen PCB zu verifizieren.

**Load-Cap-Feinabstimmung (Phase 5):** 2× 22 pF ergeben mit ~5 pF Stray nur
~16 pF effektiv (< CL=18 pF → Quarz läuft minimal schnell, wenige ppm). Für
exakte 18 pF wären ~24–27 pF nötig. Da die echte Stray-Kapazität erst nach dem
Layout messbar ist: 22 pF als Startwert behalten, in Phase 5 gegen die
gemessene Frequenz nachjustieren (Load-Cap-Wert tauschen, beide C_HSE gleich).

HSI bewusst nicht als Primär-Clock — ±2 % Drift wäre für SAI/I²S-Sync
unbrauchbar (Tonhöhen-Drift hörbar bei Drone-Voices über Minuten).

### 5.10 Power-Supply-Pins (LQFP100)

| Pin | Signal | Anschluss |
|---|---|---|
| 6 | VBAT | mit +3V3 verbunden (kein RTC-Backup-Akku nötig) |
| 10, 26, 49, 74 | VSS (4×) | GND |
| 11, 27, 50, 75, 100 | VDD (5×) | +3V3 |
| 19 | VSSA | analoge GND (mit VSS verbunden, Single-Star-Point empfohlen) |
| 20 | VREF+ | mit VDDA verbunden |
| 21 | VDDA | +3V3 via Ferrit BLM18AG601 + 1 µF + 100 nF |
| 48 | VCAP1 | 2.2 µF X5R 0603 zu VSS (interner SMPS Bulk) |
| 73 | VCAP2 | 2.2 µF X5R 0603 zu VSS (interner SMPS Bulk) |

Decoupling: pro VDD-Pin (5 Stück) ein 4.7 µF X5R 0805 + ein 100 nF X7R 0603
Bulk-Cap, möglichst nah am Pin platziert. Ferrit-Bead BLM18AG601 (LCSC C84094)
in der VDDA-Versorgung zur Trennung Analog-Digital. Power-Sequenz: keine
explizite Reihenfolge nötig — H7 hat keinen VDDA-Sequencing-Constraint
(Datasheet S. 91), und das interne SMPS bringt VCAP1/VCAP2 selbst hoch.

### 5.11 Active-Low-Konvention + Pop-Suppression-Sequenz (unverändert)

PAM8403H /SHDN und /MUTE sind beide ACTIVE LOW. GPIO HIGH = Funktion AUS
(= enabled/un-muted), GPIO LOW = Funktion AKTIV (= shutdown/muted).
Hardware-Pull-Downs R_SHDN_PD und R_MUTE_PD (10 kΩ) ziehen beide LOW
während MCU-Boot → Amp ist default OFF + MUTED.

**Pop-Suppression-Sequenz (Firmware) — KORREKTE Reihenfolge (unverändert):**

- **Power-on**:
  1. Boot: PB14 (/SHDN) und PB15 (/MUTE) LOW (Pull-Downs) + MCP-GPA5 (PCM XSMT) LOW → alles stumm
  2. Warten ~50 ms bis +5V/+3V3 stabil
  3. PB14 (/SHDN) = HIGH → Chip wacht auf, interne Referenzen settlen
  4. ~50 ms später: PB15 (/MUTE) = HIGH + MCP-GPA5 (PCM XSMT) = HIGH → Audio frei
- **Power-off (umgekehrt)**:
  1. MCP-GPA5 (PCM XSMT) = LOW (DAC stumm)
  2. PB15 (/MUTE) = LOW (Amp-Output muted)
  3. ~50 ms später: PB14 (/SHDN) = LOW (Chip aus)
- Resultat: kein „Klick" beim An-/Ausschalten

### 5.12 Belegungs-Übersicht (alle ~30 belegten GPIOs in LQFP100)

| Pin-Bereich | Funktion |
|---|---|
| 3, 4, 5 | Audio I²S (SAI1) |
| 7, 92, 93 | I²C-Bus + Interrupt |
| 22, 23, 67, 68 | Encoder TIM-QEI (4 Pins für 2× Encoder) |
| 59, 60, 63, 64 | Encoder TIM-QEI (4 Pins für 2× Encoder) |
| 2, 97, 98 | Encoder-Switches (3, EN4 auf MCP) |
| 25 | ADC BAT_SENSE |
| 29, 30, 31, 32, 33 | LCD (SPI1 + 3 GPIOs) |
| 53, 54 | PAM8403 Amp-Control |
| 55 | Status-LED |
| 70, 71 | USB-OTG-FS |
| 72, 76 | SWD Debug |
| 86 | MIDI-TX |
| 89 | SWO Trace (optional) |
| 12, 13, 14, 94 | dedicated (HSE, NRST, BOOT0) |
| 6, 10, 11, 19, 20, 21, 26, 27, 48, 49, 50, 73, 74, 75, 100 | Power/GND/VCAP/VBAT/VDDA |

**Frei** (für zukünftige Erweiterungen, ohne PCB-Re-Spin): PE2, PA2, PA10,
PA15, PB2, PB4, PB5, PB8, PB9, PB10-PB13, PC2_C, PC3_C, PC8-PC12,
PD0-PD4, PD6, PD7, PD9-PD11, PD14, PD15, PE7-PE15 — über 35 freie GPIOs.

> r18.20 (Audit-Fix): PC0/PC1/PA4/PB0/PB1 aus der Frei-Liste entfernt — sie
> sind seit r18.9 für CELL1..5_SENSE (Hall-Velocity, §5.6a) vergeben.

---

---

## 6. Display ST7789 Konfiguration (1.9" 170×320 IPS-LCD) — r16

**r16: Panel-Wechsel SSD1322 256×64 OLED → 1.9" ST7789(V2) 320×170 IPS-LCD.**
Grund: Lesbarkeit, Outdoor-Helligkeit, ~3,3× Pixeldichte, glatte UI-Animation,
kein Burn-in. **Visuell bleibt es monochrom** (schwarz/weiß/grau, OP-1-Sprache);
die Firmware rendert das 4-bit-Grey-Framebuffer nur nach RGB565 um (Treiber
`firmware-c-next/src/lcd_st7789.c`). **Pins unverändert** — das LCD nutzt
dieselbe SPI0-Gruppe wie das alte OLED (kein GPIO-Reallocation).

### Controller-Setup (vom Treiber gefahren)

| Schritt | Command | Wert | Zweck |
|---|---|---|---|
| 1 | SWRESET (0x01) | — | Soft-Reset, 150 ms warten |
| 2 | SLPOUT (0x11) | — | Sleep aus, 120 ms warten |
| 3 | COLMOD (0x3A) | 0x55 | 16-bit/Pixel RGB565 |
| 4 | MADCTL (0x36) | 0x60 | Landscape (MX\|MV); ggf. 0xA0/0xC0 falls gespiegelt |
| 5 | INVON (0x21) | — | Display-Inversion (IPS-Panels invertiert) |
| 6 | NORON (0x13) | — | Normal-Modus |
| 7 | DISPON (0x29) | — | Display an |

**GRAM-Offset**: Die 170-px-Seite sitzt versetzt im 240×320-Controller-GRAM.
In Landscape ⇒ **Spalten (CASET) 0..319, Zeilen (RASET) 35..204 (Y-Offset 35)**.

### Pinout 8-pin SPI-Header

| Header Pin | Signal | Verbindung |
|---|---|---|
| 1 | GND | GND |
| 2 | VCC | +3V3 (Logic, 10 µF + 100 nF lokal — C6b/C6c) |
| 3 | SCL (SCLK) | OLED_SCK (Pico GP6) |
| 4 | SDA (MOSI) | OLED_MOSI (Pico GP7) |
| 5 | RES | OLED_RES (Pico GP9) |
| 6 | DC | OLED_DC (Pico GP8) |
| 7 | CS | OLED_CS (Pico GP5) |
| 8 | BLK (Backlight) | PCA9685-PWM-Kanal über N-FET (Low-Side) — Helligkeit per I²C |

**Backlight**: Alle 24 Pico-GPIOs sind belegt (§5), daher fährt BLK über einen
freien **PCA9685-PWM-Kanal** + kleinen N-FET (z. B. 2N7002, SOT-23) als
Low-Side-Switch. Helligkeit damit per I²C.
~~Die Net-Namen `OLED_*` in §5 sind aus Kompatibilität beibehalten~~ —
**r18.5: Schematic-Netnamen sind jetzt `LCD_*`** (gemäß §5.2; die alte
OLED_*-Benennung existiert nur noch in `kicad/legacy_pico2/`).

> **r17-Klarstellung (EN2-Konflikt aufgelöst):** Der **Brightness-Encoder EN2
> regelt im Normalbetrieb die AUDIO-Tonfarbe** (Pad-Filter-Cutoff,
> Sound-Constitution `/fam/brightness`), **nicht** das Display-Backlight. Die
> **LCD-Helligkeit liegt auf der Sekundärfunktion SHIFT+EN2** (§12.3) und wird
> als transientes Overlay angezeigt — wie Volume/Drive, **kein** Menü-Eintrag.
> Frühere Formulierungen, die EN2 direkt mit der Display-Helligkeit
> gleichsetzten, waren falsch und sind hiermit korrigiert.

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
| **GPB5** | **VOL_SW** | **EN4 (Volume) Push-Switch (r15: verlagert von Pico GP21, der für MIDI_TX frei wurde). Mensch-Buttondruck >50 ms, MCP-IRQ-Latenz <5 ms → musikalisch ununterscheidbar von Direktanschluss.** |
| GPB6, GPB7 | reserve | frei für zukünftige Switches/Sensoren |

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

**LED-Kanal-Belegung (r18.8 — IMG_9713-Stand, 5×2 Cell-LEDs XOR-Logik, ADR-0008):**

| PCA9685 LEDn | Funktion | LED-Ref | LED-Farbe | Anzeige-Logik |
|---|---|---|---|---|
| LED0 | Modifier SHIFT | LED6 | **Grün** | An wenn Shift-Latch aktiv (toggle, persistiert) |
| LED1 | Modifier HOLD | LED7 | **Gelb** | An wenn Hold-Modifier-Latch aktiv |
| LED2 | Modifier DRONE | LED8 | **Weiß** | An wenn Drone spielt, sanfter Fade |
| LED3 | Modifier GENERATE | LED9 | **Weiß** | An wenn Generative-Mode aktiv |
| LED4 | Modifier CLEAR | LED10 | **Weiß** | Flash-on-press 200 ms, kein Latch |
| LED5 | Cell-1 Hold @ Basis | LED11Y | **Gelb** | An wenn Cell-1 = HOLD_BASE |
| LED6 | Cell-1 Hold @ Shift | LED11G | **Grün** | An wenn Cell-1 = HOLD_SHIFT (XOR mit LED5) |
| LED7 | Cell-2 Hold @ Basis | LED12Y | **Gelb** | An wenn Cell-2 = HOLD_BASE |
| LED8 | Cell-2 Hold @ Shift | LED12G | **Grün** | An wenn Cell-2 = HOLD_SHIFT (XOR mit LED7) |
| LED9 | Cell-3 Hold @ Basis | LED13Y | **Gelb** | An wenn Cell-3 = HOLD_BASE |
| LED10 | Cell-3 Hold @ Shift | LED13G | **Grün** | An wenn Cell-3 = HOLD_SHIFT (XOR mit LED9) |
| LED11 | Cell-4 Hold @ Basis | LED14Y | **Gelb** | An wenn Cell-4 = HOLD_BASE |
| LED12 | Cell-4 Hold @ Shift | LED14G | **Grün** | An wenn Cell-4 = HOLD_SHIFT (XOR mit LED11) |
| LED13 | Cell-5 Hold @ Basis | LED15Y | **Gelb** | An wenn Cell-5 = HOLD_BASE |
| LED14 | Cell-5 Hold @ Shift | LED15G | **Grün** | An wenn Cell-5 = HOLD_SHIFT (XOR mit LED13) |
| LED15 | **LCD_BLK_PWM** | (via Q2 N-FET) | n/a | PCA-PWM steuert Backlight-Helligkeit |

**Budget: 16/16 PCA-Kanäle belegt, exakt.** Kein Status-LED-Reserve mehr —
System-Status (Heartbeat/Battery-Low) wandert auf MCU-Direkt-GPIO (PD8 per
SPEC §5.6), das ist der STATUS_LED-Pin am STM32H743. Damit ist die r12-Lösung
„System-LED via PCA10" obsolet.

**XOR-Logik (ADR-0008):** Pro Cell ist immer höchstens **eine** LED an —
entweder Gelb (Cell-Hold @ Basis-Oktave) oder Grün (Cell-Hold @ Shift-Oktave).
Niemals beide. Firmware-State-Machine hält pro Cell ein 3-Werte-Enum
{OFF, HOLD_BASE, HOLD_SHIFT} und gibt das an die PCA9685-Treiber.

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

2× **Same Sky (CUI) CMS-402811-28SP** (r18.18-Wechsel von PUI weg —
Stoff-Membran statt behandeltem Papier; identischer Footprint, halber Preis):
**40 × 28.3 × 11.5 mm** rechteckiger Rahmen, **8 Ω**, 2 W RMS / 3 W max,
**Stoff-Konus (Cloth Cone)**, NdFeB-Neodym-Magnet, **F0 = 450 Hz**,
84 ± 3 dB @ 1 W/50 cm, **Löt-Eyelets — keine Kabel ab Werk** → Hand-Assembly,
kein JLC-Bestücken. DigiKey-Stock-OK + Arrow + Mouser.
Quelle: [Same Sky CMS-402811-28SP Datenblatt](https://www.sameskydevices.com/product/resource/cms-402811-28sp.pdf).

**Zweitquelle (dokumentiert):** PUI Audio **AS04008PS-4W-WR-R** — identischer
40 × 28.3 × 11.5-Footprint, 8 Ω, 2 W, Löt-Eyelets, F0 = 380 Hz (etwas
mehr Tiefgang), aber **behandeltes Papier statt Stoff** → akustisch boxiger
in den unteren Mitten + feuchteanfälliger. Stoff-Variante CMS bevorzugt.

**Warum der Wechsel (r18.18):** Drei Datenblatt-Quellen bestätigen: PUI-Konus
ist „treated paper" — eine Papier-Pulpe-Membran mit Wasserschutz-Beschichtung.
Cloth-Cone ist eine spürbar bessere Membran-Klasse: niedrigere innere
Verluste, glattere Mitten ohne „Papier-Boxigkeit", besser feucht-/
schweißbeständig. Trade-Off ist 70 Hz höhere F0 — in einer 15–30 cm³ Sealed-
Box mit F-Rolloff ohnehin bei ~500 Hz nicht hörbar. Cloth-Mitten-Klarheit
gewinnt deutlich. Bonus: ~$3–5 statt ~$6.78 pro Treiber.

**Dust-Mesh-Cover (ADR-0007):** Saati Acoustex 020–032 (transparent, ~25–32
g/m²), PSA-Klebering-Konvertierung via Marian Inc. für Serie; AliExpress-
Klebe-Mesh für Prototyp. Ovaler Frontpanel-Cutout 36 × 24 mm (passt zum
40×28.3-Treiber, 2 mm Rim-Seat).

**Mechanik-Hinweis (11.5 mm Tiefe):** Gehäuse-Außenhöhe 21.6 mm (war 19.6) —
der von der Top-Platte hängende Treiber braucht 12 mm Above-PCB-Raum + 42×32-
Bauteil-Keepout je Treiber-Footprint. Details `../mechanical/coordinates/mechanical_coordinates.md` §2/§7.
**Mechanik unverändert beim Wechsel** — der CMS-Treiber hat identische
Außenmaße + Tiefe + Eyelet-Position.

**Akustik-Konzept: Sealed Box + Top-Firing (kein Bass-Reflex, kein Passivradiator)**

| Element | Wert | Begründung |
|---|---|---|
| Kammer | **Geschlossen pro Kanal**, Trennsteg L/R im Bottom-Case-Inlay | Einzige sinnvolle Kammerform für einen Treiber mit F0=450 Hz (CMS, r18.18; auch bei der 380-Hz-Zweitquelle PUI gültig). Reflex-Systeme (Port oder PR) lassen sich physikalisch nicht unter F0 abstimmen — eine PR mit Fb≈400 Hz würde nur eine Resonanzspitze in den unteren Mitten machen, schlimmster Fehlerfall für Drone/Sustain-Audio (One-Note-Boom). Sealed = saubere monotonische Roll-Off, kein Dröhnen. |
| Treiber-Ausrichtung | **Top-Firing in der Top-Plate**, nicht down-firing | Down-firing nutzt nur Boundary-Coupled-Bass — den dieser Treiber nicht erzeugt (F0=450 Hz). Top-firing maximiert die *einzige* echte Stärke (Mitten/Höhen-Klarheit) durch direkten Schallweg zum Ohr, ohne Tisch-Reflexion und Kammfilter. |
| Mount | Speaker-Rahmen von unten gegen die Top-Plate, 4× M2 | PCB-Speaker-Cutouts (alt: 41 mm dia bei Y=30) **entfallen** — Treiber sitzen nicht mehr im PCB. Akustik-Kammer wird durch Top-Plate + Bottom-Case + Trennsteg gebildet. |
| Top-Plate-Grille | 2× **ovale Dust-Mesh-Aussparung 50 × 30 mm** bei **(28, 50)** und **(224, 50)** (r18.16-Mechanik-Koordinaten), schwarzes Akustik-Mesh statt Lochmuster (ADR-0007) | Schallaustritt direkt nach oben. Cutout-Höhe < Treiber-Außenmaß (40 mm) damit die Membran am Rand abgedeckt bleibt; Mesh schließt die Öffnung staubdicht + akustisch transparent. |

**Realistische akustische Erwartung** (CMS-Cloth-Konus, r18.18):

- Onboard ehrlich nutzbar etwa **250 Hz – 20 kHz** (F0=450 Hz, nutzbar etwa
  −10 dB unter F0; war ~200 Hz mit der PUI-Papier-Zweitquelle bei F0=380 Hz).
- Was klingt: **glattere Mitten** als beim Papier-Treiber (kein
  Papier-Boxig-Klang im Sprach-Bereich), präsente Höhen, Pad-Saws fett und
  transparent, Reverb-Fahnen sauber.
- Was *nicht* klingt: alles unter ~250 Hz. famSubBass (LP90) und der untere
  Teil von famDeepBass (HP50/LP350) sind **onboard schlicht nicht hörbar** —
  sie sind direkter Beitrag null. Indirekt sind sie über den Reverb-Send
  (`verbSend 0.03` bzw. `0.08`) als Wärme der Fahne präsent, aber das ist
  alles. Voller Tiefgang ausschließlich über **Line-Out J8** → externe Boxen.

Ein sanfter DSP-Low-Shelf bei ~450 Hz kann später optional die wahrgenommene
Wärme im Treiber-Eigenbereich anheben (Firmware-seitig, Engine-Step 11/Master
oder Engine-Step 8/Bass). Echten Bass *erzeugt* DSP nicht — der Treiber hat
schlicht keinen Hub unter ~250 Hz.

**Mechanik-Konsequenzen** (siehe `../mechanical/coordinates/mechanical_coordinates.md`
§3.4/§5/§7, r18.16-Stand):
- PCB-Speaker-Cutouts entfallen (mehr nutzbare PCB-Fläche im unteren Streifen)
- Top-Plate bekommt 2× ovale Dust-Mesh-Aussparung 50 × 30 mm bei (28, 50) /
  (224, 50) (r18.16-Koordinaten, neuer 252×102-Outline — die alten
  (50,30)/(270,30) galten für den 333×143-Outline und sind retired)
- PCB-seitige Speaker-Treiber-Zone hat Höhen-Limit 4 mm (Treiber hängt 5 mm
  vom Top-Panel; siehe mechanical §7) — L1-Boost-Inductor + JST-PH dort verboten
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

### MIDI Out (J9, r15 — 2026-06)

3.5-mm-TRS-Klinke, **MIDI 1.0 / MMA TRS Type A** (offizieller Standard seit
2018). Keine USB-MIDI, kein TinyUSB. User-Use-Case: das Gerät als
„denkender Controller" — du spielst 5 Cells, der Harmonic Brain übersetzt
sie in echte Akkord-Noten, und ein externer Synth/DAW empfängt die ferige
Akkord-Progression auf MIDI.

| Element | Wert |
|---|---|
| J9 | 3.5mm TRS-Buchse, **selber MPN wie J8** (PJ-320A / LCSC C431535) — Mechanik & Sourcing wiederverwenden |
| Treiber | **Pico GP21 → PIO-UART TX** @ 31250 Baud, 8N1 |
| R_MIDI_TX | **220 Ω 0603** zwischen GP21 und TRS Tip (Daten / „cold side") |
| R_MIDI_REF | **220 Ω 0603** zwischen +3V3 und TRS Ring (Strom-Referenz / „hot side") |
| Schirm | TRS Sleeve → GND |

**Pegel**: 3,3 V direkt vom Pico-GPIO. MIDI-Spec-Update **CA-033 (2020)
erlaubt 3.3-V-Treiber** explizit → kein Level-Shifter, kein 5-V-Buffer
nötig. Industrie-Status quo: Korg, Make Noise, Novation, Roland (neuere
Modelle) machen das genauso.

**Galvanische Isolation**: nicht nötig am OUT (nur am IN). Die 220-Ω-
Widerstände begrenzen den Kurzschlussstrom und definieren die Source-
Impedanz nach Spec.

**Keine MIDI-IN-Buchse** (bewusste Entscheidung, siehe CHANGELOG r15):
das Gerät ist um die 5 Cells als primäres Interface gebaut; eine 1:1-MIDI-
Notenzuordnung („Note 60 = Cell 1?") würde den Harmonic Brain aushebeln.
MIDI-Clock-Sync wäre die einzige technisch sinnvolle Anwendung, aber das
Generate-Bed ist absichtlich langsam-ambient → tight Clock-Lock wäre
musikalisch falsch.

**GPIO-Reallocation für r15** (siehe §5 + §7):
- VOL_SW wandert vom Pico (GP21) auf den MCP23017 (GPB5)
- Encoder-Drehen bleibt auf GP19/GP20 (zeitkritisch, low-latency-IRQ)
- Encoder-Drücken über MCP — Mensch-Buttondruck (>50 ms), MCP-Latenz
  <5 ms → musikalisch ununterscheidbar

**Firmware** (kommt mit Step 12b): PIO-State-Machine auf PIO1 oder PIO2
(PIO0 hat I²S), 31250 Baud → 16 Bits/Baud-Takt × 31250 ≈ trivial,
keine Belastung. Ring-Buffer Note-On/Off-Builder mit Channel + Velocity-
Mapping.

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
| Brightness (EN2, **Audio**-Ton) | Last-Saved | ✓ | Pad-Tonfarbe (`/fam/brightness`) ist gewollt persistent |
| **LCD-Backlight** (SHIFT+EN2) | **Werks-Default (80 %)** | ✗ | **Anti-Lockout**: Boot nie auf einen dunklen Wert, sonst sieht der User den Screen nicht und kommt nicht mehr raus. Wird NICHT aus dem Snapshot wiederhergestellt. |
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
| HOLD aktiv + GENERATE | **Koexistenz, aber HELD-Voices sind geschützt** (r17): GENERATE läuft weiter und legt neue Layer drauf, darf aber **keine gehaltenen Drone-Voices stehlen** — er allokiert nur aus dem freien Voice-Pool. So gehen keine Klangebenen verloren und nichts wird überschrieben. (Ersetzt die alte „GENERATE wird ignoriert"-Regel, die den Knopf tot wirken ließ.) Reichen die freien Voices nicht, wird die ÄLTESTE *nicht-gehaltene* Generator-Voice recycelt. |
| GENERATE aktiv + Cell-Press | Cell wird zum Generator-Seed (Tonhöhe/Timbre-Quelle für nächsten Generator-Voice) |
| SHIFT (gehalten) + EN1-Drehung | EN1 → Secondary (z.B. Filter-Cutoff statt Drive) — Per-Encoder-Mapping in Firmware |
| SHIFT (gehalten) + EN2-Drehung | **EN2 → LCD-Backlight** (statt Audio-Brightness). Overlay „Backlight", 0–100 %. Darf bis 0 % (kein Min-Floor) — recoverable, weil der Encoder physisch ist und auch bei dunklem Screen zurückgedreht werden kann. |
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

### 12.7 Encoder-Feel & Display-Menü-Verhalten (NEU r17, 2026-06-07)

Festgelegt nach UX-Review der Display-Simulation (`firmware-c-next/tools/
display_sim.html`).

**Velocity-Acceleration (alle 4 Encoder, Wert-Edits):** Die Schrittweite hängt
vom Drehtempo ab. Ein bewusster, langsamer Detent = **1 %** (feine, exakt
reproduzierbare Kontrolle — man kann auf einen genauen Wert landen). Schnelles
Drehen vergrößert die Ticks pro Detent, sodass 0→100 ein kurzer Flick ist.
Referenz-Kurve (ms seit letztem Detent → Ticks): ≤28→8, ≤60→5, ≤120→3, ≤240→2,
sonst 1. Architektur-Vertrag: Der **Input-Layer** (Encoder-ISR) berechnet den
Tick-Count, der **Menü-Layer** (`menu.c`) ist granularitäts-agnostisch und
addiert nur den Tick-Count. **Browse-Navigation und diskrete Werte
(Key/Mode/Vibe/Voice) beschleunigen NICHT** — sie steppen pro Geste um genau
einen Slot/Schritt (Vorzeichen), damit ein schneller Spin keine Menü-Einträge
überspringt (es gibt nur 8).

**Kein Edit-Auto-Exit:** Der Display-Encoder-Push wechselt Browse ⇄ Edit. Es
gibt **bewusst kein** Timeout, das selbsttätig aus dem Edit-Modus springt — der
User kontrolliert den Zustandswechsel allein. Ein automatischer Wechsel ohne
User-Aktion wäre verwirrend (Zustand ändert sich „von selbst").

**Overlay-Vorrang:** Jede Display-Encoder-Aktion (Drehen oder Push) räumt ein
offenes transientes Overlay (Drive/Brightness/Volume/Backlight) sofort weg —
eine konsistente Regel für alle Overlays, kein Stapeln.

**Menü-Inventar (8 Pills):** Key, Mode, Vibe, Voice, Texture, Bass, Space, Mood.
Das Menü enthält **nur einstellbare Wertebereiche ohne eigenen Hardware-Regler**.
Dedizierte Controls (Volume/Drive/Brightness-Encoder; Drone/Generate/Hold-
Buttons; LCD-Backlight via SHIFT+EN2) sind **kein** Menü-Eintrag, damit Software
und Hardware nie um denselben Wert konkurrieren.

**Idle-Backlight-Dimm (kein Display-Timeout):** Statt das Display ganz
abzuschalten, dimmt das Backlight nach **30 Min ohne Encoder-/Button-Event sanft
auf 20 %** (Akku-Schonung, aber jederzeit sofort ablesbar). Erste Interaktion
fährt sofort auf den eingestellten Wert zurück. Beim Reboot zählt der
Werks-Default (§12.5), nicht der gedimmte Idle-Wert.

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
