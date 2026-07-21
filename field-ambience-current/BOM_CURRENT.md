# Field Ambience — Production BOM (current)

**Generated:** 2026-07-20 · **Source of truth:** `kicad/generate_kicad_project.py` → `kicad/jlc_bom.csv`

**On-PCB totals:** 59 unique parts · 200 placements · 59 verified LCSC · 0 without LCSC

> Audio chain current as of **r19.37** (PCM5102A + **PAM8406** + TPA6132A2; PAM8403 was NRND — swapped, ADR-0025). Power-path current as of **r19.18** (BQ24074).

## 1 · MCU · clock · memory

| Qty | Ref(s) | Part / MPN | Package | LCSC |
|----:|--------|------------|---------|------|
| 2 | C_HSE1,C_HSE2 | CC0603JRNPO9BN270 | C_0603_1608Metric | C107045 |
| 1 | U1 | STM32H743VIT6 | LQFP-100_14x14mm_P0.5mm | C114409 |
| 1 | U9 | APS6404L-3SQN-SN | SOIC-8_3.9x4.9mm_P1.27mm | C3028887 |
| 1 | Y1 | ABLS-8.000MHZ-B4-T | Crystal_HC49-US-SMD_ABLS | C596838 |

## 2 · Power

| Qty | Ref(s) | Part / MPN | Package | LCSC |
|----:|--------|------------|---------|------|
| 3 | C_CHG_IN,C_LDO_IN,C_LDO_OUT | GRM188R61A475KE15D | C_0603_1608Metric | C46653 |
| 3 | R_BOOT_SW,R_CHRG,R_ISET | 0603WAF1001T5E | R_0603_1608Metric | C21190 |
| 2 | R_PWR_PD,R_VBUS_PD | 0603WAF1003T5E | R_0603_1608Metric | C25803 |
| 1 | C_BULK | TPSE477K010R0100 (Kyocera AVX, Polymer-Tantal) | CP_Tantalum_Case-E_EIA-7343-43_Reflow | C444831 |
| 1 | C_BULK2 | CL32A107MPVNNNE (Samsung, 100uF 10V X5R) | C_1210_3225Metric | C23742 |
| 1 | D2 | SMAJ5.0A | D_SMA | C113952 |
| 1 | D3 | SS34 | D_SMA | C8678 |
| 1 | F1 | 1812L300/16GR | Fuse_1812_4532Metric | C18198349 |
| 1 | F2 | SMD1812P260TF/16 | Fuse_1812_4532Metric | C438899 |
| 1 | L1 | SWPA6045S2R2NT | L_Sunlord_SWPA6045 | C36500 |
| 1 | R23 | 0603WAF1213T5E | R_0603_1608Metric | C25809 |
| 1 | R24 | 0603WAF3902T5E | R_0603_1608Metric | C23153 |
| 1 | R_COMP | 0603WAF6201T5E | R_0603_1608Metric | C4260 |
| 1 | R_FSW | 0603WAF3603T5E | R_0603_1608Metric | C23146 |
| 1 | R_ILIM_IN | RC0603FR-071K2L | R_0603_1608Metric | C114605 |
| 1 | SW_PWR | SSSS811101 | SW_ALPS-SSSS811101_SlideSwitch_SMD | C109335 |
| 1 | U5 | AP7361C-33Y5-13 | SOT-89-5 | C460397 |
| 1 | U7 | BQ24074RGTR | QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm | C54313 |
| 1 | U8 | TPS61089RNR | Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A | C165129 |
| 1 | U_PWR | TPS22918DBVR | SOT-23-6 | C131941 |

## 3 · Audio

