# BOM Master — Field Ambience (detailed / working view)

> 🏭 **Production handoff → [`field-ambience-current/PCB_BOM.md`](field-ambience-current/PCB_BOM.md)**
> (English, **on-PCB parts only**, off-board parts excluded, no duplicates,
> grouped by function). *This* file is the detailed working/rationale view
> (sourcing history, off-board parts, audit trail) — for Claude + deep dives.

**State: v0.7-r18.65 (2026-06-27).** Single source of truth for all actively
fitted components. Per entry: current choice, footprint source, 3D source,
order link. **Footprint and STEP columns are clickable since r18.36** and point
to the respective file in the repo (relative Markdown links, work on GitHub and
in any Markdown editor).

> The generator (`field-ambience-current/kicad/generate_kicad_project.py`) is the
> technical source of truth (LCSC, MPN, footprint in the schematic). This file is
> the reading/sourcing view. If an entry here disagrees, the generator wins.

## Legend

- **FP source:** `KiCad-Standard` (in the official kicad-footprints repo),
  `field_ambience` (vendored under `kicad/libraries/field_ambience.pretty/`),
  or `Module` (plug-in module, no PCB footprint except the header).
- **3D source:** `STEP in repo` (`kicad/libraries/field_ambience.3dshapes/`),
  `easyeda2kicad` (regenerable via `pip install easyeda2kicad; easyeda2kicad
  --full --lcsc_id=Cxxxxxx`), `Vendor-CAD` (obtain externally — no STEP in repo),
  `Standard-Lib-3D` (ships with the KiCad footprint).

---

> **✅ JLC assembly sourcing check (2026-06-29) — resolved.** Verified every
> soldered part against JLC's SMT parts API. All in JLC's assembly library; the
> 5 problem parts below are **fixed with verified JLC-stocked replacements**
> (specs + stock confirmed via API). Browsable: [`field-ambience-current/docs/hardware/bom_overview.html`](field-ambience-current/docs/hardware/bom_overview.html).
>
> | Was | Problem | → Replacement (verified) | JLC |
> |---|---|---|---|
> | FB2 `C84094` | not in JLC assembly | **`C19330`** for both FB1+FB2 (Murata, same MPN) | Extended, 950 k |
> | VCAP `C24539` | not in JLC assembly | **`C23630`** (2.2 µF 16 V X5R 0603) | **Basic**, 1.9 M |
> | LED_CHRG `C72041` | **wrong part — blue + EOL** (not amber!) | **`C965800`** (XL-1608UOC-06 orange 605 nm 0603) | Extended, 650 k |
> | C_BULK2 `C2880380` | stock ≈1 | **`C23742`** (100 µF 10 V X5R 1210) | Extended, 41 k |
> | SW6-10 `C2845240` | stock ≈30 (too few) | **`C36498965`** (HX B3F, square head for caps, 20 k, ~$0.06) — verify THT footprint/pinout | Extended, 20 k |

## 1. MCU + Clock

