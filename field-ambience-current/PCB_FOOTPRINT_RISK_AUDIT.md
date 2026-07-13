# AMBIENT PCB Footprint Risk Audit

**Stand:** r18.75 (2026-07-01, cells → digital MCP wiring kept, real Kailh Choc V1 keyswitch feel, direct-solder THT instead of an unverified hot-swap socket; Hall path removed)
**Scope:** Risk-based footprint classification per the AMBIENT Footprint Verification Brief. Not a full manual audit of every passive — those pass with package-match only.
**Source of truth:** `BOM_MASTER.md` (Bestell/Lese-Sicht) + `kicad/generate_kicad_project.py` (technisch). Where this audit disagrees with the generator, the generator wins.
**Coverage caveat:** This audit classifies risk and flags what needs to be done before layout/order. It does NOT itself perform pad-by-pad geometry comparison or 1:1 print verification — those are the targeted next actions listed in §12.

---

## 1. Executive Summary

The repository is **NOT blocked by standard passive footprints**. All 0603/0805/1210 R + C land on KiCad-Standard packages and only need the matching audit (BOM ↔ LCSC ↔ KiCad package), which is already encoded in the generator.

The PCB is blocked by **a small number of high-risk parts that still need targeted verification**:

- **POWER_CRITICAL (2):** U8 TPS61089 (VQFN-HR HotRod, custom FP — datasheet layout review not yet done) and L1 Sunlord SWPA6045 (custom FP, current path matters).
- **MECH_CRITICAL (10):** USB-C J1, Audio Jack J8, Battery JST J_BAT, Display Header J3, Encoders EN1–EN4, Cell Keyswitches SW1–SW5 (Kailh Choc V1, direct-solder, digital, r18.75), Modifier Buttons SW6–SW10, Service Buttons SW11 + SW_BOOT, Cell-LEDs (15× under cell-cap windows). None has a *1:1 print or CAD overlay against the enclosure recorded*.
- **EXACT_MODEL_SAFE electrically, MECH_CRITICAL physically (overlap):** all connectors above.
- **PACKAGE_SAFE pinout-pending (9):** U1 STM32H743, U2 MCP23017, U3 PCM5102A, U4 PAM8403H, U5 AP7361C LDO, U6 PCA9685, U7 BQ24074 (r19.18: pin map verified against TI SLUS810N + JLC land pattern, ERC double-check remains cheap), D1 USBLC6, Q2 2N7002. Symbol↔footprint pin mapping must be checked once in KiCad ERC. (Q1 removed r18.79; diode-OR D3B removed r19.18 — single-source rail per ADR-0023.)
- **UNKNOWN (0):** r18.74 briefly reopened this at 1 (a Kailh Choc hot-swap
  socket with no clean manufacturer/LCSC part number). **r18.75 closed it**:
  switched to direct-solder Kailh Choc V1 (CPG135001D01, LCSC C400229) — a
  real, verified part with a footprint + 3D STEP pulled straight from
  LCSC/EasyEDA (`easyeda2kicad --full --lcsc_id=C400229`), same method as
  this repo's other custom footprints. See SW1–SW5 row below.

**Bottom line:** The next step is not a full-footprint audit of everything. It is (a) one datasheet layout pass on TPS61089, (b) a 1:1-print pass against enclosure for the ten mech-critical parts, and (c) one ERC + symbol-pin check on the nine package-safe ICs. After that the project is layout-ready.

---

## 2. Current Repository Footprint Status

### Custom footprints in `field-ambience-current/kicad/libraries/field_ambience.pretty/` (6 total, all actively referenced)

| File | Used by | Source quality |
|---|---|---|
| `Crystal_HC49-US-SMD_ABLS.kicad_mod` | Y1 | EasyEDA-derived, matches MPN ABLS-8.000MHZ-B4-T |
| `Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A.kicad_mod` | U8 (TPS61089RNR) | Custom-built to TI drawing; **needs datasheet layout review** |
| `L_Sunlord_SWPA6045.kicad_mod` | L1 | EasyEDA, r18.20c phantom-name fix verified |
| `Jack_3.5mm_PJ-320D_SMT.kicad_mod` | J8 (and J9 DNP) | EasyEDA, SHOU HAN PJ-320D verified r18.19 |
| `SW_HX_12x12x7.3_SMD-4P.kicad_mod` | SW6–SW10 (5×) | EasyEDA, MPN HX 12×12×7.3 TPFT-B |
| `SW_TS1088_SMD.kicad_mod` | SW11 + SW_BOOT | EasyEDA-verified, MPN XUNPU TS-1088-AR02016 |

