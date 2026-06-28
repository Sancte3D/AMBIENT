# BOM Master — Field Ambience (detailed / working view, DE)

> 🏭 **Production handoff → [`field-ambience-current/PCB_BOM.md`](field-ambience-current/PCB_BOM.md)**
> (English, **on-PCB parts only**, off-board parts excluded, no duplicates,
> grouped by function). *This* file is the detailed working/rationale view
> (sourcing history, off-board parts, audit-trail) — for Claude + deep dives.

**Stand: v0.7-r18.65 (2026-06-27).** Single Source of Truth für alle aktiv
verbauten Komponenten. Pro Eintrag: aktuelle Wahl, Footprint-Quelle, 3D-Quelle,
Bestell-Link. **Footprint- und STEP-Spalten sind ab r18.36 klickbar** und
zeigen auf die jeweilige Datei im Repo (relative Markdown-Links, funktionieren
auf GitHub und in jedem Markdown-Editor).

> Generator (`field-ambience-current/kicad/generate_kicad_project.py`) ist die
> technische Quelle (LCSC, MPN, Footprint im Schematic). Diese Datei ist die
> Lese-/Sourcing-Sicht. Wenn ein Eintrag hier abweicht, gilt der Generator.

## Legende

- **FP-Quelle:** `KiCad-Standard` (im offiziellen kicad-footprints repo),
  `field_ambience` (vendored unter `kicad/libraries/field_ambience.pretty/`),
  oder `Modul` (Steckmodul, kein PCB-Footprint außer Header).
- **3D-Quelle:** `STEP im Repo` (`kicad/libraries/field_ambience.3dshapes/`),
  `easyeda2kicad` (regenerierbar via `pip install easyeda2kicad; easyeda2kicad
  --full --lcsc_id=Cxxxxxx`), `Vendor-CAD` (extern besorgen — kein STEP im Repo),
  `Standard-Lib-3D` (kommt mit KiCad-Footprint).

---

## 1. MCU + Clock

