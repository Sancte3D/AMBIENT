# Project Map

The honest answer to "where is the X?" — by topic and by role.

## By question

| Question | Answer |
|---|---|
| Where is the **active firmware**? | [`field-ambience-current/firmware-c-next/`](field-ambience-current/firmware-c-next/) — host-portable C + Pico-2 bench + display sim |
| Where is the **frozen firmware**? | [`field-ambience-current/firmware-c/`](field-ambience-current/firmware-c/) — listening-test snapshot from Step 11/12a |
| Where is the **KiCad project**? | [`field-ambience-current/kicad/`](field-ambience-current/kicad/) |
| Where is the **schematic**? | [`field-ambience-current/kicad/field_ambience.kicad_sch`](field-ambience-current/kicad/) (+ 7 child sheets) |
| Where will the **PCB layout** live? | Same folder, as `field_ambience.kicad_pcb` — doesn't exist yet, see [`PCB_LAYOUT_STATUS.md`](field-ambience-current/PCB_LAYOUT_STATUS.md) |
| Where is the **schematic generator**? | [`field-ambience-current/kicad/generate_kicad_project.py`](field-ambience-current/kicad/generate_kicad_project.py) — the source of truth, all `.kicad_sch` files are output |
| Where is the **BOM**? | Inline in the schematic (every symbol has MPN/LCSC/Manufacturer properties). No flat BOM CSV — that gets generated from KiCad once layout exists |
| Where are **datasheets**? | [`field-ambience-current/kicad/datasheets/`](field-ambience-current/kicad/datasheets/) — current parts. `datasheets/legacy/` for replaced/wrong-variant PDFs |
| Where are **mechanical coordinates**? | [`field-ambience-current/mechanical_coordinates.md`](field-ambience-current/mechanical_coordinates.md) — needs update for STM32 layout |
| Where are **manufacturing outputs** (Gerbers, CPL)? | Not generated yet. They will live in `field-ambience-current/kicad/manufacturing/` once layout exists |
| Where are **old/legacy files**? | `field-ambience-current/legacy/` (bridge.py, MicroPython firmware), `field-ambience-current/kicad/legacy_pico2/` (old sheets), `field-ambience-current/docs/archive/` (pre-Step-6 specs), `demos/old/` (legacy demos) |
| What should a **new person open first**? | [`README.md`](README.md) → [`field-ambience-current/START_HERE.md`](field-ambience-current/START_HERE.md) → role-based onboarding file |
| What is the **current production gate**? | **Gate 1 + partial Gate 2**: firmware/audio prototype validated on Pico 2 bench, schematic done but ERC not yet run in GUI. See "Production Gate" below |

## By role

### Industrial designer
1. [`field-ambience-current/PITCH.md`](field-ambience-current/PITCH.md) — what this is, who it's for
2. [`field-ambience-current/mechanical_coordinates.md`](field-ambience-current/mechanical_coordinates.md) — frontplate / encoder positions / button layout
3. [`field-ambience-current/firmware-c-next/tools/display_sim.html`](field-ambience-current/firmware-c-next/tools/display_sim.html) — interact with the menu in a browser
4. [`field-ambience-current/docs/decisions/`](field-ambience-current/docs/decisions/) — read ADRs to understand why things are as they are
5. **Don't touch:** schematic generator, firmware, KiCad files

### Hardware engineer
1. [`field-ambience-current/PCB_LAYOUT_STATUS.md`](field-ambience-current/PCB_LAYOUT_STATUS.md) — current state, current blockers
2. [`field-ambience-current/field_ambience_pcb_SPEC_v0.7.md`](field-ambience-current/field_ambience_pcb_SPEC_v0.7.md) — full hardware spec, especially §5 pin allocation
3. [`field-ambience-current/kicad/`](field-ambience-current/kicad/) — open `field_ambience.kicad_pro` in KiCad 9
4. [`field-ambience-current/docs/hardware/ERC_DRC_CHECKLIST.md`](field-ambience-current/docs/hardware/ERC_DRC_CHECKLIST.md) — what ERC/DRC must pass before layout/order
5. [`field-ambience-current/docs/component_reviews/`](field-ambience-current/docs/component_reviews/) — verified parts and open verifications

### Firmware engineer
1. [`field-ambience-current/firmware-c-next/README.md`](field-ambience-current/firmware-c-next/README.md) — current step
2. [`field-ambience-current/firmware-c-next/test/run_tests.sh`](field-ambience-current/firmware-c-next/test/run_tests.sh) — runs the 12 host suites
3. [`field-ambience-current/NATIVE_PORT_PLAN.md`](field-ambience-current/NATIVE_PORT_PLAN.md) — migration plan
4. [`field-ambience-current/firmware-c/`](field-ambience-current/firmware-c/) — *only* as the frozen reference when the active branch breaks
5. Bench: [`firmware-c-next/tools/display_hw_test.c`](field-ambience-current/firmware-c-next/tools/display_hw_test.c) — flashable on Pico 2 + ST7789

### Anyone trying to order PCBs
**You can't yet.** Read [`field-ambience-current/PCB_LAYOUT_STATUS.md`](field-ambience-current/PCB_LAYOUT_STATUS.md). Production gate is currently 1 + partial 2 (of 9).

## Production gate (current)

| Gate | Description | Status |
|---|---|---|
| 0 | Concept defined | ✅ |
| 1 | Firmware/audio prototype validated | ✅ (host tests + bench bring-up + 8-min performance render) |
| 2 | Schematic ERC clean | 🟡 partial — schematic exists and is structurally validated, GUI-ERC not yet run |
| 3 | BOM/footprints verified | 🟡 partial — main parts verified, ~10 FP_VERIFY properties still open |
| 4 | PCB layout exists | ❌ — Phase-5-Profiling-Gate übersprungen (ADR-0005); jetzt blockiert nur noch Gate 2 (ERC) + Gate 3 (FP_VERIFY) |
| 5 | DRC clean | ❌ — needs layout first |
| 6 | Gerbers/BOM/CPL generated | ❌ |
| 7 | Prototype ordered | ❌ |
| 8 | Bring-up test passed | ❌ |
| 9 | Production candidate | ❌ |

**Honest current state: Gate 1.5.** Pfad nach vorn: ERC (Gate 2) → FP_VERIFY (Gate 3) → Layout (Gate 4). Phase-5-Profiling-Gate ist mit ADR-0005 entschärft — Layout darf vor Firmware-Migration starten.
