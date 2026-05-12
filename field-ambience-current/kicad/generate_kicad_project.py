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
            [det_uuid("sheet_pico"), "Pico"],
            [det_uuid("sheet_oled"), "OLED"],
            [det_uuid("sheet_mcp"), "MCP23017"],
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
    (symbol "Power:+3V3" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "#PWR" (at 0 -3.81 0)
        (effects (font (size 1.27 1.27)) hide))
      (property "Value" "+3V3" (at 0 3.556 0)
        (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Power:+3V3_0_1"
        (polyline (pts (xy -0.762 1.27) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 0) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 2.54) (xy 0.762 1.27)) (stroke (width 0) (type default)) (fill (type none))))
      (symbol "Power:+3V3_1_1"
        (pin power_in line (at 0 0 90) (length 0) hide
          (name "+3V3" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))))
    (symbol "Device:LED" (pin_numbers hide) (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)
      (property "Reference" "D" (at 0 2.54 0) (effects (font (size 1.27 1.27))))
      (property "Value" "LED" (at 0 -2.54 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:LED_0_1"
        (polyline (pts (xy -1.27 -1.27) (xy -1.27 1.27))
          (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -1.27 0) (xy 1.27 1.27) (xy 1.27 -1.27) (xy -1.27 0))
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Device:LED_1_1"
        (pin passive line (at -3.81 0 0) (length 2.54)
          (name "K" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 3.81 0 180) (length 2.54)
          (name "A" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
    (symbol "Switch:SW_Push" (pin_numbers hide) (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)
      (property "Reference" "SW" (at 1.27 2.54 0) (effects (font (size 1.27 1.27))))
      (property "Value" "SW_Push" (at 0 -1.524 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Switch:SW_Push_0_1"
        (circle (center -2.032 0) (radius 0.508) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 1.27) (xy 0 2.032)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy -2.032 1.27) (xy 2.032 3.302)) (stroke (width 0) (type default)) (fill (type none)))
        (circle (center 2.032 0) (radius 0.508) (stroke (width 0) (type default)) (fill (type none))))
      (symbol "Switch:SW_Push_1_1"
        (pin passive line (at -5.08 0 0) (length 2.54)
          (name "1" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 5.08 0 180) (length 2.54)
          (name "2" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
    (symbol "Connector:Conn_01x03" (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)
      (property "Reference" "J" (at 0 5.08 0) (effects (font (size 1.27 1.27))))
      (property "Value" "Conn_01x03" (at 0 -5.08 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Connector:Conn_01x03_0_1"
        (rectangle (start -1.27 3.81) (end 1.27 -3.81)
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Connector:Conn_01x03_1_1"
        (pin passive line (at 3.81 2.54 180) (length 2.54)
          (name "Pin_1" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 3.81 0 180) (length 2.54)
          (name "Pin_2" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 3.81 -2.54 180) (length 2.54)
          (name "Pin_3" (effects (font (size 1.27 1.27))))
          (number "3" (effects (font (size 1.27 1.27)))))))
""".strip()


def _pico2_lib_symbol() -> str:
    """Build the MCU:Pico2 40-pin module symbol inline.

    Pin layout matches RP2350 / Pico 2 module 2.54mm header:
      Left  pins 1..20  (top→bottom)
      Right pins 40..21 (top→bottom)
    """
    pins_left = [
        (1, "GP0", "bidirectional"),
        (2, "GP1", "bidirectional"),
        (3, "GND", "power_in"),
        (4, "GP2", "bidirectional"),
        (5, "GP3", "bidirectional"),
        (6, "GP4", "bidirectional"),
        (7, "GP5", "bidirectional"),
        (8, "GND", "power_in"),
        (9, "GP6", "bidirectional"),
        (10, "GP7", "bidirectional"),
        (11, "GP8", "bidirectional"),
        (12, "GP9", "bidirectional"),
        (13, "GND", "power_in"),
        (14, "GP10", "bidirectional"),
        (15, "GP11", "bidirectional"),
        (16, "GP12", "bidirectional"),
        (17, "GP13", "bidirectional"),
        (18, "GND", "power_in"),
        (19, "GP14", "bidirectional"),
        (20, "GP15", "bidirectional"),
    ]
    # Right side numbered top-down 40..21:
    pins_right = [
        (40, "VBUS", "power_in"),
        (39, "VSYS", "power_in"),
        (38, "GND", "power_in"),
        (37, "3V3_EN", "input"),
        (36, "3V3_OUT", "power_out"),
        (35, "ADC_VREF", "input"),
        (34, "GP28", "bidirectional"),
        (33, "AGND", "power_in"),
        (32, "GP27", "bidirectional"),
        (31, "GP26", "bidirectional"),
        (30, "RUN", "input"),
        (29, "GP22", "bidirectional"),
        (28, "GND", "power_in"),
        (27, "GP21", "bidirectional"),
        (26, "GP20", "bidirectional"),
        (25, "GP19", "bidirectional"),
        (24, "GP18", "bidirectional"),
        (23, "GND", "power_in"),
        (22, "GP17", "bidirectional"),
        (21, "GP16", "bidirectional"),
    ]
    # 20 pins per side × 2.54mm spacing centered on y=0
    y_top = 24.13  # = (20-1)*2.54/2
    rect_top = y_top + 2.54
    rect_bot = -y_top - 2.54
    out = ['    (symbol "MCU:Pico2" (in_bom yes) (on_board yes)']
    out.append('      (property "Reference" "U" (at 0 31.75 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Value" "Raspberry_Pi_Pico2" (at 0 29.21 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "https://datasheets.raspberrypi.com/picow/pico-2-datasheet.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "MCU:Pico2_0_1"')
    out.append(f'        (rectangle (start -15 {rect_top}) (end 15 {rect_bot})')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append('      (symbol "MCU:Pico2_1_1"')
    # left-side pins
    for idx, (num, name, ptype) in enumerate(pins_left):
        y = y_top - idx * 2.54
        out.append(
            f'        (pin {ptype} line (at -17.54 {y:.3f} 0) (length 2.54)\n'
            f'          (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{num}" (effects (font (size 1.27 1.27)))))'
        )
    # right-side pins
    for idx, (num, name, ptype) in enumerate(pins_right):
        y = y_top - idx * 2.54
        out.append(
            f'        (pin {ptype} line (at 17.54 {y:.3f} 180) (length 2.54)\n'
            f'          (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{num}" (effects (font (size 1.27 1.27)))))'
        )
    out.append('        )')
    out.append('      )')
    return "\n".join(out)


def _conn_01xN_lib_symbol(n: int) -> str:
    """Generic N-pin 1-row header symbol (pin 1 at top in lib Y-UP).

    Pin local positions: x=+3.81, y=+top..-bottom (KiCad Y-UP). Pin angle 180.
    Pins display from top (pin 1) to bottom (pin N) with 2.54mm spacing.
    """
    y_top = (n - 1) * 1.27  # half of (n-1)*2.54
    rect_top = y_top + 2.54
    rect_bot = -y_top - 2.54
    out = [f'    (symbol "Connector:Conn_01x{n:02d}" (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)']
    out.append(f'      (property "Reference" "J" (at 0 {rect_top + 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append(f'      (property "Value" "Conn_01x{n:02d}" (at 0 {rect_bot - 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append(f'      (symbol "Connector:Conn_01x{n:02d}_0_1"')
    out.append(f'        (rectangle (start -1.27 {rect_top}) (end 1.27 {rect_bot})')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append(f'      (symbol "Connector:Conn_01x{n:02d}_1_1"')
    for idx in range(n):
        ly = y_top - idx * 2.54
        pin_num = idx + 1
        out.append(
            f'        (pin passive line (at 3.81 {ly:.3f} 180) (length 2.54)\n'
            f'          (name "Pin_{pin_num}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{pin_num}" (effects (font (size 1.27 1.27)))))'
        )
    out.append('        )')
    out.append('      )')
    return "\n".join(out)


def _mcp23017_lib_symbol() -> str:
    """MCP23017 SSOP-28 I/O-Expander. 14 Pins pro Seite. Chip-Pinout-Layout:
    Left (top→bottom): GPB0-7, VDD, VSS, NC, SCL, SDA, NC = pins 1..14
    Right (top→bottom): GPA7..GPA0, INTA, INTB, ~RESET, A2, A1, A0 = pins 28..15
    """
    pins_left = [
        (1, "GPB0", "bidirectional"),
        (2, "GPB1", "bidirectional"),
        (3, "GPB2", "bidirectional"),
        (4, "GPB3", "bidirectional"),
        (5, "GPB4", "bidirectional"),
        (6, "GPB5", "bidirectional"),
        (7, "GPB6", "bidirectional"),
        (8, "GPB7", "bidirectional"),
        (9, "VDD", "power_in"),
        (10, "VSS", "power_in"),
        (11, "NC", "no_connect"),
        (12, "SCL", "input"),
        (13, "SDA", "bidirectional"),
        (14, "NC", "no_connect"),
    ]
    pins_right = [
        (28, "GPA7", "bidirectional"),
        (27, "GPA6", "bidirectional"),
        (26, "GPA5", "bidirectional"),
        (25, "GPA4", "bidirectional"),
        (24, "GPA3", "bidirectional"),
        (23, "GPA2", "bidirectional"),
        (22, "GPA1", "bidirectional"),
        (21, "GPA0", "bidirectional"),
        (20, "INTA", "output"),
        (19, "INTB", "output"),
        (18, "~RESET", "input"),
        (17, "A2", "input"),
        (16, "A1", "input"),
        (15, "A0", "input"),
    ]
    y_top = 16.51  # (14-1)*2.54/2 = 16.51
    rect_top = y_top + 2.54
    rect_bot = -y_top - 2.54
    out = ['    (symbol "MCU:MCP23017" (in_bom yes) (on_board yes)']
    out.append(f'      (property "Reference" "U" (at 0 {rect_top + 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append(f'      (property "Value" "MCP23017-E_SS" (at 0 {rect_bot - 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "https://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "MCU:MCP23017_0_1"')
    out.append(f'        (rectangle (start -10.16 {rect_top}) (end 10.16 {rect_bot})')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append('      (symbol "MCU:MCP23017_1_1"')
    for idx, (num, name, ptype) in enumerate(pins_left):
        ly = y_top - idx * 2.54
        out.append(
            f'        (pin {ptype} line (at -12.7 {ly:.3f} 0) (length 2.54)\n'
            f'          (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{num}" (effects (font (size 1.27 1.27)))))'
        )
    for idx, (num, name, ptype) in enumerate(pins_right):
        ly = y_top - idx * 2.54
        out.append(
            f'        (pin {ptype} line (at 12.7 {ly:.3f} 180) (length 2.54)\n'
            f'          (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{num}" (effects (font (size 1.27 1.27)))))'
        )
    out.append('        )')
    out.append('      )')
    return "\n".join(out)


LIB_SYMBOLS = (
    LIB_SYMBOLS
    + "\n" + _pico2_lib_symbol()
    + "\n" + _conn_01xN_lib_symbol(16)
    + "\n" + _mcp23017_lib_symbol()
)


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
    sheet_uuid_seed: str = "sheet_power_tree",
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
        f"        (path \"/{det_uuid(sheet_uuid_seed)}\" (reference \"{ref}\") (unit 1)))))\n"
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
# KiCad-Achsen-Konvention Helper:
# lib_symbol-Pins nutzen Y-UP (math), Schematic-Placement nutzt Y-DOWN (screen).
# Beim Platzieren: abs_pin_y = sym_y - local_pin_y (bei rotation=0).
# Beim Rotation: erst Local-Pin rotieren, dann (Y-Komponente negieren, dann sym addieren).
# Wir kapseln die Konversion in pin_abs() um Bug-Quellen zu eliminieren.
# ----------------------------------------------------------------------------


def pin_abs(sym_x: float, sym_y: float, local_x: float, local_y: float, rotation: int = 0) -> tuple[float, float]:
    """Berechne absolute Pin-Position für einen Pin mit local (lx, ly) am Symbol (sx, sy, rot)."""
    import math
    rad = math.radians(rotation)
    cos_r, sin_r = math.cos(rad), math.sin(rad)
    # KiCad symbol rotation is CCW in lib-coord (Y-up). Rotated local:
    rx = local_x * cos_r - local_y * sin_r
    ry = local_x * sin_r + local_y * cos_r
    return (sym_x + rx, sym_y - ry)


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
    """Sheet 1: Power Tree per SPEC v0.6 §3.

    Y-DOWN convention: kleinere y = oben auf Schirm. lib_symbol-Pins werden
    via abs_y = sym_y - local_y konvertiert. USB-C-Pin-Positionen sind als
    explizite Konstanten dokumentiert um die Y-Achsen-Flip nie wieder zu
    verlieren.
    """
    sheet_uuid = det_uuid("sheet_power_tree")
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    # USB-C J1 platziert @ (50, 80). Body-rect spannt y=67.30..102.86 absolut.
    # Pin-Tabelle (alle absolut, berechnet via pin_abs(50, 80, local_x, local_y)):
    #   left-VBUS:  A1 y=69.84 | A4 y=72.38 | B4 y=74.92 | B1 y=77.46
    #   left-GND:   A12 y=82.54 | A9 y=85.08 | B9 y=87.62 | B12 y=90.16
    #   right CC:   A5 (CC1) y=72.38 | B5 (CC2) y=74.92
    #   right D±:   A6 (D+) y=77.46 | A7 (D-) y=80.0 | B6 (D+) y=82.54 | B7 (D-) y=85.08
    #   right SBU:  A8 y=87.62 | B8 y=90.16
    #   shield S1:  abs (50, 105.4)
    J1_X, J1_Y = 50.0, 80.0
    VBUS_PINS_Y = [69.84, 72.38, 74.92, 77.46]  # A1, A4, B4, B1
    GND_PINS_Y = [82.54, 85.08, 87.62, 90.16]   # A12, A9, B9, B12
    A5_CC1_Y, B5_CC2_Y = 72.38, 74.92
    A6_DP_Y, A7_DN_Y, B6_DP_Y, B7_DN_Y = 77.46, 80.0, 82.54, 85.08
    A8_SBU1_Y, B8_SBU2_Y = 87.62, 90.16
    S1_SHIELD_X, S1_SHIELD_Y = 50.0, 105.4
    # Rail position (+5V_USB) oberhalb des USB-C-Body, also kleinere y
    RAIL_Y = 60.0
    GND_LABEL_Y = 96.0  # GND-Bus unterhalb des USB-C-Body

    symbols.append(
        place_symbol(
            lib_id="Connector:USB_C_Receptacle",
            ref="J1",
            value="USB_C_Receptacle (TYPE-C-31-M-12, C165948)",
            x=J1_X,
            y=J1_Y,
            footprint="Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12",
            datasheet="https://datasheet.lcsc.com/lcsc/2004081102_Korean-Hroparts-Elec-TYPE-C-31-M-12_C165948.pdf",
            extra_props={"MPN": "TYPE-C-31-M-12", "LCSC": "C165948"},
            seed_suffix="J1",
        )
    )
    # ---- VBUS-Trunk: 4 Stubs (A1, A4, B4, B1) → Trunk x=30.48 → F1 → Rail
    for py in VBUS_PINS_Y:
        wires.append(wire(34.76, py, 30.48, py, seed_suffix=f"vbus-stub-{py}"))
    # Trunk segmentiert (Analyzer folgt Wire-Endpoints, nicht Mid-Points)
    for ya, yb in zip(VBUS_PINS_Y, VBUS_PINS_Y[1:]):
        wires.append(wire(30.48, ya, 30.48, yb, seed_suffix=f"vbus-trunk-{ya}-{yb}"))
    # Junctions an allen Trunk-Stub-Schnittpunkten (außer den zwei End-Pins die nur 2 Wires haben)
    for py in VBUS_PINS_Y[1:-1]:
        junctions.append(junction(30.48, py))
    # Trunk-Top (kleinste y) → vertikal hoch zur Rail-Höhe
    vbus_top = VBUS_PINS_Y[0]  # 69.84
    wires.append(wire(30.48, vbus_top, 30.48, RAIL_Y, seed_suffix="vbus-up"))
    # Junction am Trunk-Top (3-Wege: stub A1 + trunk-end + vbus-up)
    junctions.append(junction(30.48, vbus_top))

    # ---- GND-Trunk: 4 Stubs (A12, A9, B9, B12) → Trunk x=30.48 → unten zu GND-Bus
    for py in GND_PINS_Y:
        wires.append(wire(34.76, py, 30.48, py, seed_suffix=f"gnd-stub-{py}"))
    for ya, yb in zip(GND_PINS_Y, GND_PINS_Y[1:]):
        wires.append(wire(30.48, ya, 30.48, yb, seed_suffix=f"gnd-trunk-{ya}-{yb}"))
    for py in GND_PINS_Y[1:-1]:
        junctions.append(junction(30.48, py))
    gnd_bottom = GND_PINS_Y[-1]  # 90.16
    wires.append(wire(30.48, gnd_bottom, 30.48, GND_LABEL_Y, seed_suffix="gnd-down"))
    junctions.append(junction(30.48, gnd_bottom))
    # GND Power-Symbol unten am Trunk
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_USBCGND",
            value="GND",
            x=30.48,
            y=GND_LABEL_Y,
            seed_suffix="usbc-gnd",
        )
    )

    # ---- F1 Polyfuse auf der Rail @ (45, 60) horizontal.
    # Polyfuse pin1 (local -3.81, 0) → abs (41.19, 60). pin2 (local +3.81, 0) → abs (48.81, 60).
    symbols.append(
        place_symbol(
            lib_id="Device:Polyfuse",
            ref="F1",
            value="2A/4A 1812 (MF-MSMF200, C210837)",
            x=45,
            y=RAIL_Y,
            footprint="Fuse:Fuse_1812_4532Metric",
            datasheet="https://www.bourns.com/docs/Product-Datasheets/mfmsmf.pdf",
            extra_props={"MPN": "MF-MSMF200", "LCSC": "C210837"},
            seed_suffix="F1",
        )
    )
    # VBUS-up endpoint (30.48, RAIL_Y) → F1 pin1 (41.19, RAIL_Y) als horizontaler Rail-Anfang
    wires.append(wire(30.48, RAIL_Y, 41.19, RAIL_Y, seed_suffix="rail-pre-f1"))

    # ---- +5V_USB Power-Flag links von F1 auf der Rail (Y-DOWN: Pin oberhalb Symbol-Origin)
    # Power:VBUS hat pin at (0, 0) local, mit Arrow-Vertices bei local +2.54y.
    # Bei Y-DOWN: pin-anchor abs = (sx, sy - 0) = sy. Arrow zeigt nach oben (kleinere y).
    # Wir platzieren das Symbol ON THE RAIL (sym_y=RAIL_Y). Pin sitzt direkt auf Rail.
    symbols.append(
        place_symbol(
            lib_id="Power:VBUS",
            ref="#PWR_VBUS_FLAG",
            value="+5V_USB",
            x=35,
            y=RAIL_Y,
            seed_suffix="vbus-flag",
        )
    )
    # Rail-Segment durch die Flag-Position: F1 erweitert über (35, RAIL_Y)
    junctions.append(junction(35, RAIL_Y))
    # (no separate stub: Rail-pre-f1 wire from 30.48 to 41.19 passes through x=35)
    # But analyzer requires segmented wires! Replace single rail-pre-f1 wire with two segments.
    wires.pop()  # remove rail-pre-f1
    wires.append(wire(30.48, RAIL_Y, 35, RAIL_Y, seed_suffix="rail-30-35"))
    wires.append(wire(35, RAIL_Y, 41.19, RAIL_Y, seed_suffix="rail-35-f1"))

    # ---- F1 pin2 → Rail nach rechts (48.81 → 110)
    # CBULK an der Rail @ (65, sy), D2 TVS @ (55, sy)
    # Rail-x-Positionen mit Junctions: 48.81, 55, 65, 80, 110
    rail_xs = [48.81, 55, 65, 80, 110]
    for xa, xb in zip(rail_xs, rail_xs[1:]):
        wires.append(wire(xa, RAIL_Y, xb, RAIL_Y, seed_suffix=f"rail-{xa}-{xb}"))
    # Junctions an allen Stützen außer Endpunkten
    for x in rail_xs[1:-1]:
        junctions.append(junction(x, RAIL_Y))

    # ---- C_BULK 1000µF polarized: pin1 (+) muss zur +5V_USB Rail
    # Device:CP lib: pin 1 at local (0, +3.81), pin 2 at local (0, -3.81).
    # Y-DOWN: pin1 abs = (sx, sy - 3.81). Wir wollen pin1 (+) auf Rail (y=RAIL_Y=60).
    # → sy - 3.81 = 60 → sy = 63.81. C_BULK center @ (65, 63.81).
    # pin1 (+) abs (65, 60), pin2 (-) abs (65, 67.62).
    symbols.append(
        place_symbol(
            lib_id="Device:CP",
            ref="C_BULK",
            value="1000uF 10V SMD Elko",
            x=65,
            y=63.81,
            footprint="Capacitor_SMD:CP_Elec_8x6.7",
            extra_props={"MPN": "EEE-FK1A102P (Panasonic)", "LCSC": "TBD"},
            seed_suffix="CBULK",
        )
    )
    # pin1 (+) (65, 60) sits exactly on rail → connection via rail-segment
    # pin2 (-) (65, 67.62) → GND via wire down
    wires.append(wire(65, 67.62, 65, GND_LABEL_Y, seed_suffix="cbulk-neg-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_CBULK",
            value="GND",
            x=65,
            y=GND_LABEL_Y,
            seed_suffix="cbulk-gnd",
        )
    )

    # ---- D2 TVS auf Rail. Device:D_TVS pin 1 (K) at local (-3.81, 0), pin 2 (A) at local (+3.81, 0).
    # Bei rotation=270 (= -90 CCW = 90 CW): (lx, ly) → (ly, -lx). Pin 1 (-3.81, 0) → (0, 3.81).
    # Abs pin 1 = (sx + 0, sy - 3.81). Wir wollen K auf Rail (y=60) → sy = 63.81.
    # pin 2 (A) abs = (sx, sy + 3.81) = (55, 67.62). → GND.
    symbols.append(
        place_symbol(
            lib_id="Device:D_TVS",
            ref="D2",
            value="SMAJ5.0A TVS (C113952)",
            x=55,
            y=63.81,
            rotation=270,
            footprint="Diode_SMD:D_SMA",
            extra_props={"MPN": "SMAJ5.0A", "LCSC": "C113952"},
            seed_suffix="D2",
        )
    )
    wires.append(wire(55, 67.62, 55, GND_LABEL_Y, seed_suffix="d2-a-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_D2",
            value="GND",
            x=55,
            y=GND_LABEL_Y,
            seed_suffix="d2-gnd",
        )
    )

    # ---- R2 CC1-Pulldown: A5 (CC1) abs (65.24, 72.38) → R2 → GND
    # Device:R rotation=90: pin1 (0, 3.81) → (-3.81, 0). Abs pin1 = (sx - 3.81, sy).
    # Wir wollen pin1 auf x=65.24 (A5-Stub-Ende), y=72.38 → sx = 69.05, sy = 72.38. Hmm
    # Aber pin2 (0, -3.81) rotated → (3.81, 0). pin2 abs = (sx + 3.81, sy).
    # Wir wollen pin2 nach rechts ins GND-Label. → R2 center @ (75, 72.38) gibt pin1 (71.19, 72.38), pin2 (78.81, 72.38).
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R2",
            value="5.1k 0603 (CC1 pull-down)",
            x=75,
            y=A5_CC1_Y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-075K1L", "LCSC": "C23186"},
            seed_suffix="R2",
        )
    )
    wires.append(wire(65.24, A5_CC1_Y, 71.19, A5_CC1_Y, seed_suffix="a5-to-r2"))
    wires.append(wire(78.81, A5_CC1_Y, 82, A5_CC1_Y, seed_suffix="r2-to-gnd-stub"))
    labels.append(label(82, A5_CC1_Y, "GND"))

    # ---- R3 CC2-Pulldown
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R3",
            value="5.1k 0603 (CC2 pull-down)",
            x=75,
            y=B5_CC2_Y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-075K1L", "LCSC": "C23186"},
            seed_suffix="R3",
        )
    )
    wires.append(wire(65.24, B5_CC2_Y, 71.19, B5_CC2_Y, seed_suffix="b5-to-r3"))
    wires.append(wire(78.81, B5_CC2_Y, 82, B5_CC2_Y, seed_suffix="r3-to-gnd-stub"))
    labels.append(label(82, B5_CC2_Y, "GND"))

    # ---- D+ / D- Bridging via Labels (USB-C dual-orientation hat doppelte D+/D-)
    # D+ A6 @ y=77.46 und D+ B6 @ y=82.54 sollen identisches USB_DP-Netz haben.
    # D- A7 @ y=80.0 und D- B7 @ y=85.08 sollen identisches USB_DN-Netz haben.
    wires.append(wire(65.24, A6_DP_Y, 70, A6_DP_Y, seed_suffix="a6-out"))
    labels.append(label(70, A6_DP_Y, "USB_DP"))
    wires.append(wire(65.24, B6_DP_Y, 70, B6_DP_Y, seed_suffix="b6-out"))
    labels.append(label(70, B6_DP_Y, "USB_DP"))
    wires.append(wire(65.24, A7_DN_Y, 70, A7_DN_Y, seed_suffix="a7-out"))
    labels.append(label(70, A7_DN_Y, "USB_DN"))
    wires.append(wire(65.24, B7_DN_Y, 70, B7_DN_Y, seed_suffix="b7-out"))
    labels.append(label(70, B7_DN_Y, "USB_DN"))

    # ---- SBU1/SBU2 als NC mit Labels (verhindert ERC-dangling-wire-Warning)
    wires.append(wire(65.24, A8_SBU1_Y, 70, A8_SBU1_Y, seed_suffix="a8-nc"))
    labels.append(label(70, A8_SBU1_Y, "NC_SBU1"))
    wires.append(wire(65.24, B8_SBU2_Y, 70, B8_SBU2_Y, seed_suffix="b8-nc"))
    labels.append(label(70, B8_SBU2_Y, "NC_SBU2"))

    # ---- USBLC6-2SC6 D1 @ (100, 80) — ESD-Schutz auf D+/D-
    # Pin-Positionen abs (sym 100,80):
    #   pin 1 I/O1 local (-7.62, +2.54) → abs (92.38, 77.46)  [identisch zu A6 D+!]
    #   pin 2 GND  local (-7.62, 0)    → abs (92.38, 80.0)
    #   pin 3 I/O2 local (-7.62, -2.54) → abs (92.38, 82.54)
    #   pin 4 I/O2 local (+7.62, -2.54) → abs (107.62, 82.54)  [downstream]
    #   pin 5 VBUS local (+7.62, 0)    → abs (107.62, 80.0)
    #   pin 6 I/O1 local (+7.62, +2.54) → abs (107.62, 77.46)  [downstream]
    #
    # Topologie: I/O1 = D+ (USB upstream → downstream). I/O2 = D-.
    # Pin 1 (upstream D+) ← Label USB_DP. Pin 6 (downstream D+) → PICO_USB_DP hier_label.
    # Pin 3 (upstream D-) ← Label USB_DN. Pin 4 (downstream D-) → PICO_USB_DN hier_label.
    # Pin 2 GND. Pin 5 → +5V_USB Rail-Anschluss.
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
    # Pin 1 (I/O1 upstream D+) @ y=77.46 — label-bridge zu USB_DP
    wires.append(wire(92.38, 77.46, 88, 77.46, seed_suffix="d1-p1-stub"))
    labels.append(label(88, 77.46, "USB_DP"))
    # Pin 3 (I/O2 upstream D-) @ y=82.54 — label-bridge zu USB_DN
    wires.append(wire(92.38, 82.54, 88, 82.54, seed_suffix="d1-p3-stub"))
    labels.append(label(88, 82.54, "USB_DN"))
    # Pin 2 (GND) @ y=80 — Power-Symbol direkt
    wires.append(wire(92.38, 80, 88, 80, seed_suffix="d1-gnd-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_D1",
            value="GND",
            x=88,
            y=80,
            rotation=90,  # Rotate so symbol body extends to the left of pin
            seed_suffix="d1-gnd",
        )
    )
    # Pin 5 (VBUS, +5V Reference für ESD) @ y=80 — Connect to +5V_USB rail at top
    wires.append(wire(107.62, 80, 110, 80, seed_suffix="d1-vbus-stub"))
    wires.append(wire(110, 80, 110, RAIL_Y, seed_suffix="d1-vbus-up"))
    junctions.append(junction(110, RAIL_Y))
    # Pin 6 (I/O1 downstream D+) @ y=77.46 → PICO_USB_DP hier_label
    wires.append(wire(107.62, 77.46, 115, 77.46, seed_suffix="d1-p6-stub"))
    hlabels.append(hier_label(115, 77.46, "PICO_USB_DP", shape="output", rotation=0))
    # Pin 4 (I/O2 downstream D-) @ y=82.54 → PICO_USB_DN hier_label
    wires.append(wire(107.62, 82.54, 115, 82.54, seed_suffix="d1-p4-stub"))
    hlabels.append(hier_label(115, 82.54, "PICO_USB_DN", shape="output", rotation=0))

    # ---- USB-C Shield S1 (local 0, -25.4 angle 90) → abs (50, 105.4)
    # Y-DOWN: S1 sits below USB-C body. Connect to GND-Bus.
    wires.append(wire(50, 105.4, 50, GND_LABEL_Y + 5, seed_suffix="shield-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_SHIELD",
            value="GND",
            x=50,
            y=GND_LABEL_Y + 5,
            seed_suffix="shield-gnd",
        )
    )

    # ---- Hierarchical outputs nach rechts
    # +5V_OUT auf der Rail (y=RAIL_Y, x=110)
    hlabels.append(hier_label(115, RAIL_Y, "+5V_OUT", shape="output", rotation=0))
    wires.append(wire(110, RAIL_Y, 115, RAIL_Y, seed_suffix="rail-to-hlbl"))
    # GND_OUT — vom GND-Bus
    wires.append(wire(30.48, GND_LABEL_Y, 115, GND_LABEL_Y, seed_suffix="gnd-bus"))
    hlabels.append(hier_label(115, GND_LABEL_Y, "GND_OUT", shape="passive", rotation=0))

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
# Sheet 2 — Pico 2 (RP2350) + SWD + Reset + BOOTSEL + Status-LED
# Pin-Allocation per SPEC v0.6 §5. Inputs: +5V_IN, GND, PICO_USB_DP/DN.
# Outputs: +3V3_OUT (Pico SMPS) + alle GP-Funktionsleitungen zu Sheets 3-6.
# ----------------------------------------------------------------------------


def pico_sheet() -> str:
    """Sheet 2: Pico 2 (RP2350) per SPEC v0.6 §5.

    Y-DOWN convention durchgängig. Pin 1 oben (abs y=75.87), Pin 20 unten
    (abs y=124.13). Symbol bei (100, 100), Pin-Anchors links bei x=82.46,
    rechts bei x=117.54.
    """
    sheet_uuid = det_uuid("sheet_pico")
    sus = "sheet_pico"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    SYM_X, SYM_Y = 100.0, 100.0
    PIN_L_X = 82.46  # Pin-Anchor x links
    PIN_R_X = 117.54  # Pin-Anchor x rechts

    def pico_left_pin_y(pin: int) -> float:
        """Pin 1 oben (abs y=75.87), Pin 20 unten (abs y=124.13)."""
        return 75.87 + (pin - 1) * 2.54

    def pico_right_pin_y(pin: int) -> float:
        """Pin 40 oben (abs y=75.87), Pin 21 unten (abs y=124.13)."""
        return 75.87 + (40 - pin) * 2.54

    # ---- U1 Pico 2 platzieren
    symbols.append(
        place_symbol(
            lib_id="MCU:Pico2",
            ref="U1",
            value="Raspberry_Pi_Pico2 (RP2350)",
            x=SYM_X,
            y=SYM_Y,
            footprint="MCU_Module:RPi_Pico_SMD_TH",
            datasheet="https://datasheets.raspberrypi.com/pico/pico-2-datasheet.pdf",
            extra_props={
                "MPN": "SC1631 (Pico 2 module)",
                "LCSC": "TBD (separat bestellen, JLC stockt Pico-Module nicht)",
            },
            seed_suffix="U1",
            sheet_uuid_seed=sus,
        )
    )

    # ---- GND-Stub + Power-Symbol für einen Pico-GND-Pin
    def attach_gnd(pin: int, side: str, ref_seed: str) -> None:
        if side == "left":
            py = pico_left_pin_y(pin)
            sym_x = PIN_L_X - 5.0
            wires.append(wire(PIN_L_X, py, sym_x, py, seed_suffix=f"u1-gnd-l-{pin}"))
        else:
            py = pico_right_pin_y(pin)
            sym_x = PIN_R_X + 5.0
            wires.append(wire(PIN_R_X, py, sym_x, py, seed_suffix=f"u1-gnd-r-{pin}"))
        symbols.append(
            place_symbol(
                lib_id="Power:GND",
                ref=f"#PWR_U1_GND_{ref_seed}",
                value="GND",
                x=sym_x,
                y=py,
                rotation=90 if side == "left" else 270,
                seed_suffix=f"u1-gnd-{ref_seed}",
                sheet_uuid_seed=sus,
            )
        )

    for pin in (3, 8, 13, 18):
        attach_gnd(pin, "left", f"L{pin}")
    for pin in (23, 28, 33, 38):
        attach_gnd(pin, "right", f"R{pin}")

    # ---- VBUS (Pin 40, y=75.87) ← +5V_IN hierarchical input
    p40_y = pico_right_pin_y(40)
    symbols.append(
        place_symbol(
            lib_id="Power:+5V",
            ref="#PWR_PICO_VBUS",
            value="+5V",
            x=125,
            y=p40_y,
            rotation=270,
            seed_suffix="pico-vbus-flag",
            sheet_uuid_seed=sus,
        )
    )
    junctions.append(junction(125, p40_y))
    junctions.append(junction(126, p40_y))
    # Segmentiertes VBUS-Wire mit Junctions an x=125 (Flag) und x=126 (3V3_EN-Trunk)
    wires.append(wire(PIN_R_X, p40_y, 125, p40_y, seed_suffix="u1-vbus-seg-1"))
    wires.append(wire(125, p40_y, 126, p40_y, seed_suffix="u1-vbus-seg-2"))
    wires.append(wire(126, p40_y, 130, p40_y, seed_suffix="u1-vbus-seg-3"))
    hlabels.append(hier_label(130, p40_y, "+5V_IN", shape="input", rotation=180))

    # ---- VSYS (Pin 39, y=78.41) — label PICO_VSYS für optional J5 0Ω-Bridge zu +5V
    p39_y = pico_right_pin_y(39)
    wires.append(wire(PIN_R_X, p39_y, 122, p39_y, seed_suffix="u1-vsys-stub"))
    labels.append(label(122, p39_y, "PICO_VSYS"))

    # ---- 3V3_EN (Pin 37, y=83.49) → tied to VBUS via Trunk bei x=126
    p37_y = pico_right_pin_y(37)
    wires.append(wire(PIN_R_X, p37_y, 126, p37_y, seed_suffix="u1-3v3en-stub"))
    wires.append(wire(126, p37_y, 126, p40_y, seed_suffix="u1-3v3en-trunk"))

    # ---- 3V3_OUT (Pin 36, y=86.03) — Pico SMPS → +3V3 power flag + hier_label
    p36_y = pico_right_pin_y(36)
    symbols.append(
        place_symbol(
            lib_id="Power:+3V3",
            ref="#PWR_PICO_3V3",
            value="+3V3",
            x=124,
            y=p36_y,
            rotation=270,
            seed_suffix="pico-3v3-flag",
            sheet_uuid_seed=sus,
        )
    )
    junctions.append(junction(124, p36_y))
    wires.append(wire(PIN_R_X, p36_y, 124, p36_y, seed_suffix="u1-3v3out-seg-1"))
    wires.append(wire(124, p36_y, 130, p36_y, seed_suffix="u1-3v3out-seg-2"))
    hlabels.append(hier_label(130, p36_y, "+3V3_OUT", shape="output", rotation=180))

    # ---- Decoupling C3 (10µF) + C4 (100nF) auf +3V3 (unter dem 3V3-Pin in Y-DOWN, also größere y)
    # Device:C lib: pin1 (0, +3.81) abs (sx, sy-3.81). pin2 (0, -3.81) abs (sx, sy+3.81).
    # Wir wollen pin1 auf +3V3 rail (y=p36_y=86.03) → sy = 89.84. pin2 abs (sx, 93.65).
    c_y = 89.84
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C3",
            value="10uF X5R 0805 (3V3 decoupling)",
            x=135,
            y=c_y,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A106KOQNNNE", "LCSC": "C15850"},
            seed_suffix="C3",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(130, p36_y, 135, p36_y, seed_suffix="c3-to-rail"))
    junctions.append(junction(130, p36_y))
    wires.append(wire(135, 93.65, 135, 96, seed_suffix="c3-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_C3",
            value="GND",
            x=135,
            y=96,
            seed_suffix="c3-gnd",
            sheet_uuid_seed=sus,
        )
    )

    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C4",
            value="100nF X7R 0603 (3V3 decoupling)",
            x=140,
            y=c_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="C4",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(135, p36_y, 140, p36_y, seed_suffix="c4-to-rail"))
    junctions.append(junction(135, p36_y))
    wires.append(wire(140, 93.65, 140, 96, seed_suffix="c4-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_C4",
            value="GND",
            x=140,
            y=96,
            seed_suffix="c4-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- RUN (Pin 30, y=101.27) — Reset via SW11 + Pull-up R_RUN auf +3V3
    p30_y = pico_right_pin_y(30)
    labels.append(label(125, p30_y, "RUN"))
    # R_RUN rotation=0 vertikal: pin1 (top) abs (sx, sy-3.81), pin2 (bottom) abs (sx, sy+3.81).
    # pin2 (bottom) auf RUN-Linie (y=p30_y=101.27) → sy = 97.46. pin1 (top) abs (130, 93.65) → +3V3 label.
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_RUN",
            value="10k 0603 (RUN pull-up)",
            x=130,
            y=97.46,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
            seed_suffix="RRUN",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(130, 93.65, 130, 91, seed_suffix="rrun-up"))
    labels.append(label(130, 91, "+3V3"))
    junctions.append(junction(130, p30_y))
    # SW11 (Reset) at (137, p30_y). pin1 abs (131.92, p30_y), pin2 abs (142.08, p30_y).
    symbols.append(
        place_symbol(
            lib_id="Switch:SW_Push",
            ref="SW11",
            value="Reset 6mm SMD",
            x=137,
            y=p30_y,
            footprint="Button_Switch_SMD:SW_SPST_TL3342",
            extra_props={"MPN": "TL3342F160QG", "LCSC": "C720477"},
            seed_suffix="SW11",
            sheet_uuid_seed=sus,
        )
    )
    # RUN-Wire segmentiert: PIN_R_X → 125 (label) → 130 (junction R_RUN) → 131.92 (SW11 pin1)
    wires.append(wire(PIN_R_X, p30_y, 125, p30_y, seed_suffix="run-seg-1"))
    wires.append(wire(125, p30_y, 130, p30_y, seed_suffix="run-seg-2"))
    wires.append(wire(130, p30_y, 131.92, p30_y, seed_suffix="run-seg-3"))
    wires.append(wire(142.08, p30_y, 145, p30_y, seed_suffix="sw11-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_SW11",
            value="GND",
            x=145,
            y=p30_y,
            rotation=270,
            seed_suffix="sw11-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- GP26 (Pin 31, y=98.73) → STATUS_LED via R19 (820Ω) + LED1 → GND
    p31_y = pico_right_pin_y(31)
    labels.append(label(120, p31_y, "STATUS_LED"))
    # R19 rotation=90 horizontal: pin1 abs (sx-3.81, sy), pin2 abs (sx+3.81, sy).
    # Wir wollen pin1 nahe Pico-pin31 (PIN_R_X=117.54). sx = 121.35 → pin1 (117.54, p31_y), pin2 (125.16, p31_y).
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R19",
            value="820R 0603 (LED limit)",
            x=121.35,
            y=p31_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-07820RL", "LCSC": "C23253"},
            seed_suffix="R19",
            sheet_uuid_seed=sus,
        )
    )
    # LED rotation=180: pin1 (K) abs (sx+3.81, sy), pin2 (A) abs (sx-3.81, sy).
    # Anode (pin2) faces left (von R19), Cathode (pin1) faces right (zu GND).
    # R19 pin2 at (125.16, p31_y). LED pin2 at sx-3.81. → sx = 128.97. pin1 abs (132.78, p31_y).
    symbols.append(
        place_symbol(
            lib_id="Device:LED",
            ref="LED1",
            value="warm white 0805 STATUS",
            x=128.97,
            y=p31_y,
            rotation=180,
            footprint="LED_SMD:LED_0805_2012Metric",
            extra_props={"MPN": "Generic warm-white 2700K", "LCSC": "TBD"},
            seed_suffix="LED1",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(125.16, p31_y, 125.16, p31_y, seed_suffix="r19-led-stub"))  # touching wire
    wires.append(wire(132.78, p31_y, 135, p31_y, seed_suffix="led-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_LED1",
            value="GND",
            x=135,
            y=p31_y,
            rotation=270,
            seed_suffix="led1-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- SWD-Header J4 (3-pin)
    symbols.append(
        place_symbol(
            lib_id="Connector:Conn_01x03",
            ref="J4",
            value="SWD 1.27mm (SWCLK/GND/SWDIO)",
            x=145,
            y=115,
            footprint="Connector_PinHeader_1.27mm:PinHeader_1x03_P1.27mm_Vertical",
            extra_props={"MPN": "TC2030-IDC", "LCSC": "TBD"},
            seed_suffix="J4",
            sheet_uuid_seed=sus,
        )
    )
    # Conn_01x03 pin local (3.81, +2.54), (3.81, 0), (3.81, -2.54) at angle 180.
    # Y-DOWN abs: pin1 (sx+3.81, sy-2.54), pin2 (sx+3.81, sy), pin3 (sx+3.81, sy+2.54).
    # sym (145, 115): pin1 (148.81, 112.46), pin2 (148.81, 115), pin3 (148.81, 117.54).
    wires.append(wire(148.81, 112.46, 152, 112.46, seed_suffix="j4-p1-swclk"))
    labels.append(label(152, 112.46, "SWCLK"))
    wires.append(wire(148.81, 115, 152, 115, seed_suffix="j4-p2-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_J4",
            value="GND",
            x=152,
            y=115,
            rotation=270,
            seed_suffix="j4-gnd",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(148.81, 117.54, 152, 117.54, seed_suffix="j4-p3-swdio"))
    labels.append(label(152, 117.54, "SWDIO"))

    # ---- SW12 BOOTSEL (DNP für THT-Pico-Variante)
    symbols.append(
        place_symbol(
            lib_id="Switch:SW_Push",
            ref="SW12",
            value="BOOTSEL 6mm SMD (DNP for THT-Pico)",
            x=137,
            y=70,
            footprint="Button_Switch_SMD:SW_SPST_TL3342",
            extra_props={"MPN": "TL3342F160QG", "LCSC": "C720477", "DNP": "true (THT-Pico)"},
            seed_suffix="SW12",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(131.92, 70, 128, 70, seed_suffix="sw12-to-label"))
    labels.append(label(128, 70, "PICO_BOOTSEL"))
    wires.append(wire(142.08, 70, 145, 70, seed_suffix="sw12-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_SW12",
            value="GND",
            x=145,
            y=70,
            rotation=270,
            seed_suffix="sw12-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- USB-Daten von Sheet 1 → Hier-Labels (Pico USB-Pads via TP2/TP3 verbunden in PCB-Layout)
    hlabels.append(hier_label(75, 65, "PICO_USB_DP", shape="input", rotation=0))
    hlabels.append(hier_label(75, 68, "PICO_USB_DN", shape="input", rotation=0))

    # ---- Funktionale GP-Pins → Hierarchical Outputs per SPEC v0.6 §5
    left_signals = {
        1: "UART0_TX",     # GP0
        2: "UART0_RX",     # GP1 (downstream R1 1k series in Sheet 7)
        4: "I2C_SDA",      # GP2
        5: "I2C_SCL",      # GP3
        6: "OLED_MISO_NC", # GP4 (unused)
        7: "OLED_CS",      # GP5
        9: "OLED_SCK",     # GP6
        10: "OLED_MOSI",   # GP7
        11: "OLED_DC",     # GP8
        12: "OLED_RES",    # GP9
        14: "DRIVE_A",     # GP10
        15: "DRIVE_B",     # GP11
        16: "DRIVE_SW",    # GP12
        17: "BRIGHT_A",    # GP13
        19: "BRIGHT_B",    # GP14
        20: "BRIGHT_SW",   # GP15
    }
    for pnum, netname in left_signals.items():
        py = pico_left_pin_y(pnum)
        wires.append(wire(PIN_L_X, py, 70, py, seed_suffix=f"u1-left-{pnum}"))
        hlabels.append(hier_label(70, py, netname, shape="output", rotation=0))

    right_signals = {
        21: "DISPLAY_A",   # GP16
        22: "DISPLAY_B",   # GP17
        24: "DISPLAY_SW",  # GP18
        25: "VOL_A",       # GP19
        26: "VOL_B",       # GP20
        27: "VOL_SW",      # GP21
        29: "MCP_INT",     # GP22
        32: "AMP_SHUTDOWN",# GP27
        34: "AMP_MUTE",    # GP28
        35: "ADC_VREF_NC", # ADC_VREF — DNP / tie via 47Ω ferrite externally if used
    }
    for pnum, netname in right_signals.items():
        py = pico_right_pin_y(pnum)
        wires.append(wire(PIN_R_X, py, 162, py, seed_suffix=f"u1-right-{pnum}"))
        hlabels.append(hier_label(162, py, netname, shape="output", rotation=180))

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 2: Pico 2 (RP2350)")\n'
        f'    (date "2026-05-11")\n'
        f'    (rev "0.6")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6 §5")\n'
        f'    (comment 2 "USB-C D+/- via TP2/TP3 (BOOTSEL drag-drop)")\n'
        f'    (comment 3 "+3V3_OUT (Pico SMPS Pin 36) speist OLED VDDIO + MCP23017 + Pull-Ups")\n'
        f'    (comment 4 "SW12 BOOTSEL DNP für THT-Pico-Variante"))\n'
        "  (lib_symbols\n"
        + LIB_SYMBOLS
        + "\n  )\n"
        + "".join(symbols)
        + "".join(wires)
        + "".join(junctions)
        + "".join(labels)
        + "".join(hlabels)
        + f'  (sheet_instances\n    (path "/" (page "2")))\n'
        ")\n"
    )
    return body


# ----------------------------------------------------------------------------
# Sheet 3 — OLED Header (ER-OLEDM032-1W 256×64 SSD1322) per SPEC v0.6 §6
# 16-pin Header J3 + VDD/VBAT Decoupling. BS0=BS1=GND für 4-wire SPI mode.
# Inputs: GND, +5V, +3V3, OLED_SCK/MOSI/CS/DC/RES (von Sheet 2 Pico).
# ----------------------------------------------------------------------------


def oled_sheet() -> str:
    sheet_uuid = det_uuid("sheet_oled")
    sus = "sheet_oled"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    # J3 16-pin header @ (100, 100). Pin local x=+3.81 angle 180 → abs x=103.81.
    # Pin 1 (top, local y=+19.05) abs y = 100 - 19.05 = 80.95.
    # Pin 16 (bottom, local y=-19.05) abs y = 119.05.
    # Pin spacing 2.54mm: pin N abs y = 80.95 + (N-1)*2.54.
    J3_X, J3_Y = 100.0, 100.0
    PIN_X = 103.81

    def oled_pin_y(pin: int) -> float:
        return 80.95 + (pin - 1) * 2.54

    symbols.append(
        place_symbol(
            lib_id="Connector:Conn_01x16",
            ref="J3",
            value="OLED-Header 2.54mm (ER-OLEDM032-1W 256x64 SSD1322)",
            x=J3_X,
            y=J3_Y,
            footprint="Connector_PinHeader_2.54mm:PinHeader_1x16_P2.54mm_Vertical",
            datasheet="https://www.buydisplay.com/download/manual/ER-OLEDM032-1_Series_Datasheet.pdf",
            extra_props={
                "MPN": "ER-OLEDM032-1W (Buydisplay)",
                "LCSC": "TBD (Modul, separat bestellen)",
            },
            seed_suffix="J3",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Helper: attach a GND power-symbol direkt rechts vom Pin
    def pin_to_gnd(pin: int, label_suffix: str) -> None:
        py = oled_pin_y(pin)
        wires.append(wire(PIN_X, py, 108, py, seed_suffix=f"j3-p{pin}-gnd"))
        symbols.append(
            place_symbol(
                lib_id="Power:GND",
                ref=f"#PWR_J3_GND_{label_suffix}",
                value="GND",
                x=108,
                y=py,
                rotation=270,
                seed_suffix=f"j3-gnd-{label_suffix}",
                sheet_uuid_seed=sus,
            )
        )

    # ---- Helper: attach a hier_label rechts vom Pin
    def pin_to_hier(pin: int, netname: str, shape: str = "input") -> None:
        py = oled_pin_y(pin)
        wires.append(wire(PIN_X, py, 112, py, seed_suffix=f"j3-p{pin}-hier"))
        hlabels.append(hier_label(112, py, netname, shape=shape, rotation=180))

    # ---- Helper: attach a plain label (für intra-Sheet-Bridging)
    def pin_to_label(pin: int, name: str) -> None:
        py = oled_pin_y(pin)
        wires.append(wire(PIN_X, py, 108, py, seed_suffix=f"j3-p{pin}-lbl"))
        labels.append(label(108, py, name))

    # ---- Pin 1: VSS → GND
    pin_to_gnd(1, "VSS")

    # ---- Pin 2: VBAT → +5V (mit lokalem 10µF + 100nF Decoupling, neu in v0.6)
    p2_y = oled_pin_y(2)
    wires.append(wire(PIN_X, p2_y, 115, p2_y, seed_suffix="j3-p2-vbat-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+5V",
            ref="#PWR_OLED_VBAT",
            value="+5V",
            x=115,
            y=p2_y,
            rotation=270,
            seed_suffix="oled-vbat-flag",
            sheet_uuid_seed=sus,
        )
    )
    junctions.append(junction(115, p2_y))
    # C6b = 10µF X5R 0805 lokal an VBAT pin
    # Device:C lib: pin1 (0, +3.81) abs (sx, sy-3.81). pin2 (0, -3.81) abs (sx, sy+3.81).
    # Wir wollen pin1 auf VBAT-Linie (y=p2_y) → sy = p2_y + 3.81.
    c6b_x = 120
    c6b_y = p2_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C6b",
            value="10uF X5R 0805 (VBAT bulk)",
            x=c6b_x,
            y=c6b_y,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A106KOQNNNE", "LCSC": "C15850"},
            seed_suffix="C6b",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(115, p2_y, 120, p2_y, seed_suffix="j3-p2-to-c6b"))
    wires.append(wire(120, c6b_y + 3.81, 120, c6b_y + 6.0, seed_suffix="c6b-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_C6b",
            value="GND",
            x=120,
            y=c6b_y + 6.0,
            seed_suffix="c6b-gnd",
            sheet_uuid_seed=sus,
        )
    )
    # C6c = 100nF X7R 0603 lokal an VBAT pin
    c6c_x = 125
    c6c_y = c6b_y
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C6c",
            value="100nF X7R 0603 (VBAT HF)",
            x=c6c_x,
            y=c6c_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="C6c",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(120, p2_y, 125, p2_y, seed_suffix="j3-p2-to-c6c"))
    junctions.append(junction(120, p2_y))
    wires.append(wire(125, c6c_y + 3.81, 125, c6c_y + 6.0, seed_suffix="c6c-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_C6c",
            value="GND",
            x=125,
            y=c6c_y + 6.0,
            seed_suffix="c6c-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Pin 3: VDD → +3V3 (mit lokalem 100nF Decoupling C6)
    p3_y = oled_pin_y(3)
    wires.append(wire(PIN_X, p3_y, 115, p3_y, seed_suffix="j3-p3-vdd-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+3V3",
            ref="#PWR_OLED_VDD",
            value="+3V3",
            x=115,
            y=p3_y,
            rotation=270,
            seed_suffix="oled-vdd-flag",
            sheet_uuid_seed=sus,
        )
    )
    junctions.append(junction(115, p3_y))
    c6_x = 120
    c6_y = p3_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C6",
            value="100nF X7R 0603 (VDD logic)",
            x=c6_x,
            y=c6_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="C6",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(115, p3_y, 120, p3_y, seed_suffix="j3-p3-to-c6"))
    wires.append(wire(120, c6_y + 3.81, 120, c6_y + 6.0, seed_suffix="c6-to-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_C6",
            value="GND",
            x=120,
            y=c6_y + 6.0,
            seed_suffix="c6-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Pin 4: NC (no connection) — separate Label um Dangling-Wire-Warning zu vermeiden
    pin_to_label(4, "NC_J3_4")

    # ---- Pin 5: BS1 → GND (4-wire SPI mode)
    pin_to_gnd(5, "BS1")

    # ---- Pin 6: BS0 → GND (4-wire SPI mode)
    pin_to_gnd(6, "BS0")

    # ---- Pin 7: RES# → OLED_RES (Pico GP9 input)
    pin_to_hier(7, "OLED_RES")

    # ---- Pin 8: CS# → OLED_CS (Pico GP5 input)
    pin_to_hier(8, "OLED_CS")

    # ---- Pin 9: D/C# → OLED_DC (Pico GP8 input)
    pin_to_hier(9, "OLED_DC")

    # ---- Pin 10: E or R/W# → GND (4-wire SPI)
    pin_to_gnd(10, "E")

    # ---- Pin 11: R/W# or E → GND (4-wire SPI)
    pin_to_gnd(11, "RW")

    # ---- Pin 12: SCLK → OLED_SCK (Pico GP6 input)
    pin_to_hier(12, "OLED_SCK")

    # ---- Pin 13: SDIN → OLED_MOSI (Pico GP7 input)
    pin_to_hier(13, "OLED_MOSI")

    # ---- Pin 14-16: NC — separate Labels
    pin_to_label(14, "NC_J3_14")
    pin_to_label(15, "NC_J3_15")
    pin_to_label(16, "NC_J3_16")

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 3: OLED (ER-OLEDM032-1W)")\n'
        f'    (date "2026-05-12")\n'
        f'    (rev "0.6")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6 §6")\n'
        f'    (comment 2 "256x64 SSD1322 OLED, 4-wire SPI mode (BS0=BS1=GND)")\n'
        f'    (comment 3 "VBAT auf +5V (250mA peak), VDDIO auf +3V3 (Logic)")\n'
        f'    (comment 4 "Modul-Loetbruecken pruefen bei Empfang"))\n'
        "  (lib_symbols\n"
        + LIB_SYMBOLS
        + "\n  )\n"
        + "".join(symbols)
        + "".join(wires)
        + "".join(junctions)
        + "".join(labels)
        + "".join(hlabels)
        + f'  (sheet_instances\n    (path "/" (page "3")))\n'
        ")\n"
    )
    return body


# ----------------------------------------------------------------------------
# Sheet 4 — MCP23017 + 10 Switches + INTA/RESET Pull-Ups per SPEC v0.6 §7
# Inputs: +3V3, GND, I2C_SDA, I2C_SCL (von Sheet 2 Pico).
# Outputs: MCP_INT (zu Sheet 2 Pico GP22).
# Components: U2 MCP23017-E/SS, SW1-SW10, R4/R5 I2C-Pull-Ups, R6 RESET-Pull-Up,
#             R20 INTA-Pull-Up (v0.6 H3-Fix), C5+C5b Decoupling.
# ----------------------------------------------------------------------------


def mcp_sheet() -> str:
    sheet_uuid = det_uuid("sheet_mcp")
    sus = "sheet_mcp"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    # U2 MCP23017 @ (130, 110). Body x=119.84..140.16, y=93.49..126.51.
    # Pin local anchor x = ±12.7 (Symbol-Body bei ±10.16, Pin-Länge 2.54).
    # Abs anchor: links x=130-12.7=117.3, rechts x=130+12.7=142.7.
    U2_X, U2_Y = 130.0, 110.0
    PIN_L_X = 117.3
    PIN_R_X = 142.7

    # Pin-Y-Tabelle (abs):
    # Links idx 0 (Pin 1 GPB0) abs y = 110 - 16.51 = 93.49. Pin N (idx N-1) abs y = 93.49 + (N-1)*2.54.
    def mcp_left_pin_y(pin: int) -> float:
        return 93.49 + (pin - 1) * 2.54

    # Rechts idx 0 (Pin 28 GPA7) abs y = 93.49. pin nummerierung absteigend.
    # Right pins ordered (28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15).
    # idx = 28 - pin für pins 15-28.
    def mcp_right_pin_y(pin: int) -> float:
        idx = 28 - pin
        return 93.49 + idx * 2.54

    symbols.append(
        place_symbol(
            lib_id="MCU:MCP23017",
            ref="U2",
            value="MCP23017-E/SS (I/O-Expander)",
            x=U2_X,
            y=U2_Y,
            footprint="Package_SO:SSOP-28_5.3x10.2mm_P0.65mm",
            datasheet="https://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf",
            extra_props={
                "MPN": "MCP23017-E/SS",
                "LCSC": "C506653",
            },
            seed_suffix="U2",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Helper für GND-Stubs (Power-Symbol direkt am Pin)
    def attach_gnd(x: float, y: float, ref: str) -> None:
        symbols.append(
            place_symbol(
                lib_id="Power:GND",
                ref=f"#PWR_{ref}",
                value="GND",
                x=x,
                y=y,
                rotation=270,
                seed_suffix=f"gnd-{ref}",
                sheet_uuid_seed=sus,
            )
        )

    # ---- Address pins A0/A1/A2 → GND (I²C Adresse 0x20)
    for pin in (15, 16, 17):  # A0, A1, A2
        py = mcp_right_pin_y(pin)
        wires.append(wire(PIN_R_X, py, 147, py, seed_suffix=f"u2-a-{pin}"))
        attach_gnd(147, py, f"U2_A{pin-15}")

    # ---- VSS (Pin 10) → GND
    p10_y = mcp_left_pin_y(10)
    wires.append(wire(PIN_L_X, p10_y, 113, p10_y, seed_suffix="u2-vss"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_U2_VSS",
            value="GND",
            x=113,
            y=p10_y,
            rotation=90,
            seed_suffix="u2-vss-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- NC pins 11, 14: NC labels gegen Dangling-Wire-Warnings
    for pin in (11, 14):
        py = mcp_left_pin_y(pin)
        wires.append(wire(PIN_L_X, py, 113, py, seed_suffix=f"u2-nc-{pin}"))
        labels.append(label(113, py, f"NC_U2_{pin}"))

    # ---- VDD (Pin 9) → +3V3 + C5 100nF + C5b 10nF lokal decoupling
    p9_y = mcp_left_pin_y(9)
    wires.append(wire(PIN_L_X, p9_y, 113, p9_y, seed_suffix="u2-vdd-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+3V3",
            ref="#PWR_U2_VDD",
            value="+3V3",
            x=113,
            y=p9_y,
            rotation=90,
            seed_suffix="u2-vdd-flag",
            sheet_uuid_seed=sus,
        )
    )
    # C5 = 100nF lokal
    # Device:C pin1 (top, +3V3) at sy-3.81. → sy = p9_y + 3.81.
    # Wir wollen C5 darunter (sy = p9_y + 3.81 in Y-DOWN heißt visuell unter dem Pin)... aber das überlappt
    # mit dem VDD Power-Flag bei (113, p9_y). Besser: C5 weiter links bei x=108, sy=p9_y+3.81.
    # Actually: VDD-Flag liegt bei x=113 rotation=90, sein Pin endet bei (113, p9_y). Es zeigt nach LINKS visuell.
    # C5 setze ich bei (108, p9_y+3.81). Pin1 abs (108, p9_y), pin2 abs (108, p9_y + 7.62).
    c5_x, c5_y = 108, p9_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C5",
            value="100nF X7R 0603 (VDD HF)",
            x=c5_x,
            y=c5_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="C5",
            sheet_uuid_seed=sus,
        )
    )
    # C5 pin1 (108, p9_y) connects to VDD stub. Need to extend stub from x=113 to x=108.
    wires.append(wire(108, p9_y, 113, p9_y, seed_suffix="vdd-to-c5"))
    junctions.append(junction(113, p9_y))
    wires.append(wire(108, p9_y + 7.62, 108, p9_y + 10, seed_suffix="c5-to-gnd"))
    attach_gnd(108, p9_y + 10, "C5")

    # C5b = 10nF lokal
    c5b_x, c5b_y = 103, c5_y
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C5b",
            value="10nF X7R 0603 (VDD HF²)",
            x=c5b_x,
            y=c5b_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB103", "LCSC": "C57112"},
            seed_suffix="C5b",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(103, p9_y, 108, p9_y, seed_suffix="c5-to-c5b"))
    junctions.append(junction(108, p9_y))
    wires.append(wire(103, c5b_y + 3.81, 103, c5b_y + 6.19, seed_suffix="c5b-to-gnd"))
    attach_gnd(103, c5b_y + 6.19, "C5b")

    # ---- SCL (Pin 12) → I2C_SCL + R5 4.7k pull-up
    p12_y = mcp_left_pin_y(12)
    wires.append(wire(PIN_L_X, p12_y, 113, p12_y, seed_suffix="u2-scl-stub"))
    hlabels.append(hier_label(113, p12_y, "I2C_SCL", shape="input", rotation=0))

    # ---- SDA (Pin 13) → I2C_SDA + R4 4.7k pull-up
    p13_y = mcp_left_pin_y(13)
    wires.append(wire(PIN_L_X, p13_y, 113, p13_y, seed_suffix="u2-sda-stub"))
    hlabels.append(hier_label(113, p13_y, "I2C_SDA", shape="input", rotation=0))

    # I²C Pull-Ups R4, R5 zwischen I2C-Linien und +3V3.
    # Verticale Pull-Ups bei x=109 für SDA (y=p13_y) und x=105 für SCL (y=p12_y).
    # R rot=0: pin1 abs (sx, sy-3.81), pin2 abs (sx, sy+3.81). pin1 oben→+3V3, pin2 unten→I2C-Linie.
    # Hmm. Bei Pull-Up von Linie auf +3V3 in Y-DOWN visuell:
    #   +3V3 oberhalb der Linie (kleinere y). R zwischen.
    # R bei (sx, sy) gibt pin1 (oben, -3.81y) → +3V3 label, pin2 (unten, +3.81y) → Linie.
    # I2C-Linien bei y=p12_y, p13_y. Pull-Ups WEITER oben (kleinere y).
    # R sym y = p12_y - 3.81 → pin2 abs (sx, p12_y) ✓ (auf SCL), pin1 abs (sx, p12_y - 7.62) → +3V3 label
    r5_y = p12_y - 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R5",
            value="4.7k 0603 (SCL pull-up)",
            x=104,
            y=r5_y,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-074K7L", "LCSC": "C23162"},
            seed_suffix="R5",
            sheet_uuid_seed=sus,
        )
    )
    # pin2 abs (104, p12_y) connects to SCL line via short wire
    wires.append(wire(104, p12_y, 113, p12_y, seed_suffix="r5-to-scl"))
    junctions.append(junction(113, p12_y))
    wires.append(wire(104, r5_y - 3.81, 104, r5_y - 6.19, seed_suffix="r5-to-3v3"))
    labels.append(label(104, r5_y - 6.19, "+3V3"))

    r4_y = p13_y - 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R4",
            value="4.7k 0603 (SDA pull-up)",
            x=99,
            y=r4_y,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-074K7L", "LCSC": "C23162"},
            seed_suffix="R4",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(99, p13_y, 113, p13_y, seed_suffix="r4-to-sda"))
    junctions.append(junction(113, p13_y))
    wires.append(wire(99, r4_y - 3.81, 99, r4_y - 6.19, seed_suffix="r4-to-3v3"))
    labels.append(label(99, r4_y - 6.19, "+3V3"))

    # ---- ~RESET (Pin 18) → R6 10k Pull-Up zu +3V3
    p18_y = mcp_right_pin_y(18)
    wires.append(wire(PIN_R_X, p18_y, 152, p18_y, seed_suffix="u2-reset-stub"))
    r6_y = p18_y + 3.81  # below the line in Y-DOWN visually means r6 sym between line and bottom
    # Actually we want R6 below the line (larger y) so it doesn't conflict with other components.
    # R rotation=0: pin1 (top, -3.81y) → ~RESET line, pin2 (bottom, +3.81y) → +3V3
    # Hmm but +3V3 should be VISUALLY ABOVE in conventional schematic (smaller y).
    # Let me flip: R6 rotation=180 — pin 1 becomes bottom, pin 2 becomes top.
    # Or simpler: put pull-up R6 to the side (right) of MCP, rotated 90:
    #   sym (153, p18_y), rotation=90. pin1 abs (153-3.81, p18_y) = (149.19, p18_y) — left, on reset line.
    #   pin2 abs (153+3.81, p18_y) = (156.81, p18_y) → +3V3
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R6",
            value="10k 0603 (RESET pull-up)",
            x=156,
            y=p18_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
            seed_suffix="R6",
            sheet_uuid_seed=sus,
        )
    )
    # R6 rotation=90: pin1 abs (sx-3.81, sy) = (152.19, p18_y). pin2 abs (sx+3.81, sy) = (159.81, p18_y).
    wires.append(wire(152, p18_y, 152.19, p18_y, seed_suffix="reset-to-r6-stub"))
    junctions.append(junction(152, p18_y))
    wires.append(wire(159.81, p18_y, 162, p18_y, seed_suffix="r6-to-3v3"))
    labels.append(label(162, p18_y, "+3V3"))

    # ---- INTA (Pin 20) → R20 10k Pull-Up zu +3V3 (v0.6 H3-Fix) + Hier-Output MCP_INT
    p20_y = mcp_right_pin_y(20)
    wires.append(wire(PIN_R_X, p20_y, 152, p20_y, seed_suffix="u2-inta-stub"))
    # R20 rotation=90 (horizontal): pin1 (sx-3.81, sy) auf INTA line, pin2 (sx+3.81, sy) auf +3V3
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R20",
            value="10k 0603 (INTA pull-up, v0.6 H3)",
            x=156,
            y=p20_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
            seed_suffix="R20",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(152, p20_y, 152.19, p20_y, seed_suffix="inta-to-r20-stub"))
    junctions.append(junction(152, p20_y))
    wires.append(wire(159.81, p20_y, 162, p20_y, seed_suffix="r20-to-3v3"))
    labels.append(label(162, p20_y, "+3V3"))
    # MCP_INT hier_label nach unten (via wire extension)
    wires.append(wire(152, p20_y, 152, p20_y + 5, seed_suffix="inta-down"))
    junctions.append(junction(152, p20_y))
    hlabels.append(hier_label(152, p20_y + 5, "MCP_INT", shape="output", rotation=270))

    # ---- INTB (Pin 19) → NC label
    p19_y = mcp_right_pin_y(19)
    wires.append(wire(PIN_R_X, p19_y, 147, p19_y, seed_suffix="u2-intb-nc"))
    labels.append(label(147, p19_y, "NC_INTB"))

    # ---- Switches SW1-SW5 (Cells) auf GPA0-GPA4 (pins 21-25). Rechts vom MCP.
    # Each switch: pin1 → MCP-GPIO, pin2 → GND. SW_Push pin1 (-5.08, 0), pin2 (+5.08, 0).
    # Wir wollen SW horizontal: pin1 (left) connects to MCP, pin2 (right) to GND.
    # SW center @ (sx, sy), sym placed so pin1 abs at x=PIN_R_X + 30 (right of MCP signal labels).
    # For pin1 abs x = 165: sx = 165 + 5.08 = 170.08. pin2 abs x = 175.16.
    cell_pins = [(21, "CELL1", 1), (22, "CELL2", 2), (23, "CELL3", 3), (24, "CELL4", 4), (25, "CELL5", 5)]
    for mcp_pin, netname, sw_num in cell_pins:
        py = mcp_right_pin_y(mcp_pin)
        # Pin-Wire vom MCP nach rechts mit Net-Label
        wires.append(wire(PIN_R_X, py, 168, py, seed_suffix=f"u2-cell-stub-{mcp_pin}"))
        labels.append(label(145, py, netname))
        # SW Symbol
        symbols.append(
            place_symbol(
                lib_id="Switch:SW_Push",
                ref=f"SW{sw_num}",
                value=f"Cell {sw_num} (2u Choc V2 Hot-Swap)",
                x=173.08,
                y=py,
                footprint="Button_Switch_Keyboard:SW_Hotswap_Kailh_MX_2.00u",
                extra_props={
                    "MPN": "Kailh Choc V2 hot-swap socket",
                    "LCSC": "TBD (separat bestellen, JLC stockt keine Hot-Swap-Sockets)",
                },
                seed_suffix=f"SW{sw_num}",
                sheet_uuid_seed=sus,
            )
        )
        # SW pin1 abs (168, py), pin2 abs (178.16, py)
        # Tiny touch wire pin1 (already part of u2-cell-stub).
        # GND via pin2
        wires.append(wire(178.16, py, 181, py, seed_suffix=f"sw{sw_num}-to-gnd"))
        attach_gnd(181, py, f"SW{sw_num}")

    # ---- Switches SW6-SW10 (Modifier) auf GPB0-GPB4 (pins 1-5). Links vom MCP.
    # SW_Push reversed: pin2 (right) connects to MCP via pin1 to GND.
    # Or: simpler: place SW left of MCP, SW pin2 → MCP-pin, SW pin1 → GND.
    # SW at (sx, sy) — pin1 abs (sx-5.08, sy), pin2 abs (sx+5.08, sy).
    # We want pin2 at x=PIN_L_X - 5 (left of left-stub). For pin2 abs x = 90: sx = 84.92. pin1 abs x = 79.84.
    mod_pins = [(1, "MOD_SHIFT", 6), (2, "MOD_HOLD", 7), (3, "MOD_DRONE", 8), (4, "MOD_GENERATE", 9), (5, "MOD_CLEAR", 10)]
    for mcp_pin, netname, sw_num in mod_pins:
        py = mcp_left_pin_y(mcp_pin)
        wires.append(wire(PIN_L_X, py, 90, py, seed_suffix=f"u2-mod-stub-{mcp_pin}"))
        labels.append(label(115, py, netname))
        symbols.append(
            place_symbol(
                lib_id="Switch:SW_Push",
                ref=f"SW{sw_num}",
                value=f"Modifier {netname.replace('MOD_','')} (1u Choc V2 Hot-Swap)",
                x=84.92,
                y=py,
                footprint="Button_Switch_Keyboard:SW_Hotswap_Kailh_MX_1.00u",
                extra_props={
                    "MPN": "Kailh Choc V2 hot-swap socket",
                    "LCSC": "TBD (separat bestellen)",
                },
                seed_suffix=f"SW{sw_num}",
                sheet_uuid_seed=sus,
            )
        )
        # SW pin1 abs (79.84, py) → GND
        wires.append(wire(79.84, py, 77, py, seed_suffix=f"sw{sw_num}-to-gnd"))
        attach_gnd(77, py, f"SW{sw_num}")

    # ---- Reserve-Pins (NC labels gegen Dangling-Warnings):
    # GPB5-7 (pins 6-8): NC
    # GPA5-7 (pins 26-28): NC
    for pin in (6, 7, 8):
        py = mcp_left_pin_y(pin)
        wires.append(wire(PIN_L_X, py, 115, py, seed_suffix=f"u2-gpb-nc-{pin}"))
        labels.append(label(115, py, f"NC_GPB{pin-1}"))
    for pin in (26, 27, 28):
        py = mcp_right_pin_y(pin)
        wires.append(wire(PIN_R_X, py, 145, py, seed_suffix=f"u2-gpa-nc-{pin}"))
        labels.append(label(145, py, f"NC_GPA{pin-21}"))

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 4: MCP23017 + 10 Switches")\n'
        f'    (date "2026-05-12")\n'
        f'    (rev "0.6")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6 §7")\n'
        f'    (comment 2 "I2C Adresse 0x20 (A0=A1=A2=GND)")\n'
        f'    (comment 3 "INTA Pull-Up R20 10k zu +3V3 (v0.6 H3-Fix)")\n'
        f'    (comment 4 "10 Switches Kailh Choc V2 Hot-Swap, MIRROR=1 in IOCON"))\n'
        "  (lib_symbols\n"
        + LIB_SYMBOLS
        + "\n  )\n"
        + "".join(symbols)
        + "".join(wires)
        + "".join(junctions)
        + "".join(labels)
        + "".join(hlabels)
        + f'  (sheet_instances\n    (path "/" (page "4")))\n'
        ")\n"
    )
    return body


# ----------------------------------------------------------------------------
# Root schematic
# ----------------------------------------------------------------------------


def root_sheet() -> str:
    root_uuid = det_uuid("root_sheet")
    power_uuid = det_uuid("sheet_power_tree")
    pico_uuid = det_uuid("sheet_pico")
    oled_uuid = det_uuid("sheet_oled")
    mcp_uuid = det_uuid("sheet_mcp")
    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{root_uuid}")\n'
        f'  (paper "A4")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Root")\n'
        f'    (date "2026-05-12")\n'
        f'    (rev "0.6")\n'
        f'    (company "Field Ambience Project"))\n'
        f'  (lib_symbols)\n'
        # ---- Sheet 1: Power Tree ----
        f'  (sheet (at 30 40) (size 60 60) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{power_uuid}")\n'
        f'    (property "Sheetname" "PowerTree" (at 30 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "power_tree.kicad_sch" (at 30 100.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "+5V_OUT" output (at 90 50 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_5v")}"))\n'
        f'    (pin "GND_OUT" passive (at 90 55 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_gnd")}"))\n'
        f'    (pin "PICO_USB_DP" output (at 90 75 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_dp")}"))\n'
        f'    (pin "PICO_USB_DN" output (at 90 80 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_dn")}")))\n'
        # ---- Sheet 2: Pico ----
        f'  (sheet (at 130 40) (size 60 60) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{pico_uuid}")\n'
        f'    (property "Sheetname" "Pico" (at 130 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "pico.kicad_sch" (at 130 100.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "+5V_IN" input (at 130 50 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_5v")}"))\n'
        f'    (pin "PICO_USB_DP" input (at 130 75 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_dp")}"))\n'
        f'    (pin "PICO_USB_DN" input (at 130 80 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_dn")}"))\n'
        f'    (pin "+3V3_OUT" output (at 190 50 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_3v3")}"))\n'
        f'    (pin "OLED_SCK" output (at 190 60 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_osck")}"))\n'
        f'    (pin "OLED_MOSI" output (at 190 65 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_omosi")}"))\n'
        f'    (pin "OLED_CS" output (at 190 70 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_ocs")}"))\n'
        f'    (pin "OLED_DC" output (at 190 75 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_odc")}"))\n'
        f'    (pin "OLED_RES" output (at 190 80 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_ores")}"))\n'
        f'    (pin "I2C_SDA" bidirectional (at 190 85 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_sda")}"))\n'
        f'    (pin "I2C_SCL" bidirectional (at 190 90 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_scl")}"))\n'
        f'    (pin "MCP_INT" input (at 190 95 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_mcpint")}")))\n'
        # ---- Sheet 3: OLED ----
        f'  (sheet (at 230 40) (size 60 60) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{oled_uuid}")\n'
        f'    (property "Sheetname" "OLED" (at 230 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "oled.kicad_sch" (at 230 100.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "OLED_SCK" input (at 230 60 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_sck")}"))\n'
        f'    (pin "OLED_MOSI" input (at 230 65 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_mosi")}"))\n'
        f'    (pin "OLED_CS" input (at 230 70 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_cs")}"))\n'
        f'    (pin "OLED_DC" input (at 230 75 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_dc")}"))\n'
        f'    (pin "OLED_RES" input (at 230 80 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_res")}")))\n'
        # ---- Sheet 4: MCP23017 ----
        f'  (sheet (at 130 110) (size 60 30) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{mcp_uuid}")\n'
        f'    (property "Sheetname" "MCP23017" (at 130 109 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "mcp.kicad_sch" (at 130 140.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "I2C_SDA" bidirectional (at 130 120 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("mcppin_sda")}"))\n'
        f'    (pin "I2C_SCL" bidirectional (at 130 125 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("mcppin_scl")}"))\n'
        f'    (pin "MCP_INT" output (at 130 130 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("mcppin_int")}")))\n'
        # ---- Inter-sheet wires Sheet 1 → Sheet 2 ----
        f'  (wire (pts (xy 90 50) (xy 130 50)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_5v")}"))\n'
        f'  (wire (pts (xy 90 75) (xy 130 75)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_dp")}"))\n'
        f'  (wire (pts (xy 90 80) (xy 130 80)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_dn")}"))\n'
        # ---- Inter-sheet wires Sheet 2 → Sheet 3 (OLED SPI bus) ----
        f'  (wire (pts (xy 190 60) (xy 230 60)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_osck")}"))\n'
        f'  (wire (pts (xy 190 65) (xy 230 65)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_omosi")}"))\n'
        f'  (wire (pts (xy 190 70) (xy 230 70)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ocs")}"))\n'
        f'  (wire (pts (xy 190 75) (xy 230 75)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_odc")}"))\n'
        f'  (wire (pts (xy 190 80) (xy 230 80)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ores")}"))\n'
        # ---- Inter-sheet wiring Pico (Sheet 2 right edge x=190) → MCP (Sheet 4 left edge x=130)
        # via labels (Sheet 4 ist räumlich unter Sheet 2). Pico-Pins für I2C+INT enden bei (190, 85..95).
        # MCP-Pins liegen bei (130, 120..130). Labels schließen die Netze über Sheet-Grenzen.
        f'  (wire (pts (xy 190 85) (xy 195 85)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_sda_pico")}"))\n'
        f'  (label "I2C_SDA" (at 195 85 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_sda_pico")}"))\n'
        f'  (wire (pts (xy 190 90) (xy 195 90)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_scl_pico")}"))\n'
        f'  (label "I2C_SCL" (at 195 90 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_scl_pico")}"))\n'
        f'  (wire (pts (xy 190 95) (xy 195 95)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_mcpint_pico")}"))\n'
        f'  (label "MCP_INT" (at 195 95 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_mcpint_pico")}"))\n'
        f'  (wire (pts (xy 125 120) (xy 130 120)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_sda_mcp")}"))\n'
        f'  (label "I2C_SDA" (at 125 120 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_sda_mcp")}"))\n'
        f'  (wire (pts (xy 125 125) (xy 130 125)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_scl_mcp")}"))\n'
        f'  (label "I2C_SCL" (at 125 125 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_scl_mcp")}"))\n'
        f'  (wire (pts (xy 125 130) (xy 130 130)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_mcpint_mcp")}"))\n'
        f'  (label "MCP_INT" (at 125 130 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_mcpint_mcp")}"))\n'
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
    (OUT_DIR / "pico.kicad_sch").write_text(pico_sheet())
    (OUT_DIR / "oled.kicad_sch").write_text(oled_sheet())
    (OUT_DIR / "mcp.kicad_sch").write_text(mcp_sheet())
    print(f"Wrote KiCad project + Sheets 1+2+3+4 to {OUT_DIR}")


if __name__ == "__main__":
    main()
