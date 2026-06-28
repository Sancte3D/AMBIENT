# PCB Production BOM — Field Ambience

**On-PCB parts only.** Everything here is soldered to the board. Off-board parts
(battery cell, speaker drivers, display module, magnetic switches, knobs/caps,
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
| L1 | SWPA6045 inductor | 2.2 µH | 6×6 | C83455 |
| D3 | SS34 Schottky | — | SMA | C8678 |
| U5 | AP7361C-33Y5 LDO | 3.3 V | SOT-89-5 | C460397 |
| U7 | MCP73831 charger | 500 mA | SOT-23-5 | C424093 |
| Q1 | DMG2305UX P-MOS (power-path) | — | SOT-23 | C150470 |
| D2 | SMAJ5.0A TVS | — | SMA | C113952 |
| F1 | 1812L300 PTC fuse | 3 A | 1812 | C18198349 |
| C_BULK | Polymer-tantalum | 470 µF/10 V | Case-E | C444831 |
| C_BULK2 | MLCC | 100 µF/10 V | 1210 | C2880380 |
| R23 / R24 | boost FB divider | 200 k / 39 k | 0603 | C25811 / C23153 |
| R_FSW / R_ILIM | boost set | 360 k / 20 k | 0603 | C23146 / C4184 |
| R_COMP / C_COMP | boost comp | 22 k / 1 nF | 0603 | C31850 / C1588 |
| C_BOOT / C_BOOST_OUT / C_BOOST_HF | boost caps | 100 nF / 4.7 µF / 100 nF | 0603/0805 | C14663 / C45783 / C14663 |
| C_LDO_IN / C_LDO_OUT | LDO caps | 4.7 µF | 0603 | C46653 |
| R21 / R_CHRG | charger PROG / status | 2 k / 1 k | 0603 | C22975 / C21190 |
| R_BAT_DIV_TOP/BOT, R_VBUS_SENSE, R_VBUS_PD | battery/VBUS sense | 100 k / 10 k / 100 k | 0603 | C25804 / C25803 |
| C_BAT_IN / C_BAT_HF / C_BAT_FILT | battery caps | 4.7 µF / 100 nF / 10 nF | 0603/0805 | C45783 / C14663 / C57112 |

### Audio
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| U3 | PCM5102A DAC | — | TSSOP-20 | C107671 |
| U4 | PAM8403 Class-D amp | — | SOIC-16 | C17337 |
| FB1, FB2 | ferrite bead (AVDD/analog isolate) | 600 Ω | 0603 | C19330 / C84094 |
| J8 | PJ-320D 3.5 mm TRS line-out | — | custom FP | C431535 |
| R_LO_L/R | line-out series | 220 Ω | 0603 | C23345 |
| R_VOL_L/R | input/level | 20 k | 0603 | C4184 |
| R_in/C_in_L/R, C_FLY, C_VNEG, C_PVDDR(_HF), C_CPVDD_BULK/HF | DAC/amp caps | 1 µF/10 µF/100 nF | 0603/0805 | C15849 / C15850 / C14663 |
| R_SHDN_PD / R_MUTE_PD / R_XSMT_PD | amp/DAC default pull-downs | 10 k | 0603 | C25804 |

### I/O expander + LED drivers + status LEDs
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| U2 | MCP23017 (I²C 0x20) | — | SSOP-28 | C506653 |
| U6 | PCA9685 #1 (I²C 0x40, status LEDs) | — | TSSOP-28 | C2678753 |
| U10 | PCA9685 #2 (I²C 0x41, VU meter) | — | TSSOP-28 | C2678753 |
| Q2 | 2N7002 LCD-backlight FET | — | SOT-23 | C8545 |
| LED6 | Shift indicator | yellow 0603 | 0603 | C2287 |
| LED7 | Hold indicator | green 0603 | 0603 | C12624 |
| LED8, LED9, LED10 | Drone/Generate/Clear | white 0603 | 0603 | C965808 |
| LED11Y–LED15Y (5) | cell base-hold | yellow 0603 | 0603 | C2287 |
| LED11G–LED15G (5) | cell shift-hold | green 0603 | 0603 | C12624 |
| LED_VU1–LED_VU8 (8) | VU level meter — **all white** (was blue; uses the existing white) | white 0603 | 0603 | C965808 |
| LED_HB | heartbeat | white 0603 | 0603 | C965808 |
| LED_CHRG | charge status | amber 0603 | 0603 | C72041 |
| R_LED6–15 (15), R_VU1–8 (8) | LED ballast | 390 Ω | 0603 | C23151 |
| R_OE, R_OE2 | PCA /OE pull-up | 10 k | 0603 | C25804 |
| R_BLK_PD, R_SLED, R_BOOT0/_SW, R_NRST | misc pulls | 5.1k/820/1k/10k | 0603 | C23186/C23253/C21190/C25804 |
| C_PCA_VDD(_HF), C_PCA2_VDD(_HF) | PCA decoupling | 10 µF / 100 nF | 0603/0805 | C15850 / C14663 |

### Cells — Hall sensors (the magnetic switches in §C sit above these)
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| J_CELL1–5 (5) | DRV5056A4 linear Hall | — | SOT-23 | C2152902 |
| R_CELL1–5 (5) | Hall RC series | 1 k | 0603 | C21190 |
| C_CELL1–5 (5) | Hall RC filter | 10 nF | 0603 | C57112 |

### Buttons & encoders (THT/SMD, **on the PCB**)
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| **EN1–EN4 (4)** | **ALPS EC11E18244AU — ALL 4 IDENTICAL** (rotary + push, 36 detents) | — | EC11E vertical | C202365 |
| **SW6–SW10 (5)** | HCTL TC-1212-7.3-260G tactile (Shift/Hold/Drone/Generate/Clear) — **square-head plunger w/ hole → clip-on caps** ✅ | — | THT 12×12, 4-pin | C2845240 |
| SW_BOOT, SW11 (2) | TS-1088 service tactile (BOOT0 / reset) | — | custom FP | C720477 |

### Connectors + protection (on PCB)
| Ref | Part | Value | Package | LCSC |
|---|---|---|---|---|
| J1 | USB-C TYPE-C-31-M-12 (16-pin, USB-2.0 data) | — | HRO TYPE-C-31-M-12 | C165948 |
| D1 | USBLC6-2SC6 USB ESD | — | SOT-23-6 | C2687116 |
| J9 | JST-PH 2-pin battery connector | — | JST-PH | C295747 |
| J10 | PJ-320D 3.5 mm TRS MIDI-out | — | custom FP | C431535 |
| R_MIDI_TX, R_MIDI_REF | MIDI Type-A series | 220 Ω | 0603 | ⚠ **see flag #3** |
| J3 | 1×8 2.54 mm header (LCD module plugs in — module is §C) | — | PinHeader 1×08 | C124383 (2.54 strip, cut to 1×8) — or a 1×8 female socket for a removable module |
| J6, J7 | 1×2 2.54 mm header (speaker wires — drivers are §C) | — | PinHeader 1×02 | C124375 |
| J4 | TC2030-IDC Tag-Connect — **footprint/pads only, no part placed** | — | TC2030 | — (no BOM line) |

### Generic decoupling/bypass passives (grouped by value)
| Value | Package | Qty (designators in `jlc_bom.csv`) | LCSC |
|---|---|---|---|
| 100 nF X7R | 0603 | ~20 (C_VDD*B, C_*_HF, …) | C14663 |
| 1 µF X5R | 0603 | ~10 (C_VCC, C_VDDA1, C_VREF*, …) | C15849 |
| 10 µF X5R | 0805 | ~9 (C_VDD*A, C9, C_BAT_IN, …) | C45783 |
| 4.7 µF X5R | 0805 | C1, C6b, C7a, C8a, C_PCA*, C_CPVDD_BULK | C15850 |
| 2.2 µF (VCAP) | 0603 | C_VCAP1, C_VCAP2 | C24539 |
| 22 R | 0603 | (audio/series where used) | C23345 |
| 4.7 k / 5.1 k | 0603 | R4/R5 (I²C pulls) · R2/R3 | C23162 / C23186 |
| 10 k (generic) | 0603 | R6–R18, R20, R22 | C25804 |

---

## B · On-PCB but **needs sourcing / not yet in generator** (flags)

| Ref | Part | Status |
|---|---|---|
| R_MIDI_TX, R_MIDI_REF | 220 Ω 0603 | ⚠ **NO LCSC** (the only remaining one) — `0603WAF2200T5E`, confirm PN |
| **SW_PWR** | **MST-12D18G3** right-angle SMD slide switch (SPDT, **side-actuated** → operated from the enclosure edge); drives `U_PWR.ON` only | ✅ **C49023766** · FP `field_ambience:SW_MST-12D18_SlideSwitch_RA` (+STEP) in repo |
| **U_PWR** | TPS22918 load-switch (ADR-0016; gates +5V_RAIL→+5V_SW = whole 3V3 domain) | C68913 · SOT-23-6 (KiCad-standard FP). **Pin-level drop-in spec in ADR-0016** — add at schematic rebuild |
| R_PWR_PD / C_PWR_SW | 100 k / 10 µF | with U_PWR (C25803 / C15850) |
| **SW6–SW10 footprint** | TC-1212-7.3-260G is **THT** (C2845240) | **Verified THT footprint `field_ambience:SW_TC1212-7.3_THT_4P` (+STEP) is now in the repo.** Use it (4-pin), **not** the old SMD `SW_HX_…_SMD-4P` |

---

## C · **NOT on the PCB** — off-board, separate procurement (do NOT assemble)

| Item | Qty | Connects via | Note |
|---|---|---|---|
| LiPo cell 2000 mAh (503759) | 1 | J9 | bottom-case slot |
| Speaker driver CMS-402811-28SP | 2 | J6 / J7 | 40 mm, cloth-cone (PUI AS04008PS = backup) |
| LCD module Waveshare 1.9″ ST7789 | 1 | J3 | plug-in module |
| Gateron LP Magnetic Jade switch | 5 | — (plate, over Hall sensors) | not on PCB |
| Encoder knobs | 4 | EN1–4 shafts | 3D-print |
| Cell caps | 5 | over magnetic switches | 3D-print |
| Modifier button caps | 5 | over SW6–10 | 3D-print, clip onto the square head of C2845240 |
| Speaker dust mesh, enclosure, screws/standoffs | — | — | mechanical |

---

## Correctness flags (rules to confirm before order)

1. **Modifier button cap fit — RESOLVED.** `SW6–SW10 = HCTL TC-1212-7.3-260G`
   (C2845240): **square-head plunger with a centre hole** → the printed caps
   clip on. No anti-rotation needed (user clarified — only needs the cap to
   hold). THT 12×12, 4-pin; footprint swap (SMD→THT) noted in §B.
2. **All 4 encoders are identical** — `EN1–EN4 = ALPS EC11E18244AU` (rotary +
   push). ✅ confirmed against the schematic.
3. **No-LCSC parts:** only the **2× 220 Ω MIDI** resistors remain — confirm
   `0603WAF2200T5E` before a JLC order. (VU LEDs are now white/C965808, slide
   switch = C49023766, headers sourced — all resolved.)

## Return-current / layout notes (for the layout engineer — keep short)
- One **solid GND plane** (4-layer); do **not** split analog/digital — steer
  returns by placement, not by cuts.
- **Class-D amp (U4)** speaker-output return loops kept **off** the DAC/analog
  ground region; outputs short + symmetric.
- **Boost (U8)** hot loop `L1 → D3 → C_BOOST_OUT` tight + local; switching node
  away from audio/DAC.
- **DAC (U3)** AVDD via `FB1`; local decoupling right at the pins; `J8` close to U3.
- **USB D±** routed as a pair over solid GND; ESD `D1` at the connector.
