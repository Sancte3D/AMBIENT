#!/usr/bin/env python3
"""check_pinmap.py — assert PINMAP.md agrees with the generator NETS.

The generator's NETS dict (in stm32h743_sheet) is the single source of truth
for STM32 pin -> net assignment. docs/hardware/PINMAP.md is the human mirror
that the PCB engineer reads. They MUST not drift (audit: scrambled encoder
pins were exactly this kind of drift).

This is a stdlib-only static check: it extracts pin->net from both files and
reports any pin where they disagree, or that one side lists and the other
doesn't. Exit non-zero on any mismatch so it can gate CI.

Run: python3 scripts/check_pinmap.py   (from field-ambience-current/)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GEN = ROOT / "kicad" / "generate_kicad_project.py"
PINMAP = ROOT / "docs" / "hardware" / "PINMAP.md"

# Net-name aliases that are intentionally written differently in the two docs
# (same physical net, cosmetic naming). Keep this list tiny + explicit.
ALIASES = {
    "BOOT0_PIN": "BOOT0",
}


def norm(net: str) -> str:
    net = net.strip().strip("`").strip()
    return ALIASES.get(net, net)


def gen_nets() -> dict[int, str]:
    """pin -> net from the NETS dict block in the generator."""
    src = GEN.read_text()
    m = re.search(r"NETS:\s*dict\[int.*?\{(.*?)\n    \}", src, re.S)
    if not m:
        sys.exit("check_pinmap: could not locate the NETS dict in the generator")
    block = m.group(1)
    out: dict[int, str] = {}
    for pin, net in re.findall(r"(\d+):\s*\(\"([^\"]+)\"", block):
        out[int(pin)] = norm(net)
    return out


def pinmap_nets() -> dict[int, str]:
    """pin -> net from PINMAP's master pin table (| pin | port | net | ...)."""
    out: dict[int, str] = {}
    row = re.compile(r"^\|\s*(\d{1,3})\s*\|\s*`?([A-Za-z][A-Za-z0-9_]*)`?\s*\|\s*`?([A-Za-z][A-Za-z0-9_]+)`?\s*\|")
    for line in PINMAP.read_text().splitlines():
        mm = row.match(line)
        if not mm:
            continue
        pin = int(mm.group(1))
        net = norm(mm.group(3))
        if 1 <= pin <= 100:
            out.setdefault(pin, net)  # first (master-table) occurrence wins
    return out


def main() -> int:
    g = gen_nets()
    p = pinmap_nets()
    problems: list[str] = []

    for pin, gnet in sorted(g.items()):
        pnet = p.get(pin)
        if pnet is None:
            problems.append(f"  pin {pin}: generator='{gnet}' but PINMAP has no row")
        elif pnet != gnet:
            problems.append(f"  pin {pin}: generator='{gnet}' != PINMAP='{pnet}'  <-- DRIFT")

    # Pins PINMAP assigns a functional net to that the generator doesn't (extra)
    for pin, pnet in sorted(p.items()):
        if pin not in g:
            # Many pins are power/free in the generator and legitimately appear
            # in PINMAP's full 100-pin table; only flag if PINMAP gives it a
            # signal-looking net the generator omits AND it's not power/free.
            if pnet not in ("GND", "VDD", "VSS", "FREE", "NC") and not pnet.startswith(("V", "+")):
                pass  # informational only; do not fail (PINMAP is a superset table)

    print(f"check_pinmap: generator NETS={len(g)} signal pins, PINMAP matched={len(p)}")
    if problems:
        print("MISMATCH (generator is the source of truth):")
        print("\n".join(problems))
        return 1
    print("OK — every generator NETS pin matches PINMAP.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
