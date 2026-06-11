# Project Status

**Updated: 2026-06-11 (r18.6)**

## Where we are right now

**Hardware:** Schematic verified (r18.6 engineering pass), no PCB layout yet.
**Firmware:** Active development in `firmware-c-next` (Pico 2 target); STM32H743 migration on the schematic is done, on the firmware side it's planned for Phase 2-5 of the native-port plan.
**Audio:** Engine runs end-to-end. 8-minute generative performance render available in `demos/audio/`.
**Industrial design:** Pitch + interaction model locked. No enclosure CAD yet.
**Manufacturing:** Production gate **1.5 of 9**. Not orderable.

## Health by area

| Area | State | Open issues |
|---|---|---|
| **Sound engine** | 🟢 Working, 12+ host test suites green | Phase 5 profiling on STM32H743 hardware (not started) |
| **Web simulator** | 🟢 Live on GitHub Pages | — |
| **Bench bring-up** (Pico 2 + ST7789 + encoder + button) | 🟢 Works on real hardware | — |
| **Schematic** | 🟢 Verified r18.6 | Run GUI-ERC (B3), close ~10 FP_VERIFY properties |
| **MIDI hardware** | 🟡 Open design decision | ADR-0004 — 5 axes to decide before parts get drawn |
| **PCB layout** | 🔴 Not started | Waiting on Phase-5-Profiling gate |
| **BOM + sourcing** | 🟡 Main parts verified, sourcing-pass needed | VERIFY-STOCK properties on ~5 parts |
| **Mechanical / enclosure** | 🟡 Coordinates on paper, no CAD | Industrial designer pickup |
| **Documentation** | 🟢 Solid baseline (this commit) | Onboarding docs done, refactor of repo structure phased |

## What the last 5 commits did

(See `field-ambience-current/CHANGELOG.md` for full history.)

- `r18.6` (this revision): AP7361A→AP7361C, correct SOT-89-5 pinout, TPS61089 verified, encoder = C209762 NRND, custom HX-12×12 footprint, ADR-0004 for MIDI, ERC/DRC checklist
- `r18.5`: schematic migration STM32H743 + LCD, AP7361A in power tree, repo-audit fixes
- bench: SHIFT toggle mode + transient backlight readout (sim parity)
- bench: review fixes — detent loss + event ordering + gamma floor + permanent CI test
- bench: display_hw_test sim-on-real-hardware

## Next 5 concrete actions

1. **Industrial designer** picks up enclosure / panel CAD from `mechanical_coordinates.md`
2. **Hardware engineer** runs ERC in KiCad 9 GUI on r18.6 schematic, logs results, closes real errors → unblocks gate 2
3. **Firmware engineer** sets up STM32H743 dev hardware (Nucleo-H743 + PCM5102A breakout) for Phase-5 profiling → unblocks gate 4 prerequisite
4. **Anyone** resolves ADR-0004 (MIDI design decision) — 5 axes need a meeting, not more code
5. **Anyone** updates `mechanical_coordinates.md` to reflect STM32 chip footprint (it's still on Pico-era positions)

## What this file is not

This file is **not** a roadmap (that's `field-ambience-current/ROADMAP.md`)
and **not** a changelog (that's `field-ambience-current/CHANGELOG.md`).
It's the answer to "what's the project's state today?" — a snapshot, kept
short and updated by hand when the answer changes.
