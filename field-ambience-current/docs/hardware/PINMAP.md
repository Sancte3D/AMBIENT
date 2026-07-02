# PINMAP — STM32H743VIT6 complete pin & net connectivity

**Purpose:** every single MCU pin, its net, and what it connects to — plus
per-module net tables (source → sinks) — so a PCB engineer knows exactly how
each pin is wired, all traces, per module.

> **Authoritative source = the generator + generated schematics.**
> The machine-readable truth is `kicad/*.kicad_sch` (produced by
> `kicad/generate_kicad_project.py`). This file is the human-readable
> distillation, derived directly from the generator's `NETS` table
> (`generate_kicad_project.py` ~line 2942) and `STM32_PIN_LOC` (~line 873).
> If this doc and the generator ever disagree, **the generator wins** — regen
> and re-derive. A CI gate (`scripts/check_pinmap.py`, workflow `hw-consistency`)
> fails the build if this pin table and the generator `NETS` disagree, so they
> cannot silently drift. Prose context per module is in
> [`SCHEMATIC_WALKTHROUGH.md`](SCHEMATIC_WALKTHROUGH.md); part numbers are in
> [`BOM_MASTER.md`](../../../BOM_MASTER.md).

MCU: **STM32H743VIT6, LQFP-100, 0.5 mm pitch.** Verified against DS12110 Rev 5
Table 8 in [`component_reviews/U1_STM32H743VIT6.md`](../component_reviews/U1_STM32H743VIT6.md).

---

## 1. Master pin table (all 100 pins)

Legend: **Net** = net name in the schematic · **AF/Func** = STM32 alternate
function or dedicated role · **Module** = which sheet/sub-circuit · **Connects to**
= the external component pin it lands on. "(free)" = unassigned, available for
Rev-B.