### Custom STEP models in `field-ambience-current/kicad/libraries/field_ambience.3dshapes/` (7 total)

All present, all referenced: LQFP-100 (U1), HC-49 SMD (Y1), VQFN-HR (U8), SWPA6045 (L1), PJ-320D (J8/J9), JST-PH-SMD (J_BAT), TS-1088 (SW11/SW_BOOT). Z-height table in `mechanical/3d_models/MANIFEST.md`.

### Everything else
KiCad-Standard libraries (`Package_QFP`, `Package_SO`, `Package_TO_SOT_SMD`, `Resistor_SMD`, `Capacitor_SMD`, `LED_SMD`, `Connector_PinHeader_2.54mm`, `Connector_USB`, `Connector_JST`, `Rotary_Encoder`, `Fuse`, `Diode_SMD`, `Inductor_SMD`). Package-match enforced by generator string.

---

## 3. Component Risk Classification Table

| Ref / Group | Component | MPN | LCSC | KiCad Footprint | Risk Class | Required Check | Current Status | Blocking? | Notes |
|---|---|---|---|---|---|---|---|---|---|
| **U1** | STM32H743VIT6 | STM32H743VIT6 | C114409 | `Package_QFP:LQFP-100_14x14mm_P0.5mm` | PACKAGE_SAFE | Pin 1 + symbol↔pad pin map (ERC) | OK pkg, pinout not re-checked since gen | No | Symbol generated programmatically; verify VCAP/VBAT/BOOT0/VREF+ wiring in ERC |
| **Y1** | HSE crystal | ABLS-8.000MHZ-B4-T | C596838 | `field_ambience:Crystal_HC49-US-SMD_ABLS` | EXACT_MODEL_SAFE | Pad layout + 2× 27 pF C0G placement | OK | No | 2× C107045 already in BOM |
| **J1** | USB-C 16-pin | TYPE-C-31-M-12 (HRO) | C165948 | `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12` | EXACT_MODEL_SAFE + **MECH_CRITICAL** | Board-edge cutout, mouth alignment, shell pads, VBUS/GND/CC1/CC2/D±/SBU | Electrical OK, **mech not verified vs case** | **YES (mech)** | r18.19 revert ensured 16-pin variant; confirm board-edge X/Y once enclosure CAD lands |
| **D1** | USB-C ESD | USBLC6-2SC6 | C2687116 | `Package_TO_SOT_SMD:SOT-23-6` | PACKAGE_SAFE | Pin 1 + D±/CC mapping | OK pkg | No | Standard 6-pin SOT-23, check pins in ERC |
| **F1** | PTC fuse | 1812L300/16GR | C18198349 | `Fuse:Fuse_1812_4532Metric` | DEFAULT_SAFE | Package | OK | No | 3 A trip |
| **D2** | TVS | SMAJ5.0A | C113952 | `Diode_SMD:D_SMA` | DEFAULT_SAFE | Package + polarity mark | OK | No | |
| **U8** | Boost converter | TPS61089RNR | C165129 | `field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A` | **POWER_CRITICAL** | Pad geom incl exposed thermal, pin 1, SW/VIN/VOUT/FB/EN/BOOT, datasheet layout, current loops, L1+C_OUT placement | Pin 11 = SW + thermal verified r18.7; **layout not yet drawn** | **YES** | TI datasheet SLVSD38C §10 recommended layout MUST be followed for HotRod package |
| **L1** | Boost inductor | SWPA6045S2R2NT 2.2 µH (r18.77: was SWPA6045S2R2MT — that MPN doesn't exist per Sunlord's own datasheet) | C36500 (r18.77: was C83455, a dead link) | `field_ambience:L_Sunlord_SWPA6045` | POWER_CRITICAL | Footprint dimensions vs datasheet, current rating ≥3 A, placement next to U8 pin 11 | OK pkg post-r18.20c phantom fix; placement still open | YES (layout) | Verified against Sunlord's official SWPA6045S datasheet (Item 12 table): Isat 6.75 A max / 7.40 A typ, Irms 4.60 A max / 5.00 A typ (r18.77 corrected — previous "4.5A/4.3A" figures here did not match any datasheet row) — confirmed adequate for 5 V/1 A boost |
| **D3** | Schottky | SS34 | C8678 | `Diode_SMD:D_SMA` | DEFAULT_SAFE | Package + polarity | OK | No | If used as boost rectifier check VR/IF; if TVS-style protection treat as default |
| **F2** | Battery-path PTC (r19.18, ADR-0023 — replaces the removed D3B diode-OR row) | SMD1812P260TF/16 | C438899 | `Fuse:Fuse_1812_4532Metric` | DEFAULT_SAFE | 1812 standard footprint | OK | No | 2.6 A hold / 5 A trip / 16 V |
| **U5** | 3.3 V LDO | AP7361C-33Y5-13 | C460397 | `Package_TO_SOT_SMD:SOT-89-5` | PACKAGE_SAFE (close to POWER_CRITICAL) | Pin 1 (1=EN, 2=GND, 3=ADJ, 4=IN, 5=OUT) + thermal pad copper for >500 mA loads | Pinout DS-verified r18.6 | No | Diodes DS confirmed; ensure GND tab has copper area |
| **U7** | Li-Ion power-path charger (r19.18, ADR-0023) | BQ24074RGTR | C54313 | `Package_DFN_QFN:QFN-16-1EP_3x3mm_P0.5mm_EP1.7x1.7mm` | PACKAGE_SAFE + POWER-adjacent | Pin map per TI SLUS810N Table 7-1 ('74 column); footprint matched against JLC/EasyEDA land pattern for C54313 (pitch 0.5, EP 1.7×1.7) | OK pkg | No | ICHG 0.89 A (R_ISET 1k), IIN 1.34 A (R_ILIM_IN 1.2k); EP must tie to VSS |
| **C_BULK** | Polymer tantal 470 µF/10 V | TPSE477K010R0100 (Kyocera) | C444831 | `Capacitor_SMD:CP_Tantalum_Case-E_EIA-7343-43_Reflow` | DEFAULT_SAFE | Pkg + polarity, but **polarised** | OK | No | Case-E 7.3×4.3 mm, ESR 100 mΩ |
| **C_BULK2** | MLCC 100 µF/10 V 1210 | LMK325ABJ107MM-T | C2880380 | `Capacitor_SMD:C_1210_3225Metric` | DEFAULT_SAFE | Pkg | OK | No | Parallel to C_BULK to handle transient ESR |
| **J_BAT** | JST-PH 2-pin SMT | S2B-PH-SM4-TB(LF)(SN) | C295747 | `Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal` | EXACT_MODEL_SAFE + **MECH_CRITICAL** | Orientation vs LiPo pouch slot in bottom case | Electrical OK | YES (mech) | Bottom-case slot per ADR-0011 (LiPo 503759, 2000 mAh) |
| **U3** | DAC | PCM5102APWR | C107671 | `Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm` | PACKAGE_SAFE | Pin 1, I²S pins (BCK/LRCK/DIN), power, analog out, mode pins (FMT/FLT/DEMP/XSMT) | OK pkg | No | Verify mode pin pull strategy in ERC |
| **U4** | Class-D amp | PAM8403DR-H | C17337 | `Package_SO:SOIC-16_3.9x9.9mm_P1.27mm` | PACKAGE_SAFE | Pin 1, OUT_L+/–, OUT_R+/–, power | OK pkg | No | BTL outputs — never short to GND |
| **FB1 / FB2** | Ferrite 0603 600 Ω | BLM18AG601SN1D | C19330/C84094 | `Inductor_SMD:L_0603_1608Metric` | DEFAULT_SAFE | Pkg | OK | No | Power-supply decoupling |
| **J8** | 3.5 mm TRS jack | PJ-320D (SHOU HAN) | C431535 | `field_ambience:Jack_3.5mm_PJ-320D_SMT` | EXACT_MODEL_SAFE + **MECH_CRITICAL** | 4 SMD + 2 NPTH pad layout, switch contact, board-edge | EasyEDA r18.19 vendored | YES (mech) | Cutout must match exact PJ-320D — generic SJ-3523 was wrong, do not regress |
| ~~J9~~ | MIDI jack DNP | – | – | s. J8 | N/A | – | DNP per ADR-0004 r18.30 | No | FP + edge cutout conserved for later reactivation |
| **J6 / J7** | Speaker pads (2-pin) | – | – | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | DEFAULT_SAFE | Pkg | Placeholder OK | No | Manual solder of CMS-402811-28SP leads |
| **U2** | GPIO expander | MCP23017-E/SS | C506653 | `Package_SO:SSOP-28_5.3x10.2mm_P0.65mm` | PACKAGE_SAFE | Pin 1, I²C (SDA/SCL), GPA/GPB ports, A0/A1/A2 address pins, RESET, INT | OK pkg | No | I²C address conflict check vs PCA9685 (U6) — both default 0x20/0x40 ranges, no overlap |
| **U6** | PWM LED driver | PCA9685PW,118 | C2678753 | `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm` | PACKAGE_SAFE | Pin 1, I²C (SDA/SCL), 16× LED outputs, OE, A0–A5 address | OK pkg | No | 16/16 channels used (5 modifier + 10 cell + 1 backlight); confirm no spare-pin shorts |
| **J3** | 1×8 receptacle | (generic) | – | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | DEFAULT_SAFE + **MECH_CRITICAL** | Display module pin order verification | Pkg OK; **pin order vs Waveshare 1.9" module not 1:1 verified** | YES | r18.22 note explicit: Waveshare pin order varies per vendor — must verify against shipped module before final layout |
| **LCD module** | Waveshare 1.9" 170×320 ST7789V2 | – | – | Module (sits in J3) | MECH_CRITICAL | Window cutout + screen-to-bezel alignment | Module-only | YES (mech) | Per r18.22 pivot from bare AliExpress |
| **Q2** | Backlight FET | 2N7002,215 | C8545 | `Package_TO_SOT_SMD:SOT-23` | PACKAGE_SAFE | Pin 1, G/D/S | OK pkg | No | |
| **EN1–EN4** | Rotary encoders | ALPS EC11E18244AU | C202365 | `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` | EXACT_MODEL_SAFE + **MECH_CRITICAL** | Shaft height 20 mm + tab spacing + knob clearance + panel cutout, A/B/C + push-switch pins | Pkg OK per ADR-0012 r18.22 pivot | YES (mech) | All four use same MPN (NRND-pivot consolidated); confirm 3D-printed knob ID matches shaft diameter |
| **SW1–SW5** | Cell keyswitches, direct-solder (digital, r18.75) | Kailh Choc V1, CPG135001D01 | [C400229](https://www.lcsc.com/product-detail/C400229.html) — verified real listing; ⚠ 0 stock at time of writing (restocks common) | `field_ambience:SW_KailhChoc_CPG1350_THT_2P` | EXACT_MODEL_SAFE + **MECH_CRITICAL** | Cap window/optional plate cutouts in top panel, MX 19 mm pitch, 15×15 mm switch envelope, ~3 mm real travel, 2 THT pins + 3 unplated locator holes | Footprint + 3D STEP pulled directly from LCSC/EasyEDA for this exact part via `easyeda2kicad` — sourced from the manufacturer's own listing, same method as this repo's other custom footprints (TS-1088, MST-12D18, PJ-320D) | YES (mech) | Cells went digital on MCP23017 GPA0–4 (ADR-0013 superseded, r18.73); r18.74 tried a hot-swap socket (had no clean part number, needed fiddly hand-soldering); r18.75 simplified to direct-solder — same THT technique as every other button here, switch now permanent instead of swappable. Replaces the former DRV5056A4 Hall + RC. |
| **SW6–SW10** | Modifier tactiles | HX B3F-4055-Y | C36498965 | `field_ambience:SW_TC1212-7.3_THT_4P` | EXACT_MODEL_SAFE + **MECH_CRITICAL** | Cap window cutouts in top panel, button height ≈7.3 mm | Custom FP — verify HX B3F THT pin pattern at GUI-ERC | YES (mech) | All 5 identical — Shift / Hold / Drone / Generate / Clear; same part as the cells |
| **SW11** | Reset tactile | XUNPU TS-1088-AR02016 | C720477 | `field_ambience:SW_TS1088_SMD` | EXACT_MODEL_SAFE + **MECH_CRITICAL** | Service hole in bottom plate aligned to button | EasyEDA-verified r18.14 | YES (mech) | Bottom-plate access hole |
| **SW_BOOT** | BOOT0 service | (= SW11 part) | C720477 | (= SW11 FP) | EXACT_MODEL_SAFE + **MECH_CRITICAL** | Service hole in bottom plate | OK | YES (mech) | DFU-flash access; 1 kΩ pull-up R_BOOT_SW present |
| **R_BOOT_SW** | 1 kΩ 0603 | (= R_CELL) | C21190 | (= R_CELL) | DEFAULT_SAFE | Pkg | OK | No | |
| **Modifier-LED Hold (yellow)** | KT-0603Y | KT-0603Y | C2287 | `LED_SMD:LED_0603_1608Metric` | DEFAULT_SAFE | Pkg + polarity | OK | No | |
| **Modifier-LED Shift (green)** | KT-0603G | KT-0603G | C12624 | `LED_SMD:LED_0603_1608Metric` | DEFAULT_SAFE | Pkg + polarity | OK | No | |
| **Modifier-LEDs Drone/Gen/Clear (3× white)** | XL-1608UWC-04 | – | C965808 | `LED_SMD:LED_0603_1608Metric` | DEFAULT_SAFE | Pkg + polarity | OK | No | |
| **Cell-LEDs 5× yellow + 5× green** | KT-0603Y / KT-0603G | – | C2287 / C12624 | `LED_SMD:LED_0603_1608Metric` | DEFAULT_SAFE + **MECH_CRITICAL** | LED X/Y must align under cell-cap light window | OK electrically; mech position not 1:1 verified vs printed cell caps | YES (mech) | XOR latch per ADR-0008 r2 — each cell has one yellow + one green |
| **LED_HB** | Heartbeat | XL-1608UWC-04 (0603) | C965808 | `LED_SMD:LED_0603_1608Metric` | DEFAULT_SAFE | Pkg | OK (r18.24 fixed 0805 mismatch) | No | |
| **LED_CHRG** | Charger status amber | (generic) | C72041 | `LED_SMD:LED_0603_1608Metric` | DEFAULT_SAFE | Pkg | OK | No | |
| **LED ballast 15× 390 Ω 0603** | 0603WAF3900T5E | – | C23151 | `Resistor_SMD:R_0603_1608Metric` | DEFAULT_SAFE | Pkg | OK | No | Anode-to-+5V, sinked by PCA9685 |
| **J4** | SWD service header | TC2030-IDC 3-pin | – | `Connector_PinHeader_1.27mm:PinHeader_1x03_P1.27mm_Vertical` | EXACT_MODEL_SAFE + light mech | TC2030 pad spacing 1.27 mm — verify against Tag-Connect drawing; alignment notches in PCB | OK pkg | No (light) | Optional dev-only; PCB notches per Tag-Connect TC2030 footprint guide |
| **All §10 generic passives** | 0603 R + C, 0805 C, 1210 C | various | various | `*_SMD:*_1608Metric` / `*_2012Metric` / `*_3225Metric` | DEFAULT_SAFE | Pkg + value | OK (encoded in generator) | No | Generator string-matches package per LCSC |

---

## 4. Components That Do Not Need Full Manual Audit (DEFAULT_SAFE)

All 0603 / 0805 / 1210 / 1812 / SOD passives + simple LEDs + LED ballast resistors + boot pullup. Verification = (a) value matches BOM, (b) LCSC package matches KiCad footprint package. This is encoded in `generate_kicad_project.py` string-by-string and does not need a separate pass.

Specifically: F1 (Polyfuse 1812), D2 (D_SMA), D3 (D_SMA), C_BULK (Case-E reflow), C_BULK2 (1210), FB1/FB2 (L_0603), all §10 R + C, all LEDs except where flagged mech-critical, R_CELL, C_CELL, R_BOOT_SW, all LED ballasts.

---

## 5. Components That Need Pinout Verification Only (PACKAGE_SAFE)

These pass a generic KiCad ERC + a one-time human eyeball comparing symbol pin order against datasheet table. None requires custom geometry.

- **U1 STM32H743VIT6** — LQFP-100, 100 pins; the highest-leverage one to ERC because of VCAP/VBAT/BOOT0/VREF+/PA13/PA14 (SWD) wiring.
- **U2 MCP23017** — SSOP-28, I²C address pins A0/A1/A2.
- **U3 PCM5102A** — TSSOP-20, mode pins (FMT/FLT/DEMP/XSMT) need explicit pull strategy.
- **U4 PAM8403H** — SOIC-16, BTL outputs OUTL±/OUTR±.
- **U5 AP7361C** — SOT-89-5, non-standard pin order 1=EN/2=GND/3=ADJ/4=IN/5=OUT (DS-verified r18.6, easy to wire wrong).
- **U6 PCA9685** — TSSOP-28, 16 LED outputs + I²C address.
- **U7 BQ24074** — VQFN-16 3×3 EP; ISET/ILIM/TS resistors set charge/input current (r19.18).
- **D1 USBLC6-2SC6** — SOT-23-6, D± routing.
- ~~Q1 DMG2305UX~~ — removed r18.79 (diode-OR replaces the power-path).
- **Q2 2N7002** — SOT-23 N-Ch.

**Action:** Run ERC once + visually compare each IC symbol pin numbers against its datasheet Table 4-1 / Pin Configuration. ~30 minutes total.

---

## 6. Mechanically Critical Components

Order roughly by tolerance pain (tight → loose):

1. **SW1–SW5 (Cell keyswitches, digital, direct-solder — r18.75)** — 5× Kailh Choc V1 switches directly soldered on the MX 19 mm grid (15×15 mm envelope, ~3 mm travel), cap windows/optional plate cutouts in the top panel must align to switch centres. Replaces the former Hall sensors under Gateron magnets (no more magnet-to-sensor stack-height constraint) — and replaces r18.73's small tactile button (which had made the cells feel identical to the modifiers) and r18.74's hot-swap socket (unverified sourcing).
2. **EN1–EN4 (Encoders)** — 20 mm shaft + knob alignment with top panel + tab spacing for through-hole mounting tabs.
3. **SW6–SW10 (Modifier buttons)** — 5× cap windows in top panel must align to button centres.
4. **J1 (USB-C)** — board-edge cutout, mouth alignment with side wall, shell-to-shield contact.
5. **J8 (Audio jack)** — board-edge / side wall cutout for plug insertion.
6. **J_BAT (Battery JST)** — orientation vs LiPo pouch slot in bottom case.
7. **J3 + LCD module** — pin order varies per Waveshare batch (r18.22 note), and the screen window + bezel must align.
8. **SW11 + SW_BOOT** — service holes in bottom plate.
9. **Cell-LEDs (10×)** — each must sit under the cell-cap's light window for the XOR-latch indicator to be visible.
10. **J6 / J7 (Speaker leads)** — pad position vs speaker driver position when mounted in top panel.

**Action:** Once `mechanical/coordinates/mechanical_coordinates.md` covers all of these, generate a 1:1 print of the PCB + enclosure outline + cutouts and physically check.

---

## 7. Power-Critical Components

Only two:

### U8 TPS61089RNR (boost converter, VQFN-HR HotRod 2×2.5 mm)
- Exposed thermal pad on pin 11 = SW (TI DS SLVSD38C Table 6-1 verified r18.7).
- **Datasheet §10 "Layout Guidelines" not yet visibly executed**: input cap on VIN/PGND with shortest possible loop; output cap on VOUT/PGND with shortest possible loop; SW node small but with enough copper to handle inductor current; do NOT run sensitive signals under SW node.
- L1 (SWPA6045) must sit immediately next to SW pin with thick polygon.
- Recommend GND pour on inner layer right under U8 + L1.

### L1 Sunlord SWPA6045S2R2NT (r18.77: corrected from the nonexistent "...MT" MPN)
- Custom FP from EasyEDA (r18.20c phantom-name fix). Pad geometry should match Sunlord drawing — **not yet 1:1 verified against the PDF**.
- Saturation Isat 6.75 A max / 7.40 A typ (r18.77: corrected from a fabricated "4.5 A" figure that didn't match any datasheet row) is fine for 5 V/1 A boost (peak ≈1.5 A).

### Soft-power-critical (mentioned for completeness, not flagged blocking):
- U7 BQ24074 charger: QFN-16 exposed pad MUST be soldered to a VSS copper pour with thermal vias (linear power path dissipates ~0.7 W worst case while charging).
- U5 AP7361C LDO: same — copper for the GND tab (pin 2) to act as heatsink at the typical 100-300 mA loads here.

---

## 8. Missing or Ambiguous Footprints (UNKNOWN)

**None as of r18.75.** Three historic UNKNOWNs were closed:
- Phantom `L_0630` name on the Sunlord inductor → fixed r18.20c to `L_Sunlord_SWPA6045`.
- Dead `RotaryEncoder_ALPS_EC11J_SMD.kicad_mod` (left over from ADR-0012 pre-pivot) → deleted r18.36.
- Cell keyswitch hot-swap socket (r18.74, no clean manufacturer/LCSC part number) → closed r18.75 by switching to a direct-solder Kailh Choc V1 (C400229) with a footprint sourced straight from LCSC/EasyEDA.

No "placeholder" footprints remain. (The former Hall-sensor placeholder note is moot — the Hall path was removed in r18.73 when the cells went digital on the MCP23017.)

---

## 9. JLC/LCSC Matching Issues

**None open as of r18.36.** Historic mismatches that were caught and fixed:

| When | Issue | Fix |
|---|---|---|
| r18.14 | USB-C: BOM said C165935 (= STF18N65M5 MOSFET, wrong part); then r18.10 had switched to C283540 (= TYPE-C-31-M-17, 6-pin power-only) | Reverted to **C165948** (TYPE-C-31-M-12, 16-pin full USB-C) in r18.19 |
| r18.14 | SW_BOOT MPN was TS-1185A in BOM | Corrected to **TS-1088-AR02016** (C720477) |
| r18.19 | Audio jack FP was CUI SJ-3523-SMT-FP, pad layout didn't match SHOU HAN PJ-320D | Custom `field_ambience:Jack_3.5mm_PJ-320D_SMT` vendored |
| r18.19 | Speaker schematic value was stale PUI AS04008PS 4 Ω | Updated to CMS-402811-28SP 8 Ω cloth-cone primary |
| r18.24 | LED_HB had LCSC C965818 (= XL-2012UWC 0805, wrong package) | Corrected to **C965808** (XL-1608UWC-04 0603) |
| r18.20c | Sunlord inductor had phantom FP name `L_0630` | Renamed to `L_Sunlord_SWPA6045`, EasyEDA-CAD vendored |

**LCSC string-diff performed r18.37:**

```
BOM unique LCSC codes:        60
Generator unique LCSC codes:  53

BOM-only codes (8): C12624, C2287, C2880380, C1804, C2030, C165935, C283540, C965818
Generator-only codes (1): C23140
```

Breakdown:
- **C12624 (green LED), C2287 (yellow LED), C2880380 (100 µF MLCC)** — all present in generator, just at non-LCSC-tagged reference lines that the regex missed. No actual mismatch.
- **C1804** — labelled "auto-generated" in BOM §10 passives sortiment, no specific component slot. Reference-only.
- **C2030** — false positive: regex matched the "2030" in "TC2030-IDC" (Tag-Connect dev-header name).
- **C165935, C283540, C965818** — historical mentions in the BOM audit-trail (USB-C MOSFET-mismatch, M-17 6-pin-power-only revert, 0805-vs-0603 LED fix). Doc-only, all already corrected.
- **C23140 (33 Ω 0603 I²S series-termination)** — lives in `pi_sheet()` which is marked `LEGACY r18: nicht mehr geschrieben`. `main()` does NOT call `(OUT_DIR / "pi.kicad_sch").write_text(pi_sheet())`, so this resistor never lands in any generated schematic. Effectively dead code; not a BOM gap.

**Conclusion: 0 actual JLC/LCSC mismatches between BOM and generator.** Re-run this diff before order to catch any new doc-drift.

---

## 10. Blocking Issues Before PCB Layout

In priority order:

1. **TPS61089 layout pre-study.** Read TI SLVSD38C §10, sketch the U8 + L1 + C_IN + C_OUT placement on paper or in KiCad before committing routes — re-routing a HotRod boost layout after the fact is painful.
2. **ERC + symbol-pin walkthrough** of the 9 PACKAGE_SAFE ICs (§5). One sitting, ~30 min.

These two are *layout-blocking*. The mech-critical items in §6 are *order-blocking* but not layout-blocking — layout can proceed in parallel with enclosure CAD.

(Previously a third blocker was listed — the Hall-sensor footprint — but verification of `generate_kicad_project.py` line 3342 showed the FP was already finalised to `Package_TO_SOT_SMD:SOT-23` in r18.20. The blocker was a BOM_MASTER doc-drift, not an actual generator state. Closed in r18.37.)

---

## 11. Blocking Issues Before JLC Order

In priority order:

1. **1:1 print + enclosure overlay** for all ten MECH_CRITICAL parts (§6). Cells / encoders / USB-C / audio jack are the highest-pain ones.
2. **TPS61089 final layout review** against TI DS §10 recommended layout, plus 3D-render check that U8 + L1 + bulk caps don't collide with bottom-case battery slot.
3. **L1 SWPA6045 pad geometry 1:1 print** vs Sunlord drawing (we trust EasyEDA, but the inductor sits on the boost loop so a 0.2 mm pad mismatch could matter for current path / soldering quality).
4. **Waveshare 1.9" display pin order verification** against the actually-shipped module (r18.22 note — varies per vendor batch).
5. **BOM ↔ generator LCSC re-diff** + JLC assembly preview check (pin 1 markers, polarity arrows, orientation).
6. **Confirm JLC has stock** of all parts marked "Extended" — particularly C444831 (Polymer Tantal C_BULK), which had only 517 stock at r18.20b — and consider re-checking the day before order.

---

## 12. Recommended Next Actions

### Today / this week (cheap, unblocks layout)

- [x] ~~Switch Hall sensor footprint to SOT-23 in generator.~~ Already done in r18.20; doc-fixed in BOM_MASTER r18.37.
- [x] ~~BOM ↔ generator LCSC string-diff.~~ Performed r18.37, 0 actual mismatches.
- [ ] One sitting of ERC + symbol-pin-vs-datasheet walkthrough for U1, U2, U3, U4, U5, U6, U7, D1, Q1, Q2 (≈30 min). Requires KiCad GUI — not autonomous.

### Before committing to layout (technical study)

- [ ] Print TI SLVSD38C and sketch TPS61089 + L1 + caps placement on paper or KiCad before drawing routes.
- [ ] Print Sunlord SWPA6045 drawing and overlay against the custom FP at 1:1 to confirm pad geometry.

### Before sending to JLC (mech + paperwork)

- [ ] 1:1 print of full PCB outline + cutouts vs enclosure / bottom case for the 10 MECH_CRITICAL parts. Cell keyswitch (Kailh Choc V1, direct-solder) positions get an extra pass with a digital caliper (no more Hall/magnet — r18.73/75).
- [ ] Bring up actually-shipped Waveshare 1.9" LCD module and verify pin order on J3.
- [ ] Generate BOM + CPL, load into JLC, walk through assembly preview screen-by-screen looking for flipped pin-1 markers and missing footprints (esp. the 6 custom ones).
- [ ] Re-check JLC stock the day of order, especially C444831.

### Not on this audit's critical path

- Drum/menu plans, V2/world refactor, mechanical post-PCB (knobs, cell-caps STL) — none of these block PCB-layout or JLC-order.

---

## Final Decision Language

```
The PCB is not blocked by standard passive footprints.
The PCB is blocked only by unresolved critical mechanical, connector, and power footprints —
specifically: TPS61089 datasheet layout review, LCD pin-order verification, and 1:1 print
of the ten mech-critical parts against enclosure.
The next step is not a full-footprint audit of everything,
but targeted verification of those high-risk parts and a one-time ERC pass on the IC pinouts.

r18.37 cleared two items from the prior list:
  - Hall-sensor FP was already SOT-23 in the generator (BOM doc-drift, fixed).
  - LCSC string-diff between BOM and generator showed 0 actual mismatches.
```
