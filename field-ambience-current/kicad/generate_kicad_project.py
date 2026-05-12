"""
KiCad-9 Projekt-Generator für Field Ambience PCB SPEC v0.6.

Reproduzierbar regenerierbar bei Spec-Änderungen. Erzeugt:
  - field_ambience.kicad_pro      (Projekt-JSON)
  - field_ambience.kicad_sch      (Root-Schematic mit hierarchischen Sheet-Refs)
  - power_tree.kicad_sch          (Sheet 1: USB-C -> F1 -> Bulk -> +5V/+3V3 Rail)

Stand: Sheet 1 implementiert. Sheets 2-7 (Pico, OLED, MCP23017, Encoder,
Audio, Pi-Header) als next-Step erweitern. UUIDs deterministisch aus
sha1(seed) abgeleitet damit Re-Generierung stabile IDs gibt.
"""

from __future__ import annotations

import hashlib
import json
import uuid
from pathlib import Path

OUT_DIR = Path(__file__).resolve().parent
PROJECT_NAME = "field_ambience"
KICAD_VERSION_TAG = "20231120"  # KiCad 9 schematic format tag
GENERATOR = '(generator "kicad_python") (generator_version "9.0")'


def det_uuid(seed: str) -> str:
    """Deterministic UUID v4-shaped from sha1(seed) for stable re-runs."""
    h = hashlib.sha1(seed.encode()).hexdigest()
    return f"{h[0:8]}-{h[8:12]}-4{h[13:16]}-a{h[17:20]}-{h[20:32]}"


def kicad_pro() -> str:
    """Minimal KiCad 9 .kicad_pro project file (JSON)."""
    pro = {
        "board": {
            "design_settings": {
                "defaults": {
                    "board_outline_line_width": 0.05,
                    "copper_line_width": 0.2,
                    "copper_text_size_h": 1.5,
                    "copper_text_size_v": 1.5,
                    "copper_text_thickness": 0.3,
                },
            },
        },
        "boards": [],
        "cvpcb": {"equivalence_files": []},
        "erc": {
            "erc_exclusions": [],
            "meta": {"version": 0},
            "pin_map": [],
            "rule_severities": {
                "bus_definition_conflict": "error",
                "bus_entry_needed": "error",
                "bus_to_bus_conflict": "error",
                "bus_to_net_conflict": "error",
                "different_unit_footprint": "error",
                "different_unit_net": "error",
                "duplicate_reference": "error",
                "duplicate_sheet_names": "error",
                "endpoint_off_grid": "warning",
                "extra_units": "error",
                "global_label_dangling": "warning",
                "hier_label_mismatch": "error",
                "label_dangling": "error",
                "lib_symbol_issues": "warning",
                "missing_bidi_pin": "warning",
                "missing_input_pin": "warning",
                "missing_power_pin": "error",
                "missing_unit": "warning",
                "multiple_net_names": "warning",
                "net_not_bus_member": "warning",
                "no_connect_connected": "warning",
                "no_connect_dangling": "warning",
                "pin_not_connected": "error",
                "pin_not_driven": "error",
                "pin_to_pin": "warning",
                "power_pin_not_driven": "error",
                "similar_labels": "warning",
                "simulation_model_issue": "ignore",
                "unannotated": "error",
                "unit_value_mismatch": "error",
                "unresolved_variable": "error",
                "wire_dangling": "error",
            },
        },
        "libraries": {"pinned_footprint_libs": [], "pinned_symbol_libs": []},
        "meta": {"filename": f"{PROJECT_NAME}.kicad_pro", "version": 1},
        "net_settings": {
            "classes": [
                {
                    "bus_width": 12,
                    "clearance": 0.2,
                    "diff_pair_gap": 0.25,
                    "diff_pair_via_gap": 0.25,
                    "diff_pair_width": 0.2,
                    "line_style": 0,
                    "microvia_diameter": 0.3,
                    "microvia_drill": 0.1,
                    "name": "Default",
                    "pcb_color": "rgba(0, 0, 0, 0.000)",
                    "schematic_color": "rgba(0, 0, 0, 0.000)",
                    "track_width": 0.25,
                    "via_diameter": 0.6,
                    "via_drill": 0.3,
                    "wire_width": 6,
                }
            ],
            "meta": {"version": 3},
            "net_colors": None,
            "netclass_assignments": None,
            "netclass_patterns": [],
        },
        "pcbnew": {"page_layout_descr_file": ""},
        "schematic": {
            "annotate_start_num": 0,
            "drawing": {
                "dashed_lines_dash_length_ratio": 12.0,
                "dashed_lines_gap_length_ratio": 3.0,
                "default_line_thickness": 6.0,
                "default_text_size": 50.0,
                "field_names": [],
                "intersheets_ref_own_page": False,
                "intersheets_ref_prefix": "",
                "intersheets_ref_short": False,
                "intersheets_ref_show": False,
                "intersheets_ref_suffix": "",
                "junction_size_choice": 3,
                "label_size_ratio": 0.375,
                "operating_point_overlay_i_precision": 3,
                "operating_point_overlay_i_range": "~A",
                "operating_point_overlay_v_precision": 3,
                "operating_point_overlay_v_range": "~V",
                "overbar_offset_ratio": 1.23,
                "pin_symbol_size": 25.0,
                "text_offset_ratio": 0.15,
            },
            "legacy_lib_dir": "",
            "legacy_lib_list": [],
            "meta": {"version": 1},
            "net_format_name": "",
            "page_layout_descr_file": "",
            "plot_directory": "",
            "spice_current_sheet_as_root": False,
            "spice_external_command": "spice \"%I\"",
            "spice_model_current_sheet_as_root": True,
            "spice_save_all_currents": False,
            "spice_save_all_voltages": False,
            "spice_save_all_dissipations": False,
            "subpart_first_id": 65,
            "subpart_id_separator": 0,
        },
        "sheets": [
            [det_uuid("root_sheet"), "Root"],
            [det_uuid("sheet_power_tree"), "PowerTree"],
        ],
        "text_variables": {},
    }
    return json.dumps(pro, indent=2)