| Pin | Port | Net | AF / Function | Module | Connects to |
|----:|------|-----|---------------|--------|-------------|
| 1 | PE2 | (free) | SAI1_MCLK_A (unused — DAC runs 3-wire) | — | free (ADR-0016 would use for `LSW_EN`) |
| 2 | PE3 | DISPLAY_SW | GPIO / EXTI | Encoder | EN3 (Display) push-switch |
| 3 | PE4 | I2S_LRCK | SAI1_FS_A (AF6) | Audio | PCM5102A `LRCK` (pin 15) |
| 4 | PE5 | I2S_BCK | SAI1_SCK_A (AF6) | Audio | PCM5102A `BCK` (pin 13) |
| 5 | PE6 | I2S_DOUT | SAI1_SD_A (AF6) | Audio | PCM5102A `DIN` (pin 14) |
| 6 | VBAT | +3V3 | dedicated | Power | +3V3 rail (no RTC backup cell) |
| 7 | PC13 | MCP_INT | GPIO / EXTI | MCP | MCP23017 `INTA` (pin 20) |
| 8 | PC14 | (free) | OSC32_IN | — | free (no 32 kHz crystal) |
| 9 | PC15 | (free) | OSC32_OUT | — | free |
| 10 | VSS | GND | dedicated | Power | GND |
| 11 | VDD | +3V3 | dedicated | Power | +3V3 + 4.7 µF‖100 nF local |
| 12 | PH0 | HSE_IN | OSC_IN | MCU | Crystal Y1 in (+ 22 pF load cap) |
| 13 | PH1 | HSE_OUT | OSC_OUT | MCU | Crystal Y1 out (+ 22 pF load cap) |
| 14 | NRST | NRST | dedicated reset | MCU | SW11 reset button + 10 kΩ PU + 100 nF |
| 15 | PC0 | NC_PC0_ADC_RSVD | ADC123_INP10 | — | free (r18.73: cells went digital on MCP23017; ADC pin freed, Rev-B reserve) |
| 16 | PC1 | NC_PC1_ADC_RSVD | ADC123_INP11 | — | free (r18.73 reserve) |
| 17 | PC2_C | (free) | analog | — | free |
| 18 | PC3_C | (free) | analog | — | free |
| 19 | VSSA | GND | analog ground | Power | GND (single-star) |
| 20 | VREF+ | VDDA | ADC reference | Power | tied to VDDA |
| 21 | VDDA | +3V3 (filtered) | analog supply | Power | +3V3 via ferrite BLM18AG601 + 1 µF‖100 nF |
| 22 | PA0 | DRIVE_A | TIM2_CH1 (AF1) | Encoder | EN1 (Drive) phase A |
| 23 | PA1 | DRIVE_B | TIM2_CH2 (AF1) | Encoder | EN1 (Drive) phase B |
| 24 | PA2 | (free) | TIM2_CH3 (AF1) | — | free |
| 25 | PA3 | BAT_SENSE | ADC1_INP15 | Battery | LiPo+ via 100k:100k divider |
| 26 | VSS | GND | dedicated | Power | GND |
| 27 | VDD | +3V3 | dedicated | Power | +3V3 + decoupling |
| 28 | PA4 | NC_PA4_ADC_RSVD | ADC12_INP18 | — | free (r18.73 reserve) |
| 29 | PA5 | LCD_SCK | SPI1_SCK (AF5) | LCD | J3 header pin 3 (SCL) |
| 30 | PA6 | LCD_CS | GPIO | LCD | J3 header pin 7 (CS) |
| 31 | PA7 | LCD_MOSI | SPI1_MOSI (AF5) | LCD | J3 header pin 4 (SDA) |
| 32 | PC4 | LCD_DC | GPIO | LCD | J3 header pin 6 (DC) |
| 33 | PC5 | LCD_RES | GPIO | LCD | J3 header pin 5 (RES) |
| 34 | PB0 | NC_PB0_ADC_RSVD | ADC12_INP9 | — | free (r18.73 reserve) |
| 35 | PB1 | NC_PB1_ADC_RSVD | ADC12_INP5 | — | free (r18.73 reserve) |
| 36 | PB2 | (free) | BOOT1 / GPIO | — | free |
| 37 | PE7 | (free) | TIM1_ETR | — | free |
| 38 | PE8 | (free) | TIM1_CH1N | — | free |
| 39 | PE9 | (free) | TIM1_CH1 | — | free |
| 40 | PE10 | (free) | — | — | free |
| 41 | PE11 | (free) | TIM1_CH2 | — | free |
| 42 | PE12 | (free) | — | — | free |
| 43 | PE13 | (free) | TIM1_CH3 | — | free |
| 44 | PE14 | (free) | TIM1_CH4 | — | free |
| 45 | PE15 | (free) | — | — | free |
| 46 | PB10 | (free) | TIM2_CH3 / USART3 | — | free |
| 47 | PB11 | (free) | TIM2_CH4 / USART3 | — | free |
| 48 | VCAP1 | (cap) | SMPS bulk | Power | 2.2 µF X5R → GND |
| 49 | VSS | GND | dedicated | Power | GND |
| 50 | VDD | +3V3 | dedicated | Power | +3V3 + decoupling |
| 51 | PB12 | (free) | — | — | free |
| 52 | PB13 | (free) | — | — | free |
| 53 | PB14 | AMP_nSHDN | GPIO (active-low) | Audio | PAM8403 `/SHDN` + 10 kΩ PD |
| 54 | PB15 | AMP_nMUTE | GPIO (active-low) | Audio | PAM8403 `/MUTE` + 10 kΩ PD |
| 55 | PD8 | STATUS_LED | GPIO | MCP | heartbeat LED `LED_HB` (via 390 Ω) |
| 56 | PD9 | (free) | USART3_RX | — | free |
| 57 | PD10 | (free) | — | — | free |
| 58 | PD11 | (free) | — | — | free |
| 59 | PD12 | DISPLAY_A | TIM4_CH1 (AF2) | Encoder | EN3 (Display) phase A |
| 60 | PD13 | DISPLAY_B | TIM4_CH2 (AF2) | Encoder | EN3 (Display) phase B |
| 61 | PD14 | (free) | — | — | free |
| 62 | PD15 | (free) | — | — | free |
| 63 | PC6 | BRIGHT_A | TIM3_CH1 (AF2) | Encoder | EN2 (Brightness) phase A |
| 64 | PC7 | BRIGHT_B | TIM3_CH2 (AF2) | Encoder | EN2 (Brightness) phase B |
| 65 | PC8 | (free) | — | — | free |
| 66 | PC9 | (free) | — | — | free |
| 67 | PA8 | VOL_A | TIM1_CH1 (AF1) | Encoder | EN4 (Volume) phase A |
| 68 | PA9 | VOL_B | TIM1_CH2 (AF1) | Encoder | EN4 (Volume) phase B |
| 69 | PA10 | (free) | TIM1_CH3 | — | free |
| 70 | PA11 | USB_DM | OTG_FS_DM (AF10) | MCU/USB | USBLC6 → USB-C D− |
| 71 | PA12 | USB_DP | OTG_FS_DP (AF10) | MCU/USB | USBLC6 → USB-C D+ |
| 72 | PA13 | SWDIO | JTMS/SWDIO | MCU/SWD | SWD header J4 pin 1 |
| 73 | VCAP2 | (cap) | SMPS bulk | Power | 2.2 µF X5R → GND |
| 74 | VSS | GND | dedicated | Power | GND |
| 75 | VDD | +3V3 | dedicated | Power | +3V3 + decoupling |
| 76 | PA14 | SWCLK | JTCK/SWCLK | MCU/SWD | SWD header J4 pin 2 |
| 77 | PA15 | (free) | JTDI / TIM2_CH1 | — | free |
| 78 | PC10 | (free) | — | — | free |
| 79 | PC11 | (free) | — | — | free |
| 80 | PC12 | (free) | — | — | free |
| 81 | PD0 | (free) | — | — | free |
| 82 | PD1 | (free) | — | — | free |
| 83 | PD2 | (free) | — | — | free |
| 84 | PD3 | (free) | — | — | free |
| 85 | PD4 | (free) | — | — | free |
| 86 | PD5 | MIDI_TX | USART2_TX (AF7) | Audio | **MIDI jack J10** (3.5 mm TRS Type A, implementiert r18.67; J9 = Akku) |
| 87 | PD6 | (free) | — | — | free |
| 88 | PD7 | (free) | — | — | free |
| 89 | PB3 | SWO | TRACESWO (optional) | MCU/SWD | SWD header J4 (SWO, optional) |
| 90 | PB4 | (free) | — | — | free |
| 91 | PB5 | (free) | — | — | free |
| 92 | PB6 | I2C_SCL | I2C1_SCL (AF4) | MCP | MCP23017 + PCA9685 SCL + 4.7 kΩ PU |
| 93 | PB7 | I2C_SDA | I2C1_SDA (AF4) | MCP | MCP23017 + PCA9685 SDA + 4.7 kΩ PU |
| 94 | BOOT0 | BOOT0_PIN | dedicated boot | MCU | SW_BOOT button + 10 kΩ PD |
| 95 | PB8 | (free) | — | — | free |
| 96 | PB9 | (free) | — | — | free |
| 97 | PE0 | DRIVE_SW | GPIO / EXTI | Encoder | EN1 (Drive) push-switch |
| 98 | PE1 | BRIGHT_SW | GPIO / EXTI | Encoder | EN2 (Brightness) push-switch |
| 99 | VSS | GND | dedicated | Power | GND |
| 100 | VDD | +3V3 | dedicated | Power | +3V3 + decoupling |

