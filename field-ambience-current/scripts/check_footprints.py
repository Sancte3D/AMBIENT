#!/usr/bin/env python3
"""Field Ambience — Symbol-Pin vs. Footprint-Pad checker.

Reads the GENERATED schematics, extracts every placed part's symbol pin
numbers and its footprint's pad numbers, and reports any pin that would land
on no pad. That is the failure mode that silently produces an unbuildable
board: the netlist looks fine, ERC is happy, and the connection simply does
not exist in copper.

Real finds:
  r18.82  all 4 encoders — symbol pins 1-5 vs. footprint pads A/B/C/S1/S2:
          NOT ONE encoder pad was connected.
  r19.65  J1 USB-C shield — symbol pin "S1" vs. footprint pad "SH": the
          connector shell would have been off-net (schematic ties it to GND).
  r19.65  C_BULK — footprint "Capacitor_SMD:CP_Tantalum_Case-E_EIA-7343-43_
          Reflow" exists in no KiCad library; the name was invented.

Unlike the r18.82 version, which compared a hand-maintained list of four
parts, this checks EVERY placed part automatically — a newly added part is
covered the day it appears.

Usage:
    python3 scripts/check_footprints.py                # auto-locate libraries
    python3 scripts/check_footprints.py --fetch        # download what's missing
    python3 scripts/check_footprints.py /path/to/kicad/footprints

Exit code 0 = clean, 1 = at least one pin without a pad.
"""
from __future__ import annotations

import os
import re
import sys
import urllib.request
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
KICAD = REPO / "kicad"
CACHE = KICAD / ".footprint_cache"
GITLAB = ("https://gitlab.com/kicad/libraries/kicad-footprints/-/raw/master"
          "/{lib}.pretty/{name}.kicad_mod")

SYSTEM_DIRS = [
    os.environ.get("KICAD9_FOOTPRINT_DIR"),
    os.environ.get("KICAD8_FOOTPRINT_DIR"),
    os.environ.get("KICAD_FOOTPRINT_DIR"),
    "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints",
    "/usr/share/kicad/footprints",
    "/usr/local/share/kicad/footprints",
    "C:/Program Files/KiCad/9.0/share/kicad/footprints",
    "C:/Program Files/KiCad/8.0/share/kicad/footprints",
]


# ---------------------------------------------------------------------------
# S-expression helpers — plain regex mis-attributes pins across symbols.
# ---------------------------------------------------------------------------

def sexpr_blocks(text: str, tag: str):
    """Return [(name, body)] for every ``(tag "name" ...)``, paren- and
    quote-aware so nested sub-symbols stay with their parent."""
    out, i, n, needle = [], 0, len(text), "(" + tag + " "
    while True:
        j = text.find(needle, i)
        if j < 0:
            return out
        k, depth, inq, esc = j, 0, False, False
        while k < n:
            c = text[k]
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                inq = not inq
            elif not inq:
                if c == "(":
                    depth += 1
                elif c == ")":
                    depth -= 1
                    if depth == 0:
                        break
            k += 1
        body = text[j:k + 1]
        m = re.match(r"\(" + tag + r'\s+"([^"]*)"', body)
        out.append((m.group(1) if m else None, body))
        i = j + 1


def footprint_pads(text: str) -> set:
    """Pad names. KiCad 6+ quotes them; the legacy KiCad 5 ``(module ...)``
    format does not — several project-local footprints are still KiCad 5."""
    pads = set()
    for m in re.finditer(r'\(pad\s+(?:"([^"]*)"|([^\s"()]+))\s', text):
        pads.add(m.group(1) if m.group(1) is not None else m.group(2))
    pads.discard("")           # unnumbered mechanical / paste-only pads
    return pads


# ---------------------------------------------------------------------------
# Footprint sources: system KiCad install, project-local, cache, GitLab.
# ---------------------------------------------------------------------------