| Qty | Ref(s) | Part / MPN | Package | LCSC |
|----:|--------|------------|---------|------|
| 5 | C5b,C_BAT_FILT,C_COMP,C_in_L,C_in_R | 0603B103K500NT | C_0603_1608Metric | C57112 |
| 3 | R_ILIM,R_VOL_L,R_VOL_R | 0603WAF1743T5E | R_0603_1608Metric | C22890 |
| 2 | C_HPVDD,C_HP_VDD | CL10A225KP8NNNC | C_0603_1608Metric | C1607 |
| 2 | FB1,FB2 | BLM18AG601SN1D | L_0603_1608Metric | C19330 |
| 2 | J8,J10 | PJ-320D (3.5mm TRS w/ switch) | Jack_3.5mm_PJ-320D_SMT | C431535 |
| 2 | R_LO_L,R_LO_R | 0603WAF220JT5E | R_0603_1608Metric | C23345 |
| 1 | U11 | TPA6132A2RTER | QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm | C69901 |
| 1 | U3 | PCM5102APWR | TSSOP-20_4.4x6.5mm_P0.65mm | C107671 |
| 1 | U4 | PAM8406DR | SOIC-16_3.9x9.9mm_P1.27mm | C86270 |

## 4 · I/O expander + LEDs

| Qty | Ref(s) | Part / MPN | Package | LCSC |
|----:|--------|------------|---------|------|
| 15 | R_LED6,R_LED7,R_LED8,R_LED9,R_LED10,R_LED11G,R_LED11Y,R_LED12G,R_LED12Y,R_LED13G,R_LED13Y,R_LED14G,R_LED14Y,R_LED15G,R_LED15Y | 0603WAF3900T5E | R_0603_1608Metric | C23151 |
| 6 | LED6,LED11Y,LED12Y,LED13Y,LED14Y,LED15Y | KT-0603Y (Hubei KENTO, yellow 0603, Vf 2.4V) | LED_0603_1608Metric | C2287 |
| 6 | LED7,LED11G,LED12G,LED13G,LED14G,LED15G | KT-0603G (Hubei KENTO, pure green 525nm 0603, Vf 3.1V) | LED_0603_1608Metric | C12624 |
| 3 | LED8,LED9,LED10 | XL-1608UWC-04 (warm-white 0603) | LED_0603_1608Metric | C965808 |
| 1 | LED_CHRG | XL-1608UOC-06 | LED_0603_1608Metric | C965800 |
| 1 | Q2 | 2N7002,215 | SOT-23 | C8545 |
| 1 | U2 | MCP23017-E/SS | SSOP-28_5.3x10.2mm_P0.65mm | C506653 |
| 1 | U6 | PCA9685PW,118 | TSSOP-28_4.4x9.7mm_P0.65mm | C2678753 |

## 5 · Switches & encoders

| Qty | Ref(s) | Part / MPN | Package | LCSC |
|----:|--------|------------|---------|------|
| 5 | SW1,SW2,SW3,SW4,SW5 | CPG135001D01 (Kailh Choc V1, red/linear) | SW_KailhChoc_CPG1350_THT_2P | C400229 |
| 5 | SW6,SW7,SW8,SW9,SW10 | HX B3F-4055-Y | SW_TC1212-7.3_THT_4P | C36498965 |
| 4 | EN1,EN2,EN3,EN4 | EC11E18244AU (ALPS EC11E, 18 Pulse, 36 Detents, mit Push-Switch, Flat-Shaft 20 mm) | RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm | C202365 |
| 1 | SW_BOOT | TS-1088-AR02016 | SW_TS1088_SMD | C720477 |

## 6 · Connectors + protection

| Qty | Ref(s) | Part / MPN | Package | LCSC |
|----:|--------|------------|---------|------|
| 2 | J6,J7 | 1x2 2.54mm pin header (the on-board connector) | PinHeader_1x02_P2.54mm_Vertical | C124375 |
| 2 | R_MIDI_REF,R_MIDI_TX | 0603WAF2200T5E (220R 0603) | R_0603_1608Metric | C22962 |
| 1 | D1 | USBLC6-2SC6 | SOT-23-6 | C2687116 |
| 1 | J1 | TYPE-C-31-M-12 | USB_C_Receptacle_HRO_TYPE-C-31-M-12 | C165948 |
| 1 | J3 | 1x8 2.54mm pin header (the on-board connector; the LCD module itself plugs in separately, off-board, see §C / BOM_MASTER §5) | PinHeader_1x08_P2.54mm_Vertical | C124383 |
| 1 | J9 | S2B-PH-SM4-TB(LF)(SN) | JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal | C295747 |

## 7 · Shared decoupling / bypass (grouped by value, spans domains)