**Power-pin summary:** VDD ×5 (11/27/50/75/100), VSS ×4 (10/26/49/74) + VSSA
(19) + VBAT (6) → +3V3/GND; VDDA (21) via ferrite; VREF+ (20) = VDDA;
VCAP1/2 (48/73) = 2× 2.2 µF X5R to GND.

> **Encoder note:** EN4 (Volume) push-switch (`VOL_SW`) is **not** on the MCU —
> it lands on the MCP23017 expander (GPB5), because the MCU's encoder-switch
> pins were full. The other 3 encoder switches are on PE0/PE1/PE3.

---

## 2. Per-module net connectivity (source → sinks)

Each net below lists where it is driven and everything it reaches, per module.
This is the "welche Pin mit welcher, alle Leitungen, pro Modul" view.

### Power tree (`power_tree.kicad_sch` + `battery.kicad_sch`)
| Net | Source | Sinks |
|---|---|---|
| `+5V_USB` | USB-C VBUS → F1 polyfuse | D3B (SS34) Anode — Dioden-OR in den +5V-Rail (r18.79; Q1-Power-Path entfernt) |
| `+5V_RAIL` | Dioden-OR (r18.79): USB via F1→D3B ‖ TPS61089-Boost (4,97 V) via D3 | PAM8403 PVDD, AP7361C LDO IN |
| `+3V3` | AP7361C-33 LDO OUT (r18.79: Doku-Drift „AP7361A-33ER“ korrigiert — BOM/Schematic haben AP7361C-33Y5-13) | MCU VDD×5+VBAT, MCP23017, PCA9685, PCM5102A, LCD module, encoders' pull-ups |
| `VDDA` | +3V3 via FB1 ferrite | MCU pin 21 (+ 1 µF‖100 nF) |
| `BAT_PLUS` (LiPo+) | J_BAT / charger | TPS61089 VIN, BAT_SENSE divider, MCP73831 VBAT |
| `GND` | star point | everything |

