---
name: ambient-smt-selection
description: Select, source, substitute, or verify SMT/SMD ICs and passives for AMBIENT, including display-related chips, LCSC/JLCPCB availability, lifecycle, package variants, datasheet pinout, symbol and footprint matching, derating, assembly constraints, and alternate parts.
---

# AMBIENT SMT/SMD Selection

Select orderable, assemblable parts rather than plausible part families.

## Start from requirements

Read `../ambient-ui-lcd-smt/references/project-baseline.md`.

Write the non-negotiable electrical, mechanical, firmware, supply-chain, and assembly constraints before searching.

For display-related ICs, include:

- supply rails and logic levels;
- interface type and peak clock;
- pixel format and memory bandwidth;
- backlight voltage/current and dimming;
- reset/power sequencing;
- temperature, ESD, EMC, and idle-power requirements;
- package pitch, exposed pad, height, and PCB area.

## Candidate evidence

For every candidate record:

| Field | Required evidence |
|---|---|
| Exact MPN and suffix | Manufacturer datasheet/orderable table |
| Lifecycle | Manufacturer page or authorized distributor |
| Electrical fit | Datasheet limits at worst-case voltage and temperature |
| Pinout | Pin-functions table and package drawing |
| Package | Exact JEDEC/vendor code, body, pitch, pad count, exposed pad |
| Supplier PN | Exact LCSC C-number or authorized-distributor SKU |
| Assembly status | JLCPCB current availability, Basic/Extended, stock, MOQ |
| Footprint | Datasheet land pattern or verified supplier CAD |
| Firmware impact | Driver/API/register compatibility, not family-name similarity |
| Alternate | Pin-compatible or redesign-required, stated explicitly |

Do not use a search snippet, marketplace title, or 3D model as pinout evidence.

## Verification gates

1. Match the exact MPN suffix to the datasheet orderable table.
2. Match symbol pin numbers to the datasheet pin table.
3. Match footprint pad numbers, pin 1, exposed pad, pitch, body, and courtyard to the package drawing.
4. Compare the KiCad footprint with the exact LCSC/EasyEDA land pattern when JLCPCB will assemble it.
5. Verify polarity and orientation in the 3D/assembly view.
6. Check decoupling, programming resistors, thermal pad vias, and forbidden copper from the datasheet.
7. Recheck live stock and assembly availability immediately before ordering.

Use `UNVERIFIED — NEEDS HUMAN CHECK` for any failed gate.

## Passives

- Derate voltage, current, power, temperature, and DC-bias capacitance.
- Use C0G/NP0 where value stability matters.
- Check MLCC effective capacitance at operating bias.
- Check resistor pulse and voltage ratings, not only steady-state watts.
- Do not change package size without checking parasitics, power, tombstoning, and assembly yield.

## Output

Use `references/component-evidence-template.md`. Return a ranked candidate table, evidence links, rejection reasons, footprint verification, BOM impact, alternate strategy, risk list, and exact next verification actions.