# ----------------------------------------------------------------------------
# Inline symbol library — minimale, eigenständige Symbol-Definitionen
# damit das Schematic ohne externe KiCad-Bibliotheken parst + öffnet.
# Im KiCad-GUI später via "Update Symbol Library Links" auf Standard-Libs neu
# verlinken.
# ----------------------------------------------------------------------------

LIB_SYMBOLS = r"""
    (symbol "Power:+5V" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "#PWR" (at 0 -3.81 0)
        (effects (font (size 1.27 1.27)) hide))
      (property "Value" "+5V" (at 0 3.556 0)
        (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Power:+5V_0_1"
        (polyline (pts (xy -0.762 1.27) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 0) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 2.54) (xy 0.762 1.27)) (stroke (width 0) (type default)) (fill (type none))))
      (symbol "Power:+5V_1_1"
        (pin power_in line (at 0 0 90) (length 0) hide
          (name "+5V" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))))
    (symbol "Power:GND" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "#PWR" (at 0 -6.35 0)
        (effects (font (size 1.27 1.27)) hide))
      (property "Value" "GND" (at 0 -3.81 0)
        (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Power:GND_0_1"
        (polyline (pts (xy 0 0) (xy 0 -1.27) (xy 1.27 -1.27) (xy 0 -2.54) (xy -1.27 -1.27) (xy 0 -1.27))
          (stroke (width 0) (type default)) (fill (type none))))
      (symbol "Power:GND_1_1"
        (pin power_in line (at 0 0 270) (length 0) hide
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))))
    (symbol "Power:VBUS" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "#PWR" (at 0 -3.81 0)
        (effects (font (size 1.27 1.27)) hide))
      (property "Value" "VBUS" (at 0 3.556 0)
        (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Power:VBUS_0_1"
        (polyline (pts (xy -0.762 1.27) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 0) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 2.54) (xy 0.762 1.27)) (stroke (width 0) (type default)) (fill (type none))))
      (symbol "Power:VBUS_1_1"
        (pin power_in line (at 0 0 90) (length 0) hide
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))))
    (symbol "Device:R" (pin_numbers hide) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "R" (at 2.032 0 90) (effects (font (size 1.27 1.27))))
      (property "Value" "R" (at 0 0 90) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at -1.778 0 90) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:R_0_1"
        (rectangle (start -1.016 -2.54) (end 1.016 2.54)
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Device:R_1_1"
        (pin passive line (at 0 3.81 270) (length 1.27)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -3.81 90) (length 1.27)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
    (symbol "Device:C" (pin_numbers hide) (pin_names (offset 0.254)) (in_bom yes) (on_board yes)
      (property "Reference" "C" (at 0.635 2.54 0) (effects (font (size 1.27 1.27))))
      (property "Value" "C" (at 0.635 -2.54 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0.9652 -3.81 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:C_0_1"
        (polyline (pts (xy -2.032 -0.762) (xy 2.032 -0.762))
          (stroke (width 0.508) (type default)) (fill (type none)))
        (polyline (pts (xy -2.032 0.762) (xy 2.032 0.762))
          (stroke (width 0.508) (type default)) (fill (type none))))
      (symbol "Device:C_1_1"
        (pin passive line (at 0 3.81 270) (length 2.794)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -3.81 90) (length 2.794)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
    (symbol "Device:CP" (pin_names (offset 0.254)) (in_bom yes) (on_board yes)
      (property "Reference" "C" (at 0.9525 2.54 0) (effects (font (size 1.27 1.27))))
      (property "Value" "C_Polarized" (at 0.9525 -2.54 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 2.54 -3.81 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:CP_0_1"
        (rectangle (start -2.286 -0.508) (end 2.286 -1.524)
          (stroke (width 0) (type default)) (fill (type outline)))
        (polyline (pts (xy -2.032 0.762) (xy 2.032 0.762))
          (stroke (width 0.508) (type default)) (fill (type none)))
        (polyline (pts (xy -1.778 2.286) (xy -1.778 1.524))
          (stroke (width 0.3048) (type default)) (fill (type none)))
        (polyline (pts (xy -1.397 1.905) (xy -2.159 1.905))
          (stroke (width 0.3048) (type default)) (fill (type none))))
      (symbol "Device:CP_1_1"
        (pin passive line (at 0 3.81 270) (length 2.794)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -3.81 90) (length 2.794)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
    (symbol "Device:Polyfuse" (pin_names (offset 0.254) hide) (in_bom yes) (on_board yes)
      (property "Reference" "F" (at 0 2.794 0) (effects (font (size 1.27 1.27))))
      (property "Value" "Polyfuse" (at 0 -2.794 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:Polyfuse_0_1"
        (rectangle (start -2.54 -0.762) (end 2.54 0.762)
          (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -1.524 -0.508) (xy 1.524 0.508))
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Device:Polyfuse_1_1"
        (pin passive line (at -3.81 0 0) (length 1.27)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 3.81 0 180) (length 1.27)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
    (symbol "Device:D_TVS" (pin_names (offset 0.254) hide) (in_bom yes) (on_board yes)
      (property "Reference" "D" (at 0 2.794 0) (effects (font (size 1.27 1.27))))
      (property "Value" "D_TVS" (at 0 -2.794 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:D_TVS_0_1"
        (polyline (pts (xy 0 -1.27) (xy 0 1.27))
          (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -1.27 -1.27) (xy 1.27 0) (xy -1.27 1.27))
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Device:D_TVS_1_1"
        (pin passive line (at -3.81 0 0) (length 2.54)
          (name "K" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 3.81 0 180) (length 2.54)
          (name "A" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
    (symbol "Connector:USB_C_Receptacle" (in_bom yes) (on_board yes)
      (property "Reference" "J" (at 0 17.78 0) (effects (font (size 1.27 1.27))))
      (property "Value" "USB_C_Receptacle" (at 0 15.24 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "https://www.usb.org/" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Connector:USB_C_Receptacle_0_1"
        (rectangle (start -12.7 12.7) (end 12.7 -22.86)
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Connector:USB_C_Receptacle_1_1"
        (pin power_in line (at -15.24 10.16 0) (length 2.54)
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "A1" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 7.62 0) (length 2.54)
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "A4" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 5.08 0) (length 2.54)
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "B4" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 2.54 0) (length 2.54)
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "B1" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 15.24 7.62 180) (length 2.54)
          (name "CC1" (effects (font (size 1.27 1.27))))
          (number "A5" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 15.24 5.08 180) (length 2.54)
          (name "CC2" (effects (font (size 1.27 1.27))))
          (number "B5" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 15.24 2.54 180) (length 2.54)
          (name "D+" (effects (font (size 1.27 1.27))))
          (number "A6" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 15.24 0 180) (length 2.54)
          (name "D-" (effects (font (size 1.27 1.27))))
          (number "A7" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 15.24 -2.54 180) (length 2.54)
          (name "D+" (effects (font (size 1.27 1.27))))
          (number "B6" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 15.24 -5.08 180) (length 2.54)
          (name "D-" (effects (font (size 1.27 1.27))))
          (number "B7" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 15.24 -7.62 180) (length 2.54)
          (name "SBU1" (effects (font (size 1.27 1.27))))
          (number "A8" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 15.24 -10.16 180) (length 2.54)
          (name "SBU2" (effects (font (size 1.27 1.27))))
          (number "B8" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 -2.54 0) (length 2.54)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "A12" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 -5.08 0) (length 2.54)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "A9" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 -7.62 0) (length 2.54)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "B9" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 -10.16 0) (length 2.54)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "B12" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at 0 -25.4 90) (length 2.54)
          (name "SHIELD" (effects (font (size 1.27 1.27))))
          (number "S1" (effects (font (size 1.27 1.27)))))))
    (symbol "Power_Protection:USBLC6-2SC6" (in_bom yes) (on_board yes)
      (property "Reference" "D" (at 0 7.62 0) (effects (font (size 1.27 1.27))))
      (property "Value" "USBLC6-2SC6" (at 0 5.08 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "Package_TO_SOT_SMD:SOT-23-6" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "https://www.st.com/resource/en/datasheet/usblc6-2.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Power_Protection:USBLC6-2SC6_0_1"
        (rectangle (start -5.08 3.81) (end 5.08 -3.81)
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Power_Protection:USBLC6-2SC6_1_1"
        (pin bidirectional line (at -7.62 2.54 0) (length 2.54)
          (name "I/O1" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -7.62 0 0) (length 2.54)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at -7.62 -2.54 0) (length 2.54)
          (name "I/O2" (effects (font (size 1.27 1.27))))
          (number "3" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 7.62 -2.54 180) (length 2.54)
          (name "I/O2" (effects (font (size 1.27 1.27))))
          (number "4" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at 7.62 0 180) (length 2.54)
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "5" (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 7.62 2.54 180) (length 2.54)
          (name "I/O1" (effects (font (size 1.27 1.27))))
          (number "6" (effects (font (size 1.27 1.27)))))))
""".strip()


