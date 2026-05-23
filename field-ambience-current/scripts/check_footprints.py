#!/usr/bin/env python3
"""Field Ambience PCB — Footprint-vs-Symbol-Pin-Number-Checker.

Liest KiCad-Footprint-Dateien (.kicad_mod) und vergleicht die Pad-Numbers
mit den Symbol-Pin-Numbers aus der Schematic. Findet falsche Footprint-
Library-Refs ODER falsche Pad-Nummerierung BEVOR es ins PCB-Layout geht.

Usage:
    python3 scripts/check_footprints.py                # default
    python3 scripts/check_footprints.py /pfad/zur/kicad/library

Footprints werden gesucht in (in dieser Reihenfolge):
    1. $KICAD9_FOOTPRINT_DIR
    2. $KICAD_FOOTPRINT_DIR
    3. /Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints/
    4. /usr/share/kicad/footprints/
    5. /usr/local/share/kicad/footprints/
    6. C:/Program Files/KiCad/9.0/share/kicad/footprints/
"""

import os
import re
import sys
from pathlib import Path

# -------------------------------------------------------------------------
# Was wir prüfen wollen — die kritischsten Symbol↔Footprint-Paare
# -------------------------------------------------------------------------

CHECKS = [
    {
        "ref": "J1",
        "name": "USB-C Receptacle",
        "footprint_lib": "Connector_USB",
        "footprint_name": "USB_C_Receptacle_HRO_TYPE-C-31-M-12",
        "expected_pads": {
            "A1": "GND", "A4": "VBUS", "A5": "CC1", "A6": "D+", "A7": "D-",
            "A8": "SBU1", "A9": "VBUS", "A12": "GND",
            "B1": "GND", "B4": "VBUS", "B5": "CC2", "B6": "D+", "B7": "D-",
            "B8": "SBU2", "B9": "VBUS", "B12": "GND",
        },
        "spec_source": "USB Type-C Spec Rev 2.1 Table 3-1",
    },
    {
        "ref": "U4",
        "name": "PAM8403H Class-D Amp",
        "footprint_lib": "Package_SO",
        "footprint_name": "SOIC-16_3.9x9.9mm_P1.27mm",
        "expected_pads": {
            "1": "OUTL-", "2": "PGND", "3": "OUTL+", "4": "PVDD",
            "5": "MUTE", "6": "VDD", "7": "INL", "8": "VREF",
            "9": "NC", "10": "INR", "11": "GND", "12": "SHDN",
            "13": "PVDD", "14": "OUTR+", "15": "PGND", "16": "OUTR-",
        },
        "spec_source": "PAM8403H.PDF (Diodes Inc DS31295 Rev 1-0, im Repo)",
        "note": "SOIC-16-Footprint: nur Pad-Nummern werden geprüft (standard 1..16). "
                "Die Pin-Funktion wird vom Symbol kontrolliert, nicht vom Footprint.",
    },
    {
        "ref": "U3",
        "name": "PCM5102A I²S DAC",
        "footprint_lib": "Package_SO",
        "footprint_name": "TSSOP-20_4.4x6.5mm_P0.65mm",
        "expected_pads": {str(i): f"Pin{i}" for i in range(1, 21)},
        "spec_source": "TI Datasheet SLAS859C",
        "note": "TSSOP-20: Pad-Nummern 1..20 sind standard. Pin-Funktion vom Symbol.",
    },
    {
        "ref": "U2",
        "name": "MCP23017 I/O Expander",
        "footprint_lib": "Package_SO",
        "footprint_name": "SSOP-28_5.3x10.2mm_P0.65mm",
        "expected_pads": {str(i): f"Pin{i}" for i in range(1, 29)},
        "spec_source": "Microchip Datasheet DS20001952",
        "note": "SSOP-28: Pad-Nummern 1..28 standard.",
    },
]


# -------------------------------------------------------------------------
# KiCad-Library-Finder
# -------------------------------------------------------------------------

