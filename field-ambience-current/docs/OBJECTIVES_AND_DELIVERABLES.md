# Field Ambience — Objectives, Deliverables & Component Reference

**Status of THIS document:** honest reference, last verified **2026-07-07**.
Written after a critical hallucination audit (below). It exists because a large
part of this repo was AI-generated and Aron rightly asked: *which of it can I
trust before I commit hardware?*

**How to read the flags** — every non-trivial claim carries one:

- ✅ **MACHINE-VERIFIED** — re-measured or re-run from source on 2026-07-07
  (build, `nm`/`size`, test suite, or a datasheet PDF actually opened). Trust it.
- ⚠️ **NEEDS HUMAN CHECK** — an AI *wrote* this as "verified" but it is **not
  independently confirmable by an AI**. Confirm against the datasheet / vendor
  page yourself before relying on it.
- 🔴 **BENCH-PENDING** — has **never run on real hardware**. There is no
  prototype. Cannot be validated until a board exists.
- ✍️ **HUMAN TO AUTHOR** — product intent that only Mischi should write. The
  factual description below is what the *current code/schematic implements*, so
  you can confirm or correct it — not a substitute for your own objective.

---

## 0. Hallucination audit — read this first

What I could re-measure from source today, and what I could not:

| Area | Verdict | Evidence |
|---|---|---|
| RAM/CPU distribution (`RESOURCE_BUDGET.md`) | ✅ real | cross-build + `size`: DTCM 90.9 %, RAM_D1 96.4 %, RAM_D2 95.9 %, D3/ITCM 0 % — matches the doc |
| Biggest RAM symbols (reverb 152 K, echo 207 K, FFT scratch 128 K, …) | ✅ real | `nm --print-size` matches every figure |
| Firmware builds to a flashable image | ✅ real | ARM cross-build links, FLASH 10.2 %, no region overflow |
| Host test suite (engine, harmony, tuning, DSP, …) | ✅ real | 61 suites, **0 failures**, re-run today |
| APS6404L (PSRAM) pinout | ✅ real | checked against AP Memory datasheet Rev 2.1 PDF |
| **JLC stock counts + Basic/Extended tiers** (`gen_bom_overview.py`) | ⚠️ **unconfirmable** | header claims "checked via JLC's SMT parts API" — an AI cannot be trusted to have done this. 47 parts carry these numbers; ~10 have none. **Re-check every one on the LCSC/JLC page before ordering.** |
| LCSC part ↔ exact variant + pinout, for the ICs/connectors/regulator | ⚠️ needs check | prior-session "datasheet-verified" claims are not AI-trustworthy |
| Custom / generic footprints (APS6404L SOIC-8, encoder, jack, switches) | ⚠️ needs check | pull exact land pattern + confirm pin-1 (per `AI_READY_SCHEMATIC_STANDARD.md`) |
| Audio "measured −xx dBFS" figures | ⚠️ soft | from **host renders**, not a real DAC — indicative, not hardware-true |
| Compliance (CE/EMC/RoHS/UN38.3, `SOURCING_COMPLIANCE.md`) | ⚠️ plan only | a list of *what is required* — **nothing has been tested or certified** |
| Anything on real silicon (CPU headroom, audio timing, async display, PSRAM, boost, pop sequence, GUI-ERC) | 🔴 bench-pending | the board has never existed |
| Enclosure / mechanical images Aron referenced | ❌ not in repo | only 3 audio spectrograms exist under `docs/audio/`. **Supply them yourself — I will not fabricate images.** |

**Bottom line:** the *engineering substrate* (firmware, RAM/CPU budget, tests,
schematic connectivity) is measured and real. The *supplier/assembly/compliance
metadata and everything hardware-behavioural* is either AI-asserted (re-check)
or untested (bench). Design/firmware maturity ≈ 90 %; **product** ≈ 40 %,
because the whole hardware-validation phase is still ahead.

---

## 1. What the device IS  ✍️ HUMAN TO AUTHOR (Mischi)