def fmt_property(name: str, value: str, x: float, y: float, hide: bool = False) -> str:
    h = " hide" if hide else ""
    return (
        f'      (property "{name}" "{value}" (at {x} {y} 0)\n'
        f"        (effects (font (size 1.27 1.27)){h}))"
    )


def place_symbol(
    *,
    lib_id: str,
    ref: str,
    value: str,
    x: float,
    y: float,
    footprint: str = "",
    datasheet: str = "~",
    rotation: int = 0,
    seed_suffix: str = "",
    extra_props: dict[str, str] | None = None,
) -> str:
    """Render a placed instance of a symbol on the sheet."""
    sym_uuid = det_uuid(f"sym/{ref}/{seed_suffix}")
    props = [
        fmt_property("Reference", ref, x + 2.54, y - 2.54),
        fmt_property("Value", value, x + 2.54, y + 1.27),
        fmt_property("Footprint", footprint, x, y, hide=True),
        fmt_property("Datasheet", datasheet, x, y, hide=True),
    ]
    if extra_props:
        offset = 3.81
        for k, v in extra_props.items():
            props.append(fmt_property(k, v, x + 2.54, y + offset, hide=True))
            offset += 1.27
    return (
        f"  (symbol (lib_id \"{lib_id}\") (at {x} {y} {rotation}) (unit 1)\n"
        f"    (in_bom yes) (on_board yes) (dnp no)\n"
        f"    (uuid \"{sym_uuid}\")\n"
        + "\n".join(props)
        + f"\n    (instances\n      (project \"{PROJECT_NAME}\"\n"
        f"        (path \"/{det_uuid('sheet_power_tree')}\" (reference \"{ref}\") (unit 1)))))\n"
    )