### MCU support (`stm32h743.kicad_sch`)
| Net | From | To |
|---|---|---|
| `HSE_IN`/`HSE_OUT` | MCU PH0/PH1 | Crystal Y1 (+2× 22 pF) |
| `NRST` | MCU pin 14 | SW11 + 10 kΩ PU(+3V3) + 100 nF(GND) |
| `BOOT0_PIN` | MCU pin 94 | SW_BOOT + 10 kΩ PD(GND) |
| `SWDIO`/`SWCLK`/`SWO` | MCU PA13/PA14/PB3 | SWD header J4 (Tag-Connect TC2030) |
| `USB_DM`/`USB_DP` | MCU PA11/PA12 | USBLC6-2SC6 → USB-C D−/D+ |

### LCD (`lcd.kicad_sch`)
| Net | MCU pin | J3 header pin |
|---|---|---|
| `LCD_SCK` | PA5 (29) | 3 (SCL) |
| `LCD_MOSI` | PA7 (31) | 4 (SDA) |
| `LCD_RES` | PC5 (33) | 5 (RES) |
| `LCD_DC` | PC4 (32) | 6 (DC) |
| `LCD_CS` | PA6 (30) | 7 (CS) |
| `LCD_BLK_PWM` | PCA9685 ch15 → Q2 gate | J3 pin 8 (BLK) backlight — low-side via Q2 |

### MCP23017 + PCA9685 + LEDs + Cells (`mcp.kicad_sch`)
| Net | From | To |
|---|---|---|
| `I2C_SCL`/`I2C_SDA` | MCU PB6/PB7 (+4.7 kΩ PU) | MCP23017 + PCA9685 (shared bus) |
| `MCP_INT` | MCP23017 INTA | MCU PC13 |
| `CELL1..5_BTN` | MCP23017 GPA0..4 (PU) | 5 cell keyswitches SW1..5 (Kailh Choc V1, direct-solder THT, digital triggers, r18.75) → GND |
| modifier buttons SW6..10 | MCP23017 GPB0..4 (PU) | 5 tactile buttons (Shift/Hold/Drone/Generate/Clear) |
| `VOL_SW` | EN4 push | MCP23017 GPB5 |
| 5 modifier LEDs | PCA9685 ch0..4 (via 390 Ω) | Shift=gelb, Hold=grün, Drone/Gen/Clear=weiß |
| 10 cell LEDs | PCA9685 ch5..14 (via 390 Ω) | 2 per cell: gelb (base-hold) + grün (shift-hold) |
| `LCD_BLK_PWM` | PCA9685 ch15 | Q2 backlight FET (→ LCD sheet) |