| Ref | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| U1 | **STM32H743VIT6** LQFP-100 | [C114409](https://www.lcsc.com/product-detail/C114409.html) | `Package_QFP:LQFP-100_14x14mm_P0.5mm` | KiCad-Standard | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/LQFP-100_L14.0-W14.0-H1.4-LS16.0-P0.50.step) |
| Y1 | **ABLS-8.000MHZ-B4-T** HC-49/US-SMD | [C596838](https://www.lcsc.com/product-detail/C596838.html) | [`field_ambience:Crystal_HC49-US-SMD_ABLS`](field-ambience-current/kicad/libraries/field_ambience.pretty/Crystal_HC49-US-SMD_ABLS.kicad_mod) | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/CRYSTAL-SMD_L11.4-W4.7-LS12.7.step) |

## 2. Power Stack

| Ref | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **J1** USB-C | **TYPE-C-31-M-12** (Korean Hroparts) — 16-pin full USB-C pinout (VBUS/GND/CC1/CC2/D+/D−/SBU). r18.19-REVERT: r18.10 had wrongly "upgraded" to M-17, but M-17 is 6-pin power-only → USB-DFU would have been broken. | [C165948](https://www.lcsc.com/product-detail/C165948.html) | `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12` | KiCad-Standard | Standard-Lib-3D (r18.36 fix: previously wrongly "STEP in repo" — no custom STEP exists) |
| D1 | **USBLC6-2SC6** ESD protection | [C2687116](https://www.lcsc.com/product-detail/C2687116.html) | `Package_TO_SOT_SMD:SOT-23-6` | KiCad-Standard | Standard-Lib-3D |
| F1 | **1812L300/16GR** PTC 3 A | [C18198349](https://www.lcsc.com/product-detail/C18198349.html) | `Fuse:Fuse_1812_4532Metric` | KiCad-Standard | Standard-Lib-3D |
| D2 | **SMAJ5.0A** TVS | [C113952](https://www.lcsc.com/product-detail/C113952.html) | `Diode_SMD:D_SMA` | KiCad-Standard | Standard-Lib-3D |
| **U8** TPS61089 boost VQFN-HR | [C165129](https://www.lcsc.com/product-detail/C165129.html) | [`field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A`](field-ambience-current/kicad/libraries/field_ambience.pretty/Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A.kicad_mod) | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/VQFN-HR_L2.5-W2.0-H1.0-P0.50.step) |
| L1 boost inductor | **SWPA6045S2R2NT** 2.2 µH (r18.77: corrected — schematic had `SWPA6045S2R2MT`, which per Sunlord's own datasheet does not exist for 2.2 µH; only the "N"-tolerance suffix is offered below 4.3 µH) | [C36500](https://www.lcsc.com/product-detail/C36500.html) (r18.77: was `C83455`, a dead LCSC listing — never actually valid despite the r18.20c "✓ Extended" note; verified live against Sunlord's SWPA6045S datasheet Item 12 table: 2.2 µH, DCR 18 mΩ max, Isat 7.40 A typ, Irms 5.00 A typ) | [`field_ambience:L_Sunlord_SWPA6045`](field-ambience-current/kicad/libraries/field_ambience.pretty/L_Sunlord_SWPA6045.kicad_mod) (EasyEDA CAD vendored r18.20c — phantom name L_0630 replaced) | field_ambience (custom) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/IND-SMD_L6.0-W6.0-H4.5.step) |
| D3 | **SS34** Schottky diode | [C8678](https://www.lcsc.com/product-detail/C8678.html) | `Diode_SMD:D_SMA` | KiCad-Standard | Standard-Lib-3D |
| **R_COMP / C_COMP** boost loop comp (r18.80) | **6,2 k 1 % (0603WAF6201T5E) / 10 nF (0603B103K500NT)** — nach TI SLVSD38C Gl. 17/18 (fc ≈ 8 kHz ≤ fRHPZ/5 ≈ 9,5 kHz @VIN 3,0 V/2 A); **war 22k/1nF = fc ≈ 87 kHz ÜBER der RHP-Nullstelle → Loop-Oszillation unter Last**. Formeln gegen TIs eigenes 9-V/2-A-Referenzdesign validiert (reproduziert exakt 17,4k/4,7nF). | [C4260](https://www.lcsc.com/product-detail/C4260.html) (Basic, 219k Stock, live verifiziert 2026-07-02) / [C57112](https://www.lcsc.com/product-detail/C57112.html) | `Resistor_SMD:R_0603_1608Metric` / `Capacitor_SMD:C_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **C_BOOST_OUT ×3** (r18.80: war 1×) | **CL21A226MAQNNNE** 22 µF 25 V X5R 0805 ×3 — TI DS §9.2.2.7 „typically three 22 µF"; EC-Tabelle rechnet Soft-Start mit COUT_eff 47 µF; 1× 0805 hätte bei 5-V-Bias nur ~12 µF eff. (Ripple ~150 mV), und der 470-µF-Bulk liegt HINTER D3 (der Regelknoten sieht ihn nicht). **Achtung: C45783 ist 22 µF, nicht 4,7 µF** — die alte „CL21A475"-Beschriftung der STM32-VDD-Bank war falsch (r18.80 korrigiert). | [C45783](https://www.lcsc.com/product-detail/C45783.html) (Basic, 1,6 M, live verifiziert 2026-07-02) | `Capacitor_SMD:C_0805_2012Metric` | KiCad-Standard | Standard-Lib-3D |
| ~~Q1 DMG2305UX-7 P-Ch MOSFET~~ + ~~R22 10k~~ | ⛔ **ENTFERNT r18.79 (Elektrik-Audit):** der Power-Path (S=VBUS pre-fuse, D=+5V-Rail, G→GND) speiste ohne USB die Boost-5V über Body-Diode + durchgeschaltetes Gate zurück auf VBUS → Lader lud den Akku aus dem eigenen Boost (Selbstentladeschleife ~1,5× Ladestrom), USB-Detect dauerhaft HIGH, 5 V am offenen Stecker; zusätzlich überbrückte Q1 die Sicherung F1. Ersetzt durch **Dioden-OR**: USB → F1 → **D3B** (2. SS34, C8678) → Rail ‖ Boost → D3 → Rail. | — | — | — |
| ~~**D3B** USB-Pfad-Schottky~~ | ⛔ **ENTFERNT r19.18 (ADR-0023):** das Dioden-OR ist Geschichte — die +5V-Rail hat genau EINE Quelle (Boost via D3). Der USB-Pfad endet am BQ24074-IN. | — | — | — | — |
| **F2** Batterie-PTC (r19.18, ADR-0023) | **SMD1812P260TF/16** (PTTC) 2,6 A hold / 5 A trip / 16 V / 15 mΩ — Hard-Short-Backup im BAT+-Pfad (Zellen-Schutz-PCB bleibt Pflicht) | [C438899](https://www.lcsc.com/product-detail/C438899.html) (live verifiziert 2026-07-13: 1.730 Stk., JLC Economic+Standard) | `Fuse:Fuse_1812_4532Metric` | KiCad-Standard | Standard-Lib-3D |
| **U5** LDO | **AP7361C-33Y5-13** SOT-89-5 (pinout 1=EN, 2=GND, 3=ADJ, 4=IN, 5=OUT verified) | [C460397](https://www.lcsc.com/product-detail/C460397.html) | `Package_TO_SOT_SMD:SOT-89-5` | KiCad-Standard | Standard-Lib-3D |
| **U_PWR** power-off load switch (ADR-0016) | **TPS22918DBVR** SOT-23-6 — gates the LDO input `+5V_RAIL → +5V_SW` (= the whole 3V3 domain: MCU + MCP + LEDs + LCD). ON comes from `SW_PWR`. Charger U7 sits **ahead** of it → keeps charging when off. "Dark, but charging." | [C131941](https://www.lcsc.com/product-detail/C131941.html) (r18.81: live verifiziert — TPS22918DBVR, 42k Stock; **jetzt im power_tree-Schematic verdrahtet**, Pinout gegen TI SLVSD76C: 1=VIN,2=GND,3=ON,4=CT float,5=QOD→VOUT,6=VOUT) | `Package_TO_SOT_SMD:SOT-23-6` | KiCad-Standard | Standard-Lib-3D |
| **SW_PWR** slide switch = main power-off | **ALPS ALPINE SSSS811101** (r18.85 — **premium/TE-grade swap**, user direction "high quality like Teenage Engineering"; ALPS is exactly the brand class TE uses). SMD micro slide, SPDT, **natively side-actuated**: ALPS datasheet "Actuator directions: **Horizontal**" — knob (1.1 mm) protrudes sideways from the body, travel 1.5 mm, operating force 1.5 N, ground-frame terminal, reflow (JLC assembles it). Switches **only** `U_PWR.ON` (signal level, µA); 10 k cycles / 0.3 A 5 V — ample. **Common is LABELED in the ALPS circuit diagram (C = terminal 2)** — closes the old MST "terminals unlabeled" gap. ⚠ **Mechanical note for the case:** body is only **1.4 mm tall** → stem window sits **z ≈ 0–1.4 mm above the PCB** (much lower than the old MST's 2.3–3.8 mm) — side-wall slot + slider cap must reach down near board level; cap still takes the user's force (same-axis sliding, no motion redirection). **Budget fallback (stays vendored):** MST-12D18G3 (SHOU HAN, [C49023766](https://jlcpcb.com/partdetail/C49023766), FP `SW_MST-12D18_SlideSwitch_RA`, r18.81-verified). | [C109335](https://www.lcsc.com/product-detail/C109335.html) (JLC Extended, ~33 k stock, ~$0.15) | [`field_ambience:SW_ALPS-SSSS811101_SlideSwitch_SMD`](field-ambience-current/kicad/libraries/field_ambience.pretty/SW_ALPS-SSSS811101_SlideSwitch_SMD.kicad_mod) — **vendored r18.85 (easyeda2kicad C109335) + verified against the ALPS land pattern (p.3)**: 3 signal pads 0.7 mm @ asymmetric 3.0/1.5 mm spacing + 4 ground-frame pads 0.8×1.0 @ ±3.65 mm (inner edge 6.3 mm) + 2 locator holes ø0.9 @ 3.0 mm — exact match. Ground pads 4–7 are not in the 3-pin symbol (solder-anchor the frame; optionally tie to GND in layout for EMC). | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/SW_ALPS-SSSS811101_SlideSwitch_SMD.step) |
| **R_PWR_PD** 100 kΩ 0603 (`U_PWR.ON` pull-down → default OFF) | 0603WAF1003T5E | [C25803](https://www.lcsc.com/product-detail/C25803.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **C_PWR_SW** 10 µF X5R 0805 (output cap on `+5V_SW`) | LMK212ABJ106 class | [C15850](https://www.lcsc.com/product-detail/C15850.html) | `Capacitor_SMD:C_0805_2012Metric` | KiCad-Standard | Standard-Lib-3D |
| **U7** charger (r19.18, ADR-0023) | **BQ24074RGTR** (TI, VQFN-16 RGT 3×3) — 1,5-A-Power-Path-Charger mit DPPM. ICHG = 890/R_ISET(1k) = **0,89 A**; IIN-MAX = 1610/R_ILIM_IN(1,2k) = **1,34 A**; ITERM/TMR = NC-Default (10 % / 5 h); TS = 10k fest; EN2=VSYS/EN1=GND; OUT=`VSYS` speist den Boost, BAT via F2 an J9. Ersetzt MCP73831 (Audit P0-1/2/5/6/7). | [C54313](https://www.lcsc.com/product-detail/C54313.html) (live verifiziert 2026-07-13: 1.074 Stk., $2,24@1, JLC Economic+Standard) | `Package_DFN_QFN:QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm` (gegen JLC/EasyEDA-Landpattern C54313 abgeglichen) | KiCad-Standard | Standard-Lib-3D |
| **R_ISET / R_ILIM_IN / R_TS** BQ24074-Programmierung (r19.18) | 1 kΩ (ICHG 0,89 A) / **1,2 kΩ YAGEO RC0603FR-071K2L** (IIN 1,34 A) / 10 kΩ (TS fest, DS-Vorgabe ohne Pack-NTC) | [C21190](https://www.lcsc.com/product-detail/C21190.html) / [C114605](https://www.lcsc.com/product-detail/C114605.html) (live verifiziert: 407k Stk.) / [C25804](https://www.lcsc.com/product-detail/C25804.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **C_BAT / C_SYS1 / C_SYS_HF** BQ24074-Bypass (r19.18) | 22 µF 0805 (BAT, DS 4,7–47 µF) / 22 µF 0805 (VSYS = OUT-Bypass + Boost-Input) / 100 nF 0603 | [C45783](https://www.lcsc.com/product-detail/C45783.html) / [C45783](https://www.lcsc.com/product-detail/C45783.html) / [C14663](https://www.lcsc.com/product-detail/C14663.html) | 0805 / 0805 / 0603 | KiCad-Standard | Standard-Lib-3D |
| **C_BULK** polymer-tantalum 470 µF / 10 V case-E 7343-43 (ESR 100 mΩ; <25 mΩ flat parts not in stock at LCSC — transient ESR is supplied by C_BULK2) | TPSE477K010R0100 (Kyocera AVX) | [C444831](https://www.lcsc.com/product-detail/C444831.html) | `Capacitor_SMD:CP_Tantalum_Case-E_EIA-7343-43_Reflow` | KiCad-Standard | Standard-Lib-3D |
| **C_BULK2** MLCC **100 µF / 10 V** 1210 (parallel; 220µF/10V/1210 does not exist → 100µF of real headroom instead of 220µF/6.3V derating) | CL32A107MPVNNNE (Samsung) — r18.70: was C2880380 (Taiyo Yuden, JLC stock ≈1) | [C23742](https://www.lcsc.com/product-detail/C23742.html) | `Capacitor_SMD:C_1210_3225Metric` | KiCad-Standard | Standard-Lib-3D |
| **J9** JST-PH 2.0 2-pin (LiPo battery — generator refdes, **not** J_BAT) | **S2B-PH-SM4-TB(LF)(SN)** | [C295747](https://www.lcsc.com/product-detail/C295747.html) | `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal` | KiCad-Standard | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/CONN-SMD_P2.00_S2B-PH-SM4-TB-LF-SN.step) |
| LiPo **2000 mAh JST-PH** (~6.6 h @ 300 mA) — r18.21 right-sized down from 5000mAh. **r19.4: DE-Quelle = Adafruit 2011** (60×36×**7 mm**, integriertes PCM, RoHS-2, UN38.3-Report) via Mouser/DigiKey/BerryBase. ⚠ 7 mm = **+2 mm** vs 5-mm-503759-Referenz (Arons Layout); **Steckerpolung vs J9 Pin 1 = BAT_PLUS** vor dem ersten Einstecken per Multimeter prüfen. Details → `field-ambience-current/docs/SOURCING_COMPLIANCE.md §2`. PiHut-503759 (5 mm) bleibt Alternative, liefert aber nicht nach DE. | LiPo 2000mAh JST-PH 2.0 mit PCM | [Adafruit 2011](https://www.adafruit.com/product/2011) · Mouser/DigiKey/BerryBase | — (no PCB FP, bottom-case slot) | — | Vendor |

> **Power architecture / rail ownership (ADR-0016, audit #8/#9).** So the PCB
> maker sees the sources unambiguously — *one* source→system-rail place each:
> - `VBUS_USB` (USB-C 5 V, via F1 PTC) ──┐
> - r19.18 (ADR-0023): `VBUS_FUSED` (USB via F1) → **U7 BQ24074-IN**; **U7-OUT = `VSYS`** (4,4 V @USB / VBAT @Akku) → boost **U8** (4,97 V) → **D3** ──┤→ **`+5V_RAIL`** (EINZIGE Quelle; Dioden-OR/D3B entfernt) — feeds amp U4 + U_PWR. `PWR_ON` schaltet U8-EN **und** U_PWR; SW_PWR-Pull-Quelle = `VSYS`.
> - **`+5V_SW`** = `+5V_RAIL` behind **U_PWR** (switched by `SW_PWR`) → LDO **U5** → **`+3V3`** (MCU, MCP, both PCA9685, LCD)
> - Charger **U7** hangs off `VBUS_USB`/`BAT_PLUS` **ahead of** U_PWR → charges independently of the off state.
>
> The generator nets are today still partly mixed `+5V_OUT`/`+5V_RAIL`; since
> the PCB maker re-draws the schematic anyway, **this ownership view is
> authoritative** (source → exactly one system rail).

## 3. Audio Path

| Ref | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **U3** DAC | **PCM5102APWR** TSSOP-20 | [C107671](https://www.lcsc.com/product-detail/C107671.html) | `Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm` | KiCad-Standard | Standard-Lib-3D |
| **U4** Class-D amp | **PAM8403DR-H** SOIC-16 | [C17337](https://www.lcsc.com/product-detail/C17337.html) | `Package_SO:SOIC-16_3.9x9.9mm_P1.27mm` | KiCad-Standard | Standard-Lib-3D |
| FB1/FB2 ferrite bead | **BLM18AG601SN1D** 0603 600 Ω | [C19330](https://www.lcsc.com/product-detail/C19330.html) (**both FB1+FB2** — r18.70: dropped C84094, not in JLC assembly; C19330 is JLC Extended, 950 k stock) | `Inductor_SMD:L_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **U11** Kopfhoererverstaerker (r19.19, ADR-0024) | **TPA6132A2RTER** (TI, WQFN-16 RTE 3×3) — DirectPath (keine Auskoppel-Elkos), Gain −6 dB (G0=G1=GND), EN=AMP_nSHDN, Pop-Suppression, Kurzschluss-Schutz. Externe: 2× CIN 1 µF, C_FLY 1 µF, C_HPVSS 1 µF, C_HP_VDD+C_HPVDD 2,2 µF ([C1607](https://www.lcsc.com/product-detail/C1607.html)). J8 damit **PHONES / LINE OUT**. | [C69901](https://www.lcsc.com/product-detail/C69901.html) (live verifiziert 2026-07-13: 370 Stk., $1,35@1, JLC Economic+Standard) | `Package_DFN_QFN:QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm` (gegen JLC-Landpattern C69901 + TI RTE0016C abgeglichen) | KiCad-Standard | Standard-Lib-3D |
| **J8** phones/line-out 3.5 mm TRS | **PJ-320D** (SHOU HAN, SMT) with insertion-detect | [C431535](https://www.lcsc.com/product-detail/C431535.html) | [`field_ambience:Jack_3.5mm_PJ-320D_SMT`](field-ambience-current/kicad/libraries/field_ambience.pretty/Jack_3.5mm_PJ-320D_SMT.kicad_mod) (EasyEDA CAD vendored r18.19) | field_ambience (custom) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/AUDIO-SMD_PJ-320D-1.step) |
| **J10** MIDI-out 3.5 mm TRS (r18.67) | **PJ-320D** (same MPN as J8) — TRS **Type A**, OUT-only, 3.3 V/CA-033 (no level shifter). MIDI_TX=PD5→R_MIDI_TX 220 Ω→Tip; +3V3→R_MIDI_REF 220 Ω→Ring; Sleeve→GND. ⚠️ **J9 = battery, J10 = MIDI** | [C431535](https://www.lcsc.com/product-detail/C431535.html) | see J8 (`field_ambience:Jack_3.5mm_PJ-320D_SMT`) | field_ambience (custom) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/AUDIO-SMD_PJ-320D-1.step) |
| **R_MIDI_TX / R_MIDI_REF** 220 Ω 0603 (2×, MIDI Type-A) | 0603WAF2200T5E | [C22962](https://www.lcsc.com/product-detail/C22962.html) — **verified r18.72: JLC Basic, 1.69M stock** (NEEDS-VERIFY cleared) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |

### Speaker (r18.18 — cloth cone primary)

| Ref | MPN | Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **J6 / J7** 1×2 2.54 mm header (speaker leads) | B-2100S02P-A110 (1×2 2.54 mm male) | [C124375](https://www.lcsc.com/product-detail/C124375.html) | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | KiCad-Standard | Standard-Lib-3D |
| **SPK-L / SPK-R** driver **primary** | **Same Sky CMS-402811-28SP** (cloth cone, 40 × 28.3 × 11.5 mm, 8 Ω, 2 W, F0 = 450 Hz, NdFeB, solder eyelets) | [DigiKey 102-CMS-402811-28SP-ND](https://www.digikey.com/en/products/detail/cui-devices/CMS-402811-28SP/10821307) · [vendor page](https://www.sameskydevices.com/product/audio/speakers/miniature-(10-mm~40-mm)/cms-402811-28sp) | — (no PCB mount, depends on top panel) | — | [Vendor-CAD](https://www.sameskydevices.com/product/resource/cms-402811-28sp.pdf) |
| **SPK** driver **secondary** (backup) | **PUI AS04008PS-4W-WR-R** (treated paper, same footprint, F0 = 380 Hz) | [DigiKey](https://www.digikey.com/en/products/detail/pui-audio-inc/AS04008PS-4W-WR-R/1464855) | — | — | [Vendor-CAD](https://puiaudio.com/file/specs-AS04008PS-4W-WR-R.pdf) |

### Speaker cover (dust mesh)

| Element | Choice | Link | Source |
|---|---|---|---|
| Mesh **series** | Saati **Acoustex 020–032** (transparent class, ~25–32 g/m²) + PSA conversion via Marian Inc. | [Saati Acoustex](https://www.saati.com/products/filtration/brands/saatifil-acoustex/) · [Marian (converter)](https://marianinc.com/materials/filters-venting-fabric/) | vendor direct purchase, custom quote |
| Mesh **prototype** | Generic self-adhesive "phone-speaker dustproof mesh" stickers, punch out 36 × 24 mm ovals | AliExpress / Amazon (multipack ~€5) | Generic |

## 4. I/O Expander + LED Driver

| Ref | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **U2** GPIO expander | **MCP23017-E/SS** SSOP-28 | [C506653](https://www.lcsc.com/product-detail/C506653.html) | `Package_SO:SSOP-28_5.3x10.2mm_P0.65mm` | KiCad-Standard | Standard-Lib-3D |
| **U6** PWM LED driver | **PCA9685PW,118** TSSOP-28 (16 channels: 5 modifier + 10 cell + 1 backlight, exactly 16/16) | [C2678753](https://www.lcsc.com/product-detail/C2678753.html) | `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm` | KiCad-Standard | Standard-Lib-3D |

## 5. Display

| Ref | MPN | Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **J3** 1×8 header 2.54 mm (LCD module) | 2.54 mm strip cut to 1×8 (B-2100S40P or similar) — or 1×8 female socket for a pluggable module | [C124383](https://www.lcsc.com/product-detail/C124383.html) | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | KiCad-Standard | Standard-Lib-3D |
| **LCD module** **Waveshare 1.9in 170×320 ST7789V2** IPS module (r18.22 pivot from bare AliExpress: same panel as Adafruit + branded QC + level shifter + documented. **r19.4: DE-Quelle = Eckstein Shop 13,95 €** (exp-tech/BerryBase/Amazon.de alt). **Pin-Order weicht von J3 ab → immer per 8-Draht-Kabel** mappen, nie Steck-Header. Details → `field-ambience-current/docs/SOURCING_COMPLIANCE.md §1`) | Waveshare 1.9inch LCD Module 170x320 SPI | [Eckstein 13,95 €](https://eckstein-shop.de/WaveShare-19inch-LCD-Display-Module-170320-IPS-262K-Colors-SPI-Interface-EN) · [Waveshare](https://www.waveshare.com/1.9inch-lcd-module.htm) · exp-tech/BerryBase | Module (Kabel an J3) | — | Vendor-CAD |
| **LCD module — PROPOSED r18.43 / ADR-0015** **Waveshare 2.0in 240×320 ST7789** IPS module. Pivot 1.9″ → 2.0″ for more area + animation headroom + native RGB565 color. Same ST7789 controller family, same 8-pin SPI interface, same backlight path (Q2/PCA9685 channel 12). **UNVERIFIED — NEEDS HUMAN CHECK:** (a) sourcing path/SKU (LCSC PN may not exist; source via Waveshare/PiHut/Amazon like the 1.9″), (b) 8-pin header order against the delivered module, (c) outer dimensions + active-area position against the bezel window. Don't change the bezel cutout + generator sheet until (a)–(c) are confirmed. | Waveshare 2inch LCD Module 240×320 SPI | [Waveshare](https://www.waveshare.com/2inch-lcd-module.htm) | Module (plugs into J3, possible pin-order adaptation) | — | Vendor-CAD |
| Q2 backlight driver | **2N7002,215** SOT-23 | [C8545](https://www.lcsc.com/product-detail/C8545.html) | `Package_TO_SOT_SMD:SOT-23` | KiCad-Standard | Standard-Lib-3D |

## 6. Encoders (ADR-0012 — **4× push encoder**, all the same height)

> **r18.65:** All 4 are the same push part **EC11E18244AU** and all 4 push
> switches are wired (DISPLAY=PE3, VOL=MCP-GPB5, DRIVE=PE0, BRIGHT=PE1) → every
> encoder can carry a push function. Detent feel: Display with detents,
> parameter encoders "smooth" via firmware acceleration.

| Ref | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **EN3** (Display: push + detent) | **ALPS EC11E18244AU** (18 pulses, 36 detents, with push switch, flat shaft 20 mm) | [C202365](https://www.lcsc.com/product-detail/C202365.html) | `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` | KiCad-Standard | Standard-Lib-3D |
| **EN1 / EN2 / EN4** (r18.22 pivot: unified on the ACTIVE display encoder, because both EC11E183440C and the candidate EC11E1834403 are NRND and the whole ALPS "0-detent + push" family is phased out) | **ALPS EC11E18244AU** (= the display encoder, 36 detents/rev + push; firmware acceleration makes slow = 1 %/click, fast = ×8/click — UX-functionally identical to smooth) | [C202365](https://www.lcsc.com/product-detail/C202365.html) (~3,052 stock) | same FP as EN3 | KiCad-Standard | Standard-Lib-3D |
| Knobs (4×) Ø 19–20 × 8–10 mm, **self 3D-printed** (r18.21 — instead of Al CNC, saves $50-200/5-pack run) | own 3D print | — | — | — | Custom-CAD (user) |

## 7. Cells (ADR-0013 **SUPERSEDED r18.75** — digital, direct-solder Kailh Choc keyswitch)

> **r18.73 (User direction 2026-06-30, HiChord Batch 4+ reference):** the 5 cells
> changed from Gateron-magnetic + DRV5056A4 Hall (analog velocity into the STM32
> ADC) to a **plain digital on/off switch on the MCP23017 I²C expander** — the
> HiChord Batch 4+ topology *switch → I²C GPIO-expander → MCU*. **Removed from
> the board:** 5× DRV5056A4 Hall sensors (J_CELL1–5), 5× R_CELL 1 kΩ, 5× C_CELL
> 10 nF; the 5 STM32 ADC pins (PC0/PC1/PA4/PB0/PB1) are freed as Rev-B reserves.
>
> **r18.74 (User UX correction 2026-07-01):** r18.73 had put the cells on the
> **same small HX B3F-4055 tactile button as the modifiers** — that makes the
> playable cells feel identical to a plain modifier button and kills the
> "these are keyboard keys" UX. Cells stayed electrically digital, but were
> given a Kailh Choc hot-swap socket for real keyswitch travel.
>
> **r18.75 (User follow-up 2026-07-01 — "how is this meant to be soldered?"):**
> the r18.74 hot-swap socket had **no clean manufacturer/LCSC part number**
> and needed a fiddly small-SMD hand-soldering technique that isn't obvious
> without prior hobbyist-keyboard experience. **Fixed:** the Kailh Choc V1
> switch (**CPG135001D01**) is now **directly soldered to the board** — 2 THT
> legs through 2 holes, soldered from the back, exactly the same technique as
> every other button/connector in this design. No hot-swap socket, no special
> technique. Footprint + 3D STEP pulled straight from LCSC/EasyEDA for this
> exact part via `easyeda2kicad --full --lcsc_id=C400229` (this repo's own
> documented method for custom footprints — same as TS-1088/MST-12D18/
> PJ-320D), vendored as `field_ambience:SW_KailhChoc_CPG1350_THT_2P` with a
> real 3D STEP. Electrically unchanged since r18.73/74 (same nets/pins/
> pull-up/IRQ). **Trade-off:** the switch is now permanent (not swappable
> without desoldering) — traded for assembly simplicity and dropping the
> unverified socket part. Modifier buttons SW6–SW10 stay the small HX B3F
> tactile — deliberate UX split (cells feel like keys, modifiers feel like
> buttons). Hall stays documented as the option **only if** true press-depth/
> velocity is wanted later — see ADR-0013.

| Ref | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **Cell switch (SW1–SW5, 5×)** | **Kailh Choc V1, CPG135001D01** (red/linear) — direct-solder THT, 2 electrical pins + 3 unplated mechanical locator holes (switch's own pegs give lateral stability without a plate). One pin → MCP23017 GPA0–GPA4 (`CELL1..5_BTN`), other → GND; MCP internal pull-up, shared INT. **Any Choc V1 color works — same footprint** (see r18.78 sourcing note below). [Datasheet](https://cdn-shop.adafruit.com/product-files/5113/CHOC+keyswitch_Kailh-CPG135001D01_C400229.pdf) | [C400229](https://www.lcsc.com/product-detail/C400229.html) — real, verified listing. ⚠ 0 stock at time of writing (restocks common); JLC notes their automated PCBA needs a custom fixture for this THT part — hand-soldering the 2 legs needs neither. | `field_ambience:SW_KailhChoc_CPG1350_THT_2P` — pulled directly from LCSC/EasyEDA via `easyeda2kicad --full --lcsc_id=C400229` | repo custom, sourced from the part's own manufacturer listing (not a third-party library) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/SW_KailhChoc_CPG1350_THT_2P.step) |
| **Cell caps (5×), self 3D-printed** (clip onto the Choc V1 switch stem) | own 3D print | — | — | — | Custom-CAD (user) |

> **r18.78 sourcing note (User, 2026-07-01):** all three Kailh Choc V1 colors
> at LCSC are currently **0 stock**: red/linear [C400229](https://www.lcsc.com/product-detail/C400229.html),
> brown/tactile [C400230](https://www.lcsc.com/product-detail/C400230.html),
> white/clicky [C400231](https://www.lcsc.com/product-detail/C400231.html) —
> all real, verified, just temporarily out of stock (checked live 2026-07-01).
> **This is not a blocker:** since the switch is hand-soldered (not part of
> JLC's automated SMT pass), it doesn't need to come from LCSC at all — any
> Kailh Choc V1 switch (any color/feel, any vendor: Kailh direct, KBDfans,
> NovelKeys, AliExpress) uses the identical 2-pin THT footprint. Feel/color
> is a free choice, not a schematic constraint.
>
> **r19.4 fixed DE source:** [keycapsss.com](https://keycapsss.com) (1–3 day
> DE delivery), EU alt 42keebs.eu / splitkb.com. Pick a **tactile** variant
> (Choc Brown) for the keyboard-key feel. For the compliance file, request
> Kailh's RoHS manufacturer declaration. See `docs/SOURCING_COMPLIANCE.md §1`.
>
> **r18.78 open item for Aron (PCB layout):** user wants cell caps **2–3 cm
> long** eventually. No stabilizer needed at that length (well under the
> ~38 mm/2u stabilizer threshold), but a 20–30 mm-wide cap on the current
> 19 mm cell pitch collides with the neighboring cell if oriented left-right.
> Two ways to resolve (widen the pitch, or elongate the cap front-to-back
> instead — ~26 mm clearance to the front edge and ~32 mm to the modifier
> row are available for that) — deliberately left open for Aron to decide
> during actual layout, not fixed here. Details: `mechanical_coordinates.md`
> §3.4.

## 8. Modifier Buttons + Service Buttons

| Ref | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **SW6–SW10** modifier (Shift / Hold / Drone / Generate / Clear — all 5 identical, momentary) — the cap clips onto the **square head** on top; no anti-rotation needed (user 2026-06-28) | **HX B3F-4055-Y** (THT, 11.8×11.8×7.3 mm, **square** head, ~$0.06). **r18.71: was TC-1212-7.3-260G C2845240 (JLC stock ≈30, too few)** → HX B3F (JLC 20 k stock, same 12×12 4-pin THT body, square head ideal for caps). ⚠ verify THT footprint/pinout vs the existing `SW_TC1212-7.3_THT_4P` (or make an HX-THT footprint) before fab. Alt: C2845239 (TC-1212 round). | [C36498965](https://jlcpcb.com/partdetail/C36498965) | **THT 12×12 4-pin** — verify footprint/pinout (HX B3F) | Vendor | Vendor-CAD |
| **SW11** Reset (mini SMD tactile, service hole in bottom plate) | **XUNPU TS-1088-AR02016** | [C720477](https://www.lcsc.com/product-detail/C720477.html) | [`field_ambience:SW_TS1088_SMD`](field-ambience-current/kicad/libraries/field_ambience.pretty/SW_TS1088_SMD.kicad_mod) (EasyEDA-verified) | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/SW-SMD_L3.9-W2.9-H2.0-LS4.8.step) (r18.36 fix: previously wrongly "Vendor-CAD" — STEP is in the repo) |
| **SW_BOOT** BOOT0 (USB-DFU flash, service hole in bottom plate) | same part as SW11 | [C720477](https://www.lcsc.com/product-detail/C720477.html) | same FP as SW11 | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/SW-SMD_L3.9-W2.9-H2.0-LS4.8.step) (see SW11) |
| R_BOOT_SW 1 kΩ 0603 (BOOT0 pull-up) | see R_CELL | [C21190](https://www.lcsc.com/product-detail/C21190.html) | see R_CELL | KiCad-Standard | Standard-Lib-3D |

## 9. LEDs (cell + modifier — ADR-0008 XOR logic)

> **r18.64 — discrete mono-LED system confirmed (ADR-0019 SK6812 rejected).**
> The LEDs are **fixed status indicators, not world colors/RGB**. 15 discrete
> mono LEDs, PWM-driven from `U6` PCA9685 (§4), each with one series resistor:
> **5 modifier LEDs** (1 per tactile button) + **10 cell LEDs** (2 per cell,
> yellow + green). Semantics: persistent active status; only **Clear** flashes
> briefly on click and goes out immediately (`leds.c` `s_clear_until`). Backlight
> stays PCA9685 channel 12 → Q2. See [ADR-0019](field-ambience-current/docs/decisions/ADR-0019-led-system-smart-rgb.md)
> (SUPERSEDED).

| Function | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **Modifier LED Shift (yellow)** | Hubei KENTO KT-0603Y (Vf 2.4V → 6.7mA @ 5V/390Ω) | [C2287](https://www.lcsc.com/product-detail/C2287.html) | `LED_SMD:LED_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **Modifier LED Hold (green)** | Hubei KENTO KT-0603G pure-green 525nm (Vf 3.1V → 4.9mA @ 5V/390Ω) | [C12624](https://www.lcsc.com/product-detail/C12624.html) | `LED_SMD:LED_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **Modifier LEDs Drone / Generate / Clear (3× white)** | XL-1608UWC-04 (warm white 0603) | [C965808](https://www.lcsc.com/product-detail/C965808.html) | see above | KiCad-Standard | Standard-Lib-3D |
| **Cell LEDs 5× yellow** (base-hold status) | Hubei KENTO KT-0603Y | [C2287](https://www.lcsc.com/product-detail/C2287.html) | see above | KiCad-Standard | Standard-Lib-3D |
| **Cell LEDs 5× green** (shift-hold status) | Hubei KENTO KT-0603G | [C12624](https://www.lcsc.com/product-detail/C12624.html) | see above | KiCad-Standard | Standard-Lib-3D |
| **LED_CHRG** charger status (orange) | **XL-1608UOC-06** (XINGLIGHT, orange 605 nm 0603) — r18.70: was C72041, which is actually a **blue + discontinued** LED with stock ≈4 (wrong part, caught by JLC API check) | [C965800](https://www.lcsc.com/product-detail/C965800.html) | see above | KiCad-Standard | Standard-Lib-3D |
| LED series resistors **390 Ω** 0603 (15×, on +5V anode → LED → PCA9685 sink) | 0603WAF3900T5E | [C23151](https://www.lcsc.com/product-detail/C23151.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |

> **Color mapping (r18.64, user spec):** Shift=yellow, Hold=green, Generate/
> Drone/Clear=white. This swaps Shift/Hold versus the previous firmware
> convention (which coupled the modifier color to the cell-hold color:
> base-hold=yellow, shift-hold=green). `firmware-c-next/src/leds.c` is adapted to
> the user spec.

### Live level meter (VU) — REMOVED r18.87

> **r18.87 (user):** only the LEDs above the cells and above the modifier
> buttons remain. The 8-LED live level meter (U10 = 2nd PCA9685 @ 0x41,
> LED_VU1–8, R_VU1–8, R_OE2, C_PCA2_VDD/_HF — r18.66/r18.68/r18.84/r18.85)
> and the PD8 heartbeat LED (LED_HB + R_SLED 820 Ω) are deleted from the
> schematic, BOM and firmware. LED_CHRG stays: with the ADR-0016 power-off
> scheme the device charges while switched OFF (no firmware running), so the
> hardware-driven BQ24074-CHG LED is the only charge feedback (r19.18: fed
> from `VBUS_FUSED`, lights only when USB is present AND charging).

## 10. Standard Passives (assortment)

All in 0603 (capacitive 0603/0805 where needed). They are linked per location in
the generator with an LCSC number; the assortment here is for reference only:

| Class | LCSC examples |
|---|---|
| Resistors 0603 1 % | C21190 (1 k), C25804 (10 k), C23186 (5.1 k), C23146 (36 k), C23153 (39 k), C23162 (470), C23345 (22), C25803 (100 k), C25811 (200 k), C4184 (20 k), C22975 (2 k), C31850 (22 k) |
| Caps 0603 X7R / X5R | C14663 (100 n), C57112 (10 n), C46653 (4.7 µ X5R), C1588 (1 n), C1804 (auto-generated), C15849 (1 µ X5R), C14858 (10 n B) |
| Caps 0805 | C15850 (10 µ X5R), C45783 (22 µ X5R) |
| MLCC 1210 | C2880380 (100 µF/10V X5R, C_BULK2); C444831 (470µF/10V polymer-tantalum case-E, C_BULK) |
| Caps specific | **C23630** (2.2 µF VCAP, 16 V X5R — r18.70: was C24539, not in JLC assembly); C107045 (27 pF C0G, HSE 2×); C15849 (1 µF, VREF+ & VDDA); C14663 (100 nF, VREF+ & VDDA) |

All standard FPs: `Resistor_SMD:R_0603_1608Metric`, `Capacitor_SMD:C_0603_1608Metric`,
`Capacitor_SMD:C_0805_2012Metric`, `Capacitor_SMD:C_1210_3225Metric` — all KiCad-Standard,
Standard-Lib-3D.

## 11. SWD Service Header

| Ref | MPN | Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **J4** TC2030-IDC 3-pin | Tag-Connect TC2030-IDC | [Tag-Connect shop](https://www.tag-connect.com/product/tc2030-idc) | `Connector_PinHeader_1.27mm:PinHeader_1x03_P1.27mm_Vertical` | KiCad-Standard | Vendor-CAD |

## 12. Mechanics (enclosure + knobs) — **post-PCB, not a schematic/layout blocker**

> All entries here are enclosure/hand hardware. They do NOT affect the PCB
> layout (no holes, no pads, no height constraints other than those already
> validated in `mechanical/coordinates/mechanical_coordinates.md`). The PCB
> sprint can start and finish without these files — they are produced only after
> schematic/layout sign-off.

| Element | Choice | Source | Status |
|---|---|---|---|
| Enclosure outer 260 × 110 × 21.6 mm | ABS / PC injection molding 2.5 mm (top + bottom) | TBD (industrial design sprint) | post-PCB |
| 4× mounting hardware M2.5 | (standard hex standoff 3 mm) | RS / Reichelt | post-PCB |
| Encoder knobs (4×) Ø 19–20 × 8–10 mm | **self 3D-printed** (r18.21) | user | post-PCB |
| Cell caps (5×, 1u) | **self 3D-printed** (r18.21) | user | post-PCB |
| Plate for cell switches (optional, r18.75) | Direct-solder Choc builds are often plateless (the switch's own locator pegs give lateral stability), but a plate adds rigidity for a frequently-pressed instrument — TBD-CAD | industrial design | post-PCB |
| **Power slider cap** (orange, engages SW_PWR) | bears the user's actuation force so the SMD switch only gets a gentle nudge (see SW_PWR §2) | TBD-CAD | post-PCB |

---

## Ordering Strategy

- **JLCPCB assembly (Extended OK):** everything in §1–§5, §8 (except SW_BOOT,
  hand-place by choice), §9, §10 — all SMD with an LCSC ID. **r18.75: the
  DRV5056A4 Hall sensors are removed. The 5 cell keyswitches (SW1–SW5, Kailh
  Choc V1 CPG135001D01, LCSC C400229) are THT** — same as the modifiers,
  order via LCSC/JLC (currently 0 stock, restocks common) and hand-solder,
  no special socket needed. The 5 modifier buttons (SW6–SW10, HX B3F-4055,
  C36498965) stay JLC-orderable, hand-place after reflow.
- **Order separately, ship to the JLC address (or your own), hand-place yourself:**
  encoders (EN1–4), the 5 modifier tactile switches (SW6–10), the 5 cell
  keyswitches (SW1–5, LCSC C400229 or keyboard-market equivalent), speaker,
  Adafruit display module, Tag-Connect SWD header, mesh, knobs.
- **Never via JLC:** mesh + custom 3D-printed knobs/caps. These are custom-CAD
  parts.

## Verification Audit Trail

Who checks what, when:

| Item | Status | Reference |
|---|---|---|
| All footprints checked | ✅ **6 custom** in [`field_ambience.pretty/`](field-ambience-current/kicad/libraries/field_ambience.pretty/) — all actively referenced (Y1, J8, L1, U8, SW6-10, SW11/SW_BOOT); the rest KiCad-Standard. No orphans | [`field-ambience-current/FP_VERIFY_LOG.md`](field-ambience-current/FP_VERIFY_LOG.md) |
| 3D STEPs for Z-/panel-critical parts | ✅ **7 of them** in [`field_ambience.3dshapes/`](field-ambience-current/kicad/libraries/field_ambience.3dshapes/) (U1, Y1, U8, L1, J9 battery, J8, SW11/SW_BOOT). USB-C receptacle uses the KiCad-Standard STEP (no custom STEP needed). Z-height table per STEP (for enclosure CAD): [`mechanical/3d_models/MANIFEST.md`](mechanical/3d_models/MANIFEST.md) | r18.36 |
| Mechanical X/Y/Z + height constraints | ✅ Python-validated, 0 conflicts | `mechanical/coordinates/mechanical_coordinates.md` |
| ~~DRV5056 pinout DS-confirmed~~ | ⛔ **SUPERSEDED r18.73** — Hall path removed; cells are now digital switches on the MCP23017 (see §7) | r18.14b → r18.73 |
| Cells → digital I²C switches | ✅ SW1–SW5 on MCP23017 GPA0–GPA4; DRV5056A4 + RC removed; PC0/PC1/PA4/PB0/PB1 freed. Restores SPEC v0.6 §7 "10 switches". | r18.73 |
| Cells → real keyswitch feel (UX correction) | ✅ r18.73 had put cells on the same small HX B3F tactile as the modifiers — made cells feel identical to a plain button. r18.74 tried a Kailh Choc hot-swap socket, but that socket had no clean MPN and needed fiddly hand-soldering. r18.75: switched to **direct-solder** Kailh Choc V1 (CPG135001D01, LCSC C400229) — real verified part, footprint + STEP pulled straight from LCSC/EasyEDA, plain THT soldering like every other button here. No hot-swap (switch is now permanent). Electrical topology unchanged since r18.73. Modifiers (SW6–10) unchanged. | r18.75 |
| BOM/functionality audit — J3/J6/J7 LCSC drift | ✅ **Found + fixed:** the generator had placeholder `"TBD"` in the LCSC field for J3 (LCD header) and J6/J7 (speaker headers) even though this file already documented the real codes (**C124383**, **C124375**) — doc↔generator drift, now synced. Also verified: 0 duplicate ref-designators across all 201 placed instances; `jlc_bom.csv` parses cleanly with embedded commas; generator output deterministic across repeated runs. The remaining bare-`"TBD"` (J4 Tag-Connect) is a genuine no-LCSC vendor part, already audited r18.37. | r18.76 |
| BOM/functionality audit — firmware cell-read gap (⚠ NOT fixed, hardware-only session) | The STM32H743 target (`src/hal_h743/main_h743.c`) still has a TODO stub describing the retired Hall-ADC cell-read path (`adc_read_norm`) — stale since r18.73 moved cells to digital MCP23017 GPIO. `src/hal_pico/main_pico.c` already has a correct, tested digital implementation (MCP-GPIO edge detection, bypasses the old velocity state machine, mirrors the modifier-button pattern) that the H743 loop needs to be ported to. Without this, cells will not function on real H743 hardware if the current TODO is implemented as written. Flagged for a firmware-focused follow-up. **→ GESCHLOSSEN r18.83:** main_h743 liest die Cells jetzt digital vom MCP23017 (GPA0-4-Edges → controls_cell_press/release, wie der Pico-Bench-Pfad); der stale Hall-TODO ist entfernt. | r18.76 → r18.83 |
| Elektrik-Audit r18.79 — Boost + Power-Path | ✅ **3 kritische Funde, alle behoben (gegen TI-Datenblatt SLVSD38C verifiziert):** (1) FB-Teiler R23/R24 200k/39k ergab **VOUT 7,43 V** statt 5 V (VREF 1,212 V) — hätte PAM8403 (5,5 V abs max) + TPS22918 zerstört → R23 = 121k (C25809, live verifiziert) → 4,97 V. (2) R_ILIM 20k = **51,5 A Stromlimit** (Gl. 4: 1,03e6/R; Kommentar behauptete „~4 A“) = faktisch kein Limit, Induktor (Isat 6,75 A) ungeschützt → 174k (C22890, live verifiziert) → 5,92 A typ. (3) Q1-Power-Path speiste ohne USB die Boost-5V zurück auf VBUS (Body-Diode + Gate-an-GND → voll durchgeschaltet) → Lader lud Akku aus eigenem Boost (Selbstentladeschleife), USB-Detect dauerhaft HIGH, 5 V am offenen Stecker; zusätzlich überbrückte Q1 die Sicherung F1 (pre-fuse→post-fuse) → Q1+R22 entfernt, **Dioden-OR** (D3B = 2. SS34 hinter F1). Dazu: R_FSW-Kommentar korrigiert (360k = ~440 kHz per Gl. 3, nicht 1,21 MHz — Widerstand ok). Strukturchecks: alle Hier-Labels ↔ Root-Sheet-Pins matchen, Root-Verdrahtung verbindet alle Partner-Pins (nach Label-Merge), 0 Refdes-Duplikate, I²C-Adressen kollisionsfrei (0x20/0x40/0x41, U10-A0 an +3V3 verifiziert), BOOT0-Polarität korrekt (10k Pull-down + 1k-Serie), CC1/CC2 je eigener 5,1k, FSW-an-SW laut DS korrekt. | r18.79 |
| Must-have-Check r18.84 — /OE + Mechanik | ✅ **KRITISCH: R_OE/R_OE2 zogen beide PCA9685-/OE auf +3V3 ohne Treiber-Netz** (Kommentar „Firmware zieht LOW" — es gab keinen Pin dafür) → alle LEDs/Backlight/VU dauerhaft disabled → **Pull-DOWN** (Boot-Dark per NXP-DS Tab. 7: LEDn_FULL_OFF Default=1*, PDF-verifiziert). **Ergänzt:** H1–H4 M2.5-Mounting-Holes (mech. Spec §6, fehlten im Schematic; DNP, GND, FP-Note 5,4-vs-6-mm-Pad) + 7 Testpunkte TP_5V/3V3/5VSW/VBUS/GND1/GND2/BAT (AI_READY-Standard; DNP). Vollständigkeits-Sweep sonst ohne Befund (USB-Schutzkette, NRST/BOOT0, VCAP, HSE, I²C-Pull-ups, SWD, Status-LED, Backlight-Pfad). Kaufnote: LiPo-Pouch MUSS eigenes Protection-PCM haben. | r18.84 |
| Datenblatt-Verifikation r18.82 — alle restlichen Komponenten | ✅ **2 kritische Pad-Mapping-Fehler gefunden (KiCad matcht Symbol-Pin-Nummer↔Pad-Name als String):** (1) **alle 4 Encoder komplett unverbunden** — offizieller KiCad-EC11E-Footprint hat benannte Pads A/B/C/S1/S2, Symbol nutzte 1–5 → korrigiert (Pad-Geometrie gegen GitLab-Footprint: Common C = Mittelpad ✓); (2) **PJ-320D (J8 Line-Out + J10 MIDI): Pad-Map falsch** — laut SHOU-HAN-Zeichnung Pad 1=A=Sleeve, 2=D=Ring, 3=C=Detect, 4=B=Tip; Symbol hatte T=1/R=2/S=3/DET=4 ⇒ Audio-L auf dem geerdeten Barrel, DETECT auf dem Tip → T=4/R=2/S=1/DET=3 + **R_DET_J8 10k + C_DET_J8 1µF** (Detect ruht unplugged am TIP=DAC-Ausgang → Clamp-Schutz; Pegel unplugged≈0,3V LOW / plugged 3,3V HIGH). Vendored-Jack-Footprint-Geometrie = Zeichnung exakt → **TODO B0b geschlossen**. **2 Lücken:** MCP73831 ohne DS-4,7µF-VDD-Cap → C_CHG_IN (C46653); STM32-VSS-99 nur zufallsgeerdet → explizit (DS12110 Fig. 5: VSS=10/26/49/74/99). **Ohne Befund:** MCP23017-SSOP (DS20001952: SSOP=SOIC-Pinout), PCA9685 TSSOP28 (Tab. 3, U6+U10), USBLC6 (1/6+3/4-Paarung), komplette H743-LQFP100-Map (Fig. 5), ABLS CL=18pF→27pF ✓, D2-TVS-Polarität, USB-C-Pads A1…B12/S1, HX-B3F-4055-Paarung ①②/③④=Footprint-Reihen ✓ (Lochabstand 5,08 vs DS 5,00 — Ø1,5-Drill absorbiert, Layout-Nicety). | r18.82 |
| ADR-0016 Power-Off r18.81 — Block war NIE im Schematic + „+5V"-Insel | ✅ **User-Frage zum Side-Switch deckte 2 kritische Lücken auf:** (1) der beschlossene Power-Off-Block (U_PWR TPS22918 + SW_PWR MST-12D18G3 + R_PWR_PD + C_PWR_SW) existierte nur in Doku/HTML, nicht im Generator/jlc_bom — **Board hätte keinen Ein/Aus gehabt** → r18.81 in power_tree verdrahtet (+5V_RAIL → U_PWR → +5V_SW → LDO-VIN; QOD→VOUT = aktive Entladung, CT floatet — beides gegen TI SLVSD76C korrigiert, ADR nannte Pin 4 fälschlich „NC"); (2) **globales „+5V"-Netz hatte auf keinem Sheet eine Brücke zur Rail** — PAM8403 + 23 LED-Anoden + LED_CHRG quellenlos → Rail trägt jetzt das +5V-Flag. **SW_PWR-Seitenmontage datenblatt-verifiziert** (Stem horizontal, 3 mm über Body-Kante, z ≈ 2,3–3,8 mm, Travel 2 mm; Footprint = MSK12D exakt). Netz-Trace 11/11 ✓, ERC 0 Fehler. OFFEN: Terminal-Common=Mitte durchpiepen (fail-safe). | r18.81 |
| Geometrisches Pin-Level-ERC r18.80 — 12 Funde, alle behoben | ✅ **Headless ERC (Python: jeder Pin-Anker vs. jedes Wire-Segment + Union-Find-Netzbau) über alle 7 Sheets. 7 Kupfer-Kurzschlüsse:** USB D+/D− auf +5V (U5-LDO-Block im USB-Korridor → verlegt), I2C_SCL auf GND (C5-GND-Pin auf SCL-Wire — 7,62-mm-Pitch-Falle: VDD- und SCL-Pin exakt 3 Rasterzeilen auseinander), PCM_XSMT + I2S_LRCK auf GND (gleiche Falle, audio), BOOT0 permanent HIGH (2 Fehler: +3V3-Flag auf BOOT0-Punkt + SW_BOOT-Wire durch Pin-2-Anker → H7 bootet nie Firmware), VDDA↔VDD verkupfert (Pin-21-Stub durch Pin-100-Anker), C_BOOT-Bootstrap via Label auf Wire-Kreuzung. **4 Unterbrechungen:** LCD-J3 alle 8 Pins + SWD-J4 alle 3 Pins floatend (Custom-Lib-Pins rechts, Verdrahtung nahm −5.08 links an), FB2 beidseitig floatend (rot=90), MCU-lib_symbols ohne „MCU:"-Prefix (KiCad löst U1 nicht auf). **1 Regelkreis:** R_COMP/C_COMP 22k/1nF → 6,2k/10nF + COUT 1×→3× 22 µF (fc lag ÜBER fRHPZ — TI Gl. 15/17/18, gegen TI-Referenzdesign validiert; neues Teil C4260 live verifiziert). MPN-Hygiene: C45783=22µF/C15850=10µF (live verifiziert; VDD-Bank war als 4,7 µF beschriftet), 15 MPN↔LCSC-Mismatches vereinheitlicht. U10 LED8–15 mit No-Connects. Nach Fix: **0 ERC-Fehler auf allen Sheets**, Hier↔Root-Pins matchen, Generator deterministisch. kicad-cli 7 kann v9-Format nicht laden → KiCad-9-GUI-ERC bleibt Human-Gate. | r18.80 |
| AP7361C pinout user-confirmed | ✅ Diodes DS | r18.6 |
| TPS61089 pin 11 = SW + thermal | ✅ TI DS | r18.7 |
| Speaker diaphragm (cloth vs paper) | ✅ cloth cone primary | r18.18 |
| USB-C LCSC ID correct | ✅ C165948 (16-pin TYPE-C-31-M-12). r18.10 had "upgraded" to M-17, r18.14 corrected the LCSC number from C165935 (MOSFET) to C283540, r18.19 audit found C283540 = 6-pin power-only → revert to M-12 | r18.19 |
| Audio-jack footprint | ✅ `field_ambience:Jack_3.5mm_PJ-320D_SMT` (EasyEDA CAD vendored, 4 SMD + 2 NPTH). Previously CUI SJ-3523-SMT-FP assigned — pad layout did not match C431535 (SHOU HAN) | r18.19 |
| Speaker value in schematic | ✅ `value="CMS-402811-28SP Cloth-Cone 8R 2W"` (J6/J7). Previously stale "PUI AS04008PS 4R 4W" — wrong impedance + wrong driver | r18.19 |
| SW_BOOT MPN correct | ✅ TS-1088-AR02016 (was wrongly TS-1185A) | r18.14 |
| CAD files clickably linked + 3D-column consistency | ✅ all 7 STEPs + 6 actively used custom footprints entered as relative Markdown links; J1 USB-C ("STEP in repo" → "Standard-Lib-3D") and SW11/SW_BOOT ("Vendor-CAD" → "STEP in repo") corrected; unreferenced orphan footprint `RotaryEncoder_ALPS_EC11J_SMD` deleted. **The PCB path is therefore schematic/layout-ready — no open CAD TODOs for PCB** | r18.36 |
| ~~Hall-sensor FP-status doc drift fixed~~ | ⛔ **SUPERSEDED r18.73** — J_CELL1–5 Hall sensors no longer on the board (cells went digital, see §7). | r18.37 → r18.73 |
| LCSC string diff BOM ↔ generator | ✅ performed: 60 BOM codes vs 53 generator codes. **0 real mismatches** — all diffs are either false positives (Tag-Connect "TC2030"), audit-trail history (old wrong LCSC IDs), or dead code in `pi_sheet()` (LEGACY, not written). Re-run before order recommended. Details in `PCB_FOOTPRINT_RISK_AUDIT.md` §9. **⚠ Caveat found r18.77:** this diff only catches BOM↔generator *disagreement* — it cannot catch a code that's wrong in **both** places identically (see L1 below). A live-fetch check against the real LCSC page is needed too. | r18.37 |
| L1 boost inductor: LCSC code was a dead link | ✅ **Found + fixed:** `C83455` (in both `BOM_MASTER.md` and the generator, hence invisible to the r18.37 string-diff) 404s on LCSC — it was never a valid listing, despite an earlier r18.20c note claiming it was "✓ Extended" verified. The MPN was also wrong (`SWPA6045S2R2MT` doesn't exist for 2.2 µH per Sunlord's own datasheet — only the `N`-tolerance suffix is offered below 4.3 µH). Real part verified live: **SWPA6045S2R2NT, LCSC C36500**, cross-checked against every value in Sunlord's official SWPA6045S datasheet (Item 12 table: 2.2 µH / 18 mΩ / 7.40 A / 5.00 A). Also fixed a stale `value` string in the schematic still describing an even older Sumida CDR63B-2R2 "0630"-case part from before the design switched to the Sunlord SWPA6045 (6.0×6.0×4.5 mm, not "0630" = 6.0×3.0mm). | r18.77 |
| Mechanics section (§12) clearly marked post-PCB | ✅ custom 3D-print files (encoder knobs, cell caps, switch plate, enclosure) are **not PCB-blocking** — they come after schematic/layout sign-off and do not affect the board (no holes / no pad pattern). §12 annotated accordingly | r18.36 |
| 3D STEP export completeness (for enclosure CAD) | 🟡 **1 known gap:** HX modifier buttons SW6–SW10 (C36498966) have **no 3D model** (EasyEDA has none, `easyeda2kicad` re-check 2026-06-27). Envelope **11.8×11.8×7.3 mm** in [`mechanical/3d_models/MANIFEST.md`](mechanical/3d_models/MANIFEST.md) → enclosure clearance covered. All others: 7× STEP-in-repo or KiCad-Standard-Lib-3D. **Footprint present → not a PCB layout blocker.** Off-board bodies (speaker/battery/display/knobs) are external (Vendor-CAD, see §2/§3/§5/§6). | r18.65 |
| MECHANICAL_REQUIREMENTS §1.6 modifier-button part corrected | ✅ §1.6 wrongly listed TS-1088 (3.9×2.9, service button) instead of the HX 12×12×7.3 (SW6–SW10, generator line 4111) → height 7.3 mm instead of a wrong ~2 mm. Enclosure clearance is now correct | r18.65 |
| SW_PWR power slide switch — type + mechanics | ✅ **Type + electrical correct** (right-angle, side-actuated SMD slide, SPDT, switches only U_PWR.ON / load-switch EN, µA). Part has **tab mounting pegs**; footprint **is vendored** (`field_ambience:SW_MST-12D18_SlideSwitch_RA`, 3 SMD + 2 peg holes + STEP). 🟡 **Pre-fab:** verify pad pitch/peg positions vs datasheet; decoupled enclosure slider mandatory for robustness (see §2). | r18.69 |