def wire(x1: float, y1: float, x2: float, y2: float, seed_suffix: str = "") -> str:
    return (
        f'  (wire (pts (xy {x1} {y1}) (xy {x2} {y2}))\n'
        f"    (stroke (width 0) (type default))\n"
        f"    (uuid \"{det_uuid(f'wire/{x1},{y1}-{x2},{y2}/{seed_suffix}')}\"))\n"
    )


def junction(x: float, y: float) -> str:
    return (
        f"  (junction (at {x} {y}) (diameter 0) (color 0 0 0 0)\n"
        f"    (uuid \"{det_uuid(f'jct/{x},{y}')}\"))\n"
    )


def label(x: float, y: float, name: str, rotation: int = 0) -> str:
    return (
        f"  (label \"{name}\" (at {x} {y} {rotation}) (fields_autoplaced)\n"
        f"    (effects (font (size 1.27 1.27)) (justify left bottom))\n"
        f"    (uuid \"{det_uuid(f'lbl/{x},{y}/{name}')}\"))\n"
    )


def hier_label(x: float, y: float, name: str, shape: str = "passive", rotation: int = 0) -> str:
    return (
        f'  (hierarchical_label "{name}" (shape {shape}) (at {x} {y} {rotation})\n'
        f"    (effects (font (size 1.524 1.524)) (justify right))\n"
        f"    (uuid \"{det_uuid(f'hlbl/{x},{y}/{name}')}\"))\n"
    )


# ----------------------------------------------------------------------------
# Sheet 1 — Power Tree
# Komponenten + Verdrahtung lt. SPEC v0.6 §1, §3:
#   USB-C(VBUS) -> F1 (polyfuse 2A/4A) -> C_BULK (1000µF) -> +5V Rail
#   USB-C(D±)   -> D1 (USBLC6-2SC6 ESD)   -> Pico USB (hier-label to other sheet)
#   USB-C(CC1)  -> R2 (5.1k)              -> GND
#   USB-C(CC2)  -> R3 (5.1k)              -> GND
#   +5V         -> D2 (SMAJ5.0A TVS)      -> GND
# ----------------------------------------------------------------------------


