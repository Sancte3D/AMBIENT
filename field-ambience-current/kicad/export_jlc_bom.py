#!/usr/bin/env python3
"""
export_jlc_bom.py — JLC production BOM exporter for the Field Ambience PCB.

Parses the generated KiCad schematics (kicad/*.kicad_sch) and emits a
JLCPCB-format BOM CSV. The generator (generate_kicad_project.py) embeds
MPN/LCSC/Footprint as hidden schematic properties — this tool extracts them,
so the BOM comes FROM the source of truth, not a hand-maintained markdown.

What it does NOT do (cannot, by design):
  - It does NOT emit a CPL / pick-and-place file. That needs per-part
    X/Y/rotation/layer PLACEMENT coordinates, which only exist in a
    .kicad_pcb board layout. The generator produces schematic only — there is
    no board layout yet, so there are no placement coordinates to export. The
    (at X Y ROT) in the schematic is a *sheet* coordinate, not a board
    placement, and must never be used as CPL data.

Usage:
    cd field-ambience-current
    python3 kicad/export_jlc_bom.py [output.csv]
        default output: kicad/jlc_bom.csv
"""

import csv
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# The 7 leaf sheets carry every placed component. The root field_ambience.sch
# is hierarchical-only (0 placed instances), so it's skipped.
LEAF_SHEETS = [
    "power_tree.kicad_sch",
    "stm32h743.kicad_sch",
    "lcd.kicad_sch",
    "mcp.kicad_sch",
    "encoder.kicad_sch",
    "audio.kicad_sch",
    "battery.kicad_sch",
]

# A placed component instance is anchored by this exact 2-space-indented token.
# lib_symbols *definitions* use `(symbol "Name"` (no `(lib_id`) and are indented
# differently, so this anchor uniquely matches placed instances.
INSTANCE_ANCHOR = '\n  (symbol (lib_id "'

PROP_RE = re.compile(r'\(property "([^"]+)" "((?:[^"\\]|\\.)*)"')


def parse_instances(text):
    """Yield a dict of properties for every placed component in one sheet."""
    chunks = text.split(INSTANCE_ANCHOR)
    for chunk in chunks[1:]:           # chunks[0] is the file header + lib_symbols
        props = {k: v for k, v in PROP_RE.findall(chunk)}
        if not props:
            continue
        # the dnp flag sits on the symbol's second line, inside this chunk
        props["_dnp"] = "(dnp yes)" in chunk
        yield props


def natural_key(designator):
    """Sort 'C2, C10' as 2 < 10, keeping the alpha prefix grouped."""
    m = re.match(r"^([^\d]*)(\d*)(.*)$", designator)
    prefix, num, rest = m.group(1), m.group(2), m.group(3)
    return (prefix, int(num) if num else -1, rest)


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "jlc_bom.csv")

    # group key = LCSC; value = {"value", "footprint", "mpn", "refs": [...]}
    groups = {}
    no_lcsc = []          # real parts JLC cannot auto-source (hand place)
    n_instances = 0
    n_skipped_pwr = 0
    n_skipped_dnp = 0

    for sheet in LEAF_SHEETS:
        path = os.path.join(HERE, sheet)
        if not os.path.exists(path):
            print(f"WARNING: missing sheet {sheet}", file=sys.stderr)
            continue
        with open(path, encoding="utf-8") as f:
            text = f.read()
        for p in parse_instances(text):
            ref = p.get("Reference", "")
            if not ref or ref.startswith("#PWR"):
                n_skipped_pwr += 1
                continue
            n_instances += 1
            dnp_prop = p.get("DNP", "").strip().lower().startswith("true")
            if p["_dnp"] or dnp_prop:
                n_skipped_dnp += 1
                continue
            lcsc = p.get("LCSC", "").strip()
            value = p.get("Value", "").strip()
            footprint = p.get("Footprint", "").strip()
            mpn = p.get("MPN", "").strip()
            if not lcsc:
                no_lcsc.append((ref, value, footprint, mpn))
                continue
            g = groups.setdefault(lcsc, {"value": value, "footprint": footprint,
                                         "mpn": mpn, "refs": []})
            g["refs"].append(ref)

    # write JLC BOM CSV (Comment, Designator, Footprint, LCSC Part #)
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Comment", "Designator", "Footprint", "LCSC Part #"])
        for lcsc in sorted(groups):
            g = groups[lcsc]
            refs = sorted(set(g["refs"]), key=natural_key)
            comment = g["mpn"] or g["value"]
            w.writerow([comment, ",".join(refs), g["footprint"], lcsc])

    # summary
    n_parts = len(groups)
    n_placements = sum(len(g["refs"]) for g in groups.values())
    print(f"JLC BOM written → {out_path}")
    print(f"  {n_parts} unique LCSC parts, {n_placements} placements")
    print(f"  skipped: {n_skipped_pwr} power-flags, {n_skipped_dnp} DNP")
    if no_lcsc:
        print(f"\n  {len(no_lcsc)} part(s) WITHOUT an LCSC number — JLC cannot "
              f"auto-source these; hand-place or add a verified LCSC PN "
              f"(anti-guess: not invented here):", file=sys.stderr)
        for ref, value, fp, mpn in sorted(no_lcsc, key=lambda t: natural_key(t[0])):
            print(f"    {ref:12s} {mpn or value}", file=sys.stderr)
    print("\nNOTE: CPL / pick-and-place is NOT produced — it requires a "
          ".kicad_pcb board layout (placement X/Y/rotation), which does not "
          "exist yet. Generator is schematic-only.")


if __name__ == "__main__":
    main()
