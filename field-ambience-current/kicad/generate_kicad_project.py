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


LIB_SYMBOLS = LIB_SYMBOLS + "\n" + _pico2_lib_symbol()


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
# Root schematic
# ----------------------------------------------------------------------------


def root_sheet() -> str:
    root_uuid = det_uuid("root_sheet")
    power_uuid = det_uuid("sheet_power_tree")
    pico_uuid = det_uuid("sheet_pico")
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
        # ---- Sheet 1: Power Tree ----
        f'  (sheet (at 30 40) (size 60 50) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{power_uuid}")\n'
        f'    (property "Sheetname" "PowerTree" (at 30 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "power_tree.kicad_sch" (at 30 90.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "+5V_OUT" output (at 90 50 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_5v")}"))\n'
        f'    (pin "GND_OUT" passive (at 90 55 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_gnd")}"))\n'
        f'    (pin "PICO_USB_DP" output (at 90 65 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_dp")}"))\n'
        f'    (pin "PICO_USB_DN" output (at 90 70 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_dn")}")))\n'
        # ---- Sheet 2: Pico ----
        f'  (sheet (at 130 40) (size 60 50) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{pico_uuid}")\n'
        f'    (property "Sheetname" "Pico" (at 130 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "pico.kicad_sch" (at 130 90.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "+5V_IN" input (at 130 50 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_5v")}"))\n'
        f'    (pin "PICO_USB_DP" input (at 130 65 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_dp")}"))\n'
        f'    (pin "PICO_USB_DN" input (at 130 70 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_dn")}"))\n'
        f'    (pin "+3V3_OUT" output (at 190 50 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_3v3")}")))\n'
        # ---- Inter-sheet wires (Sheet 1 outputs → Sheet 2 inputs) ----
        f'  (wire (pts (xy 90 50) (xy 130 50)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_5v")}"))\n'
        f'  (wire (pts (xy 90 65) (xy 130 65)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_dp")}"))\n'
        f'  (wire (pts (xy 90 70) (xy 130 70)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_dn")}"))\n'
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
    print(f"Wrote KiCad project + Sheets 1+2 to {OUT_DIR}")


if __name__ == "__main__":
    main()
