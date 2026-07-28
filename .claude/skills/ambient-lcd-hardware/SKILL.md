---
name: ambient-lcd-hardware
description: Design or review the electrical and mechanical integration of AMBIENT's LCD, including exact module identification, connector and pin order, power, logic levels, reset, SPI integrity, backlight drive, EMC/ESD, active area, bezel, cable clearance, and test points.
---

# AMBIENT LCD Hardware

Treat the display module, controller IC, connector, backlight, and mechanical stack as separate verified objects.

## Evidence packet

Read `../ambient-ui-lcd-smt/references/project-baseline.md`. Before changing the schematic or selecting a replacement, collect:

- exact vendor and orderable module number;
- module revision or photographs of both sides;
- controller and panel resolution;
- supply and logic-level requirements;
- exact connector pin order and connector orientation;
- module PCB outline, mounting holes, active-area origin, viewing area, thickness, component keepouts, and cable exit;
- backlight topology, current, forward voltage, polarity, and whether the module includes a resistor or driver;
- initialization, reset, sleep, and power-sequence requirements.

If any physical fact is missing, mark it `UNVERIFIED — NEEDS HUMAN CHECK`.

## Electrical review

- Verify every module pin against the actual purchased revision.
- Confirm MCU and module logic levels; do not add a level shifter solely because another module revision has one.
- Provide local decoupling at the connector/module supply.
- Define reset state during MCU boot and power-off.
- Keep CS, D/C, and reset from floating.
- Keep SPI traces short, referenced to solid ground, and away from high-current boost, speaker, and class-D switching loops.
- Add source series damping only from calculation or measured ringing; place it at the MCU.
- Keep backlight current out of the digital ground return path.
- Check PWM frequency, beat products, startup flash, dimming range, and audible/power-rail coupling on hardware.
- Add test access for supply, reset, SCK, MOSI, CS, D/C, and backlight control where layout permits.
- Define whether the internal connector is safe to connect while powered. Default to no hot-plug.

## Mechanical review

- Align the active area, not merely the module PCB, to the bezel opening.
- Include adhesive, glass/lens, tolerance stack, display viewing cone, and parallax.
- Keep the flex/header, tallest rear components, solder fillets, and cable bend volume clear.
- Verify serviceability and assembly order.
- Export a 1:1 drawing and compare it with the real module before freezing the PCB.

## Schematic rule

Edit `field-ambience-current/kicad/generate_kicad_project.py`, never the generated `.kicad_sch`.

## Deliver

Use `references/lcd-verification-template.md`. Provide a verification matrix containing each claim, value, source, exact module revision, schematic consequence, mechanical consequence, status, and human check. Route part sourcing to `ambient-smt-selection`, PCB implementation to `ambient-kicad-layout`, and firmware impact to `ambient-display-pipeline`.