| Ref | MPN | LCSC/Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| U1 | **STM32H743VIT6** LQFP-100 | [C114409](https://www.lcsc.com/product-detail/C114409.html) | `Package_QFP:LQFP-100_14x14mm_P0.5mm` | KiCad-Standard | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/LQFP-100_L14.0-W14.0-H1.4-LS16.0-P0.50.step) |
| Y1 | **ABLS-8.000MHZ-B4-T** HC-49/US-SMD | [C596838](https://www.lcsc.com/product-detail/C596838.html) | [`field_ambience:Crystal_HC49-US-SMD_ABLS`](field-ambience-current/kicad/libraries/field_ambience.pretty/Crystal_HC49-US-SMD_ABLS.kicad_mod) | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/CRYSTAL-SMD_L11.4-W4.7-LS12.7.step) |

## 2. Power-Stack

| Ref | MPN | LCSC/Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **J1** USB-C | **TYPE-C-31-M-12** (Korean Hroparts) — 16-Pin volle USB-C-Belegung (VBUS/GND/CC1/CC2/D+/D−/SBU). r18.19-REVERT: r18.10 hatte fälschlich auf M-17 „upgraded", M-17 ist aber 6-Pin power-only → USB-DFU wäre kaputt gewesen. | [C165948](https://www.lcsc.com/product-detail/C165948.html) | `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12` | KiCad-Standard | Standard-Lib-3D (r18.36-Fix: vorher fälschlich „STEP im Repo" — kein Custom-STEP existiert) |
| D1 | **USBLC6-2SC6** ESD-Schutz | [C2687116](https://www.lcsc.com/product-detail/C2687116.html) | `Package_TO_SOT_SMD:SOT-23-6` | KiCad-Standard | Standard-Lib-3D |
| F1 | **1812L300/16GR** PTC 3 A | [C18198349](https://www.lcsc.com/product-detail/C18198349.html) | `Fuse:Fuse_1812_4532Metric` | KiCad-Standard | Standard-Lib-3D |
| D2 | **SMAJ5.0A** TVS | [C113952](https://www.lcsc.com/product-detail/C113952.html) | `Diode_SMD:D_SMA` | KiCad-Standard | Standard-Lib-3D |
| **U8** TPS61089 Boost VQFN-HR | [C165129](https://www.lcsc.com/product-detail/C165129.html) | [`field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A`](field-ambience-current/kicad/libraries/field_ambience.pretty/Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A.kicad_mod) | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/VQFN-HR_L2.5-W2.0-H1.0-P0.50.step) |
| L1 Boost-Inductor | **SWPA6045S2R2MT** 2.2 µH | [C83455](https://www.lcsc.com/product-detail/C83455.html) | [`field_ambience:L_Sunlord_SWPA6045`](field-ambience-current/kicad/libraries/field_ambience.pretty/L_Sunlord_SWPA6045.kicad_mod) (EasyEDA-CAD vendored r18.20c — Phantom-Name L_0630 ersetzt) | field_ambience (Custom) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/IND-SMD_L6.0-W6.0-H4.5.step) |
| D3 | **SS34** Schottky-Diode | [C8678](https://www.lcsc.com/product-detail/C8678.html) | `Diode_SMD:D_SMA` | KiCad-Standard | Standard-Lib-3D |
| Q1 | **DMG2305UX-7** P-Ch MOSFET | [C150470](https://www.lcsc.com/product-detail/C150470.html) | `Package_TO_SOT_SMD:SOT-23` | KiCad-Standard | Standard-Lib-3D |
| **U5** LDO | **AP7361C-33Y5-13** SOT-89-5 (Pinout 1=EN, 2=GND, 3=ADJ, 4=IN, 5=OUT verifiziert) | [C460397](https://www.lcsc.com/product-detail/C460397.html) | `Package_TO_SOT_SMD:SOT-89-5` | KiCad-Standard | Standard-Lib-3D |
| **U_PWR** Power-Aus Load-Switch (ADR-0016) | **TPS22918DBVR** SOT-23-6 — gated den LDO-Eingang `+5V_RAIL → +5V_SW` (= gesamte 3V3-Domäne: MCU + Halls + LEDs + LCD). ON kommt von `SW_PWR`. Lader U7 sitzt **davor** → lädt im Aus weiter. „Dunkel, aber lädt." | [C68913](https://www.lcsc.com/product-detail/C68913.html) *(PN/Stock vor Order prüfen)* | `Package_TO_SOT_SMD:SOT-23-6` | KiCad-Standard | Standard-Lib-3D |
| **SW_PWR** Schiebeschalter = Haupt-Aus | **MST-12D18G3** — Right-Angle SMD Slide-Switch (SPDT, **seitlich bedienbar** vom Gehäuse-Rand; ~3,5×6,5×9,1 mm). Schaltet **nur** `U_PWR.ON` (signal-level, µA) | [C49023766](https://jlcpcb.com/partdetail/C49023766) (JLC Extended) | Right-Angle SMD Slide-Switch | KiCad/Vendor | Vendor-CAD |
| **R_PWR_PD** 100 kΩ 0603 (`U_PWR.ON` Pull-Down → Default AUS) | 0603WAF1003T5E | [C25803](https://www.lcsc.com/product-detail/C25803.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **C_PWR_SW** 10 µF X5R 0805 (Output-Cap auf `+5V_SW`) | LMK212ABJ106-Klasse | [C15850](https://www.lcsc.com/product-detail/C15850.html) | `Capacitor_SMD:C_0805_2012Metric` | KiCad-Standard | Standard-Lib-3D |
| **U7** Charger | **MCP73831T-2ACI/OT** SOT-23-5 | [C424093](https://www.lcsc.com/product-detail/C424093.html) | `Package_TO_SOT_SMD:SOT-23-5` | KiCad-Standard | Standard-Lib-3D |
| **C_BULK** Polymer-Tantal 470 µF / 10 V Case-E 7343-43 (ESR 100 mΩ; <25 mΩ-Flachteile bei LCSC nicht lagernd — Transient-ESR liefert C_BULK2) | TPSE477K010R0100 (Kyocera AVX) | [C444831](https://www.lcsc.com/product-detail/C444831.html) | `Capacitor_SMD:CP_Tantalum_Case-E_EIA-7343-43_Reflow` | KiCad-Standard | Standard-Lib-3D |
| **C_BULK2** MLCC **100 µF / 10 V** 1210 (parallel; 220µF/10V/1210 existiert nicht → 100µF echter Headroom statt 220µF/6.3V-Derating) | LMK325ABJ107MM-T (Taiyo Yuden) | [C2880380](https://www.lcsc.com/product-detail/C2880380.html) | `Capacitor_SMD:C_1210_3225Metric` | KiCad-Standard | Standard-Lib-3D |
| **J9** JST-PH 2.0 2-Pin (LiPo-Akku — Generator-Refdes, **nicht** J_BAT) | **S2B-PH-SM4-TB(LF)(SN)** | [C295747](https://www.lcsc.com/product-detail/C295747.html) | `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal` | KiCad-Standard | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/CONN-SMD_P2.00_S2B-PH-SM4-TB-LF-SN.step) |
| LiPo-Pouch **503759** (50×37×9.4 mm, **2000 mAh ~6.6 h @ 300 mA**) — r18.21 rightsize von 5000mAh-Overkill | Generic LiPo 2000mAh JST-PH 2.0 | [PiHut 2000mAh](https://thepihut.com/products/2000mah-3-7v-lipo-battery) | — (kein PCB-FP, Bottom-Case-Slot) | — | Vendor |

> **Power-Architektur / Rail-Ownership (ADR-0016, Audit #8/#9).** Damit der
> PCB-Macher die Quellen eindeutig sieht — *eine* Quelle→System-Rail-Stelle:
> - `VBUS_USB` (USB-C 5 V, via F1 PTC) ──┐
> - `BAT_PLUS` (LiPo) → Boost **U8** → `+5V_BOOST` ──┤→ Power-Path **Q1** → **`+5V_RAIL`** (System-5 V; speist Amp U4 + U_PWR)
> - **`+5V_SW`** = `+5V_RAIL` hinter **U_PWR** (geschaltet von `SW_PWR`) → LDO **U5** → **`+3V3`** (MCU, MCP, beide PCA9685, LCD, Halls)
> - Lader **U7** hängt an `VBUS_USB`/`BAT_PLUS` **vor** U_PWR → lädt unabhängig vom Aus-Zustand.
>
> Die Generator-Netze heißen heute teils noch `+5V_OUT`/`+5V_RAIL` gemischt;
> da der PCB-Macher das Schaltbild ohnehin neu zieht, ist **diese
> Ownership-Sicht maßgeblich** (Quelle → genau eine System-Rail).

## 3. Audio-Path

| Ref | MPN | LCSC/Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **U3** DAC | **PCM5102APWR** TSSOP-20 | [C107671](https://www.lcsc.com/product-detail/C107671.html) | `Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm` | KiCad-Standard | Standard-Lib-3D |
| **U4** Class-D Amp | **PAM8403DR-H** SOIC-16 | [C17337](https://www.lcsc.com/product-detail/C17337.html) | `Package_SO:SOIC-16_3.9x9.9mm_P1.27mm` | KiCad-Standard | Standard-Lib-3D |
| FB1/FB2 Ferrite Bead | **BLM18AG601SN1D** 0603 600 Ω | [C19330](https://www.lcsc.com/product-detail/C19330.html) (FB1) / [C84094](https://www.lcsc.com/product-detail/C84094.html) (FB2) | `Inductor_SMD:L_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **J8** Line-Out 3.5 mm TRS | **PJ-320D** (SHOU HAN, SMT) mit Insertion-Detect | [C431535](https://www.lcsc.com/product-detail/C431535.html) | [`field_ambience:Jack_3.5mm_PJ-320D_SMT`](field-ambience-current/kicad/libraries/field_ambience.pretty/Jack_3.5mm_PJ-320D_SMT.kicad_mod) (EasyEDA-CAD vendored r18.19) | field_ambience (Custom) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/AUDIO-SMD_PJ-320D-1.step) |
| **J10** MIDI-Out 3,5-mm-TRS (r18.67) | **PJ-320D** (selber MPN wie J8) — TRS **Type A**, OUT-only, 3,3 V/CA-033 (kein Level-Shifter). MIDI_TX=PD5→R_MIDI_TX 220 Ω→Tip; +3V3→R_MIDI_REF 220 Ω→Ring; Sleeve→GND. ⚠️ **J9 = Akku, J10 = MIDI** | [C431535](https://www.lcsc.com/product-detail/C431535.html) | s. J8 (`field_ambience:Jack_3.5mm_PJ-320D_SMT`) | field_ambience (Custom) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/AUDIO-SMD_PJ-320D-1.step) |
| **R_MIDI_TX / R_MIDI_REF** 220 Ω 0603 (2×, MIDI Type-A) | 0603WAF2200T5E | **NEEDS-VERIFY** — 220-Ω-LCSC vor Order fixieren (Anti-Guess, NO-LCSC-Liste) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |

### Speaker (r18.18 — Cloth-Cone primär)

| Ref | MPN | Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **J6 / J7** 1×2 2,54 mm Header (Speaker-Litzen) | B-2100S02P-A110 (1×2 2,54 mm Male) | [C124375](https://www.lcsc.com/product-detail/C124375.html) | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | KiCad-Standard | Standard-Lib-3D |
| **SPK-L / SPK-R** Treiber **primär** | **Same Sky CMS-402811-28SP** (Cloth-Cone, 40 × 28.3 × 11.5 mm, 8 Ω, 2 W, F0 = 450 Hz, NdFeB, Löt-Eyelets) | [DigiKey 102-CMS-402811-28SP-ND](https://www.digikey.com/en/products/detail/cui-devices/CMS-402811-28SP/10821307) · [Vendor-Page](https://www.sameskydevices.com/product/audio/speakers/miniature-(10-mm~40-mm)/cms-402811-28sp) | — (kein PCB-Mount, hängt von Top-Panel) | — | [Vendor-CAD](https://www.sameskydevices.com/product/resource/cms-402811-28sp.pdf) |
| **SPK** Treiber **sekundär** (Backup) | **PUI AS04008PS-4W-WR-R** (Treated-Paper, gleicher Footprint, F0 = 380 Hz) | [DigiKey](https://www.digikey.com/en/products/detail/pui-audio-inc/AS04008PS-4W-WR-R/1464855) | — | — | [Vendor-CAD](https://puiaudio.com/file/specs-AS04008PS-4W-WR-R.pdf) |

### Speaker-Cover (Dust-Mesh)

| Element | Wahl | Link | Quelle |
|---|---|---|---|
| Mesh **Serie** | Saati **Acoustex 020–032** (transparente Klasse, ~25–32 g/m²) + PSA-Konvertierung via Marian Inc. | [Saati Acoustex](https://www.saati.com/products/filtration/brands/saatifil-acoustex/) · [Marian (Konverter)](https://marianinc.com/materials/filters-venting-fabric/) | Vendor-Direktkauf, Custom-Quote |
| Mesh **Prototyp** | Generic selbstklebende „Phone-Speaker-Dustproof-Mesh"-Sticker, 36 × 24 mm ovale ausstanzen | AliExpress / Amazon (Multipack ~5 €) | Generic |

## 4. I/O Expander + LED Driver

| Ref | MPN | LCSC/Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **U2** GPIO-Expander | **MCP23017-E/SS** SSOP-28 | [C506653](https://www.lcsc.com/product-detail/C506653.html) | `Package_SO:SSOP-28_5.3x10.2mm_P0.65mm` | KiCad-Standard | Standard-Lib-3D |
| **U6** PWM-LED-Driver | **PCA9685PW,118** TSSOP-28 (16 Kanäle: 5 Modifier + 10 Cell + 1 Backlight, exakt 16/16) | [C2678753](https://www.lcsc.com/product-detail/C2678753.html) | `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm` | KiCad-Standard | Standard-Lib-3D |
| **U10** PWM-LED-Driver #2 | **PCA9685PW,118** TSSOP-28 (r18.66 — Level-Meter: 8 VU-LEDs auf Ch 0-7. I²C **0x41** (A0=+3V3), gleicher Bus wie U6/MCP. Decoupling 10µF+100nF, /OE-Pull-up R_OE2 10k) | [C2678753](https://www.lcsc.com/product-detail/C2678753.html) | `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm` | KiCad-Standard | Standard-Lib-3D |

## 5. Display

| Ref | MPN | Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **J3** 1×8 Header 2,54 mm (LCD-Modul) | 2,54-mm-Strip auf 1×8 abgelängt (B-2100S40P o. ä.) — oder 1×8 Female-Socket für steckbares Modul | [C124383](https://www.lcsc.com/product-detail/C124383.html) | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | KiCad-Standard | Standard-Lib-3D |
| **LCD-Modul** **Waveshare 1.9in 170×320 ST7789V2** IPS-Modul (r18.22-Pivot von bare-AliExpress: gleiches Panel wie Adafruit + branded QC + Level-Shifter + dokumentiert; ~$11–13 statt Adafruit $15 / bare-AliExpress-DOA-Lotterie. **Pin-ORDER des Moduls gegen J3-Belegung verifizieren** — variiert je Vendor) | Waveshare 1.9inch LCD Module 170x320 SPI | [PiHut £11.60](https://thepihut.com/products/1-9-ips-lcd-display-module-170x320-spi) · [Waveshare](https://www.waveshare.com/1.9inch-lcd-module.htm) · [Amazon](https://www.amazon.com/Waveshare-1-9inch-Display-Resolution-Interface/dp/B0BRXXSZC5) | Modul (steckt in J3) | — | Vendor-CAD |
| **LCD-Modul — PROPOSED r18.43 / ADR-0015** **Waveshare 2.0in 240×320 ST7789** IPS-Modul. Pivot 1.9″ → 2.0″ für mehr Fläche + Animations-Headroom + native RGB565-Farbe. Gleiche ST7789-Controller-Familie, gleiches 8-Pin-SPI-Interface, gleicher Backlight-Pfad (Q2/PCA9685-Kanal 12). **UNVERIFIED — NEEDS HUMAN CHECK:** (a) Beschaffungspfad/SKU (LCSC-PN existiert evtl. nicht; Beschaffung via Waveshare/PiHut/Amazon wie 1.9″), (b) 8-Pin-Header-Reihenfolge gegen geliefertes Modul, (c) Außenmaße + Active-Area-Position gegen Bezel-Fenster. Bezel-Ausschnitt + Generator-Sheet erst ändern wenn (a)–(c) bestätigt. | Waveshare 2inch LCD Module 240×320 SPI | [Waveshare](https://www.waveshare.com/2inch-lcd-module.htm) | Modul (steckt in J3, ggf. Pin-Order-Anpassung) | — | Vendor-CAD |
| Q2 Backlight-Driver | **2N7002,215** SOT-23 | [C8545](https://www.lcsc.com/product-detail/C8545.html) | `Package_TO_SOT_SMD:SOT-23` | KiCad-Standard | Standard-Lib-3D |

## 6. Encoder (ADR-0012 — **4× Push-Encoder**, alle gleich hoch)

> **r18.65:** Alle 4 sind dasselbe Push-Teil **EC11E18244AU** und alle 4
> Push-Switches sind verdrahtet (DISPLAY=PE3, VOL=MCP-GPB5, DRIVE=PE0,
> BRIGHT=PE1) → jeder Encoder kann eine Push-Funktion tragen. Detent-Feel:
> Display mit Rastung, Parameter-Encoder via Firmware-Acceleration „smooth".

| Ref | MPN | LCSC/Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **EN3** (Display: Push + Detent) | **ALPS EC11E18244AU** (18 Puls, 36 Detents, mit Push-Switch, Flat-Shaft 20 mm) | [C202365](https://www.lcsc.com/product-detail/C202365.html) | `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` | KiCad-Standard | Standard-Lib-3D |
| **EN1 / EN2 / EN4** (r18.22-Pivot: einheitlich auf den ACTIVE Display-Encoder, weil sowohl EC11E183440C als auch der Kandidat EC11E1834403 NRND sind und die gesamte ALPS-„0-Detent+Push"-Familie phased-out ist) | **ALPS EC11E18244AU** (= Display-Encoder, 36 Detents/U + Push; Firmware-Acceleration macht langsam = 1 %/Klick, schnell = ×8/Klick — UX-funktional identisch zum Smooth) | [C202365](https://www.lcsc.com/product-detail/C202365.html) (~3.052 Stock) | gleicher FP wie EN3 | KiCad-Standard | Standard-Lib-3D |
| Knöpfe (4×) Ø 19–20 × 8–10 mm, **selbst 3D-gedruckt** (r18.21 — statt Alu-CNC, spart $50-200/5er-Run) | Eigenes 3D-Print | — | — | — | Custom-CAD (User) |

## 7. Cells (ADR-0013 — Low-Profile Magnetic + Hall)

| Ref | MPN | Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **Switch (5×)** | **Gateron Low Profile Magnetic Jade** (pin-los, plate-mounted, 0.1–3.3 mm analog Hub, 100 M Zyklen) | [Gateron Direct](https://www.gateron.com/products/gateron-low-profile-magnetic-jade-switch) · [NuPhy](https://nuphy.com/products/gateron-low-profile-magnetic-jade-switches) · [Ukeebs (EU)](https://ukeebs.com/products/gateron-low-profile-magnetic-jade-switch-set) | — (kein PCB-Pin, Plate-Cutout 14×14 mm) | — | Community-CAD (Keyboard-Ökosystem) |
| ~~Stabilizer~~ **(r18.21 gestrichen für Prototyp)** | Nur nötig bei langen ≥2u-Caps. Cell-Caps selbst 3D-gedruckt + **kurz (1u)** → kein Stabilizer. Spart $25-75/5er-Run + vereinfacht Plate | — | — | — |
| **Hall-Sensor pro Cell (J_CELL1–5)** | **TI DRV5056A4QDBZR** (SOT-23, 3.3 V ratiometrisch, Pin 1=VCC / 2=OUT / 3=GND DS-verifiziert) | [C2152902](https://www.lcsc.com/product-detail/C2152902.html) · [TI DS](https://www.ti.com/lit/ds/symlink/drv5056.pdf) | `Package_TO_SOT_SMD:SOT-23` (r18.20 final — vorher 1x3-Header-Platzhalter, Audit-Fix) | KiCad-Standard | Standard-Lib-3D |
| **Cell-Caps (5×)** **1u, selbst 3D-gedruckt** (r18.21 — statt ≥2u; kein Stabilizer nötig) | Eigenes 3D-Print | — | — | — | Custom-CAD (User) |
| Hall-Sensor-RC pro Cell: R_CELL 1 kΩ 0603 + C_CELL 10 nF 0603 | 0603WAF1001T5E / 0603B103K500NT | [C21190](https://www.lcsc.com/product-detail/C21190.html) / [C57112](https://www.lcsc.com/product-detail/C57112.html) | `Resistor_SMD:R_0603_1608Metric` / `Capacitor_SMD:C_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |

## 8. Modifier-Buttons + Service-Buttons

| Ref | MPN | LCSC/Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **SW6–SW10** Modifier (Shift / Hold / Drone / Generate / Clear — alle 5 identisch, momentary) — Cap klemmt auf den **eckigen Kopf mit Loch** oben; keine Verdreh-Sicherung nötig (User 2026-06-28) | **HCTL TC-1212-7.3-260G** (THT, 12×12×7,3 mm, eckiger Kopf) | [C2845240](https://jlcpcb.com/partdetail/C2845240) | **THT 12×12 4-Pin** — Footprint vom PCB-Macher (bisheriger `SW_HX_…_SMD-4P` war SMD) | Vendor | Vendor-CAD |
| **SW11** Reset (Mini-SMD-Tactile, Service-Bohrung Bottom-Plate) | **XUNPU TS-1088-AR02016** | [C720477](https://www.lcsc.com/product-detail/C720477.html) | [`field_ambience:SW_TS1088_SMD`](field-ambience-current/kicad/libraries/field_ambience.pretty/SW_TS1088_SMD.kicad_mod) (EasyEDA-verifiziert) | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/SW-SMD_L3.9-W2.9-H2.0-LS4.8.step) (r18.36-Fix: vorher fälschlich „Vendor-CAD" — STEP liegt im Repo) |
| **SW_BOOT** BOOT0 (USB-DFU-Flash, Service-Bohrung Bottom-Plate) | gleiches Teil wie SW11 | [C720477](https://www.lcsc.com/product-detail/C720477.html) | gleicher FP wie SW11 | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/SW-SMD_L3.9-W2.9-H2.0-LS4.8.step) (s. SW11) |
| R_BOOT_SW 1 kΩ 0603 (BOOT0-Pull-up) | s. R_CELL | [C21190](https://www.lcsc.com/product-detail/C21190.html) | s. R_CELL | KiCad-Standard | Standard-Lib-3D |

## 9. LEDs (Cell + Modifier — ADR-0008 XOR-Logik)

> **r18.64 — diskretes Mono-LED-System bestätigt (ADR-0019 SK6812 verworfen).**
> Die LEDs sind **feste Status-Anzeigen, keine World-Farben/RGB**. 15 diskrete
> Mono-LEDs, von `U6` PCA9685 (§4) PWM-getrieben, je 1 Vorwiderstand:
> **5 Modifier-LEDs** (1 pro Tactile-Button) + **10 Cell-LEDs** (je 2 pro Cell,
> gelb + grün). Semantik: persistenter aktiver Status; nur **Clear** blinkt kurz
> beim Klick und erlischt sofort (`leds.c` `s_clear_until`). Backlight bleibt
> PCA9685-Kanal 12 → Q2. Siehe [ADR-0019](field-ambience-current/docs/decisions/ADR-0019-led-system-smart-rgb.md)
> (SUPERSEDED).

| Funktion | MPN | LCSC/Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **Modifier-LED Shift (gelb)** | Hubei KENTO KT-0603Y (Vf 2.4V → 6.7mA @ 5V/390Ω) | [C2287](https://www.lcsc.com/product-detail/C2287.html) | `LED_SMD:LED_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **Modifier-LED Hold (grün)** | Hubei KENTO KT-0603G pure-green 525nm (Vf 3.1V → 4.9mA @ 5V/390Ω) | [C12624](https://www.lcsc.com/product-detail/C12624.html) | `LED_SMD:LED_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **Modifier-LEDs Drone / Generate / Clear (3× weiß)** | XL-1608UWC-04 (warmweiß 0603) | [C965808](https://www.lcsc.com/product-detail/C965808.html) | s. o. | KiCad-Standard | Standard-Lib-3D |
| **Cell-LEDs 5× Gelb** (base-hold-Status) | Hubei KENTO KT-0603Y | [C2287](https://www.lcsc.com/product-detail/C2287.html) | s. o. | KiCad-Standard | Standard-Lib-3D |
| **Cell-LEDs 5× Grün** (shift-hold-Status) | Hubei KENTO KT-0603G | [C12624](https://www.lcsc.com/product-detail/C12624.html) | s. o. | KiCad-Standard | Standard-Lib-3D |
| **LED_HB** Heartbeat (warmweiß) | **XL-1608UWC-04** (r18.24-Fix: war C965818 = XL-2012UWC **0805**, falsches Package) | [C965808](https://www.lcsc.com/product-detail/C965808.html) | s. o. | KiCad-Standard | Standard-Lib-3D |
| **LED_CHRG** Charger-Status (amber) | Generic Amber 0603 | [C72041](https://www.lcsc.com/product-detail/C72041.html) | s. o. | KiCad-Standard | Standard-Lib-3D |
| LED-Vorwiderstände **390 Ω** 0603 (15×, an +5V-Anode → LED → PCA9685-Sink) | 0603WAF3900T5E | [C23151](https://www.lcsc.com/product-detail/C23151.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |

> **Farb-Mapping (r18.64, User-Spec):** Shift=gelb, Hold=grün, Generate/Drone/
> Clear=weiß. Dies vertauscht Shift/Hold gegenüber der bisherigen Firmware-
> Konvention (die die Modifier-Farbe an die Cell-Hold-Farbe koppelte:
> base-hold=gelb, shift-hold=grün). `firmware-c-next/src/leds.c` ist auf die
> User-Spec angepasst.

### Live-Level-Meter (VU — r18.66, OP-1-Field-Stil)

> **8 kleine LEDs in einer Reihe als Live-Pegel-Anzeige** (wie der dB-Ausschlag
> in After Effects beim Abspielen). Getrieben von **U10** (2. PCA9685, §4) auf
> Channels 0-7; je 1 Vorwiderstand 390 Ω an +5V-Anode → U10-Sink. Die Firmware
> rechnet RMS/Peak aus dem Audio-Buffer und treibt die Segmente per PWM (echtes
> Meter, nicht nur Knopfstellung). **Position der Reihe im PCB = Layout-TBD.**

| Funktion | MPN | LCSC/Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **VU-LEDs 8× Weiß** (LED_VU1–8, r18.68 — User: weiß statt blau) | XL-1608UWC-04 (warm-white 0603) — gleiche LED wie Heartbeat; **keine blaue Extra-LED nötig** | [C965808](https://www.lcsc.com/product-detail/C965808.html) | `LED_SMD:LED_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| VU-Vorwiderstände **390 Ω** 0603 (8×, R_VU1–8) | 0603WAF3900T5E | [C23151](https://www.lcsc.com/product-detail/C23151.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |

## 10. Standard-Passives (Sortiment)

Alle in 0603 (kapazitiv 0603/0805 wo nötig). Sind im Generator pro Stelle mit
LCSC-Nr verlinkt; Sortiment hier nur referenziert:

| Klasse | LCSC-Beispiele |
|---|---|
| Widerstände 0603 1 % | C21190 (1 k), C25804 (10 k), C23253 (820), C23186 (5.1 k), C23146 (36 k), C23153 (39 k), C23162 (470), C23345 (22), C25803 (100 k), C25811 (200 k), C4184 (20 k), C22975 (2 k), C31850 (22 k), C72041 |
| Caps 0603 X7R / X5R | C14663 (100 n), C57112 (10 n), C46653 (4.7 µ X5R), C1588 (1 n), C1804 (auto-generated), C15849 (1 µ X5R), C14858 (10 n B) |
| Caps 0805 | C15850 (10 µ X5R), C45783 (22 µ X5R) |
| MLCC 1210 | C2880380 (100 µF/10V X5R, C_BULK2); C444831 (470µF/10V Polymer-Tantal Case-E, C_BULK) |
| Caps spezifisch | C24539 (2.2 µF VCAP); C107045 (27 pF C0G, HSE 2×); C15849 (1 µF, VREF+ & VDDA); C14663 (100 nF, VREF+ & VDDA) |

Alle Standard-FPs: `Resistor_SMD:R_0603_1608Metric`, `Capacitor_SMD:C_0603_1608Metric`,
`Capacitor_SMD:C_0805_2012Metric`, `Capacitor_SMD:C_1210_3225Metric` — alle KiCad-Standard,
Standard-Lib-3D.

## 11. SWD-Service-Header

| Ref | MPN | Link | Footprint | FP-Quelle | 3D |
|---|---|---|---|---|---|
| **J4** TC2030-IDC 3-Pin | Tag-Connect TC2030-IDC | [Tag-Connect Shop](https://www.tag-connect.com/product/tc2030-idc) | `Connector_PinHeader_1.27mm:PinHeader_1x03_P1.27mm_Vertical` | KiCad-Standard | Vendor-CAD |

## 12. Mechanik (Gehäuse + Knöpfe) — **post-PCB, kein Schematic/Layout-Blocker**

> Alle Einträge hier sind Gehäuse-/Hand-Hardware. Sie wirken NICHT auf das
> PCB-Layout (keine Bohrungen, keine Pads, keine Höhen-Constraints außer den
> bereits in `mechanical/coordinates/mechanical_coordinates.md` validierten).
> Der PCB-Sprint kann ohne diese Files starten und abschließen — sie werden
> erst nach Schematic/Layout-Freigabe gezogen.

| Element | Wahl | Quelle | Status |
|---|---|---|---|
| Gehäuse Außen 260 × 110 × 21.6 mm | ABS / PC Spritzguss 2.5 mm (Top + Bottom) | TBD (Industrial Design Sprint) | post-PCB |
| 4× Mounting-Hardware M2.5 | (Standard Hex-Standoff 3 mm) | RS / Reichelt | post-PCB |
| Encoder-Knöpfe (4×) Ø 19–20 × 8–10 mm | **selbst 3D-gedruckt** (r18.21) | User | post-PCB |
| Cell-Caps (5×, 1u) | **selbst 3D-gedruckt** (r18.21) | User | post-PCB |
| Plate für Magnetic-Switches | TBD-CAD | Industrial Design | post-PCB |

---

## Bestell-Strategie

- **JLCPCB-Bestücken (Extended OK):** alles in §1–§5, §8 (außer SW_BOOT
  Hand-Setze nach Wahl), §9, §10 — alles SMD mit LCSC-ID.
- **Separat über JLC bestellen, an JLC-Adresse senden, Hand-Bestückung
  selber:** Encoder (EN1–4), Hall-Sensoren (J_CELL1–5), Speaker, Gateron-
  Switches, Stabilizer, Adafruit-Display-Modul, Tag-Connect-SWD-Header,
  Mesh, Knöpfe.
- **Nie über JLC:** Gateron-Switches + Stabilizer + Mesh + Custom-Knöpfe.
  Das sind Keyboard-Markt-Teile + Custom-CAD-Teile.

## Verifizierungs-Audit-Trail

Wer prüft was wann:

| Punkt | Status | Verweis |
|---|---|---|
| Alle Footprints geprüft | ✅ **6 Custom** in [`field_ambience.pretty/`](field-ambience-current/kicad/libraries/field_ambience.pretty/) — alle aktiv referenziert (Y1, J8, L1, U8, SW6-10, SW11/SW_BOOT); Rest KiCad-Standard. Keine Leichen | [`field-ambience-current/FP_VERIFY_LOG.md`](field-ambience-current/FP_VERIFY_LOG.md) |
| 3D-STEPs für Z-/Panel-kritische Teile | ✅ **7 Stück** in [`field_ambience.3dshapes/`](field-ambience-current/kicad/libraries/field_ambience.3dshapes/) (U1, Y1, U8, L1, J9-Akku, J8, SW11/SW_BOOT). USB-C-Receptacle nutzt KiCad-Standard-STEP (kein Custom-STEP nötig). Z-Höhen-Tabelle pro STEP (für Enclosure-CAD): [`mechanical/3d_models/MANIFEST.md`](mechanical/3d_models/MANIFEST.md) | r18.36 |
| Mechanische X/Y/Z + Höhen-Constraints | ✅ Python-validiert, 0 Konflikte | `mechanical/coordinates/mechanical_coordinates.md` |
| DRV5056-Pinout DS-bestätigt | ✅ TI-DS Table 4-1 | r18.14b |
| AP7361C-Pinout User-bestätigt | ✅ Diodes-DS | r18.6 |
| TPS61089 Pin 11 = SW + Thermal | ✅ TI-DS | r18.7 |
| Speaker-Membran (Cloth vs Paper) | ✅ Cloth-Cone primär | r18.18 |
| USB-C LCSC-ID korrekt | ✅ C165948 (16-Pin TYPE-C-31-M-12). r18.10 hatte auf M-17 „geupgraded", r18.14 LCSC-Nr. von C165935 (MOSFET) auf C283540 korrigiert, r18.19-Audit fand C283540 = 6-Pin power-only → Revert auf M-12 | r18.19 |
| Audio-Jack-Footprint | ✅ `field_ambience:Jack_3.5mm_PJ-320D_SMT` (EasyEDA-CAD vendored, 4 SMD + 2 NPTH). Vorher CUI SJ-3523-SMT-FP zugewiesen — Pad-Layout passte nicht zu C431535 (SHOU HAN) | r18.19 |
| Speaker-Wert in Schaltplan | ✅ `value="CMS-402811-28SP Cloth-Cone 8R 2W"` (J6/J7). Vorher stale „PUI AS04008PS 4R 4W" — falsche Impedanz + falscher Treiber | r18.19 |
| SW_BOOT-MPN korrekt | ✅ TS-1088-AR02016 (war fälschlich TS-1185A) | r18.14 |
| CAD-Files klickbar verlinkt + 3D-Spalten-Konsistenz | ✅ alle 7 STEPs + 6 aktiv genutzten Custom-Footprints als relative Markdown-Links eingetragen; J1 USB-C („STEP im Repo" → „Standard-Lib-3D") und SW11/SW_BOOT („Vendor-CAD" → „STEP im Repo") korrigiert; unreferenzierte Footprint-Leiche `RotaryEncoder_ALPS_EC11J_SMD` gelöscht. **PCB-Path ist damit Schematic/Layout-bereit — keine offenen CAD-TODOs für PCB** | r18.36 |
| Hall-Sensor FP-Status doc-drift behoben | ✅ J_CELL1–5: BOM behauptete `PinHeader_1x03` als „Phase-6-Platzhalter, → SOT-23". Generator hatte aber bereits seit r18.20 final `Package_TO_SOT_SMD:SOT-23` (s. `generate_kicad_project.py:3342` `FP_NOTE`). BOM-Eintrag korrigiert. | r18.37 |
| LCSC string-diff BOM ↔ Generator | ✅ Performed: 60 BOM-Codes vs 53 Generator-Codes. **0 echte Mismatches** — alle Diffs sind entweder false positives (Tag-Connect „TC2030"), Audit-Trail-Historie (alte falsche LCSC-IDs), oder dead code in `pi_sheet()` (LEGACY, nicht geschrieben). Re-run vor Order empfohlen. Details in `PCB_FOOTPRINT_RISK_AUDIT.md` §9. | r18.37 |
| Mechanik-Sektion (§12) klar als post-PCB markiert | ✅ Custom-3D-Print-Files (Encoder-Knöpfe, Cell-Caps, Switch-Plate, Gehäuse) sind **nicht PCB-blockierend** — sie kommen nach Schematic/Layout-Freigabe und beeinflussen das Board nicht (keine Bohrungen / kein Pad-Pattern). §12 entsprechend annotiert | r18.36 |
| 3D-STEP-Export Vollständigkeit (für Enclosure-CAD) | 🟡 **1 bekannte Lücke:** HX-Modifier-Taster SW6–SW10 (C36498966) hat **kein 3D-Modell** (EasyEDA hat keins, `easyeda2kicad`-re-check 2026-06-27). Envelope **11,8×11,8×7,3 mm** in [`mechanical/3d_models/MANIFEST.md`](mechanical/3d_models/MANIFEST.md) → Enclosure-Clearance abgedeckt. Alle anderen: 7× STEP-im-Repo oder KiCad-Standard-Lib-3D. **Footprint vorhanden → kein PCB-Layout-Blocker.** Off-Board-Bodies (Speaker/Akku/Display/Knöpfe) sind extern (Vendor-CAD, s. §2/§3/§5/§6). | r18.65 |
| MECHANICAL_REQUIREMENTS §1.6 Modifier-Button-Teil korrigiert | ✅ §1.6 führte fälschlich TS-1088 (3,9×2,9, Service-Taster) statt HX 12×12×7,3 (SW6–SW10, Generator Z. 4111) → Höhe 7,3 mm statt falscher ~2 mm. Enclosure-Clearance jetzt korrekt | r18.65 |
