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
| Q1 | **DMG2305UX-7** P-Ch MOSFET | [C150470](https://www.lcsc.com/product-detail/C150470.html) | `Package_TO_SOT_SMD:SOT-23` | KiCad-Standard | Standard-Lib-3D |
| **U5** LDO | **AP7361C-33Y5-13** SOT-89-5 (pinout 1=EN, 2=GND, 3=ADJ, 4=IN, 5=OUT verified) | [C460397](https://www.lcsc.com/product-detail/C460397.html) | `Package_TO_SOT_SMD:SOT-89-5` | KiCad-Standard | Standard-Lib-3D |
| **U_PWR** power-off load switch (ADR-0016) | **TPS22918DBVR** SOT-23-6 — gates the LDO input `+5V_RAIL → +5V_SW` (= the whole 3V3 domain: MCU + MCP + LEDs + LCD). ON comes from `SW_PWR`. Charger U7 sits **ahead** of it → keeps charging when off. "Dark, but charging." | [C131941](https://www.lcsc.com/product-detail/C131941.html) *(check PN/stock before order)* | `Package_TO_SOT_SMD:SOT-23-6` | KiCad-Standard | Standard-Lib-3D |
| **SW_PWR** slide switch = main power-off | **MST-12D18G3** (SHOU HAN) — right-angle SMD slide switch (SPDT, **side-actuated** from the enclosure edge; ~3.5×6.5×9.1 mm; **has tab mounting pegs** = through-board mechanical anchors, not just solder pads). Switches **only** `U_PWR.ON` (signal level, µA). 10 k cycles / 100 mA @ 12 V — ample for a power switch. **Mechanical requirements (for robustness):** (1) decouple actuation — the enclosure's own slider cap takes the user's force, the switch lever is only gently engaged; (2) the footprint **must** include the tab-peg NPTH holes + board-edge placement; (3) optional enclosure rib capturing the switch body. Upgrade options if more robustness wanted: DPDT **MST-12D18G4** ([C49023767](https://www.lcsc.com/product-detail/C49023767.html)) or a C&K JS-series right-angle J-bend. | [C49023766](https://jlcpcb.com/partdetail/C49023766) (JLC Extended) | [`field_ambience:SW_MST-12D18_SlideSwitch_RA`](field-ambience-current/kicad/libraries/field_ambience.pretty/SW_MST-12D18_SlideSwitch_RA.kicad_mod) — **vendored**: 3 SMD pads (2.5 mm pitch, SPDT) + 2 thru-hole peg anchors. ⚠️ verify pad pitch + peg positions against the MST-12D18G3 datasheet before fab (footprint-hygiene, not a blocker) | field_ambience | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/SW_MST-12D18_SlideSwitch_RA.step) |
| **R_PWR_PD** 100 kΩ 0603 (`U_PWR.ON` pull-down → default OFF) | 0603WAF1003T5E | [C25803](https://www.lcsc.com/product-detail/C25803.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| **C_PWR_SW** 10 µF X5R 0805 (output cap on `+5V_SW`) | LMK212ABJ106 class | [C15850](https://www.lcsc.com/product-detail/C15850.html) | `Capacitor_SMD:C_0805_2012Metric` | KiCad-Standard | Standard-Lib-3D |
| **U7** charger | **MCP73831T-2ACI/OT** SOT-23-5 | [C424093](https://www.lcsc.com/product-detail/C424093.html) | `Package_TO_SOT_SMD:SOT-23-5` | KiCad-Standard | Standard-Lib-3D |
| **C_BULK** polymer-tantalum 470 µF / 10 V case-E 7343-43 (ESR 100 mΩ; <25 mΩ flat parts not in stock at LCSC — transient ESR is supplied by C_BULK2) | TPSE477K010R0100 (Kyocera AVX) | [C444831](https://www.lcsc.com/product-detail/C444831.html) | `Capacitor_SMD:CP_Tantalum_Case-E_EIA-7343-43_Reflow` | KiCad-Standard | Standard-Lib-3D |
| **C_BULK2** MLCC **100 µF / 10 V** 1210 (parallel; 220µF/10V/1210 does not exist → 100µF of real headroom instead of 220µF/6.3V derating) | CL32A107MPVNNNE (Samsung) — r18.70: was C2880380 (Taiyo Yuden, JLC stock ≈1) | [C23742](https://www.lcsc.com/product-detail/C23742.html) | `Capacitor_SMD:C_1210_3225Metric` | KiCad-Standard | Standard-Lib-3D |
| **J9** JST-PH 2.0 2-pin (LiPo battery — generator refdes, **not** J_BAT) | **S2B-PH-SM4-TB(LF)(SN)** | [C295747](https://www.lcsc.com/product-detail/C295747.html) | `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal` | KiCad-Standard | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/CONN-SMD_P2.00_S2B-PH-SM4-TB-LF-SN.step) |
| LiPo pouch **503759** (50×37×9.4 mm, **2000 mAh ~6.6 h @ 300 mA**) — r18.21 right-sized down from 5000mAh overkill | Generic LiPo 2000mAh JST-PH 2.0 | [PiHut 2000mAh](https://thepihut.com/products/2000mah-3-7v-lipo-battery) | — (no PCB FP, bottom-case slot) | — | Vendor |

> **Power architecture / rail ownership (ADR-0016, audit #8/#9).** So the PCB
> maker sees the sources unambiguously — *one* source→system-rail place each:
> - `VBUS_USB` (USB-C 5 V, via F1 PTC) ──┐
> - `BAT_PLUS` (LiPo) → boost **U8** → `+5V_BOOST` ──┤→ power-path **Q1** → **`+5V_RAIL`** (system 5 V; feeds amp U4 + U_PWR)
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
| **J8** line-out 3.5 mm TRS | **PJ-320D** (SHOU HAN, SMT) with insertion-detect | [C431535](https://www.lcsc.com/product-detail/C431535.html) | [`field_ambience:Jack_3.5mm_PJ-320D_SMT`](field-ambience-current/kicad/libraries/field_ambience.pretty/Jack_3.5mm_PJ-320D_SMT.kicad_mod) (EasyEDA CAD vendored r18.19) | field_ambience (custom) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/AUDIO-SMD_PJ-320D-1.step) |
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
| **U10** PWM LED driver #2 | **PCA9685PW,118** TSSOP-28 (r18.66 — level meter: 8 VU LEDs on ch 0-7. I²C **0x41** (A0=+3V3), same bus as U6/MCP. Decoupling 10µF+100nF, /OE pull-up R_OE2 10k) | [C2678753](https://www.lcsc.com/product-detail/C2678753.html) | `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm` | KiCad-Standard | Standard-Lib-3D |

## 5. Display

| Ref | MPN | Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **J3** 1×8 header 2.54 mm (LCD module) | 2.54 mm strip cut to 1×8 (B-2100S40P or similar) — or 1×8 female socket for a pluggable module | [C124383](https://www.lcsc.com/product-detail/C124383.html) | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | KiCad-Standard | Standard-Lib-3D |
| **LCD module** **Waveshare 1.9in 170×320 ST7789V2** IPS module (r18.22 pivot from bare AliExpress: same panel as Adafruit + branded QC + level shifter + documented; ~$11–13 instead of Adafruit $15 / bare-AliExpress DOA lottery. **Verify the module's pin ORDER against the J3 pinout** — varies by vendor) | Waveshare 1.9inch LCD Module 170x320 SPI | [PiHut £11.60](https://thepihut.com/products/1-9-ips-lcd-display-module-170x320-spi) · [Waveshare](https://www.waveshare.com/1.9inch-lcd-module.htm) · [Amazon](https://www.amazon.com/Waveshare-1-9inch-Display-Resolution-Interface/dp/B0BRXXSZC5) | Module (plugs into J3) | — | Vendor-CAD |
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
| **Cell switch (SW1–SW5, 5×)** | **Kailh Choc V1, CPG135001D01** (red/linear) — direct-solder THT, 2 electrical pins + 3 unplated mechanical locator holes (switch's own pegs give lateral stability without a plate). One pin → MCP23017 GPA0–GPA4 (`CELL1..5_BTN`), other → GND; MCP internal pull-up, shared INT. Other Choc V1 colors (tactile/clicky) share the same footprint — feel/color is industrial-design choice. [Datasheet](https://cdn-shop.adafruit.com/product-files/5113/CHOC+keyswitch_Kailh-CPG135001D01_C400229.pdf) | [C400229](https://www.lcsc.com/product-detail/C400229.html) — real, verified listing. ⚠ 0 stock at time of writing (restocks common); JLC notes their automated PCBA needs a custom fixture for this THT part — hand-soldering the 2 legs needs neither. | `field_ambience:SW_KailhChoc_CPG1350_THT_2P` — pulled directly from LCSC/EasyEDA via `easyeda2kicad --full --lcsc_id=C400229` | repo custom, sourced from the part's own manufacturer listing (not a third-party library) | [STEP](field-ambience-current/kicad/libraries/field_ambience.3dshapes/SW_KailhChoc_CPG1350_THT_2P.step) |
| **Cell caps (5×) 1u, self 3D-printed** (clip onto the Choc V1 switch stem) | own 3D print | — | — | — | Custom-CAD (user) |

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
| **LED_HB** heartbeat (warm white) | **XL-1608UWC-04** (r18.24 fix: was C965818 = XL-2012UWC **0805**, wrong package) | [C965808](https://www.lcsc.com/product-detail/C965808.html) | see above | KiCad-Standard | Standard-Lib-3D |
| **LED_CHRG** charger status (orange) | **XL-1608UOC-06** (XINGLIGHT, orange 605 nm 0603) — r18.70: was C72041, which is actually a **blue + discontinued** LED with stock ≈4 (wrong part, caught by JLC API check) | [C965800](https://www.lcsc.com/product-detail/C965800.html) | see above | KiCad-Standard | Standard-Lib-3D |
| LED series resistors **390 Ω** 0603 (15×, on +5V anode → LED → PCA9685 sink) | 0603WAF3900T5E | [C23151](https://www.lcsc.com/product-detail/C23151.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |

> **Color mapping (r18.64, user spec):** Shift=yellow, Hold=green, Generate/
> Drone/Clear=white. This swaps Shift/Hold versus the previous firmware
> convention (which coupled the modifier color to the cell-hold color:
> base-hold=yellow, shift-hold=green). `firmware-c-next/src/leds.c` is adapted to
> the user spec.

### Live level meter (VU — r18.66, OP-1-Field style)

> **8 small LEDs in a row as a live level indicator** (like the dB swing in
> After Effects during playback). Driven by **U10** (2nd PCA9685, §4) on
> channels 0-7; each with one 390 Ω series resistor on +5V anode → U10 sink. The
> firmware computes RMS/peak from the audio buffer and drives the segments via
> PWM (a real meter, not just a knob position). **Row position on the PCB =
> layout TBD.**

| Function | MPN | LCSC/Link | Footprint | FP source | 3D |
|---|---|---|---|---|---|
| **VU LEDs 8× white** (LED_VU1–8, r18.68 — user: white instead of blue) | XL-1608UWC-04 (warm-white 0603) — same LED as the heartbeat; **no separate blue LED needed** | [C965808](https://www.lcsc.com/product-detail/C965808.html) | `LED_SMD:LED_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |
| VU series resistors **390 Ω** 0603 (8×, R_VU1–8) | 0603WAF3900T5E | [C23151](https://www.lcsc.com/product-detail/C23151.html) | `Resistor_SMD:R_0603_1608Metric` | KiCad-Standard | Standard-Lib-3D |

## 10. Standard Passives (assortment)

All in 0603 (capacitive 0603/0805 where needed). They are linked per location in
the generator with an LCSC number; the assortment here is for reference only:

| Class | LCSC examples |
|---|---|
| Resistors 0603 1 % | C21190 (1 k), C25804 (10 k), C23253 (820), C23186 (5.1 k), C23146 (36 k), C23153 (39 k), C23162 (470), C23345 (22), C25803 (100 k), C25811 (200 k), C4184 (20 k), C22975 (2 k), C31850 (22 k) |
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
| BOM/functionality audit — firmware cell-read gap (⚠ NOT fixed, hardware-only session) | The STM32H743 target (`src/hal_h743/main_h743.c`) still has a TODO stub describing the retired Hall-ADC cell-read path (`adc_read_norm`) — stale since r18.73 moved cells to digital MCP23017 GPIO. `src/hal_pico/main_pico.c` already has a correct, tested digital implementation (MCP-GPIO edge detection, bypasses the old velocity state machine, mirrors the modifier-button pattern) that the H743 loop needs to be ported to. Without this, cells will not function on real H743 hardware if the current TODO is implemented as written. Flagged for a firmware-focused follow-up. | r18.76 |
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
