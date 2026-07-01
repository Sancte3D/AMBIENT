# PCB Handoff Package — what a PCB engineer needs, and what's still open

This is the orientation doc for handing the AMBIENT board to a PCB engineer (or
JLCPCB). It says what's in the package, the exact workflow, and the **hard
blockers** that must be cleared before a fab order.

> **The generator is the truth.** `kicad/generate_kicad_project.py` produces the
> schematics; if any doc disagrees with it, regenerate and trust the generator.
> The BOM is now produced *from* the schematic via `kicad/export_jlc_bom.py`, so
> it can't silently drift.

---

## 1. What's in the package (committed, current)

| Item | Path | State |
|---|---|---|
| KiCad project | `field-ambience-current/kicad/field_ambience.kicad_pro` | ✅ generated |
| Schematics (7 sheets + root) | `field-ambience-current/kicad/*.kicad_sch` | ✅ generated, complete connectivity |
| Schematic generator (source of truth) | `field-ambience-current/kicad/generate_kicad_project.py` | ✅ |
| Footprint library (custom) | `field-ambience-current/kicad/libraries/field_ambience.pretty/` (6 footprints) | ✅ |
| 3D STEP models | `field-ambience-current/kicad/libraries/field_ambience.3dshapes/` (7 models) | ✅ |
| Footprint lib table | `field-ambience-current/kicad/fp-lib-table` | ✅ |
| **Complete pin/net map** | `docs/hardware/PINMAP.md` | ✅ every pin + per-module nets |
| Schematic walkthrough (prose + block diagram) | `docs/hardware/SCHEMATIC_WALKTHROUGH.md` | ✅ |
| BOM (human) | `BOM_MASTER.md` | ✅ |
| **JLC production BOM (machine, from schematic)** | `field-ambience-current/kicad/jlc_bom.csv` | ✅ 57 LCSC parts, all sourced |
| BOM exporter | `field-ambience-current/kicad/export_jlc_bom.py` | ✅ re-run after any generator change |
| Component datasheet reviews | `docs/component_reviews/` | 🟡 U1 + Y1 done, rest pending |
| Layer-stack decision | `docs/decisions/ADR-0018` (4-layer) | ✅ decided |
| Mechanical requirements | `docs/hardware/MECHANICAL_REQUIREMENTS.md` | 🟡 most defined, dims TBD |

---

## 2. What is NOT in the package (and why)

| Missing | Why | Who produces it |
|---|---|---|
| `.kicad_pcb` board layout | generator is schematic-only; no placement/routing exists | PCB engineer in KiCad GUI |
| Netlist (`.net`) | export from KiCad GUI after opening the project | KiCad GUI (one click) |
| **CPL / pick-and-place** | needs per-part X/Y/rotation from a routed `.kicad_pcb` — doesn't exist yet | after layout, from KiCad |
| Gerbers / drill | produced after layout | after layout, from KiCad |
| ERC report | **no `kicad-cli` in the dev env** — couldn't run headless | run in KiCad 9 GUI (`scripts/run_erc.sh` if you have kicad-cli) |

---

## 3. Engineer workflow (from these files to a fab order)

1. `cd field-ambience-current/kicad && python3 generate_kicad_project.py` →
   regenerates the 8 `.kicad_sch` + `.kicad_pro`.
2. Open `field_ambience.kicad_pro` in **KiCad 9**.
3. **Run ERC** (Inspect → Electrical Rules Checker). Target per
   `ERC_DRC_CHECKLIST.md`: **0 errors**, only the listed intentional warnings.
4. **Create the board**: Tools → Update PCB from Schematic (F8). Footprints are
   pre-assigned by the generator.
5. **Place** mechanically-fixed parts first (USB-C, jack, display header J3, the
   5 cell switches (SW1–5), 5 modifier buttons (SW6–10), 4 encoders, speakers,
   battery JST) — see `MECHANICAL_REQUIREMENTS.md` §2 for the order + the
   `PINMAP.md` for what each pin drives. Then power, MCU+decoupling, audio, I/O.
6. **Set up the 4-layer stack** + net classes per `ADR-0018` + `KICAD_BLUEPRINT.md` §6.
7. **Route** (power wide, USB-D± over solid GND, Class-D away from analog audio,
   crystal close to MCU) — rules in `ADR-0018` + `ADR-0010`.
