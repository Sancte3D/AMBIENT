# Hardware Engineer — Onboarding

## TL;DR — current state

- Schematic is **generator-based** (`kicad/generate_kicad_project.py` is the source of truth, `.kicad_sch` files are output)
- 8 hierarchical sheets: power_tree, stm32h743, lcd, mcp, encoder, audio, battery + root
- 100/100 STM32H743 pins connected, hier↔root crossref clean, paren-balance clean
- **No `.kicad_pcb` yet** — see [`PCB_LAYOUT_STATUS.md`](../../PCB_LAYOUT_STATUS.md)
- Current revision: r18.6 (engineering verification pass complete)

## What to read

0. **NEW — [`docs/hardware/KICAD_BLUEPRINT.md`](../hardware/KICAD_BLUEPRINT.md)** — idiot-proof step-by-step: open project → ERC → footprints → Update-PCB-from-Schematic → board setup (stackup, design rules, track widths) → placement/routing order → DRC → Gerber/JLC. Start here if you've never laid out a PCB. The key fact: **you don't draw the schematic, it's generated** — your work is ERC + the layout.
1. [`PCB_LAYOUT_STATUS.md`](../../PCB_LAYOUT_STATUS.md) — current state, current blockers, what's verified and what's `FP_VERIFY`
2. [`field_ambience_pcb_SPEC_v0.7.md`](../../field_ambience_pcb_SPEC_v0.7.md) §5 — full STM32H743 pin allocation (verified against DS12110 Rev 5 Table 8 AND the official KiCad symbol library, 52/52 used pins identical)
3. [`docs/component_reviews/`](../component_reviews/) — verified parts with datasheet quotes
4. [`docs/decisions/`](../decisions/) — ADR-0002 (RP2350 → STM32H743 migration), ADR-0004 (open MIDI decision)
5. [`docs/hardware/ERC_DRC_CHECKLIST.md`](../hardware/ERC_DRC_CHECKLIST.md) — what must pass before layout / order

## How the generator works

```bash
cd field-ambience-current/kicad
python3 generate_kicad_project.py
```

This regenerates all `.kicad_sch` files plus `field_ambience.kicad_pro`. The
script is ~5800 lines, organized as one function per sheet plus helper
symbol/footprint factories. Key conventions:

- **Y-DOWN screen coordinates** for placement; **Y-UP** for lib symbol pins. `pin_abs()` handles the conversion
- Each placement uses a **deterministic seed** so UUIDs stay stable across re-runs (no spurious diffs)
- Every component instance must have `MPN`, `LCSC`/`Manufacturer`, and where relevant `FP_VERIFY` or `PIN_VERIFY` properties

After editing, **always**:
```bash
python3 generate_kicad_project.py
```
…and inspect the diff. The generator is the truth; the schematic is a build artifact.

## Validation done in CI / by script (not GUI-ERC)

- Paren-balance on every `.kicad_sch`
- 100/100 STM32 pin connectivity check (wire-endpoint or `no_connect`)
- Hier-label ↔ root-pin crossref (must match per sheet)
- Root-level local labels must come in pairs (bridge wires)

What is **not** scripted (because `kicad-cli` isn't in this environment):

- Full ERC (must run in KiCad 9 GUI before layout)
- DRC (no `.kicad_pcb` yet)
- Net-name conflict against power-net symbols

See `ERC_DRC_CHECKLIST.md` for which warnings are intended vs. forbidden.

## Custom footprints

- `kicad/libraries/keyswitch-kicad-library/` — vendored kiswitch v2.4 (Kailh hotswap + stabilizers)
- `kicad/libraries/field_ambience.pretty/` — repo-local custom footprints (currently: `SW_HX_12x12x7.3_SMD-4P` from LCSC C36498966 user-verified data)

`fp-lib-table` adds both with `${KIPRJMOD}` paths.

## Top blockers (you can pick any of these up)

1. **B-FP**: ~10 `FP_VERIFY` properties open — each needs cross-check against the manufacturer mechanical drawing. List in `PCB_LAYOUT_STATUS.md`
2. **B3**: Run ERC in KiCad 9 GUI, log results, close real errors, document intentional warnings
3. **ADR-0004**: MIDI design decision — 5 axes, see ADR. Don't add MIDI parts without deciding all 5
4. **Phase 5 profiling gate**: get the audio engine running on real STM32H743 hardware (Nucleo-H743 + I²S DAC) and prove < 40% CPU. Until this passes, layout work is premature

## What not to touch

- The schematic files directly. Edit `generate_kicad_project.py`
- `firmware-c-next/` for non-firmware changes
- `legacy_pico2/` (archived, kept for git history)
