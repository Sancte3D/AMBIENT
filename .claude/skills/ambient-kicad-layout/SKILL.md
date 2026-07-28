---
name: ambient-kicad-layout
description: Create, modify, review, or verify AMBIENT's KiCad schematic-to-PCB implementation, including generated schematics, footprints, placement, four-layer stack-up, LCD and STM32 routing, audio and power separation, ERC/DRC, 3D checks, and JLCPCB fabrication outputs.
---

# AMBIENT KiCad Layout

Use evidence-first KiCad workflows and preserve the generated-schematic architecture.

## Orient

1. Read `../ambient-ui-lcd-smt/references/project-baseline.md`.
2. Read `PROJECT_STATUS.md`, `PCB_LAYOUT_STATUS.md`, the current PCB spec, mechanical coordinates, footprint risk audit, and root `CLAUDE.md`.
3. Discover current `.kicad_pro`, `.kicad_sch`, `.kicad_pcb`, library tables, generator, and KiCad version.
4. Determine whether the task needs:
   - generator/source changes;
   - live KiCad IPC;
   - `kicad-cli` render/export/ERC/DRC;
   - structured offline file editing;
   - human GUI work.
5. Never hand-edit generated schematic files.

Use the locked Ki-Stack, Seeed schematic-analyzer, and kicad-happy sources in the router's source catalog for tool patterns.

## Placement order

1. Freeze board outline, holes, enclosure-fixed controls, speakers, ports, LCD header, display active area, and keepouts.
2. Place power input, charger/power-path, boost, LDO/load switch, and their tight current loops.
3. Place STM32H743, crystal, VCAP and per-pin decoupling.
4. Place LCD connector near the mechanical target with a short SPI path and clean backlight return.
5. Place audio DAC, headphone amp, class-D amp, line/headphone jack, and speakers as an isolated signal-flow cluster.
6. Place I/O expanders, LED driver, encoders, buttons, and remaining passives.

## Routing priorities

- Preserve a continuous reference plane under SPI, clocks, I2S/SAI, USB, and other fast edges.
- Keep boost switch nodes and class-D outputs compact and away from LCD/audio/control signals.
- Route high-current and power loops before ordinary GPIO.
- Keep crystal traces short, symmetric, and isolated.
- Place each decoupler at its owning power pin with the smallest loop.
- Keep LCD SCK/MOSI together over solid ground; avoid stubs and plane splits.
- Keep backlight current and PWM return away from codec/DAC reference paths.
- Use stitching vias around board edges, noisy zones, and connector transitions where justified.
- Do not route by visual neatness at the expense of current-loop or return-path physics.

Verify the documented signal / GND / +5 V / signal stack against the latest design before using it.

## Footprints and assembly

- Re-run symbol-to-footprint verification for every changed part.
- Check pin 1, exposed pads, paste apertures, courtyard, body height, and mechanical orientation.
- Check JLCPCB assembly-side limitations, rotations, fiducials, tooling, panel constraints, and Basic/Extended status.
- Inspect every nonstandard footprint in 3D and at 1:1 scale.

## Gates

- Generated schematic structural checks pass.
- KiCad ERC has zero unexplained errors/warnings.
- PCB DRC has zero unexplained violations and no unconnected nets.
- Board render and 3D view match mechanical coordinates.
- Gerber, drill, BOM, and CPL are regenerated from the same revision.
- JLCPCB assembly preview is walked part by part.
- The report distinguishes tool-verified facts from physical checks still required.
