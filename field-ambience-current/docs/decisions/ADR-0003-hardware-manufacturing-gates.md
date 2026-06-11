# ADR-0003: Hardware Manufacturing Gates (9-Gate Model)

**Status:** ACCEPTED
**Date:** 2026-06-11

## Context

"Are we ready to order?" is the question with the most lying-to-yourself
potential in hardware development. A schematic that compiles, a BOM with
LCSC numbers, and a few verified parts can look like progress — but if
there's no PCB layout, no DRC pass, no bring-up test, you're not
manufacturing-ready. You're at gate 2 of 9.

We need a shared 9-gate vocabulary so anyone asking can be answered in
one number, and so anyone documenting work knows which gate they're
unblocking.

## Decision

Adopt a 9-gate model:

| Gate | Name | Definition |
|---|---|---|
| 0 | Concept defined | Pitch + interaction model + sound-design intent locked |
| 1 | Firmware/audio prototype validated | Engine runs end-to-end on bench; host tests green |
| 2 | Schematic ERC clean | KiCad-GUI ERC: 0 errors, only intentional warnings |
| 3 | BOM/footprints verified | Every part has MPN+LCSC+footprint verified against manufacturer drawing |
| 4 | PCB layout exists | `.kicad_pcb` with routing, mounting holes, board outline |
| 5 | DRC clean | KiCad-GUI DRC with JLC-4-layer profile: 0 errors |
| 6 | Gerbers/BOM/CPL generated | All fab outputs created from layout, stored in `manufacturing/` |
| 7 | Prototype ordered | At least one batch of boards on the way from the fab |
| 8 | Bring-up test passed | First board powers up, runs firmware, audio out works |
| 9 | Production candidate | Multiple boards built; field-test passed; ready for batch production |

## Rules

- **You report the lowest non-✅ gate** — not the highest gate you've started work on.
- **Anyone can read the gate number** from `PROJECT_MAP.md` / `PROJECT_STATUS.md`.
- **No skipping gates.** "Gate 4 done, gate 3 partial" means current state is Gate 3, not Gate 4.
- **Half-credit is "X.5"** — e.g. "1.5 of 9" means gate 1 complete + meaningful progress on gate 2.
- **Phase-5 profiling acceptance is a precondition for gate 4** — see ADR-0002. We don't lay out a board if we don't know the chip can run the engine.

## Consequences

**Positive:**
- One number that everyone interprets the same way. No more "we're almost ready" without a definition of "ready."
- Honest status in README/PROJECT_MAP. No one orders prematurely.
- The gates correspond roughly to natural review checkpoints — easy to map to PRs and milestones.

**Negative:**
- Some people will want sub-states ("gate 3a, 3b, 3c"). Keep it flat — sub-states make the model performative again.
- The 9-gate scheme is project-specific; team members from other projects may bring different mental models. Doc up front.

## Current state at adoption

**Gate 1.5 of 9.**

- ✅ Gate 0: concept locked (PITCH + ADR-0002 + ADR-0004 in progress)
- ✅ Gate 1: bench bring-up works, 8-minute performance render
- 🟡 Gate 2: schematic verified r18.6, GUI-ERC pending (B3)
- 🟡 Gate 3: main parts verified, ~10 FP_VERIFY properties open
- ❌ Gate 4+: no layout