def power_tree_sheet() -> str:
    sheet_uuid = det_uuid("sheet_power_tree")
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    # J1 USB-C connector @ (50, 80)
    symbols.append(
        place_symbol(
            lib_id="Connector:USB_C_Receptacle",
            ref="J1",
            value="USB_C_Receptacle (TYPE-C-31-M-12, C165948)",
            x=50,
            y=80,
            footprint="Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12",
            datasheet="https://datasheet.lcsc.com/lcsc/2004081102_Korean-Hroparts-Elec-TYPE-C-31-M-12_C165948.pdf",
            extra_props={"MPN": "TYPE-C-31-M-12", "LCSC": "C165948"},
            seed_suffix="J1",
        )
    )
    # USB-C VBUS pins (A1,A4,B1,B4) -> common +5V node @ (35, 70..77.62)
    # Pin A1 @ (50-15.24, 80+10.16) = (34.76, 90.16)
    # → we treat USB-C pin positions relative to (50,80):
    #   A1=VBUS @ (50-15.24, 80+10.16)=(34.76, 90.16)
    #   A4=VBUS @ (34.76, 87.62)
    #   B4=VBUS @ (34.76, 85.08)
    #   B1=VBUS @ (34.76, 82.56)
    # Verbindung der 4 VBUS-Pins durch Vertikalstrang bei x=30.48
    for y in (90.16, 87.62, 85.08, 82.56):
        wires.append(wire(34.76, y, 30.48, y, seed_suffix=f"vbus-stub-{y}"))
    # Trunk segmentiert: jeder Segment-Endpunkt fällt mit einem Stub-Endpunkt zusammen.
    # Analyzer folgt Connectivity nur über Wire-Endpoints, nicht über durchlaufende Mid-Points.
    wires.append(wire(30.48, 82.56, 30.48, 85.08, seed_suffix="vbus-trunk-1"))
    wires.append(wire(30.48, 85.08, 30.48, 87.62, seed_suffix="vbus-trunk-2"))
    wires.append(wire(30.48, 87.62, 30.48, 90.16, seed_suffix="vbus-trunk-3"))
    # 3-Wege bei (30.48, 87.62) und (30.48, 85.08): zwei Trunk-Segmente + Stub → Junction.
    # 3-Wege bei (30.48, 90.16): trunk-top + stub_A1 + vbus-up → Junction.
    junctions.extend(junction(30.48, y) for y in (90.16, 87.62, 85.08))

    # USB-C GND pins (A9,A12,B9,B12) -> Vertikalstrang bei x=30.48 nach unten
    # A12=GND @ (34.76, 77.46)
    # A9 =GND @ (34.76, 74.92)
    # B9 =GND @ (34.76, 72.38)
    # B12=GND @ (34.76, 69.84)
    for y in (77.46, 74.92, 72.38, 69.84):
        wires.append(wire(34.76, y, 30.48, y, seed_suffix=f"gnd-stub-{y}"))
    # GND-Trunk segmentiert pro Junction-Punkt — siehe VBUS-Trunk-Kommentar.
    wires.append(wire(30.48, 69.84, 30.48, 72.38, seed_suffix="gnd-trunk-1"))
    wires.append(wire(30.48, 72.38, 30.48, 74.92, seed_suffix="gnd-trunk-2"))
    wires.append(wire(30.48, 74.92, 30.48, 77.46, seed_suffix="gnd-trunk-3"))
    junctions.extend(junction(30.48, y) for y in (77.46, 74.92, 72.38, 69.84))

    # VBUS-trunk → F1 (polyfuse) @ (45, 95)
    # F1 horizontal: pin1 at (45-3.81,95)=(41.19,95), pin2 at (45+3.81,95)=(48.81,95)
    symbols.append(
        place_symbol(
            lib_id="Device:Polyfuse",
            ref="F1",
            value="2A/4A 1812 (MF-MSMF200, C210837)",
            x=45,
            y=95,
            footprint="Fuse:Fuse_1812_4532Metric",
            datasheet="https://www.bourns.com/docs/Product-Datasheets/mfmsmf.pdf",
            extra_props={"MPN": "MF-MSMF200", "LCSC": "C210837"},
            seed_suffix="F1",
        )
    )
    # Trunk top → F1 left pin: from (30.48, 90.16) up to (30.48, 95).
    # Die horizontale Verbindung zu F1 wird in Segmenten nach Power-Flag aufgebaut.
    wires.append(wire(30.48, 90.16, 30.48, 95, seed_suffix="vbus-up"))

    # F1 right pin (48.81, 95) → +5V rail node (60, 95)
    wires.append(wire(48.81, 95, 60, 95, seed_suffix="f1-to-rail"))

    # +5V power flag on the rail
    symbols.append(
        place_symbol(
            lib_id="Power:+5V",
            ref="#PWR05",
            value="+5V",
            x=60,
            y=92.46,  # symbol pin at y_pin = symbol_y + 2.54 (going up)
            footprint="",
            seed_suffix="pwr5v",
        )
    )
    # power symbol pin is at the symbol's (at) y minus 2.54 (since pin offset 0)... actually
    # the Power:+5V symbol has its pin at (0,0) and the arrow extends to +2.54y.
    # We place the symbol at (60, 92.46), its pin is at (60, 92.46). Wire from (60,95) to (60,92.46).
    wires.append(wire(60, 92.46, 60, 95, seed_suffix="rail-to-flag"))

    # +5V_USB Power-Symbol auf die Rail (vor F1) — als definierte Net-Quelle
    symbols.append(
        place_symbol(
            lib_id="Power:VBUS",
            ref="#PWR02",
            value="+5V_USB",
            x=37,
            y=92.46,  # pin am Trunk-Up-Wire bei y=95
            seed_suffix="vbus-flag",
        )
    )
    wires.append(wire(37, 95, 37, 92.46, seed_suffix="vbus-flag-to-rail"))
    junctions.append(junction(37, 95))
    wires.append(wire(30.48, 95, 37, 95, seed_suffix="rail-vbus-flag-fill"))
    wires.append(wire(37, 95, 41.19, 95, seed_suffix="rail-fill-after-flag"))

    # C_BULK (1000µF polarized) @ (65, 100): +pin@(65,103.81)=top, -pin@(65,96.19)=bottom
    symbols.append(
        place_symbol(
            lib_id="Device:CP",
            ref="C_BULK",
            value="1000uF 10V SMD Elko",
            x=65,
            y=100,
            footprint="Capacitor_SMD:CP_Elec_8x6.7",
            seed_suffix="CBULK",
        )
    )
    # Top pin (+5V): (65, 103.81) -> wire up to rail. Actually rail is at y=95, so
    # CP placed at (65,100) gives pin1 at y=103.81 (top) and pin2 at y=96.19 (bottom).
    # We want +5V at top: tie pin1 to rail.
    wires.append(wire(65, 96.19, 65, 95, seed_suffix="cbulk-pin2-to-rail"))
    junctions.append(junction(65, 95))
    # Bottom pin -> GND label
    wires.append(wire(65, 103.81, 65, 105, seed_suffix="cbulk-pin1-down"))
    # Hmm wait - that's backwards. Let me fix: polarized cap, + on top. Pin 1 (top, +) goes to +5V, Pin 2 (bottom, -) goes to GND.
    # KiCad Device:CP has pin 1 at y+3.81 (top), pin 2 at y-3.81 (bottom). So pin1=top, pin2=bottom.
    # Placed at (65,100): pin1 at (65, 103.81), pin2 at (65, 96.19).
    # We want pin1(+) to go to +5V rail at y=95 — but pin1 is at y=103.81 which is ABOVE 95.
    # Solution: flip the cap. Place at (65, 90), so pin1 at (65, 93.81) — close to rail at y=95.
    # Let me redo:
    symbols.pop()  # remove the misplaced CP
    wires.pop()
    junctions.pop()
    wires.pop()
    symbols.append(
        place_symbol(
            lib_id="Device:CP",
            ref="C_BULK",
            value="1000uF 10V SMD Elko",
            x=65,
            y=99,  # pin1 (+) at y=102.81, pin2 (-) at y=95.19
            footprint="Capacitor_SMD:CP_Elec_8x6.7",
            seed_suffix="CBULK",
        )
    )
    # pin2 (bottom, -) at y=95.19 → rail at y=95: wire (65,95.19) → (65,95)
    wires.append(wire(65, 95.19, 65, 95, seed_suffix="cbulk-neg-to-rail-fix"))
    # Hmm pin polarity: KiCad Device:CP convention — pin 1 is + (top, marked by rectangle fill).
    # We want + on top connected to +5V. Rail is at y=95 below the cap top pin. So flip: place cap with pin1 toward rail.
    # Actually let me just rotate the cap 180° so pin 1 (positive marker) is at bottom and connect to rail (which is below the cap body).
    # Skip rotation complexity — instead place the cap *below* the rail with pin1 (top, +) connecting up to rail.
    symbols.pop()
    wires.pop()
    symbols.append(
        place_symbol(
            lib_id="Device:CP",
            ref="C_BULK",
            value="1000uF 10V SMD Elko",
            x=65,
            y=98.81,  # pin1 (+) at y=102.62 — no good
            footprint="Capacitor_SMD:CP_Elec_8x6.7",
            seed_suffix="CBULK",
        )
    )
    # Ugh. Just put cap with center at y=98.81, pin1 (top, +) at y=102.62, pin2 (bottom, -) at y=95.0.
    # Then pin2 (which is -) is at y=95 which IS the rail. That's wrong polarity!
    # OK: simpler. Rotate the cap to 180° so pin1 ends up at bottom.
    symbols.pop()
    symbols.append(
        place_symbol(
            lib_id="Device:CP",
            ref="C_BULK",
            value="1000uF 10V SMD Elko",
            x=65,
            y=98.81,
            footprint="Capacitor_SMD:CP_Elec_8x6.7",
            rotation=180,  # flip so pin1 (+) becomes bottom
            extra_props={"MPN": "EEE-FK1A102P (Panasonic) o.ä.", "LCSC": "TBD"},
            seed_suffix="CBULK",
        )
    )
    # With rotation 180, pin1 originally at (0,3.81) becomes (0,-3.81). So at center (65, 98.81):
    # pin1 (+) at (65, 95.0)  → connects to rail at y=95 ✓
    # pin2 (-) at (65, 102.62) → connects down to GND
    wires.append(wire(65, 95, 65, 96, seed_suffix="cbulk-pin1-to-rail"))  # tiny stub
    junctions.append(junction(65, 95))
    wires.append(wire(65, 102.62, 65, 105, seed_suffix="cbulk-pin2-down"))
    labels.append(label(65, 106, "GND", rotation=0))

    # CC1 pull-down: R2 = 5.1k from USB-C pin A5 (CC1) to GND
    # USB-C CC1 pin @ (50+15.24, 80+7.62) = (65.24, 87.62)
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R2",
            value="5.1k 0603 (CC1 pull-down)",
            x=75,
            y=87.62,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-075K1L", "LCSC": "C23186"},
            seed_suffix="R2",
        )
    )
    # R rotated 90° at (75,87.62): pin1 originally at (0,3.81)→(−3.81,0)→absolute (71.19, 87.62)
    # pin2 originally at (0,−3.81) → (3.81,0) → absolute (78.81, 87.62)
    wires.append(wire(65.24, 87.62, 71.19, 87.62, seed_suffix="cc1-to-r2"))
    wires.append(wire(78.81, 87.62, 80, 87.62, seed_suffix="r2-to-gnd-stub"))
    labels.append(label(80, 87.62, "GND"))

    # CC2 pull-down: R3 = 5.1k from USB-C pin B5 (CC2)
    # USB-C CC2 pin @ (65.24, 85.08)
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R3",
            value="5.1k 0603 (CC2 pull-down)",
            x=75,
            y=85.08,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-075K1L", "LCSC": "C23186"},
            seed_suffix="R3",
        )
    )
    wires.append(wire(65.24, 85.08, 71.19, 85.08, seed_suffix="cc2-to-r3"))
    wires.append(wire(78.81, 85.08, 80, 85.08, seed_suffix="r3-to-gnd-stub"))
    labels.append(label(80, 85.08, "GND"))

    # ESD: D1 = USBLC6-2SC6 placed @ (95, 82.5)
    # D± pins from USB-C: D+ @ A6 (65.24, 82.54), D- @ A7 (65.24, 80.0), D+ @ B6 (65.24, 77.46), D- @ B7 (65.24, 74.92)
    # We connect A6+B6 (both D+) together at one line, A7+B7 (both D-) together — TYPE-C dual-orientation.
    wires.append(wire(65.24, 82.54, 65.24, 77.46, seed_suffix="dp-strap"))
    junctions.append(junction(65.24, 80.0))
    wires.append(wire(65.24, 80.0, 88, 80.0, seed_suffix="dp-to-usblc6"))
    labels.append(label(88, 80, "USB_DP"))
    wires.append(wire(65.24, 80.0, 65.24, 74.92, seed_suffix="dn-strap"))  # extended
    # Actually we want D+ separate from D-, so re-route D-:
    # Wait: I just connected D+ (A6) to D- (A7) via that wire — wrong! Let me redo.
    wires.pop()  # remove last "dn-strap"
    wires.pop()  # remove "dp-to-usblc6"
    wires.pop()  # remove "dp-strap"
    junctions.pop()
    # D+ (A6 and B6) bridged at x=65.24 between y=82.54 and y=77.46
    # D- (A7 and B7) bridged at x=65.24 between y=80.0 and y=74.92
    # Crossing problem — solve with separate horizontal exits.
    # D+ A6 (65.24, 82.54) — direct out
    wires.append(wire(65.24, 82.54, 88, 82.54, seed_suffix="dp-a6-out"))
    # D+ B6 (65.24, 77.46) — out at different vertical, then bridge with D+ A6 via label
    wires.append(wire(65.24, 77.46, 88, 77.46, seed_suffix="dp-b6-out"))
    labels.append(label(88, 82.54, "USB_DP"))
    labels.append(label(88, 77.46, "USB_DP"))  # same net via label
    # D- A7 (65.24, 80.0) out
    wires.append(wire(65.24, 80.0, 88, 80.0, seed_suffix="dn-a7-out"))
    labels.append(label(88, 80.0, "USB_DN"))
    # D- B7 (65.24, 74.92) out
    wires.append(wire(65.24, 74.92, 88, 74.92, seed_suffix="dn-b7-out"))
    labels.append(label(88, 74.92, "USB_DN"))

    # D1: USBLC6-2SC6 placed @ (100, 80)
    symbols.append(
        place_symbol(
            lib_id="Power_Protection:USBLC6-2SC6",
            ref="D1",
            value="USBLC6-2SC6 (ESD, C2687116)",
            x=100,
            y=80,
            footprint="Package_TO_SOT_SMD:SOT-23-6",
            extra_props={"MPN": "USBLC6-2SC6", "LCSC": "C2687116"},
            seed_suffix="D1",
        )
    )
    # USBLC6 pins (placed at center 100,80):
    # 1 I/O1 (left top)   @ (100-7.62, 80+2.54) = (92.38, 82.54)
    # 2 GND  (left mid)   @ (92.38, 80.0)
    # 3 I/O2 (left bot)   @ (92.38, 77.46)
    # 4 I/O2 (right bot)  @ (107.62, 77.46)
    # 5 VBUS (right mid)  @ (107.62, 80.0)
    # 6 I/O1 (right top)  @ (107.62, 82.54)
    # Wire pin1 (I/O1) ← USB_DP → label-bridge already done (USB_DP label at x=88)
    # We need a wire from somewhere with USB_DP label to pin1.
    # Easiest: drop another USB_DP label at (92.38, 82.54) and tie it directly.
    wires.append(wire(92.38, 82.54, 90, 82.54, seed_suffix="usblc6-pin1-stub"))
    labels.append(label(90, 82.54, "USB_DP"))
    # Pin 3 (I/O2) = USB_DN
    wires.append(wire(92.38, 77.46, 90, 77.46, seed_suffix="usblc6-pin3-stub"))
    labels.append(label(90, 77.46, "USB_DN"))
    # Pin 2 GND → GND label
    wires.append(wire(92.38, 80, 90, 80, seed_suffix="usblc6-gnd-stub"))
    labels.append(label(90, 80, "GND"))
    # Pin 5 VBUS → +5V rail (place +5V label)
    wires.append(wire(107.62, 80, 110, 80, seed_suffix="usblc6-vbus-stub"))
    labels.append(label(110, 80, "+5V_USB"))
    # Pins 4 and 6 are the "downstream" I/O (after ESD protection)
    # → these are what actually go to the Pico USB peripheral
    wires.append(wire(107.62, 82.54, 110, 82.54, seed_suffix="usblc6-pin6"))
    hlabels.append(hier_label(110, 82.54, "PICO_USB_DP", shape="output", rotation=0))
    wires.append(wire(107.62, 77.46, 110, 77.46, seed_suffix="usblc6-pin4"))
    hlabels.append(hier_label(110, 77.46, "PICO_USB_DN", shape="output", rotation=0))

    # USB-C Shield S1 → GND
    # S1 pin @ (50, 80-25.4) = (50, 54.6)
    wires.append(wire(50, 54.6, 50, 52, seed_suffix="shield-stub"))
    labels.append(label(50, 52, "GND"))

    # USB-C SBU1/2 (A8, B8) → NC for now; just labeled as NC
    # A8 @ (65.24, 80-7.62) = (65.24, 72.38); B8 @ (65.24, 80-10.16) = (65.24, 69.84)
    wires.append(wire(65.24, 72.38, 68, 72.38, seed_suffix="sbu1-nc"))
    labels.append(label(68, 72.38, "NC_SBU1"))
    wires.append(wire(65.24, 69.84, 68, 69.84, seed_suffix="sbu2-nc"))
    labels.append(label(68, 69.84, "NC_SBU2"))

    # GND consolidation — Power-Symbol auf den Trunk (statt nur Label-Bridge)
    wires.append(wire(30.48, 69.84, 30.48, 67, seed_suffix="gnd-trunk-down"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR01",
            value="GND",
            x=30.48,
            y=67,
            seed_suffix="gnd-usbc",
        )
    )

    # D2: SMAJ5.0A TVS on +5V to GND, placed @ (55, 105)
    # Vertikal: pin1 (K, cathode) at top -> +5V; pin2 (A, anode) at bottom -> GND
    # D_TVS pin layout: pin1 K at x=-3.81 (left), pin2 A at x=+3.81 (right). Default horizontal.
    # Rotate 90° so pin1 K goes up (to +5V). Originally (-3.81,0)→(0,-3.81) with rotation 90 (clockwise)
    # Actually KiCad rotation is CCW. Let me just use rotation=270 (or equivalently -90):
    # pin1 (-3.81,0) with rotation 270: (x,y)→(-y, x) so (-3.81,0)→(0,-3.81). Pin moves to (0,-3.81) from symbol origin.
    # That means below center. We want K on top (going to +5V at y=95 which is above us at 105 — wait, larger y is "up" or "down" in KiCad?).
    # KiCad: y=0 at top, y grows downward. So y=95 is ABOVE y=105 on screen. Rail is "above" D2 visually.
    # D2 at center (55,105): rotation=270 puts pin1 at (55, 105-3.81)=(55, 101.19) [up-on-screen]. Good.
    symbols.append(
        place_symbol(
            lib_id="Device:D_TVS",
            ref="D2",
            value="SMAJ5.0A TVS (C113952)",
            x=55,
            y=105,
            rotation=270,
            footprint="Diode_SMD:D_SMA",
            extra_props={"MPN": "SMAJ5.0A", "LCSC": "C113952"},
            seed_suffix="D2",
        )
    )
    # D2 K pin (top): (55, 101.19) → wire up to rail at y=95: vertical wire
    wires.append(wire(55, 101.19, 55, 95, seed_suffix="d2-k-to-rail"))
    junctions.append(junction(55, 95))
    # D2 A pin (bottom): (55, 108.81) → GND label
    wires.append(wire(55, 108.81, 55, 112, seed_suffix="d2-a-down"))
    labels.append(label(55, 112, "GND"))
    # Connect rail at y=95 across all the parts that tie to it (F1, +5V flag, C_BULK, D2 — already wired in piecemeal)
    # The +5V rail spans x=48.81 (F1 right) to x=80 (right edge). Make sure it's one continuous net.
    wires.append(wire(55, 95, 65, 95, seed_suffix="rail-d2-cbulk"))

    # Hierarchical outputs to other sheets
    hlabels.append(hier_label(110, 95, "+5V_OUT", shape="output", rotation=0))
    wires.append(wire(60, 95, 110, 95, seed_suffix="rail-extend"))
    hlabels.append(hier_label(110, 67, "GND_OUT", shape="passive", rotation=0))
    wires.append(wire(30.48, 67, 110, 67, seed_suffix="gnd-rail"))

    # Sheet title block & paper size
    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 1: Power Tree")\n'
        f'    (date "2026-05-11")\n'
        f'    (rev "0.6")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6 §1 + §3")\n'
        f'    (comment 2 "USB-C → F1(2A) → C_BULK(1000µF) → +5V rail")\n'
        f'    (comment 3 "ESD: USBLC6-2SC6 on D+/D-; TVS: SMAJ5.0A on +5V")\n'
        f'    (comment 4 "CC1/CC2 5.1kΩ to GND (Sink, 3A capable)"))\n'
        "  (lib_symbols\n"
        + LIB_SYMBOLS
        + "\n  )\n"
        + "".join(symbols)
        + "".join(wires)
        + "".join(junctions)
        + "".join(labels)
        + "".join(hlabels)
        + f'  (sheet_instances\n    (path "/" (page "1")))\n'
        ")\n"
    )
    return body