> This section is intentionally **yours to write** — Aron asked for the human
> objective, not more AI text. Below is a factual description of what the
> current design *actually implements*, so you can confirm/correct it rather
> than start from a blank page.

**As currently built:** a handheld, battery-powered **ambient synthesizer**.
The player triggers notes on 5 keyswitch "cells", shapes them with 5 modifier
buttons + 4 rotary encoders, and reads a 1.9" colour LCD. Sound is a generative
ambient engine (pads, drones, a harmonic-safety note picker, reverb/echo/blur/
shimmer/tape, world presets, just/equal tuning). Audio out via a PCM5102A DAC →
phones/line-out jack (TPA6132A2 headphone amp, r19.19) + a Class-D speaker amp.
MIDI out on a TRS jack. USB-C charging +
firmware flash. STM32H743 MCU.

**Please confirm or correct:** target user, price point, the "must sound like X"
reference, which features are must-have vs nice-to-have, form factor / size
limits, and anything the code got wrong about your intent.

---

## 2. Complete component list (from the generated BOM — the source of truth)

Grounded in `kicad/jlc_bom.csv` (regenerated from the schematic generator).
**58 placed part-types.** ⚠️ = confirm supplier status + footprint before order.

### Core
| Ref | Part | Role | LCSC | Check |
|---|---|---|---|---|
| U1 | STM32H743VIT6 (LQFP-100) | MCU, 480 MHz Cortex-M7 | C114409 | ⚠️ |
| U9 | APS6404L-3SQN-SN | 8 MB QSPI-PSRAM (ADR-0022) | C3028887 | ⚠️ footprint |
| Y1 | ABLS-8.000MHZ crystal | 8 MHz reference | C596838 | ⚠️ |

### Power
| Ref | Part | Role | LCSC | Check |
|---|---|---|---|---|
| U8 | TPS61089 boost | battery 3.7 V → 5 V | C165129 | ⚠️ |
| U5 | AP7361C-33 LDO | 5 V → 3.3 V | C460397 | ⚠️ |
| U_PWR | TPS22918 load switch | power-off gate (ADR-0016) | C131941 | ⚠️ |
| U7 | BQ24074RGTR | Li-Ion power-path charger, DPPM (r19.18, ADR-0023) | C54313 | ⚠️ |
| L1 | SWPA6045S2R2NT 2.2 µH | boost inductor | C36500 | ⚠️ |
| D3 | SS34 | boost-output series diode (rail isolation; D3B removed r19.18) | C8678 | ⚠️ |
| F2 | SMD1812P260TF/16 PTC 2.6 A | battery hard-short backup (r19.18, ADR-0023) | C438899 | ⚠️ |
| D1 | USBLC6-2SC6 | USB ESD | C2687116 | ⚠️ |
| D2 | SMAJ5.0A | 5 V TVS | C113952 | ⚠️ |
| F1 | 1812L300 PTC 3 A | USB fuse | C18198349 | ⚠️ |
| J1 | TYPE-C-31-M-12 | USB-C | C165948 | ⚠️ |
| J9 | JST-PH S2B | battery connector | C295747 | ⚠️ |
| SW_PWR | ALPS SSSS811101 | slide power switch | C109335 | ⚠️ footprint |
| — | LiPo 2000 mAh (Adafruit 2011) | battery (off-board) | — | ⚠️ DE sourcing + polarity |

### Audio
| Ref | Part | Role | LCSC | Check |
|---|---|---|---|---|
| U3 | PCM5102A | I²S DAC | C107671 | ⚠️ |
| U4 | PAM8403 | Class-D speaker amp | C17337 | ⚠️ |
| J8 / J10 | PJ-320D ×2 | phones+line-out / MIDI-out TRS | C431535 | ⚠️ pad map + TRS-A/B |
| U11 | TPA6132A2RTER | DirectPath headphone amp (r19.19, ADR-0024) | C69901 | ⚠️ |
| FB1/FB2 | BLM18AG601 | supply ferrites | C19330 | ⚠️ |
| SPK ×2 | CMS-402811-28SP | speakers (off-board) | — | ⚠️ |

