# Manufacturing — Onboarding

## You can't order yet

Read [`PCB_LAYOUT_STATUS.md`](../../PCB_LAYOUT_STATUS.md). Current
production gate is **1.5 of 9**.

**Concretely:**
- ❌ No `.kicad_pcb` exists. No layout → no Gerbers → no order.
- ❌ No BOM CSV / no CPL / no Pick-and-Place file. They're outputs of layout, not inputs to it.
- 🟡 The schematic is done and verified (r18.6) but GUI-ERC hasn't been run yet (Blocker B3).
- 🟡 Several footprints carry `FP_VERIFY` properties — see status file.

## Production gates (9-gate model)

| Gate | What | Status |
|---|---|---|
| 0 | Concept defined | ✅ |
| 1 | Firmware/audio prototype validated | ✅ |
| 2 | Schematic ERC clean | 🟡 partial |
| 3 | BOM/footprints verified | 🟡 partial |
| 4 | PCB layout exists | ❌ |
| 5 | DRC clean | ❌ |
| 6 | Gerbers/BOM/CPL generated | ❌ |
| 7 | Prototype ordered | ❌ |
| 8 | Bring-up test passed | ❌ |
| 9 | Production candidate | ❌ |

## What's blocking gate progression

| Gate | Blocker | Owner |
|---|---|---|
| 2 → ✅ | Run ERC in KiCad 9 GUI, close real errors, accept intentional warnings per [`ERC_DRC_CHECKLIST.md`](../hardware/ERC_DRC_CHECKLIST.md) | hardware engineer |
| 3 → ✅ | Close ~10 `FP_VERIFY` properties (footprint-land-pattern matches manufacturer drawing) | hardware engineer |
| 4 | **Phase-5-Profiling first**: prove the audio engine fits the STM32H743 budget on real hardware before investing in layout | firmware engineer |
| 4 | Resolve ADR-0004 (MIDI) — DNP or fully designed | industrial designer + hardware |
| 5-9 | (Sequential, after layout) | hardware + manufacturing partner |

## The "talk to the fab" file (when we get to gate 6)

JLC PCB capabilities (4-layer): JLC07101H-3313A
- Min trace: 0.127 mm (5 mil)
- Min spacing: 0.127 mm (5 mil)
- Min via: 0.3 mm with 0.15 mm hole
- Outer copper: 1 oz
- Stack-up: Sig / GND / +5V / Sig (per spec §9)

JLC assembly considerations (when we get to gate 7):
- BOM CSV from KiCad (LCSC field is the primary key)
- CPL: position, rotation, side
- Some parts are Basic (no setup fee), most are Extended (setup fee — minimize unique part count)
- Custom-footprint parts (the HX 12×12 button) need pad checks
- Vendored kiswitch hotswap sockets are hand-installed by user (DNP for JLC, deliver loose)

## When you'd come back to this file

- Once `.kicad_pcb` exists
- Once ERC and DRC are both clean
- Once the Phase-5-Profiling gate passes
- Once ADR-0004 is closed

Until then, this file is the answer to "can we order?" — and that answer is no.