# ----------------------------------------------------------------------------
# Root schematic
# ----------------------------------------------------------------------------


def root_sheet() -> str:
    root_uuid = det_uuid("root_sheet")
    power_uuid = det_uuid("sheet_power_tree")
    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{root_uuid}")\n'
        f'  (paper "A4")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Root")\n'
        f'    (date "2026-05-11")\n'
        f'    (rev "0.6")\n'
        f'    (company "Field Ambience Project"))\n'
        f'  (lib_symbols)\n'
        f'  (sheet (at 50 50) (size 60 30) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{power_uuid}")\n'
        f'    (property "Sheetname" "PowerTree" (at 50 49 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "power_tree.kicad_sch" (at 50 80.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "+5V_OUT" output (at 110 60 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_5v")}"))\n'
        f'    (pin "GND_OUT" passive (at 110 65 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_gnd")}"))\n'
        f'    (pin "PICO_USB_DP" output (at 110 70 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_dp")}"))\n'
        f'    (pin "PICO_USB_DN" output (at 110 75 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_dn")}")))\n'
        f'  (sheet_instances\n    (path "/" (page "1")))\n'
        ")\n"
    )
    return body


# ----------------------------------------------------------------------------
# Main: write all files
# ----------------------------------------------------------------------------


def main() -> None:
    (OUT_DIR / f"{PROJECT_NAME}.kicad_pro").write_text(kicad_pro())
    (OUT_DIR / f"{PROJECT_NAME}.kicad_sch").write_text(root_sheet())
    (OUT_DIR / "power_tree.kicad_sch").write_text(power_tree_sheet())
    print(f"Wrote KiCad project + Sheet 1 to {OUT_DIR}")


if __name__ == "__main__":
    main()