### I/O, controls, display
| Ref | Part | Role | LCSC | Check |
|---|---|---|---|---|
| U2 | MCP23017 | I²C GPIO expander | C506653 | ⚠️ |
| U6 | PCA9685 | 16-ch PWM (LEDs + backlight) | C2678753 | ⚠️ |
| EN1-4 | ALPS EC11E (w/ switch) | 4 encoders | C202365 | ⚠️ footprint |
| SW1-5 | Kailh Choc V1 CPG135001D01 | 5 cell keyswitches (hand-fit) | C400229 | ⚠️ stock/footprint |
| SW6-10 | HX B3F-4055 | 5 modifier buttons (hand-fit) | C36498965 | ⚠️ footprint |
| SW_BOOT / SW11 | TS-1088 | service buttons | C720477 | ⚠️ |
| LCD | Waveshare 1.9" ST7789V2 (off-board) | display | — | ⚠️ pin order per module |
| Q2 | 2N7002 | backlight FET | C8545 | ⚠️ |
| LEDs | KENTO KT-0603 Y/G, XL-1608 W/O | 16 status LEDs | C2287/C12624/C965808/C965800 | ⚠️ |
| J3 / J6 / J7 | 1×8 / 1×2 headers | LCD / speaker headers | C124383 / C124375 | ⚠️ |
| J4 | Tag-Connect TC2030-IDC | SWD debug (no LCSC) | — | hand-fit |

### Passives & service
100 nF / 1 µF / 10 µF / 22 µF / 470 µF-tant caps, 220R–100k resistors, VCAP
2.2 µF, boost-comp R/C, LED series R — all 0603/0805/1210, generated. ⚠️ values
are set in the generator; **confirm the 4 electrolytic/tantalum + the boost
divider values against the TPS61089 datasheet during layout.**

> Full machine BOM for upload: `kicad/jlc_bom.csv`. Browsable version with
> supplier links: `docs/hardware/bom_overview.html` — **but its JLC
> stock/assembly badges are the ⚠️ AI-asserted data from §0. Verify on each
> LCSC page before ordering.**

---

## 3. Deliverables — what exists vs what is open

| Deliverable | State |
|---|---|
| Generated schematic (7 sheets, `generate_kicad_project.py`) | ✅ exists, geometric-ERC clean |
| GUI-ERC in KiCad (v7+) | 🔴 **Aron to run** — final gate before layout |
| PCB layout / routing / Gerbers | 🔴 **Aron** — not started |
| BOM (machine + human) | ✅ generated · ⚠️ supplier data to re-verify |
| Firmware (engine + HAL, flashable image) | ✅ cross-builds · 🔴 never flashed |
| Host test suite | ✅ 61 suites green |
| QSPI-PSRAM firmware | ✅ written · 🔴 bench-pending (dummy cycles to tune) |
| Bring-up runbook + on-device diagnostics | ✅ exists (`BRING_UP.md`, hold CELL1) |
| Enclosure / mechanical | 🔴 not finalized · ❌ images not in repo |
| Compliance (CE/EMC/UN38.3) | ⚠️ dossier only · 🔴 no testing |

---

## 4. Before you order or fab — the human checklist
1. **GUI-ERC** the schematic in KiCad, resolve every warning.
2. **Re-verify every LCSC part**: right variant, in stock, JLC-assemblable —
   do **not** trust the AI stock/tier numbers.
3. **Pull the real footprint** for each ⚠️-flagged custom/generic part and
   confirm pin-1 / pad map against the datasheet.
4. **Confirm the power design** (boost divider, ILIM, comp) against the
   TPS61089 datasheet during layout.
5. **Battery**: confirm DE-sourceable cell + plug polarity vs J9 pin 1.
6. Only then: layout → DRC → Gerber + CPL → order.

---

## 5. Images
Aron: the enclosure/mechanical images you referenced are **not in this repo**
(only audio spectrograms under `docs/audio/reference-matching/` exist). Add
them under `docs/mechanical/` and link them here — I deliberately did not
invent or embed images that were never shared into the codebase.