class Footprints:
    def __init__(self, extra_dir, allow_fetch: bool):
        self.allow_fetch = allow_fetch
        self.roots = [Path(extra_dir)] if extra_dir else []
        self.roots += [Path(d) for d in SYSTEM_DIRS if d and Path(d).exists()]
        self.local = KICAD / "libraries"
        self._memo = {}

    def _candidates(self, lib: str, name: str):
        for r in self.roots:
            yield r / f"{lib}.pretty" / f"{name}.kicad_mod"
        yield self.local / f"{lib}.pretty" / f"{name}.kicad_mod"
        yield CACHE / f"{lib}__{name}.kicad_mod"

    def pads(self, fpid: str):
        if fpid in self._memo:
            return self._memo[fpid]
        lib, _, name = fpid.partition(":")
        result = None
        for p in self._candidates(lib, name):
            if p.exists():
                result = footprint_pads(p.read_text())
                break
        else:
            if self.allow_fetch:
                result = self._fetch(lib, name)
        self._memo[fpid] = result
        return result

    def _fetch(self, lib: str, name: str):
        try:
            data = urllib.request.urlopen(
                GITLAB.format(lib=lib, name=name), timeout=40).read().decode()
        except Exception:
            return None
        CACHE.mkdir(parents=True, exist_ok=True)
        (CACHE / f"{lib}__{name}.kicad_mod").write_text(data)
        return footprint_pads(data)


# ---------------------------------------------------------------------------

def collect():
    """(symbol pin sets, {(lib_id, footprint): {refs}}) from the schematics."""
    sheets = sorted(KICAD.glob("*.kicad_sch"))
    if not sheets:
        sys.exit("no .kicad_sch found — run kicad/generate_kicad_project.py first")

    # Every sheet embeds its own copy of the library. Merge across all of them
    # and take the UNION of pin numbers per symbol: reading only one sheet would
    # miss a divergence, and a pin that exists in any copy must have a pad.
    pins = {}
    placed = {}
    for sheet in sheets:
        text = sheet.read_text()
        lib_section = text[text.index("(lib_symbols"):]
        for name, body in sexpr_blocks(lib_section, "symbol"):
            if not name or re.search(r"_\d+_\d+$", name):
                continue
            found = {m.group(1) for m in re.finditer(r'\(number "([^"]+)"', body)}
            pins[name] = pins.get(name, set()) | found

        for _, body in sexpr_blocks(text, "symbol"):
            lib = re.search(r'\(lib_id "([^"]+)"', body)
            ref = re.search(r'\(property "Reference" "([^"]+)"', body)
            fp = re.search(r'\(property "Footprint" "([^"]*)"', body)
            if lib and ref and fp and fp.group(1):
                placed.setdefault((lib.group(1), fp.group(1)), set()).add(ref.group(1))
    return pins, placed


def main() -> int:
    args = sys.argv[1:]
    allow_fetch = "--fetch" in args
    args = [a for a in args if a != "--fetch"]
    fps = Footprints(args[0] if args else None, allow_fetch)

    pins, placed = collect()
    errors, notes, clean = [], [], 0

    for (lib_id, fpid), refs in sorted(placed.items()):
        who = ", ".join(sorted(refs)[:4]) + ("…" if len(refs) > 4 else "")
        sym, pads = pins.get(lib_id), fps.pads(fpid)
        if sym is None:
            errors.append((who, fpid, "symbol not in lib_symbols"))
        elif pads is None:
            errors.append((who, fpid, "FOOTPRINT DOES NOT EXIST in any library"))
        elif sym - pads:
            errors.append((who, fpid, f"symbol pin with no pad: {sorted(sym - pads)}"))
        else:
            clean += 1
            if pads - sym:
                notes.append((who, fpid, f"pad with no symbol pin: {sorted(pads - sym)}"))

    print(f"{len(placed)} symbol/footprint pairs · {clean} ok · {len(errors)} ERROR")
    if not fps.roots and not allow_fetch:
        print("  (no local KiCad library found — re-run with --fetch to download)")
    if notes:
        print("\nPads without a symbol pin — thermal/shield/mounting, review once:")
        for who, fpid, why in notes:
            print(f"  {who:<24} {fpid:<50} {why}")
    if errors:
        print("\nERRORS:")
        for who, fpid, why in errors:
            print(f"  {who:<24} {fpid:<50} {why}")
        return 1
    print("\nno symbol pin is left without a pad.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