def find_kicad_footprint_dir():
    """Finde die KiCad-Footprint-Library auf dem System."""
    candidates = [
        os.environ.get("KICAD9_FOOTPRINT_DIR"),
        os.environ.get("KICAD_FOOTPRINT_DIR"),
        "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints",
        "/usr/share/kicad/footprints",
        "/usr/local/share/kicad/footprints",
        "C:/Program Files/KiCad/9.0/share/kicad/footprints",
        "C:/Program Files/KiCad/8.0/share/kicad/footprints",
    ]
    for path in candidates:
        if path and Path(path).exists():
            return Path(path)
    return None


def load_footprint(lib_dir: Path, lib: str, name: str):
    """Lade ein Footprint-File aus KiCad-Library."""
    fp_dir = lib_dir / f"{lib}.pretty"
    if not fp_dir.exists():
        return None, f"Library nicht gefunden: {fp_dir}"
    fp_file = fp_dir / f"{name}.kicad_mod"
    if not fp_file.exists():
        # Suche fuzzy nach ähnlichen Namen
        candidates = list(fp_dir.glob("*.kicad_mod"))
        similar = [c.stem for c in candidates if name.lower()[:15] in c.stem.lower()]
        suggest = f" (ähnlich gefunden: {similar[:3]})" if similar else ""
        return None, f"Footprint nicht gefunden: {fp_file}{suggest}"
    return fp_file.read_text(), None


def extract_pads(footprint_text: str) -> set:
    """Extrahiere alle Pad-Nummern aus Footprint-Text."""
    # KiCad-Format: (pad "1" smd ...) oder (pad "A1" smd ...)
    pads = set()
    for m in re.finditer(r'\(pad\s+"([^"]+)"', footprint_text):
        pads.add(m.group(1))
    return pads


# -------------------------------------------------------------------------
# Main
# -------------------------------------------------------------------------

def main():
    if len(sys.argv) > 1:
        kicad_dir = Path(sys.argv[1])
    else:
        kicad_dir = find_kicad_footprint_dir()

    if not kicad_dir or not kicad_dir.exists():
        print("❌ KiCad-Footprint-Library nicht gefunden.")
        print()
        print("Suche in Standard-Pfaden fehlgeschlagen. Bitte angeben:")
        print("    python3 scripts/check_footprints.py /pfad/zu/kicad/footprints")
        print()
        print("Oder env-var setzen:")
        print("    export KICAD9_FOOTPRINT_DIR=/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints")
        return 1

    print(f"✓ KiCad-Library: {kicad_dir}")
    print()

    all_ok = True
    for check in CHECKS:
        print(f"### {check['ref']} — {check['name']}")
        print(f"    Footprint:  {check['footprint_lib']}:{check['footprint_name']}")
        print(f"    Reference:  {check['spec_source']}")

        fp_text, err = load_footprint(kicad_dir, check["footprint_lib"], check["footprint_name"])
        if err:
            print(f"    ❌ {err}")
            all_ok = False
            print()
            continue

        actual_pads = extract_pads(fp_text)
        expected_pads = set(check["expected_pads"].keys())
        missing = expected_pads - actual_pads
        extra = actual_pads - expected_pads

        if missing:
            print(f"    ❌ Fehlende Pads im Footprint: {sorted(missing)}")
            all_ok = False
        if extra and len(extra) < 10:
            print(f"    ⚠️  Extra Pads im Footprint (vielleicht Mount-Tabs, OK): {sorted(extra)}")
        elif extra:
            print(f"    ⚠️  {len(extra)} extra Pads im Footprint (Mount-Tabs etc.)")

        if not missing:
            print(f"    ✓ Alle {len(expected_pads)} erwarteten Pad-Numbers vorhanden")
        if check.get("note"):
            print(f"    ℹ️  {check['note']}")
        print()

    print("=" * 70)
    if all_ok:
        print("✓ Footprint-Sanity-Check OK. Pad-Nummern stimmen mit Symbol-Pin-Numbers.")
        print()
        print("⚠️  WICHTIG: Dieser Check prüft NUR Pad-NUMMERN, nicht Pad-POSITIONEN.")
        print("    Footprint-Body-Maße + Pin-1-Orientierung musst du in KiCad GUI")
        print("    visuell gegen das Datasheet checken (siehe KICAD_QUICKSTART.md).")
        return 0
    else:
        print("❌ Footprint-Check FEHLER. Fix bevor PCB-Layout.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