### Encoders (`encoder.kicad_sch`)
| Encoder | A pin | B pin | Switch |
|---|---|---|---|
| EN1 Drive | PA0 (TIM2) | PA1 | PE0 (`DRIVE_SW`) |
| EN2 Brightness | PC6 (TIM3) | PC7 | PE1 (`BRIGHT_SW`) |
| EN3 Display | PD12 (TIM4) | PD13 | PE3 (`DISPLAY_SW`) |
| EN4 Volume | PA8 (TIM1) | PA9 | MCP23017 GPB5 (`VOL_SW`) |
Each A/B has a 10 kΩ pull-up + 100 nF RC debounce; switches pull-up + tactile-to-GND.

### Audio (`audio.kicad_sch`)
| Net | From | To |
|---|---|---|
| `I2S_LRCK`/`I2S_BCK`/`I2S_DOUT` | MCU SAI1 (PE4/PE5/PE6) | PCM5102A LRCK/BCK/DIN |
| PCM5102A analog L/R | DAC OUT | PAM8403 IN + J8 line-out (TRS) |
| `AMP_nSHDN`/`AMP_nMUTE` | MCU PB14/PB15 (+10 kΩ PD) | PAM8403 /SHDN, /MUTE |
| speaker out | PAM8403 BTL | J6 (L+/L−), J7 (R+/R−) → 2× speaker |
| jack-detect | J8 insertion-detect | MCP23017 (auto-mute speakers) |
| `MIDI_TX` | MCU PD5 | **J10** MIDI jack (3.5 mm TRS, via 2× 220 Ω) |

### Battery / charger (`battery.kicad_sch`)
| Net | From | To |
|---|---|---|
| USB VBUS | J1 | MCP73831 VIN (pre-fuse) + F1→D3B→+5V_RAIL (r18.79) |
| charge | MCP73831 VBAT (R21=2k → 500 mA) | LiPo+ |
| boost | TPS61089 (L1 + FB-Teiler R23 121k / R24 39k → 4,97 V; R_ILIM 174k → 5,9 A; Fsw ~440 kHz, r18.79) → D3 | +5V_RAIL |
| `BAT_SENSE` | LiPo+ via 100k:100k | MCU PA3 (ADC) |

---

## 3. Free pins (verified, available for Rev-B / extensions)

PE2(1)*, PC14(8), PC15(9), PC2_C(17), PC3_C(18), PA2(24), PB2(36),
PE7–PE15(37–45), PB10(46), PB11(47), PB12(51), PB13(52), PD9(56), PD10(57),
PD11(58), PD14(61), PD15(62), PC8(65), PC9(66), PA10(69), PA15(77),
PC10–PC12(78–80), PD0–PD4(81–85), PD6(87), PD7(88), PB4(90), PB5(91),
PB8(95), PB9(96).

> *PE2 is the candidate for `LSW_EN` **if** the ADR-0016 sleep load-switch is
> added (currently not). **PA8/PA9 are NOT free** — they're VOL_A/VOL_B.

~50 free GPIOs remain. Many are TIM/DMA-capable for future PWM/encoder needs.

---

## 4. Cross-references
- [`SCHEMATIC_WALKTHROUGH.md`](SCHEMATIC_WALKTHROUGH.md) — prose per sheet + block diagram
- [`BOM_MASTER.md`](../../../BOM_MASTER.md) — part numbers, LCSC, footprints
- [`component_reviews/U1_STM32H743VIT6.md`](../component_reviews/U1_STM32H743VIT6.md) — DS12110 pin verification
- `kicad/generate_kicad_project.py` `NETS` (~2942) + `STM32_PIN_LOC` (~873) — **the source of truth**