8. **DRC** → 0 errors per `ERC_DRC_CHECKLIST.md`.
9. **Export** Gerbers + drill + the CPL/pick-and-place. For the BOM, use the
   committed `kicad/jlc_bom.csv` (re-run `export_jlc_bom.py` if the schematic
   changed).
10. Upload Gerbers + `jlc_bom.csv` + CPL to JLCPCB.

---

## 4. HARD BLOCKERS before a fab order

These must be cleared first:

1. **ERC pass** in KiCad GUI (0 errors). *Not runnable in the dev env — no
   kicad-cli.* This is the #1 gate.
2. **PCB layout + routing** — the entire board layout is greenfield.
3. **PCB outer dimensions + mechanical coordinates** — depends on the enclosure
   CAD. Currently TBD (`MECHANICAL_REQUIREMENTS.md` §0). USB-C / jack / display
   window / encoder centers / cell grid / speaker openings must be fixed.
4. **DRC pass** + Gerber/CPL export.

### At schematic (re)build — 2 drop-ins (everything else is in the generated sheets)
The generated `.kicad_sch` are the reference; the **production BOM is
[`../../PCB_BOM.md`](../../PCB_BOM.md)** and the pin map is `PINMAP.md`. Two
final decisions are in BOM/ADR but not yet in the old generated schematic — add
them when you build the board:
1. **Power-off block** `U_PWR`+`SW_PWR`+`R_PWR_PD`+`C_PWR_SW` — **pin-level
   drop-in spec in `ADR-0016`** (VIN/VOUT/ON + the one LDO-input reroute). The
   `SW_PWR` footprint (`field_ambience:SW_MST-12D18_SlideSwitch_RA`, +STEP) is in
   the repo. ~10 min.
2. **Modifier buttons** = THT part C2845240 → use the **verified THT footprint
   `field_ambience:SW_TC1212-7.3_THT_4P`** (+STEP, in repo), not the old SMD one.

### Cheaper open items (not full blockers, but fix before order)
- **Speaker connection** is bare 2-pin headers (J6/J7) + manual wire — decide if
  a JST connector + strain relief is wanted for robustness.
- **Display** frozen at **1.9″** for this rev (verified). 2.0″ is a future Rev-B
  pending physical module verification (SKU/pin-order/dims) — `ADR-0015`.
- **Power-off** is decided (`U_PWR` load-switch + side `SW_PWR` slide switch,
  `ADR-0016`) — "dark but still charges". It's a schematic-build drop-in (above),
  not optional anymore.
- **Datasheet hygiene** — the *parts* are correct; two repo PDFs are wrong
  variants (see `kicad/datasheets/`), fetch the right TPS61089RNR + ALPS EC11J
  datasheets before sign-off.
- Remaining **component datasheet reviews** (`docs/component_reviews/`) — U1+Y1
  done; U2/U3/U4/U5/U6/U7/U8 etc. are bench-proven from v0.6 but not formally
  re-reviewed.

---

## 5. Quick facts
- **Production BOM (on-PCB only):** [`../../PCB_BOM.md`](../../PCB_BOM.md). Machine: `kicad/jlc_bom.csv`.
- **MCU:** STM32H743VIT6, LQFP-100. ~30 GPIOs used, ~50 free (`PINMAP.md`).
- **Power:** USB-C 5V ‖ LiPo→boost → +5V_RAIL → **U_PWR (power-off)** → +5V_SW → LDO → +3V3. Charger MCP73831 (charges while off).
- **Audio:** I²S → PCM5102A DAC → PAM8403 Class-D (2 speakers) + PJ-320D line-out (J8). **MIDI-out = 3.5 mm TRS J10** (Type A, 3.3 V).
- **LEDs:** 15 mono status via PCA9685 #1 (Shift=gelb, Hold=grün, G/D/C=weiß; 2/cell gelb+grün) + **8 white VU meter** via PCA9685 #2 (firmware-driven). No RGB.
- **Controls:** 4× identical push-encoders (EC11E18244AU) · 5 cell keyswitches (Kailh Choc V1/V2 hot-swap socket, real ~3mm travel, r18.74) + 5 modifier buttons (HX B3F-4055 THT tactile, C36498965, square-head for caps) — both digital on the MCP23017 I²C expander, deliberately different feel (r18.73/74, ADR-0013 superseded; was Gateron-magnetic + DRV5056 Hall).
- **Layer stack:** 4-layer, 1.6 mm, JLCPCB default (`ADR-0018`).
- **JLC BOM:** 57 verified LCSC parts; only **2× 220 Ω MIDI resistors** still need an LCSC PN.
