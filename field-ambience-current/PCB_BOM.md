# PCB Production BOM — Field Ambience

**On-PCB parts only.** Everything here is soldered to the board. Off-board parts
(battery cell, speaker drivers, display module, knobs/caps,
enclosure) are in **§C** and are **NOT** part of the board assembly.

> Source of truth = the schematic (`kicad/generate_kicad_project.py`). Machine
> upload file = `kicad/jlc_bom.csv`. Detailed/rationale version (DE) =
> `BOM_MASTER.md`. No duplicate ref-designators; cross-checked against the
> generated sheets.

---

## A · SMT — JLC-assembled (verified LCSC)

### MCU & clock
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| U1 | STM32H743VIT6 | — | LQFP-100 | C114409 |
| Y1 | ABLS-8.000MHZ-B4-T | 8 MHz xtal | HC-49 SMD | C596838 |
| C_HSE1, C_HSE2 | C0G | 27 pF | 0603 | C107045 |

### Power
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| U8 | TPS61089RNR boost | — | VQFN-11 | C165129 |
| L1 | SWPA6045S2R2NT inductor (r18.77: was C83455, a dead link; MPN was also wrong — 2.2µH only exists as "NT" not "MT" suffix) | 2.2 µH | 6×6 | C36500 |
| D3 | SS34 Schottky | — | SMA | C8678 |
| U5 | AP7361C-33Y5 LDO | 3.3 V | SOT-89-5 | C460397 |
| U7 | BQ24074RGTR power-path charger (r19.18, ADR-0023; ICHG 0,89 A / IIN 1,34 A) | 1,5 A max | VQFN-16 3×3 | C54313 |
| F2 | SMD1812P260TF/16 PTC — Batterie-Hard-Short-Backup (r19.18; D3B/Dioden-OR entfernt) | 2,6 A hold / 5 A trip | 1812 | C438899 |
| D2 | SMAJ5.0A TVS | — | SMA | C113952 |
| F1 | 1812L300 PTC fuse | 3 A | 1812 | C18198349 |
| C_BULK | Polymer-tantalum | 470 µF/10 V | Case-E | C444831 |
| C_BULK2 | MLCC | 100 µF/10 V | 1210 | C23742 (r18.79 doc-sync: table still said C2880380, generator/jlc_bom have C23742 since r18.70) |
| R23 / R24 | boost FB divider → 4.97 V (r18.79: was 200k = **7.4 V!**, VREF 1.212 V per TI DS) | 121 k / 39 k | 0603 | C25809 / C23153 |
| R_FSW / R_ILIM | boost set: Fsw ~440 kHz / ILIM 5.9 A (r18.79: R_ILIM was 20k = 51 A, i.e. no limit) | 360 k / 174 k | 0603 | C23146 / C22890 |
| R_COMP / C_COMP | boost loop comp — fc ≈ 8 kHz ≤ fRHPZ/5 per TI DS eq. 17/18 (r18.80: was 22 k / 1 nF = fc ≈ 87 kHz **above** the RHP zero → oscillation under load) | 6.2 k / 10 nF | 0603 | C4260 / C57112 |
| C_BOOT / C_BOOST_OUT(×3) / C_BOOST_HF | boost caps (r18.80: output bulk now **3× 22 µF** per TI DS §9.2.2.7 “typically three 22 µF”; was 1×, and the 470 µF bulk sits behind D3 where the control loop can't see it) | 100 nF / 3× 22 µF / 100 nF | 0603/0805 | C14663 / C45783 / C14663 |
| C_LDO_IN / C_LDO_OUT | LDO caps | 4.7 µF | 0603 | C46653 |
| **U_PWR** | TPS22918 load switch — **r18.81: ADR-0016 power-off now actually in the schematic** (+5V_RAIL → +5V_SW → LDO; charger stays ahead of it = "dark, but charging"; QOD tied to VOUT = active discharge; CT floats per TI DS) | 5.5 V / 2 A | SOT-23-6 | C131941 |
| **SW_PWR** | ALPS SSSS811101 side-actuated slide switch on `U_PWR.ON` — r18.85 (ADR-0016) premium swap from MST-12D18G3. Body sits flat, actuator sticks out **horizontally** to the board edge (body only 1.4 mm tall, stem z ≈ 0–1.4 mm — much flatter than the old MST, so the enclosure slot + slider cap must reach deeper). Travel 1.5 mm. Terminal mapping **datasheet-verified** against the ALPS circuit diagram (Common = Terminal 2, labeled — unlike the old MST which was assumed); short continuity check before fab still cheap, fail-safe if wrong (100k PD holds PWR_ON low). Footprint `field_ambience:SW_ALPS-SSSS811101_SlideSwitch_SMD`. Budget fallback MST-12D18G3 (C49023766) stays vendored in the repo. | 12 V / 100 mA | slide SMD (3 sig + 4 GND-frame pads) | C109335 |
| R_PWR_PD / C_UPWR_IN / C_PWR_SW | PWR_ON pull-down (default OFF) / TPS22918 VIN bypass / +5V_SW output cap | 100 k / 1 µF / 10 µF | 0603/0603/0805 | C25803 / C15849 / C15850 |
| R_DET_J8 / C_DET_J8 | jack-detect series + AC filter (r18.82: PJ-320D detect contact rests on the TIP when unplugged = DAC output — series R limits MCP input-clamp current to <300 µA, cap kills audio AC on GPA6; unplugged ≈ 0.3 V LOW, plugged 3.3 V HIGH) | 10 k / 1 µF | 0603 | C25804 / C15849 |
| C_CHG_IN | BQ24074 IN bypass (r19.18 — TI DS 1–10 µF, am VBUS_FUSED-Knoten) | 4.7 µF | 0603 | C46653 |
| R21 / R_CHRG | charger PROG / status | 2 k / 1 k | 0603 | C22975 / C21190 |
| R_BAT_DIV_TOP/BOT, R_VBUS_SENSE, R_VBUS_PD | battery/VBUS sense | 100 k / 10 k / 100 k | 0603 | C25804 / C25803 |
| C_BAT_IN / C_BAT_HF / C_BAT_FILT | battery caps (r18.80: C45783 ist 22 µF CL21A226MAQNNNE — alte 4.7-µF-Angabe war falsch) | 22 µF / 100 nF / 10 nF | 0603/0805 | C45783 / C14663 / C57112 |

### Audio
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| U3 | PCM5102A DAC | — | TSSOP-20 | C107671 |
| U4 | PAM8403 Class-D amp | — | SOIC-16 | C17337 |
| FB1, FB2 | ferrite bead (AVDD/analog isolate) | 600 Ω | 0603 | C19330 / C84094 |
| U11 | TPA6132A2RTER DirectPath HP-Amp (r19.19, ADR-0024; Gain −6 dB) | 25 mW/16 Ω | WQFN-16 3×3 | C69901 |
| C_HP_VDD/C_HPVDD | TPA6132A2 VDD/HPVDD decoupling (HPVDD NIE an VDD!) | 2.2 µF | 0603 | C1607 |
| C_HP_INL/R, C_FLY_HP, C_HPVSS | TPA6132A2 Eingangskopplung + Ladungspumpe | 1 µF | 0603 | C15849 |
| J8 | PJ-320D 3.5 mm TRS PHONES/LINE-OUT (r19.19) | — | custom FP | C431535 |
| R_LO_L/R | phones/line-out series (hinter U11) | 22 Ω | 0603 | C23345 |
| R_VOL_L/R | input/level | 20 k | 0603 | C4184 |
| R_in/C_in_L/R, C_FLY, C_VNEG, C_PVDDR(_HF), C_CPVDD_BULK/HF | DAC/amp caps | 1 µF/10 µF/100 nF | 0603/0805 | C15849 / C15850 / C14663 |
| R_SHDN_PD / R_MUTE_PD / R_XSMT_PD | amp/DAC default pull-downs | 10 k | 0603 | C25804 |

### I/O expander + LED drivers + status LEDs
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| U2 | MCP23017 (I²C 0x20) | — | SSOP-28 | C506653 |
| U6 | PCA9685 #1 (I²C 0x40, status LEDs) | — | TSSOP-28 | C2678753 |
| Q2 | 2N7002 LCD-backlight FET | — | SOT-23 | C8545 |
| LED6 | Shift indicator | yellow 0603 | 0603 | C2287 |
| LED7 | Hold indicator | green 0603 | 0603 | C12624 |
| LED8, LED9, LED10 | Drone/Generate/Clear | white 0603 | 0603 | C965808 |
| LED11Y–LED15Y (5) | cell base-hold | yellow 0603 | 0603 | C2287 |
| LED11G–LED15G (5) | cell shift-hold | green 0603 | 0603 | C12624 |
| LED_CHRG | charge status | orange 0603 | 0603 | C965800 (r18.79 doc-sync: was C72041, blue+EOL — fixed r18.70 in generator) |
| R_LED6–15 (15) | LED ballast | 390 Ω | 0603 | C23151 |
| R_OE, R_OE2 | PCA /OE pull-up | 10 k | 0603 | C25804 |
| R_BLK_PD, R_BOOT0/_SW, R_NRST | misc pulls | 5.1k/1k/10k | 0603 | C23186/C21190/C25804 |
| C_PCA_VDD(_HF), C_PCA2_VDD(_HF) | PCA decoupling | 10 µF / 100 nF | 0603/0805 | C15850 / C14663 |

### Cells — digital on MCP23017, real keyswitch, direct-solder (r18.75, ADR-0013 superseded)
The cells are electrically digital on the I²C expander (HiChord Batch 4+ pattern:
switch → I²C GPIO-expander → MCU), **not** Hall sensors at the STM32 ADC — same
as r18.73. r18.73 had also put them on the same small tactile button as the
modifiers, which killed the "keyboard key" feel; r18.74 fixed that with a Kailh
Choc hot-swap socket, but that socket had no clean part number and needed fiddly
small-SMD hand-soldering. **r18.75 simplifies further**: the Kailh Choc V1
switch (CPG135001D01) is now **directly soldered to the board** (the on-PCB
part below, SW1–SW5) — 2 THT legs, same soldering technique as everything else
here, no hot-swap socket, switch now permanent. The 5× DRV5056A4 + R_CELL/C_CELL
RC stay removed; PC0/PC1/PA4/PB0/PB1 stay freed.

### Buttons & encoders (THT/SMD, **on the PCB**)
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| **EN1–EN4 (4)** | **ALPS EC11E18244AU — ALL 4 IDENTICAL** (rotary + push, 36 detents) | — | EC11E vertical | C202365 |
| **SW1–SW5 (5)** | **Kailh Choc V1, CPG135001D01 — CELL trigger keys, direct-solder** (digital on/off via MCP23017 GPA0–GPA4; r18.75) — real ~3 mm keyswitch travel, 2 THT legs + 3 unplated locator holes. Any Choc V1 color works (same footprint) — see r18.78 note | — | `field_ambience:SW_KailhChoc_CPG1350_THT_2P` (pulled from LCSC/EasyEDA) | C400229 — verified, ⚠ 0 stock (all 3 colors: C400229/C400230/C400231 — hand-soldered, not JLC-restricted) |
| **SW6–SW10 (5)** | **HX B3F-4055-Y tactile** (Shift/Hold/Drone/Generate/Clear) — **square-head plunger → clip-on caps** ✅ (r18.71: was TC-1212-7.3 C2845240) | — | THT 12×12, 4-pin | C36498965 |
| SW_BOOT, SW11 (2) | TS-1088 service tactile (BOOT0 / reset) | — | custom FP | C720477 |

### Connectors + protection (on PCB)
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| J1 | USB-C TYPE-C-31-M-12 (16-pin, USB-2.0 data) | — | HRO TYPE-C-31-M-12 | C165948 |
| D1 | USBLC6-2SC6 USB ESD | — | SOT-23-6 | C2687116 |
| J9 | JST-PH 2-pin battery connector | — | JST-PH | C295747 |
| J10 | PJ-320D 3.5 mm TRS MIDI-out | — | custom FP | C431535 |
| R_MIDI_TX, R_MIDI_REF | MIDI Type-A series | 220 Ω | 0603 | C22962 |
| J3 | 1×8 2.54 mm header (LCD module plugs in — module is §C) | — | PinHeader 1×08 | C124383 (2.54 strip, cut to 1×8) — or a 1×8 female socket for a removable module |
| J6, J7 | 1×2 2.54 mm header (speaker wires — drivers are §C) | — | PinHeader 1×02 | C124375 |
| J4 | TC2030-IDC Tag-Connect — **footprint/pads only, no part placed** | — | TC2030 | — (no BOM line) |

### Generic decoupling/bypass passives (grouped by value)
| Value | Package | Qty (designators in `jlc_bom.csv`) | LCSC |
|---|---|---|---|
| 100 nF X7R | 0603 | ~20 (C_VDD*B, C_*_HF, …) | C14663 |
| 1 µF X5R | 0603 | ~10 (C_VCC, C_VDDA1, C_VREF*, …) | C15849 |
| 22 µF X5R 25 V | 0805 | 11 (C_VDD*A, C9, C_BAT_IN, C_BOOST_OUT×3, C_PVDDR) — r18.80: C45783 = CL21A226MAQNNNE 22 µF; frühere 4.7/10-µF-Angaben waren falsch | C45783 |
| 10 µF X5R 25 V | 0805 | C1, C6b, C7a, C8a, C_PCA*, C_CPVDD_BULK — r18.80: C15850 = CL21A106KAYNNNE 10 µF (alte 4.7-µF-Angabe war falsch) | C15850 |
| 2.2 µF (VCAP) | 0603 | C_VCAP1, C_VCAP2 | C23630 (r18.79 doc-sync: was C24539, not in JLC assembly — fixed r18.70 in generator) |
| 22 R | 0603 | (audio/series where used) | C23345 |
| 4.7 k / 5.1 k | 0603 | R4/R5 (I²C pulls) · R2/R3 | C23162 / C23186 |
| 10 k (generic) | 0603 | R6–R18, R20 (R22 entfernt r18.79 mit Q1) | C25804 |

---

## B · Power-off block — now in the generated schematic (spec in ADR-0016)

> **r18.85 update:** these parts are **no longer pending** — they are all in the
> generator + `jlc_bom.csv` now (part of §A above; all LCSC verified, **no
> NO-LCSC parts left**). This section is kept as the power-off block reference.

| Ref | Part | LCSC |
|---|---|---|
| **U_PWR** | TPS22918 load-switch — gates `+5V_RAIL→+5V_SW` (= whole 3V3 domain) | **C131941** · SOT-23-6 |
| **SW_PWR** | ALPS SSSS811101 slide switch (side-actuated, drives `U_PWR.ON`; r18.85 swap from MST-12D18G3) · FP `field_ambience:SW_ALPS-SSSS811101_SlideSwitch_SMD` in repo | **C109335** |
| R_PWR_PD | 100 k 0603 (`U_PWR.ON` pull-down, default off) | C25803 |
| C_PWR_SW | 10 µF 0805 (`+5V_SW` output cap) | C15850 |

Pin-level wiring (VIN/VOUT/ON + the single LDO-input reroute) = **`ADR-0016`**.

---

## C · **NOT on the PCB** — off-board, separate procurement (do NOT assemble)

| Item | Qty | Connects via | Note |
|---|---|---|---|
| LiPo cell 2000 mAh (503759) | 1 | J9 | bottom-case slot |
| Speaker driver CMS-402811-28SP | 2 | J6 / J7 | 40 mm, cloth-cone (PUI AS04008PS = backup) |
| LCD module Waveshare 1.9″ ST7789 | 1 | J3 | plug-in module |
| Encoder knobs | 4 | EN1–4 shafts | 3D-print |
| Cell caps | 5 | clip onto the Choc switch stem (SW1–5, now soldered on-PCB — see §B) | 3D-print, matched to the Choc V1 stem |
| Modifier button caps | 5 | over SW6–10 | 3D-print, clip onto the square head of C36498965 |
| Speaker dust mesh, enclosure, screws/standoffs | — | — | mechanical |

---

## Correctness flags (rules to confirm before order)

1. **Modifier button cap fit — RESOLVED.** `SW6–SW10 = HCTL TC-1212-7.3-260G`
   (C2845240): **square-head plunger with a centre hole** → the printed caps
   clip on. No anti-rotation needed (user clarified — only needs the cap to
   hold). THT 12×12, 4-pin; footprint swap (SMD→THT) noted in §B.
2. **All 4 encoders are identical** — `EN1–EN4 = ALPS EC11E18244AU` (rotary +
   push). ✅ confirmed against the schematic.
3. **No-LCSC parts: NONE** — every part in the generated schematic has a
   verified LCSC (58 unique). The MIDI 220 Ω are **C22962** (UNI-ROYAL
   0603WAF2200T5E, JLC Basic). The off-board power-off block (§B) is the only
   thing still to draw into the schematic.

## Return-current / layout notes (for the layout engineer — keep short)
- One **solid GND plane** (4-layer); do **not** split analog/digital — steer
  returns by placement, not by cuts.
- **Class-D amp (U4)** speaker-output return loops kept **off** the DAC/analog
  ground region; outputs short + symmetric.
- **Boost (U8)** hot loop `L1 → D3 → C_BOOST_OUT` tight + local; switching node
  away from audio/DAC.
- **DAC (U3)** AVDD via `FB1`; local decoupling right at the pins; `J8` close to U3.
- **USB D±** routed as a pair over solid GND; ESD `D1` at the connector.