| Qty | Value | Ref(s) | MPN | Package | LCSC |
|----:|-------|--------|-----|---------|------|
| 28 | 100 nF X7R 0603 | C2,C5,C6c,C7b,C8b,C10,C11,C12,C13,C14,C15,C16,C17,C_BOOST_HF,C_BOOT,C_CPVDD_HF,C_LDOO,C_NRST,C_PCA_VDD_HF,C_QSPI,C_SYS_HF,C_VDD1B,C_VDD2B,C_VDD3B,C_VDD4B,C_VDD5B,C_VDDA2,C_VREF2 | CC0603KRX7R9BB104 (Yageo, 50V X7R) | C_0603_1608Metric | C14663 |
| 26 | 10 kΩ 0603 | R6,R7,R8,R9,R10,R11,R12,R13,R14,R15,R16,R17,R18,R20,R_BAT_DIV_BOT,R_BAT_DIV_TOP,R_BLK_PD,R_BOOT0,R_DET_J8,R_MUTE_PD,R_NRST,R_OE,R_SHDN_PD,R_TS,R_VBUS_SENSE,R_XSMT_PD | 0603WAF1002T5E | R_0603_1608Metric | C25804 |
| 14 | 1 µF X5R 0603 | C9b,C_DET_J8,C_FLY,C_FLY_HP,C_HPVSS,C_HP_INL,C_HP_INR,C_PVDDR_HF,C_UPWR_IN,C_VCC,C_VDDA1,C_VNEG,C_VREF,C_VREF1 | CL10A105KB8NNNC | C_0603_1608Metric | C15849 |
| 12 | 22 µF X5R 25V 0805 | C9,C_BAT,C_BOOST_OUT,C_BOOST_OUT2,C_BOOST_OUT3,C_PVDDR,C_SYS1,C_VDD1A,C_VDD2A,C_VDD3A,C_VDD4A,C_VDD5A | CL21A226MAQNNNE | C_0805_2012Metric | C45783 |
| 8 | 10 µF X5R 0805 | C1,C6b,C7a,C8a,C_CPVDD_BULK,C_PCA_VDD,C_PWR_SW,C_QSPI2 | CL21A106KAYNNNE (Samsung, 25V X5R) | C_0805_2012Metric | C15850 |
| 2 | 2.2 µF (VCAP) 0603 | C_VCAP1,C_VCAP2 | CL10A225KO8NNNC | C_0603_1608Metric | C23630 |

## 8 · Other passives

| Qty | Ref(s) | Part / MPN | Package | LCSC |
|----:|--------|------------|---------|------|
| 2 | R2,R3 | 0603WAF5101T5E | R_0603_1608Metric | C23186 |
| 2 | R4,R5 | 0603WAF4701T5E | R_0603_1608Metric | C23162 |

## Off-board — NOT assembled on the PCB (separate procurement, §C)

| Item | Qty | Connects via | Note |
|------|----:|--------------|------|
| LiPo cell 2000 mAh (503759) | 1 | J9 | bottom-case slot |
| Speaker driver CMS-402811-28SP | 2 | J6 / J7 | 40 mm cloth-cone (PUI AS04008PS backup) |
| LCD module Waveshare 1.9" ST7789 | 1 | J3 | plug-in module |
| Encoder knobs | 4 | EN1–EN4 shafts | 3D-print |
| Cell key caps | 5 | clip on SW1–SW5 Choc stems | 3D-print |
| Modifier button caps | 5 | over SW6–SW10 | 3D-print, clip on square head |
| Speaker mesh, enclosure, screws/standoffs | — | — | mechanical |

---

### Notes
- **U4 = PAM8406DR (C86270)** — replaces the NRND PAM8403; MODE tied +5V (Class-D), input RI set for +4.3 dB gain, speaker HPF ~91 Hz (ADR-0025). Line-out/headphone stay full-range.
- **SW_PWR = ALPS SSSS811101 (C109335)** — current part; MST-12D18G3 (C49023766) is the vendored budget fallback only.
- **J4** = TC2030-IDC Tag-Connect programming footprint — pads only, no part placed (no BOM line).
- Shared-passive lines (§7) each cover one LCSC value spread across power/audio/MCU nets — grouped by value, not by function, on purpose.
- Machine-upload file for JLC assembly = `kicad/jlc_bom.csv` (identical part data, one row per LCSC).