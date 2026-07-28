---
name: ambient-ui-lcd-smt
description: Route AMBIENT work involving visual direction, embedded LCD UX, smooth animation, ST7789/STM32H7 display firmware, LCD electronics, SMT/SMD component selection, or KiCad PCB implementation. Use for cross-domain AMBIENT display and electronics tasks or whenever the correct specialist workflow is unclear.
---

# AMBIENT UI, LCD, Motion, and SMT Router

Treat this as the entry point for AMBIENT display/product-interface work.

## Start

1. Read `PROJECT_STATUS.md` and the root `CLAUDE.md`.
2. Read `references/project-baseline.md`.
3. Inspect the current implementation before proposing architecture. Current files override this skill when they conflict.
4. Select the smallest specialist set that covers the request:

| Need | Skill |
|---|---|
| Visual identity, type, color, composition, icon language | `ambient-ui-art-direction` |
| Pixel-accurate mockups, simulator fixtures, fonts, sprites, asset conversion | `ambient-ui-prototyping` |
| Screen hierarchy, hardware-control mapping, states, feedback | `ambient-lcd-ux` |
| Easing, transitions, frame pacing, dirty regions, perceived smoothness | `ambient-lcd-motion` |
| ST7789, SPI, DMA, cache, framebuffer, DMA2D, partial flush | `ambient-display-pipeline` |
| LCD power, connector, reset, backlight, EMC, mechanics | `ambient-lcd-hardware` |
| IC/passive choice, package, LCSC/JLCPCB, footprint verification | `ambient-smt-selection` |
| Placement, routing, stack-up, ERC/DRC, fabrication outputs | `ambient-kicad-layout` |
| Tests, measurement, regression, evidence, bring-up | `ambient-embedded-verification` |

Load multiple specialists only when the change crosses their boundary. For example, a new animated world selector needs UI direction, LCD UX, motion, display pipeline, and verification; a replacement LCD module needs LCD hardware, SMT selection, display pipeline, KiCad, and verification.

## Binding priorities

Apply these priorities in order:

1. Preserve realtime audio. Never trade an unmeasured animation improvement for audio underruns or ISR latency.
2. Preserve buildability. Verify exact parts, pins, packages, footprints, connector orientation, and mechanical dimensions.
3. Preserve interaction clarity. AMBIENT is a tactile instrument, not a small website.
4. Preserve the established visual language while improving it deliberately.
5. Measure performance and memory instead of inferring them from MCU headline specifications.
6. Mark unknown physical facts exactly as `UNVERIFIED — NEEDS HUMAN CHECK`.

## Cross-domain completion contract

Do not call a display or PCB task complete until the answer identifies:

- the affected screen, state, hardware control, code path, schematic block, or PCB region;
- exact pixel dimensions and timing budgets for visual work;
- transfer bytes, SPI time, RAM placement, CPU work, and DMA/cache ownership for animation work;
- exact MPN, package variant, source, symbol, footprint, and verification evidence for component work;
- the tests or measurements run and the hardware-only checks still open.

For source provenance and locked upstream revisions, read `references/github-sources.md` and `references/sources.lock.json`.
