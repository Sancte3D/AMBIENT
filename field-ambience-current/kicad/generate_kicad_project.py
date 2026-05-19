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
            [det_uuid("sheet_encoder"), "Encoders"],
            [det_uuid("sheet_audio"), "Audio"],
            [det_uuid("sheet_pi"), "PiHeader"],
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
          (number "A4" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 7.62 0) (length 2.54)
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "B4" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 5.08 0) (length 2.54)
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "A9" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 2.54 0) (length 2.54)
          (name "VBUS" (effects (font (size 1.27 1.27))))
          (number "B9" (effects (font (size 1.27 1.27)))))
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
          (number "A1" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 -5.08 0) (length 2.54)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "B1" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -15.24 -7.62 0) (length 2.54)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "A12" (effects (font (size 1.27 1.27)))))
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


def _rotary_encoder_switch_lib_symbol() -> str:
    """EC11 Rotary Encoder mit integriertem Push-Switch (5-Pin).

    Pin-Layout:
        Pin 1 (A)   links oben    local (-5.08, +5.08)
        Pin 2 (B)   links mitte   local (-5.08, 0)
        Pin 3 (C)   links unten   local (-5.08, -5.08) — Common, to GND
        Pin 4 (SW1) rechts oben   local (+5.08, +2.54)
        Pin 5 (SW2) rechts unten  local (+5.08, -2.54) — to GND
    """
    return r"""
    (symbol "Encoder:Rotary_Encoder_Switch" (pin_names (offset 0.508)) (in_bom yes) (on_board yes)
      (property "Reference" "EN" (at 0 9.144 0) (effects (font (size 1.27 1.27))))
      (property "Value" "Rotary_Encoder_Switch" (at 0 -9.144 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "https://www.bourns.com/docs/Product-Datasheets/PEC11R.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Encoder:Rotary_Encoder_Switch_0_1"
        (rectangle (start -2.54 7.62) (end 2.54 -7.62)
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Encoder:Rotary_Encoder_Switch_1_1"
        (pin passive line (at -5.08 5.08 0) (length 2.54)
          (name "A" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at -5.08 0 0) (length 2.54)
          (name "B" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))
        (pin passive line (at -5.08 -5.08 0) (length 2.54)
          (name "C" (effects (font (size 1.27 1.27))))
          (number "3" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 5.08 2.54 180) (length 2.54)
          (name "SW1" (effects (font (size 1.27 1.27))))
          (number "4" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 5.08 -2.54 180) (length 2.54)
          (name "SW2" (effects (font (size 1.27 1.27))))
          (number "5" (effects (font (size 1.27 1.27)))))))
""".strip()


def _pcm5102a_lib_symbol() -> str:
    """PCM5102A I²S DAC (TSSOP-20). Offizielles TI-Datasheet-Pinout SLAS859C.

    Pins links (top→bottom): 1-10
        1: CPVDD   (charge pump power supply)
        2: CAPP    (charge pump fly cap +)
        3: CPGND   (charge pump ground)
        4: CAPM    (charge pump fly cap -)
        5: VNEG    (negative voltage, 1µF to GND)
        6: OUTL    (left audio output)
        7: OUTR    (right audio output)
        8: AVDD    (analog power)
        9: AGND    (analog ground)
        10: DEMP   (de-emphasis, GND=off)

    Pins rechts (top→bottom, pin 20 oben):
        20: DVDD   (digital power)
        19: DGND   (digital ground)
        18: LDOO   (1.8V LDO output, leave open or 0.1µF)
        17: XSMT   (soft mute, pull-up to DVDD)
        16: FMT    (format, GND=I²S)
        15: LRCK   (left/right clock)
        14: DIN    (audio data input)
        13: BCK    (bit clock)
        12: SCK    (system clock, GND=3-wire mode)
        11: FLT    (filter select, GND=normal)
    """
    pins_left = [
        (1, "CPVDD", "power_in"),
        (2, "CAPP", "output"),
        (3, "CPGND", "power_in"),
        (4, "CAPM", "output"),
        (5, "VNEG", "output"),
        (6, "OUTL", "output"),
        (7, "OUTR", "output"),
        (8, "AVDD", "power_in"),
        (9, "AGND", "power_in"),
        (10, "DEMP", "input"),
    ]
    pins_right = [
        (20, "DVDD", "power_in"),
        (19, "DGND", "power_in"),
        (18, "LDOO", "output"),
        (17, "XSMT", "input"),
        (16, "FMT", "input"),
        (15, "LRCK", "input"),
        (14, "DIN", "input"),
        (13, "BCK", "input"),
        (12, "SCK", "input"),
        (11, "FLT", "input"),
    ]
    y_top = 11.43  # (10-1)*2.54/2
    rect_top = y_top + 2.54
    rect_bot = -y_top - 2.54
    out = ['    (symbol "Audio:PCM5102A" (in_bom yes) (on_board yes)']
    out.append(f'      (property "Reference" "U" (at 0 {rect_top + 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append(f'      (property "Value" "PCM5102A" (at 0 {rect_bot - 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "https://www.ti.com/lit/ds/symlink/pcm5102a.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Audio:PCM5102A_0_1"')
    out.append(f'        (rectangle (start -10.16 {rect_top}) (end 10.16 {rect_bot})')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append('      (symbol "Audio:PCM5102A_1_1"')
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


def _pam8403_lib_symbol() -> str:
    """PAM8403H Class-D Stereo Amp (SOP-16).

    Pinout VERIFIED gegen Diodes Inc PAM8403H Datasheet PDF
    (Rev 1-0, November 2012, dem im Repo unter PAM8403H.PDF).

    LCSC C17337 = PAM8403H von Diodes Inc.

    Pin-Belegung per Datasheet Page 2 "Pin Descriptions":
        1:  -OUT_L  (Left Channel Negative Output, BTL)
        2:  PGND    (Power Ground)
        3:  +OUT_L  (Left Channel Positive Output, BTL)
        4:  PVDD    (Power VDD)
        5:  MUTE    (Mute Control Input, ACTIVE LOW)
        6:  VDD     (Analog VDD)
        7:  INL     (Left Channel Input)
        8:  VREF    (Internal analog reference — bypass cap to GND REQUIRED)
        9:  NC      (No connected)
        10: INR     (Right Channel Input)
        11: GND     (Analog GND)
        12: SHDN    (Shutdown Control Input, ACTIVE LOW)
        13: PVDD    (Power VDD)
        14: +OUT_R  (Right Channel Positive Output, BTL)
        15: PGND    (Power Ground)
        16: -OUT_R  (Right Channel Negative Output, BTL)
    """
    pins_left = [
        (1, "OUTL-", "output"),
        (2, "PGND", "power_in"),
        (3, "OUTL+", "output"),
        (4, "PVDD", "power_in"),
        (5, "/MUTE", "input"),
        (6, "VDD", "power_in"),
        (7, "INL", "input"),
        (8, "VREF", "output"),
    ]
    pins_right = [
        (16, "OUTR-", "output"),
        (15, "PGND", "power_in"),
        (14, "OUTR+", "output"),
        (13, "PVDD", "power_in"),
        (12, "/SHDN", "input"),
        (11, "GND", "power_in"),
        (10, "INR", "input"),
        (9, "NC", "no_connect"),
    ]
    y_top = 8.89
    rect_top = y_top + 2.54
    rect_bot = -y_top - 2.54
    out = ['    (symbol "Audio:PAM8403" (in_bom yes) (on_board yes)']
    out.append(f'      (property "Reference" "U" (at 0 {rect_top + 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append(f'      (property "Value" "PAM8403H" (at 0 {rect_bot - 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "PAM8403H.PDF (Diodes Inc Rev 1-0, Nov 2012)" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Audio:PAM8403_0_1"')
    out.append(f'        (rectangle (start -10.16 {rect_top}) (end 10.16 {rect_bot})')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append('      (symbol "Audio:PAM8403_1_1"')
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


def _ferrite_bead_lib_symbol() -> str:
    """Ferrite bead 2-pin symbol (BLM18AG601 600Ω@100MHz)."""
    return r"""
    (symbol "Device:Ferrite_Bead" (pin_numbers hide) (pin_names (offset 0.254) hide) (in_bom yes) (on_board yes)
      (property "Reference" "FB" (at 0 2.794 0) (effects (font (size 1.27 1.27))))
      (property "Value" "Ferrite_Bead" (at 0 -2.794 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:Ferrite_Bead_0_1"
        (rectangle (start -2.54 -0.762) (end 2.54 0.762)
          (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -2.032 0.762) (xy 2.032 -0.762))
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Device:Ferrite_Bead_1_1"
        (pin passive line (at -3.81 0 0) (length 1.27)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 3.81 0 180) (length 1.27)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
""".strip()


def _conn_02xN_lib_symbol(n: int) -> str:
    """Generic 2×N-row dual-column header symbol (Pi GPIO style).

    Pin local positions: left column (odd numbers) at x=-5.08, right column
    (even numbers) at x=+5.08. Row R (1..n) at local y = +(n-1)*1.27 - (R-1)*2.54
    in KiCad Y-UP lib coords. Pin 1 is top-left.
    """
    y_top = (n - 1) * 1.27
    rect_top = y_top + 2.54
    rect_bot = -y_top - 2.54
    out = [f'    (symbol "Connector:Conn_02x{n:02d}" (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)']
    out.append(f'      (property "Reference" "J" (at 0 {rect_top + 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append(f'      (property "Value" "Conn_02x{n:02d}" (at 0 {rect_bot - 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append(f'      (symbol "Connector:Conn_02x{n:02d}_0_1"')
    out.append(f'        (rectangle (start -2.54 {rect_top}) (end 2.54 {rect_bot})')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append(f'      (symbol "Connector:Conn_02x{n:02d}_1_1"')
    for row in range(n):
        ly = y_top - row * 2.54
        # Left pin = odd number 2*row+1, right pin = even number 2*row+2
        pl_num = 2 * row + 1
        pr_num = 2 * row + 2
        out.append(
            f'        (pin passive line (at -5.08 {ly:.3f} 0) (length 2.54)\n'
            f'          (name "Pin_{pl_num}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{pl_num}" (effects (font (size 1.27 1.27)))))'
        )
        out.append(
            f'        (pin passive line (at 5.08 {ly:.3f} 180) (length 2.54)\n'
            f'          (name "Pin_{pr_num}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{pr_num}" (effects (font (size 1.27 1.27)))))'
        )
    out.append('        )')
    out.append('      )')
    return "\n".join(out)


LIB_SYMBOLS = (
    LIB_SYMBOLS
    + "\n" + _pico2_lib_symbol()
    + "\n" + _conn_01xN_lib_symbol(16)
    + "\n" + _mcp23017_lib_symbol()
    + "\n" + _rotary_encoder_switch_lib_symbol()
    + "\n" + _pcm5102a_lib_symbol()
    + "\n" + _pam8403_lib_symbol()
    + "\n" + _ferrite_bead_lib_symbol()
    + "\n" + _conn_01xN_lib_symbol(2)
    + "\n" + _conn_02xN_lib_symbol(20)
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
    # Pin-Belegung VERIFIED gegen USB Type-C Spec Rev 2.1 Table 3-1.
    # WICHTIG: GND ist A1/A12/B1/B12, VBUS ist A4/A9/B4/B9 (NICHT umgekehrt!)
    # Symbol-Layout: VBUS-Group oben (Pin-Numbers A4/B4/A9/B9), GND-Group unten
    # (A1/B1/A12/B12), Signal-Pins (CC/D±/SBU) rechts.
    # Pin-Tabelle absolut (sym (50,80), abs_y = sym_y - local_y):
    #   left-VBUS:  A4 y=69.84 | B4 y=72.38 | A9 y=74.92 | B9 y=77.46
    #   left-GND:   A1 y=82.54 | B1 y=85.08 | A12 y=87.62 | B12 y=90.16
    #   right CC:   A5 (CC1) y=72.38 | B5 (CC2) y=74.92
    #   right D±:   A6 (D+) y=77.46 | A7 (D-) y=80.0 | B6 (D+) y=82.54 | B7 (D-) y=85.08
    #   right SBU:  A8 y=87.62 | B8 y=90.16
    #   shield S1:  abs (50, 105.4)
    J1_X, J1_Y = 50.0, 80.0
    VBUS_PINS_Y = [69.84, 72.38, 74.92, 77.46]  # USB-C spec: A4, B4, A9, B9
    GND_PINS_Y = [82.54, 85.08, 87.62, 90.16]   # USB-C spec: A1, B1, A12, B12
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
    # ---- VBUS-Trunk: 4 Stubs (A4, B4, A9, B9 per USB-C spec) → Trunk x=30.48 → F1 → Rail
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

    # ---- GND-Trunk: 4 Stubs (A1, B1, A12, B12 per USB-C spec) → Trunk x=30.48 → unten zu GND-Bus
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
        f'    (date "2026-05-14")\n'
        f'    (rev "0.6.3")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6.3 §1 + §3")\n'
        f'    (comment 2 "USB-C → F1(2A) → C_BULK(1000µF) → +5V rail")\n'
        f'    (comment 3 "USB-C VBUS/GND-Pin-Belegung per USB Type-C Spec Rev 2.1 (v0.6.3 fix)")\n'
        f'    (comment 4 "ESD: USBLC6-2SC6 on D+/D-; TVS: SMAJ5.0A on +5V"))\n'
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
        1: "PICO_TX_PI_RX",  # GP0 — Pico transmits to Pi RX (GPIO15, pin 10)
        2: "PI_TX_PICO_RX",  # GP1 — Pi TX (GPIO14, pin 8) via R1 1k series to Pico RX
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
    # GPA5 (Pin 26): v0.6.3-r5 N1-Fix — drives PCM5102A XSMT via hier-output
    py = mcp_right_pin_y(26)
    wires.append(wire(PIN_R_X, py, 152, py, seed_suffix="u2-gpa5-xsmt"))
    labels.append(label(145, py, "GPA5/XSMT"))
    hlabels.append(hier_label(152, py, "PCM_XSMT", shape="output", rotation=180))
    # GPA6, GPA7 (Pins 27, 28) — bleiben Reserve mit NC-Labels
    for pin in (27, 28):
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
# Sheet 5 — 4× EC11 Encoder + RC-Debounce per SPEC v0.6 §5/§M4
# 4 Encoder (Drive, Brightness, Display, Volume) à 3 Signale (A, B, SW).
# Pull-Ups: 10k auf jeden Signal-Pin zu +3V3.
# Debounce: 100nF auf A/B-Linien zu GND (v0.6 M4-Fix von 10nF auf 100nF).
# Inputs: +3V3, GND. Outputs: DRIVE_A/B/SW, BRIGHT_A/B/SW, DISPLAY_A/B/SW, VOL_A/B/SW.
# ----------------------------------------------------------------------------


def encoder_sheet() -> str:
    sheet_uuid = det_uuid("sheet_encoder")
    sus = "sheet_encoder"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

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

    def place_one_encoder(en_num: int, sy: float, net_prefix: str,
                          r_a: int, r_b: int, r_sw: int,
                          c_a: int, c_b: int) -> None:
        """Platziere einen EC11 + Pull-Ups + Debounce-Caps + Hier-Labels."""
        sx = 140.0
        # Pin Absolutpositionen (sym (sx, sy), KiCad Y-DOWN abs = sym - local_y):
        a_x, a_y = sx - 5.08, sy - 5.08     # Pin 1 A
        b_x, b_y = sx - 5.08, sy            # Pin 2 B
        c_x, c_y = sx - 5.08, sy + 5.08     # Pin 3 C → GND
        sw1_x, sw1_y = sx + 5.08, sy - 2.54  # Pin 4 SW1 → Signal
        sw2_x, sw2_y = sx + 5.08, sy + 2.54  # Pin 5 SW2 → GND

        # ---- EC11 Symbol
        symbols.append(
            place_symbol(
                lib_id="Encoder:Rotary_Encoder_Switch",
                ref=f"EN{en_num}",
                value=f"EC11 + push ({net_prefix})",
                x=sx,
                y=sy,
                footprint="Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm",
                datasheet="https://www.bourns.com/docs/Product-Datasheets/PEC11R.pdf",
                extra_props={
                    "MPN": "PEC11R-4215F-S0024",
                    "LCSC": "TBD (mehrere EC11-Varianten verfügbar)",
                },
                seed_suffix=f"EN{en_num}",
                sheet_uuid_seed=sus,
            )
        )

        # ---- A-Line: Pull-Up R + Debounce-Cap C + Hier-Label
        # R rot=0 vertikal: pin1 abs (sym_x, sym_y-3.81) (oben, +3V3), pin2 abs (sym_x, sym_y+3.81) (unten, Signal)
        # → für pin2 auf A-Line (y=a_y): sym_y = a_y - 3.81. pin1 abs = a_y - 7.62 → +3V3.
        r_a_sym_y = a_y - 3.81
        symbols.append(
            place_symbol(
                lib_id="Device:R",
                ref=f"R{r_a}",
                value="10k 0603 (A pull-up)",
                x=128,
                y=r_a_sym_y,
                footprint="Resistor_SMD:R_0603_1608Metric",
                extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
                seed_suffix=f"R{r_a}",
                sheet_uuid_seed=sus,
            )
        )
        # R pin2 abs (128, a_y) connect to A-line
        wires.append(wire(128, a_y, a_x, a_y, seed_suffix=f"en{en_num}-a-to-r"))
        # R pin1 abs (128, a_y - 7.62) → +3V3 label
        wires.append(wire(128, a_y - 7.62, 128, a_y - 10, seed_suffix=f"en{en_num}-r{r_a}-to-3v3"))
        labels.append(label(128, a_y - 10, "+3V3"))

        # C debounce — vertikal. C pin1 abs (sym_x, sym_y-3.81) (oben), pin2 abs (sym_x, sym_y+3.81) (unten, GND).
        # Für pin1 auf A-Line: sym_y = a_y + 3.81. pin2 abs = a_y + 7.62.
        c_a_sym_y = a_y + 3.81
        symbols.append(
            place_symbol(
                lib_id="Device:C",
                ref=f"C{c_a}",
                value="100nF X7R 0603 (A debounce, v0.6 M4)",
                x=122,
                y=c_a_sym_y,
                footprint="Capacitor_SMD:C_0603_1608Metric",
                extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
                seed_suffix=f"C{c_a}",
                sheet_uuid_seed=sus,
            )
        )
        # C pin1 (122, a_y) on A-line, pin2 (122, a_y+7.62) to GND
        wires.append(wire(122, a_y, 128, a_y, seed_suffix=f"en{en_num}-c-to-r-a"))
        junctions.append(junction(128, a_y))
        wires.append(wire(122, a_y + 7.62, 122, a_y + 10, seed_suffix=f"en{en_num}-c{c_a}-to-gnd"))
        attach_gnd(122, a_y + 10, f"C{c_a}")

        # A-Line bis hier-Label nach links
        wires.append(wire(122, a_y, 110, a_y, seed_suffix=f"en{en_num}-a-to-hlbl"))
        junctions.append(junction(122, a_y))
        hlabels.append(hier_label(110, a_y, f"{net_prefix}_A", shape="output", rotation=0))

        # ---- B-Line: Pull-Up R + Debounce-Cap C + Hier-Label
        r_b_sym_y = b_y - 3.81
        symbols.append(
            place_symbol(
                lib_id="Device:R",
                ref=f"R{r_b}",
                value="10k 0603 (B pull-up)",
                x=132,
                y=r_b_sym_y,
                footprint="Resistor_SMD:R_0603_1608Metric",
                extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
                seed_suffix=f"R{r_b}",
                sheet_uuid_seed=sus,
            )
        )
        # R pin2 abs (132, b_y) — but B-line is at b_y = sy. We need wire from R pin2 to B pin.
        # B pin at (a_x, b_y) = (134.92, sy). R pin2 at (132, sy). Wire (132, sy) → (134.92, sy).
        wires.append(wire(132, b_y, b_x, b_y, seed_suffix=f"en{en_num}-b-to-r"))
        wires.append(wire(132, b_y - 7.62, 132, b_y - 10, seed_suffix=f"en{en_num}-r{r_b}-to-3v3"))
        labels.append(label(132, b_y - 10, "+3V3"))

        c_b_sym_y = b_y + 3.81
        symbols.append(
            place_symbol(
                lib_id="Device:C",
                ref=f"C{c_b}",
                value="100nF X7R 0603 (B debounce, v0.6 M4)",
                x=118,
                y=c_b_sym_y,
                footprint="Capacitor_SMD:C_0603_1608Metric",
                extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
                seed_suffix=f"C{c_b}",
                sheet_uuid_seed=sus,
            )
        )
        wires.append(wire(118, b_y, 132, b_y, seed_suffix=f"en{en_num}-c-to-r-b"))
        junctions.append(junction(132, b_y))
        wires.append(wire(118, b_y + 7.62, 118, b_y + 10, seed_suffix=f"en{en_num}-c{c_b}-to-gnd"))
        attach_gnd(118, b_y + 10, f"C{c_b}")

        wires.append(wire(118, b_y, 110, b_y, seed_suffix=f"en{en_num}-b-to-hlbl"))
        junctions.append(junction(118, b_y))
        hlabels.append(hier_label(110, b_y, f"{net_prefix}_B", shape="output", rotation=0))

        # ---- C-Pin (Common) → GND direkt
        wires.append(wire(c_x, c_y, c_x - 3, c_y, seed_suffix=f"en{en_num}-c-to-gnd"))
        attach_gnd(c_x - 3, c_y, f"EN{en_num}_C")

        # ---- SW1: Pull-Up R + Hier-Label
        # SW1 pin abs (sw1_x, sw1_y) = (145.08, sy-2.54)
        # R rot=0 vertikal pull-up. pin2 (bottom, on SW1 line): sym_y = sw1_y - 3.81. pin1 abs = sw1_y - 7.62.
        r_sw_sym_y = sw1_y - 3.81
        symbols.append(
            place_symbol(
                lib_id="Device:R",
                ref=f"R{r_sw}",
                value="10k 0603 (SW pull-up)",
                x=152,
                y=r_sw_sym_y,
                footprint="Resistor_SMD:R_0603_1608Metric",
                extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
                seed_suffix=f"R{r_sw}",
                sheet_uuid_seed=sus,
            )
        )
        wires.append(wire(sw1_x, sw1_y, 152, sw1_y, seed_suffix=f"en{en_num}-sw-to-r"))
        wires.append(wire(152, sw1_y - 7.62, 152, sw1_y - 10, seed_suffix=f"en{en_num}-r{r_sw}-to-3v3"))
        labels.append(label(152, sw1_y - 10, "+3V3"))

        # SW1-Line bis hier-Label
        wires.append(wire(152, sw1_y, 162, sw1_y, seed_suffix=f"en{en_num}-sw-to-hlbl"))
        junctions.append(junction(152, sw1_y))
        hlabels.append(hier_label(162, sw1_y, f"{net_prefix}_SW", shape="output", rotation=180))

        # ---- SW2 → GND
        wires.append(wire(sw2_x, sw2_y, sw2_x + 3, sw2_y, seed_suffix=f"en{en_num}-sw2-to-gnd"))
        attach_gnd(sw2_x + 3, sw2_y, f"EN{en_num}_SW2")

    # Vier Encoder vertikal stacken (35mm Spacing für genug Platz)
    place_one_encoder(en_num=1, sy=70, net_prefix="DRIVE",
                      r_a=7, r_b=8, r_sw=15, c_a=10, c_b=11)
    place_one_encoder(en_num=2, sy=105, net_prefix="BRIGHT",
                      r_a=9, r_b=10, r_sw=16, c_a=12, c_b=13)
    place_one_encoder(en_num=3, sy=140, net_prefix="DISPLAY",
                      r_a=11, r_b=12, r_sw=17, c_a=14, c_b=15)
    place_one_encoder(en_num=4, sy=175, net_prefix="VOL",
                      r_a=13, r_b=14, r_sw=18, c_a=16, c_b=17)

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 5: 4x EC11 Encoder")\n'
        f'    (date "2026-05-12")\n'
        f'    (rev "0.6")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6 §5 + §M4")\n'
        f'    (comment 2 "EN1=Drive (GP10-12), EN2=Brightness (GP13-15)")\n'
        f'    (comment 3 "EN3=Display (GP16-18), EN4=Volume (GP19-21)")\n'
        f'    (comment 4 "Debounce 100nF (v0.6 M4 Fix von 10nF), 10k pull-ups"))\n'
        "  (lib_symbols\n"
        + LIB_SYMBOLS
        + "\n  )\n"
        + "".join(symbols)
        + "".join(wires)
        + "".join(junctions)
        + "".join(labels)
        + "".join(hlabels)
        + f'  (sheet_instances\n    (path "/" (page "5")))\n'
        ")\n"
    )
    return body


# ----------------------------------------------------------------------------
# Sheet 6 — Audio: PCM5102A I²S DAC + FB1 + PAM8403 Class-D + 2× Speaker
# per SPEC v0.6 §8 (+ H2/M2/C2 Fixes: PAM8403 10µF Bulk, AVDD/DVDD split via
# Ferrite Bead, AMP_SHUTDOWN+AMP_MUTE GPIOs).
# Inputs: +5V, +3V3, GND, I2S_BCK, I2S_LRCK, I2S_DOUT (von Pi/Sheet 7),
#         AMP_SHUTDOWN, AMP_MUTE (von Pico/Sheet 2).
# Outputs: J6 Speaker-Left BTL, J7 Speaker-Right BTL.
# ----------------------------------------------------------------------------


def audio_sheet() -> str:
    """Sheet 6: Audio (PCM5102A I²S DAC + PAM8403 Class-D Amp + Speaker Header).

    Symbol-Pinouts korrekt nach Datasheets:
      - PCM5102A: offizielles TI-Pinout SLAS859C (CPVDD=1, ..., DVDD=20).
      - PAM8403:  Diodes Inc PAM8403DR-H Datasheet DS31295 (für JLCPCB C17337).

    Hinweis: Audio-Routing-Layout wurde für die korrigierten Pinouts neu
    aufgesetzt. Alte audio.kicad_sch v0.6-commits hatten falsche PCM5102A-
    Pin-Belegung (LRCK/BCK/DIN auf Pin 1/2/3 statt 15/13/14).
    """
    sheet_uuid = det_uuid("sheet_audio")
    sus = "sheet_audio"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    def attach_gnd(x: float, y: float, ref: str, rot: int = 270) -> None:
        symbols.append(
            place_symbol(
                lib_id="Power:GND",
                ref=f"#PWR_{ref}",
                value="GND",
                x=x, y=y, rotation=rot,
                seed_suffix=f"gnd-{ref}",
                sheet_uuid_seed=sus,
            )
        )

    # ====================================================================
    # U3 PCM5102A @ (100, 100). Body x=89.84..110.16, y=86.03..113.97.
    # Pin anchor links x=87.3, rechts x=112.7.
    # ====================================================================
    U3_X, U3_Y = 100.0, 100.0
    U3_LX, U3_RX = 87.3, 112.7

    def u3_left(pin: int) -> float:
        """Pin 1 (CPVDD) abs y=88.57. Pin N (1..10) abs y=88.57+(N-1)*2.54."""
        return 88.57 + (pin - 1) * 2.54

    def u3_right(pin: int) -> float:
        """Pin 20 (DVDD) abs y=88.57. Pin N (11..20) abs y=88.57+(20-N)*2.54."""
        return 88.57 + (20 - pin) * 2.54

    symbols.append(
        place_symbol(
            lib_id="Audio:PCM5102A",
            ref="U3",
            value="PCM5102A I²S DAC (TSSOP-20)",
            x=U3_X, y=U3_Y,
            footprint="Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm",
            datasheet="https://www.ti.com/lit/ds/symlink/pcm5102a.pdf",
            extra_props={"MPN": "PCM5102APWR", "LCSC": "C107671"},
            seed_suffix="U3",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Pin 1 CPVDD → +3V3 + C_CPVDD_BULK 10µF + C_CPVDD_HF 100nF
    p1_y = u3_left(1)
    wires.append(wire(U3_LX, p1_y, U3_LX - 3, p1_y, seed_suffix="u3-p1-cpvdd-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+3V3",
            ref="#PWR_U3_CPVDD",
            value="+3V3",
            x=U3_LX - 3, y=p1_y, rotation=90,
            seed_suffix="u3-cpvdd-flag",
            sheet_uuid_seed=sus,
        )
    )
    # CPVDD decoupling: place below pin 1 row (y > p1_y in Y-DOWN)
    # C_CPVDD_BULK 10µF + C_CPVDD_HF 100nF, both vertical chained
    junctions.append(junction(U3_LX - 3, p1_y))
    # Re-route VDD line to also hit two caps to the right of the flag
    # Approach: bring +3V3 from a horizontal trunk at y=p1_y to a vertical cap chain
    c_cpvdd_y = p1_y - 3.81  # cap above pin row visually (smaller y)
    # Hmm need to be careful with orientation. Cap pin1 (top, +) abs (sx, sy-3.81), pin2 (bottom, -) abs (sx, sy+3.81).
    # For pin1 (+, top) at p1_y, sym y = p1_y + 3.81.
    cb_x, ch_x = 80, 76  # bulk at x=80, HF at x=76
    cb_y = p1_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_CPVDD_BULK",
            value="10uF X5R 0805 (CPVDD bulk)",
            x=cb_x, y=cb_y,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A106KOQNNNE", "LCSC": "C15850"},
            seed_suffix="CCPVDD_BULK",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(U3_LX - 3, p1_y, cb_x, p1_y, seed_suffix="cpvdd-rail-to-bulk"))
    wires.append(wire(cb_x, cb_y + 3.81, cb_x, cb_y + 6, seed_suffix="ccpvdd-bulk-gnd"))
    attach_gnd(cb_x, cb_y + 6, "CCPVDD_BULK", rot=270)
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_CPVDD_HF",
            value="100nF X7R 0603 (CPVDD HF)",
            x=ch_x, y=cb_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="CCPVDD_HF",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(ch_x, p1_y, cb_x, p1_y, seed_suffix="cpvdd-bulk-to-hf"))
    junctions.append(junction(cb_x, p1_y))
    wires.append(wire(ch_x, cb_y + 3.81, ch_x, cb_y + 6, seed_suffix="ccpvdd-hf-gnd"))
    attach_gnd(ch_x, cb_y + 6, "CCPVDD_HF", rot=270)

    # ---- Pin 2 CAPP and Pin 4 CAPM: 1µF X7R between them (charge pump fly cap)
    p2_y = u3_left(2)
    p4_y = u3_left(4)
    # CAPP exits left, CAPM exits left. Wire both to a vertical at x=82, place cap between them.
    # C_FLY 1µF vertical sym at x=82, y=midpoint of p2/p4 = (88.57+1*2.54 + 88.57+3*2.54)/2 = p2_y + (p4_y-p2_y)/2
    fly_x = 82
    fly_y_mid = (p2_y + p4_y) / 2  # = 93.65
    # C pin1 (top, sy-3.81), pin2 (bottom, sy+3.81). For pin1 at p2_y=91.11: sy = 94.92. For pin2 at p4_y=96.19: sy+3.81=96.19 → sy=92.38.
    # Two constraints → must offset cap. Use small offset.
    # Pragmatic: place C at (82, 93.65). pin1 abs (82, 89.84), pin2 abs (82, 97.46). Wire pin1 to p2_y=91.11 via (82, 89.84)→(82, 91.11).
    # And wire pin2 to p4_y=96.19 via (82, 97.46)→(82, 96.19). Both small wire stubs.
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_FLY",
            value="1uF X7R 0603 (charge pump fly cap CAPP-CAPM)",
            x=fly_x, y=fly_y_mid,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R7BB105", "LCSC": "C15849"},
            seed_suffix="CFLY",
            sheet_uuid_seed=sus,
        )
    )
    # CAPP (pin 2) trace
    wires.append(wire(U3_LX, p2_y, fly_x, p2_y, seed_suffix="u3-capp-trace"))
    wires.append(wire(fly_x, p2_y, fly_x, fly_y_mid - 3.81, seed_suffix="capp-to-cfly-pin1"))
    # CAPM (pin 4) trace
    wires.append(wire(U3_LX, p4_y, fly_x, p4_y, seed_suffix="u3-capm-trace"))
    wires.append(wire(fly_x, p4_y, fly_x, fly_y_mid + 3.81, seed_suffix="capm-to-cfly-pin2"))

    # ---- Pin 3 CPGND → GND
    p3_y = u3_left(3)
    wires.append(wire(U3_LX, p3_y, U3_LX - 3, p3_y, seed_suffix="u3-cpgnd"))
    attach_gnd(U3_LX - 3, p3_y, "U3_CPGND", rot=90)

    # ---- Pin 5 VNEG → 1µF to GND (charge pump reservoir)
    p5_y = u3_left(5)
    wires.append(wire(U3_LX, p5_y, 78, p5_y, seed_suffix="u3-vneg-stub"))
    cvneg_y = p5_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_VNEG",
            value="1uF X7R 0603 (VNEG reservoir)",
            x=78, y=cvneg_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R7BB105", "LCSC": "C15849"},
            seed_suffix="CVNEG",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(78, cvneg_y + 3.81, 78, cvneg_y + 6, seed_suffix="cvneg-gnd"))
    attach_gnd(78, cvneg_y + 6, "CVNEG", rot=270)

    # ---- Pin 6 OUTL → PCM_VOUTL label (to PAM8403 INL)
    p6_y = u3_left(6)
    wires.append(wire(U3_LX, p6_y, 80, p6_y, seed_suffix="u3-outl"))
    labels.append(label(80, p6_y, "PCM_VOUTL"))

    # ---- Pin 7 OUTR → PCM_VOUTR label
    p7_y = u3_left(7)
    wires.append(wire(U3_LX, p7_y, 80, p7_y, seed_suffix="u3-outr"))
    labels.append(label(80, p7_y, "PCM_VOUTR"))

    # ---- Pin 8 AVDD → via FB1 from +3V3 + C7a/C7b lokal (v0.6 M2 Fix)
    p8_y = u3_left(8)
    # FB1 horizontal: sym at (74, p8_y) rotation=180. pin1 (-3.81, 0) rotated 180 → (3.81, 0). Abs pin1 (77.81, p8_y).
    # pin2 (+3.81, 0) → (-3.81, 0). Abs pin2 (70.19, p8_y).
    # pin1 (rechts) → U3.8 AVDD via short wire.
    # pin2 (links) → +3V3 power flag.
    symbols.append(
        place_symbol(
            lib_id="Device:Ferrite_Bead",
            ref="FB1",
            value="BLM18AG601 600R@100MHz (AVDD/DVDD split, v0.6 M2)",
            x=74, y=p8_y, rotation=180,
            footprint="Inductor_SMD:L_0603_1608Metric",
            extra_props={"MPN": "BLM18AG601SN1D", "LCSC": "C19330"},
            seed_suffix="FB1",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(U3_LX, p8_y, 77.81, p8_y, seed_suffix="u3-avdd-to-fb1"))
    wires.append(wire(70.19, p8_y, 68, p8_y, seed_suffix="fb1-to-3v3-flag"))
    symbols.append(
        place_symbol(
            lib_id="Power:+3V3",
            ref="#PWR_FB1_3V3",
            value="+3V3",
            x=68, y=p8_y, rotation=90,
            seed_suffix="fb1-3v3-flag",
            sheet_uuid_seed=sus,
        )
    )
    # AVDD decoupling C7a 10µF + C7b 100nF (between U3.8 and FB1)
    c7a_y = p8_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C7a",
            value="10uF X5R 0805 (AVDD bulk)",
            x=80, y=c7a_y,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A106KOQNNNE", "LCSC": "C15850"},
            seed_suffix="C7a",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(80, p8_y, U3_LX, p8_y, seed_suffix="c7a-to-avdd"))
    junctions.append(junction(U3_LX, p8_y))
    wires.append(wire(80, c7a_y + 3.81, 80, c7a_y + 6, seed_suffix="c7a-gnd"))
    attach_gnd(80, c7a_y + 6, "C7a", rot=270)
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C7b",
            value="100nF X7R 0603 (AVDD HF)",
            x=84, y=c7a_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="C7b",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(84, p8_y, 80, p8_y, seed_suffix="c7a-c7b"))
    junctions.append(junction(80, p8_y))
    wires.append(wire(84, c7a_y + 3.81, 84, c7a_y + 6, seed_suffix="c7b-gnd"))
    attach_gnd(84, c7a_y + 6, "C7b", rot=270)

    # ---- Pin 9 AGND → GND
    p9_y = u3_left(9)
    wires.append(wire(U3_LX, p9_y, U3_LX - 3, p9_y, seed_suffix="u3-agnd"))
    attach_gnd(U3_LX - 3, p9_y, "U3_AGND", rot=90)

    # ---- Pin 10 DEMP → GND (no de-emphasis)
    p10_y = u3_left(10)
    wires.append(wire(U3_LX, p10_y, U3_LX - 3, p10_y, seed_suffix="u3-demp"))
    attach_gnd(U3_LX - 3, p10_y, "U3_DEMP", rot=90)

    # ---- Pin 11 FLT → GND (normal latency)
    p11_y = u3_right(11)
    wires.append(wire(U3_RX, p11_y, U3_RX + 3, p11_y, seed_suffix="u3-flt"))
    attach_gnd(U3_RX + 3, p11_y, "U3_FLT", rot=270)

    # ---- Pin 12 SCK → GND (3-wire mode, internal PLL)
    p12_y = u3_right(12)
    wires.append(wire(U3_RX, p12_y, U3_RX + 3, p12_y, seed_suffix="u3-sck"))
    attach_gnd(U3_RX + 3, p12_y, "U3_SCK", rot=270)

    # ---- Pin 13 BCK ← I2S_BCK hier input
    p13_y = u3_right(13)
    wires.append(wire(U3_RX, p13_y, 120, p13_y, seed_suffix="u3-bck"))
    hlabels.append(hier_label(120, p13_y, "I2S_BCK", shape="input", rotation=180))

    # ---- Pin 14 DIN ← I2S_DOUT hier input
    p14_y = u3_right(14)
    wires.append(wire(U3_RX, p14_y, 120, p14_y, seed_suffix="u3-din"))
    hlabels.append(hier_label(120, p14_y, "I2S_DOUT", shape="input", rotation=180))

    # ---- Pin 15 LRCK ← I2S_LRCK hier input
    p15_y = u3_right(15)
    wires.append(wire(U3_RX, p15_y, 120, p15_y, seed_suffix="u3-lrck"))
    hlabels.append(hier_label(120, p15_y, "I2S_LRCK", shape="input", rotation=180))

    # ---- Pin 16 FMT → GND (I²S mode)
    p16_y = u3_right(16)
    wires.append(wire(U3_RX, p16_y, U3_RX + 3, p16_y, seed_suffix="u3-fmt"))
    attach_gnd(U3_RX + 3, p16_y, "U3_FMT", rot=270)

    # ---- Pin 17 XSMT ← PCM_XSMT (von MCP23017 GPA5 via I²C-Steuerung)
    # v0.6.3-r5 N1-Fix: GPIO-controlled XSMT statt statischem Pull-Up.
    # Default LOW via R_XSMT_PD Pull-Down = PCM5102A startet stumm.
    # Pico schaltet via MCP23017 nach Power-Sequencing HIGH = un-muted.
    p17_y = u3_right(17)
    # R_XSMT_PD vertikal: sym (132, p17_y+3.81) — pin1 (top, sym-3.81) auf XSMT,
    # pin2 (bottom, sym+3.81) auf GND.
    rxsmt_pd_y = p17_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_XSMT_PD",
            value="10k 0603 (XSMT pull-down, boot-safe default LOW = muted)",
            x=132, y=rxsmt_pd_y,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
            seed_suffix="RXSMT_PD",
            sheet_uuid_seed=sus,
        )
    )
    # Wire vom U3.17 nach rechts zu pin1 von R_XSMT_PD (132, p17_y) und weiter
    # zum hier_label PCM_XSMT (input von MCP23017).
    wires.append(wire(U3_RX, p17_y, 132, p17_y, seed_suffix="u3-xsmt-to-rpd"))
    wires.append(wire(132, p17_y, 138, p17_y, seed_suffix="u3-xsmt-to-hier"))
    junctions.append(junction(132, p17_y))
    hlabels.append(hier_label(138, p17_y, "PCM_XSMT", shape="input", rotation=180))
    # R_XSMT_PD pin2 (bottom) abs (132, rxsmt_pd_y+3.81 = p17_y+7.62) → GND
    wires.append(wire(132, rxsmt_pd_y + 3.81, 132, rxsmt_pd_y + 6, seed_suffix="rxsmt-pd-gnd"))
    attach_gnd(132, rxsmt_pd_y + 6, "RXSMT_PD", rot=270)

    # ---- Pin 18 LDOO → NC (per datasheet: leave open, optional 0.1µF for stability)
    # We add a 100nF cap for stability per TI ref design
    p18_y = u3_right(18)
    wires.append(wire(U3_RX, p18_y, 117, p18_y, seed_suffix="u3-ldoo-stub"))
    cldoo_y = p18_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_LDOO",
            value="100nF X7R 0603 (LDOO stability)",
            x=117, y=cldoo_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="CLDOO",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(117, cldoo_y + 3.81, 117, cldoo_y + 6, seed_suffix="cldoo-gnd"))
    attach_gnd(117, cldoo_y + 6, "CLDOO", rot=270)

    # ---- Pin 19 DGND → GND
    p19_y = u3_right(19)
    wires.append(wire(U3_RX, p19_y, U3_RX + 3, p19_y, seed_suffix="u3-dgnd"))
    attach_gnd(U3_RX + 3, p19_y, "U3_DGND", rot=270)

    # ---- Pin 20 DVDD → +3V3 + C8a 10µF + C8b 100nF Decoupling
    p20_y = u3_right(20)
    wires.append(wire(U3_RX, p20_y, U3_RX + 3, p20_y, seed_suffix="u3-dvdd-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+3V3",
            ref="#PWR_U3_DVDD",
            value="+3V3",
            x=U3_RX + 3, y=p20_y, rotation=270,
            seed_suffix="u3-dvdd-flag",
            sheet_uuid_seed=sus,
        )
    )
    junctions.append(junction(U3_RX + 3, p20_y))
    # C8a bulk + C8b HF decoupling
    c8a_y = p20_y + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C8a",
            value="10uF X5R 0805 (DVDD bulk)",
            x=120, y=c8a_y,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A106KOQNNNE", "LCSC": "C15850"},
            seed_suffix="C8a",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(U3_RX + 3, p20_y, 120, p20_y, seed_suffix="dvdd-to-c8a"))
    wires.append(wire(120, c8a_y + 3.81, 120, c8a_y + 6, seed_suffix="c8a-gnd"))
    attach_gnd(120, c8a_y + 6, "C8a", rot=270)
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C8b",
            value="100nF X7R 0603 (DVDD HF)",
            x=124, y=c8a_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="C8b",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(124, p20_y, 120, p20_y, seed_suffix="c8a-to-c8b"))
    junctions.append(junction(120, p20_y))
    wires.append(wire(124, c8a_y + 3.81, 124, c8a_y + 6, seed_suffix="c8b-gnd"))
    attach_gnd(124, c8a_y + 6, "C8b", rot=270)

    # ====================================================================
    # U4 PAM8403H @ (160, 130). Body x=149.84..170.16, y=121.11..138.89.
    # Pin local x=±12.7. Abs anchor: links x=147.3, rechts x=172.7.
    # Pin-Belegung VERIFIED gegen PAM8403H.PDF (Diodes Inc, Repo-Root):
    #   left:  1=-OUT_L 2=PGND 3=+OUT_L 4=PVDD 5=MUTE 6=VDD 7=INL 8=VREF
    #   right: 16=-OUT_R 15=PGND 14=+OUT_R 13=PVDD 12=SHDN 11=GND 10=INR 9=NC
    # ====================================================================
    U4_X, U4_Y = 160.0, 130.0
    U4_LX, U4_RX = 147.3, 172.7

    def u4_left(pin: int) -> float:
        """Pin 1 (-OUT_L) abs y=121.11. Pin N (1..8) abs y=121.11+(N-1)*2.54."""
        return 121.11 + (pin - 1) * 2.54

    def u4_right(pin: int) -> float:
        """Pin 16 (-OUT_R) abs y=121.11. Pin N (9..16) abs y=121.11+(16-N)*2.54."""
        return 121.11 + (16 - pin) * 2.54

    symbols.append(
        place_symbol(
            lib_id="Audio:PAM8403",
            ref="U4",
            value="PAM8403H Class-D Stereo Amp (SOIC-16)",
            x=U4_X, y=U4_Y,
            footprint="Package_SO:SOIC-16_3.9x9.9mm_P1.27mm",
            datasheet="PAM8403H.PDF (Diodes Inc Rev 1-0, Nov 2012)",
            extra_props={"MPN": "PAM8403H", "LCSC": "C17337"},
            seed_suffix="U4",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Pin 1 -OUT_L → SPK_L- (Speaker L negative)
    p1uy = u4_left(1)
    wires.append(wire(U4_LX, p1uy, 144, p1uy, seed_suffix="u4-outlm-stub"))
    labels.append(label(144, p1uy, "SPK_L-"))

    # ---- Pin 2 PGND → GND (Power Ground left)
    p2uy = u4_left(2)
    wires.append(wire(U4_LX, p2uy, U4_LX - 3, p2uy, seed_suffix="u4-pgnd-l"))
    attach_gnd(U4_LX - 3, p2uy, "U4_PGND_L", rot=90)

    # ---- Pin 3 +OUT_L → SPK_L+ (Speaker L positive)
    p3uy = u4_left(3)
    wires.append(wire(U4_LX, p3uy, 144, p3uy, seed_suffix="u4-outlp-stub"))
    labels.append(label(144, p3uy, "SPK_L+"))

    # ---- Pin 4 PVDD (left) → +5V
    p4uy = u4_left(4)
    wires.append(wire(U4_LX, p4uy, U4_LX - 3, p4uy, seed_suffix="u4-pvdd-l-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+5V",
            ref="#PWR_U4_PVDDL",
            value="+5V",
            x=U4_LX - 3, y=p4uy, rotation=90,
            seed_suffix="u4-pvdd-l-flag",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Pin 5 MUTE ← AMP_MUTE hier input (ACTIVE LOW per datasheet)
    p5uy = u4_left(5)
    wires.append(wire(U4_LX, p5uy, 138, p5uy, seed_suffix="u4-mute-stub"))
    hlabels.append(hier_label(138, p5uy, "AMP_MUTE", shape="input", rotation=0))
    # R_MUTE_PD 10k pull-down auf MUTE — Default LOW = gemuted während Pico-Boot.
    # Pico zieht HIGH erst nach Power-Sequencing. Verhindert Pop beim Boot.
    rmute_y = p5uy + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_MUTE_PD",
            value="10k 0603 (MUTE pull-down, boot-safe default LOW)",
            x=142, y=rmute_y,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
            seed_suffix="RMUTE_PD",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(138, p5uy, 142, p5uy, seed_suffix="rmute-to-mute-line"))
    junctions.append(junction(138, p5uy))
    wires.append(wire(142, rmute_y + 3.81, 142, rmute_y + 6, seed_suffix="rmute-to-gnd"))
    attach_gnd(142, rmute_y + 6, "RMUTE_PD", rot=270)

    # ---- Pin 6 VDD → +5V + C9 10µF + C9b 100nF Decoupling (v0.6 H2 Fix)
    p6uy = u4_left(6)
    wires.append(wire(U4_LX, p6uy, U4_LX - 3, p6uy, seed_suffix="u4-vdd-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+5V",
            ref="#PWR_U4_VDD",
            value="+5V",
            x=U4_LX - 3, y=p6uy, rotation=90,
            seed_suffix="u4-vdd-flag",
            sheet_uuid_seed=sus,
        )
    )
    junctions.append(junction(U4_LX - 3, p6uy))
    c9b_y = p6uy + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C9b",
            value="100nF X7R 0603 (VDD HF)",
            x=140, y=c9b_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
            seed_suffix="C9b",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(140, p6uy, U4_LX - 3, p6uy, seed_suffix="c9b-to-vdd"))
    wires.append(wire(140, c9b_y + 3.81, 140, c9b_y + 6, seed_suffix="c9b-gnd"))
    attach_gnd(140, c9b_y + 6, "C9b", rot=270)
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C9",
            value="10uF X5R 0805 (VDD bulk, v0.6 H2)",
            x=136, y=c9b_y,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A106KOQNNNE", "LCSC": "C15850"},
            seed_suffix="C9",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(136, p6uy, 140, p6uy, seed_suffix="c9-to-c9b"))
    junctions.append(junction(140, p6uy))
    wires.append(wire(136, c9b_y + 3.81, 136, c9b_y + 6, seed_suffix="c9-gnd"))
    attach_gnd(136, c9b_y + 6, "C9", rot=270)

    # ---- Pin 7 INL ← PCM_VOUTL via C_in_L 1µF (DC-block) + R_VOL_L 20k series (RI)
    # PAM8403H gain = 2*RF/RI (RF=142k internal). RI=20k → AVD=14.2 = 23 dB (Datasheet-spec).
    # Mit RI<18k wird die max-Gain-Spec überschritten und Clipping wahrscheinlicher.
    p7uy = u4_left(7)
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_VOL_L",
            value="20k 0603 (L input series, RI per PAM8403H datasheet, gain 23 dB)",
            x=140, y=p7uy, rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-0720KL", "LCSC": "C25092"},
            seed_suffix="RVOLL",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(143.81, p7uy, U4_LX, p7uy, seed_suffix="rvoll-to-inl"))
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_in_L",
            value="1uF X7R 0603 (L input DC-block)",
            x=132, y=p7uy, rotation=90,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R7BB105", "LCSC": "C15849"},
            seed_suffix="CINL",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(135.81, p7uy, 136.19, p7uy, seed_suffix="cinl-to-rvoll"))
    wires.append(wire(128.19, p7uy, 125, p7uy, seed_suffix="cinl-to-label"))
    labels.append(label(125, p7uy, "PCM_VOUTL"))

    # ---- Pin 8 VREF → 1µF X7R Bypass-Cap zu GND (REQUIRED per datasheet)
    p8uy = u4_left(8)
    wires.append(wire(U4_LX, p8uy, 144, p8uy, seed_suffix="u4-vref-stub"))
    cvref_y = p8uy + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_VREF",
            value="1uF X7R 0603 (VREF bypass - PAM8403H datasheet REQUIRED)",
            x=144, y=cvref_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R7BB105", "LCSC": "C15849"},
            seed_suffix="CVREF",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(144, cvref_y + 3.81, 144, cvref_y + 6, seed_suffix="cvref-gnd"))
    attach_gnd(144, cvref_y + 6, "CVREF", rot=270)

    # ---- Pin 9 NC (per datasheet) → NC label
    p9uy = u4_right(9)
    wires.append(wire(U4_RX, p9uy, U4_RX + 3, p9uy, seed_suffix="u4-nc-9"))
    labels.append(label(U4_RX + 3, p9uy, "NC_U4_9"))

    # ---- Pin 10 INR ← PCM_VOUTR via C_in_R 1µF + R_VOL_R 20k series (RI per datasheet)
    p10uy = u4_right(10)
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_VOL_R",
            value="20k 0603 (R input series, RI per PAM8403H datasheet, gain 23 dB)",
            x=180, y=p10uy, rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-0720KL", "LCSC": "C25092"},
            seed_suffix="RVOLR",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(U4_RX, p10uy, 176.19, p10uy, seed_suffix="u4-inr-to-rvolr"))
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_in_R",
            value="1uF X7R 0603 (R input DC-block)",
            x=188, y=p10uy, rotation=90,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R7BB105", "LCSC": "C15849"},
            seed_suffix="CINR",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(183.81, p10uy, 184.19, p10uy, seed_suffix="rvolr-to-cinr"))
    wires.append(wire(191.81, p10uy, 194, p10uy, seed_suffix="cinr-to-label"))
    labels.append(label(194, p10uy, "PCM_VOUTR"))

    # ---- Pin 11 GND (analog) → GND
    p11uy = u4_right(11)
    wires.append(wire(U4_RX, p11uy, U4_RX + 3, p11uy, seed_suffix="u4-gnd-r"))
    attach_gnd(U4_RX + 3, p11uy, "U4_AGND_R", rot=270)

    # ---- Pin 12 SHDN ← AMP_SHUTDOWN hier input (ACTIVE LOW per datasheet)
    p12uy = u4_right(12)
    wires.append(wire(U4_RX, p12uy, 180, p12uy, seed_suffix="u4-shdn-stub"))
    hlabels.append(hier_label(180, p12uy, "AMP_SHUTDOWN", shape="input", rotation=180))
    # R_SHDN_PD 10k pull-down auf SHDN — Default LOW = Amp aus während Pico-Boot.
    # Pico zieht HIGH erst nach Power-Sequencing. Verhindert un-defined Amp-State.
    rshdn_y = p12uy + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_SHDN_PD",
            value="10k 0603 (SHDN pull-down, boot-safe default LOW)",
            x=176, y=rshdn_y,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-0710KL", "LCSC": "C25804"},
            seed_suffix="RSHDN_PD",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(180, p12uy, 176, p12uy, seed_suffix="rshdn-to-shdn-line"))
    junctions.append(junction(180, p12uy))
    wires.append(wire(176, rshdn_y + 3.81, 176, rshdn_y + 6, seed_suffix="rshdn-to-gnd"))
    attach_gnd(176, rshdn_y + 6, "RSHDN_PD", rot=270)

    # ---- Pin 13 PVDD (right) → +5V
    p13uy = u4_right(13)
    wires.append(wire(U4_RX, p13uy, U4_RX + 3, p13uy, seed_suffix="u4-pvdd-r-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+5V",
            ref="#PWR_U4_PVDDR",
            value="+5V",
            x=U4_RX + 3, y=p13uy, rotation=270,
            seed_suffix="u4-pvdd-r-flag",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Pin 14 +OUT_R → SPK_R+ (Speaker R positive)
    p14uy = u4_right(14)
    wires.append(wire(U4_RX, p14uy, 176, p14uy, seed_suffix="u4-outrp-stub"))
    labels.append(label(176, p14uy, "SPK_R+"))

    # ---- Pin 15 PGND → GND (Power Ground right)
    p15uy = u4_right(15)
    wires.append(wire(U4_RX, p15uy, U4_RX + 3, p15uy, seed_suffix="u4-pgnd-r"))
    attach_gnd(U4_RX + 3, p15uy, "U4_PGND_R", rot=270)

    # ---- Pin 16 -OUT_R → SPK_R- (Speaker R negative)
    p16uy = u4_right(16)
    wires.append(wire(U4_RX, p16uy, 176, p16uy, seed_suffix="u4-outrm-stub"))
    labels.append(label(176, p16uy, "SPK_R-"))

    # ---- J6 Speaker L connector (Conn_01x02)
    j6_x, j6_y = 158, 117
    symbols.append(
        place_symbol(
            lib_id="Connector:Conn_01x02",
            ref="J6",
            value="Speaker L (PUI AS04008PS, 4R 4W)",
            x=j6_x, y=j6_y,
            footprint="Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
            extra_props={"MPN": "AS04008PS-4W-WR-R", "LCSC": "TBD"},
            seed_suffix="J6",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(161.81, 115.73, 165, 115.73, seed_suffix="j6-p1"))
    labels.append(label(165, 115.73, "SPK_L+"))
    wires.append(wire(161.81, 118.27, 165, 118.27, seed_suffix="j6-p2"))
    labels.append(label(165, 118.27, "SPK_L-"))

    # ---- J7 Speaker R connector
    j7_x, j7_y = 200, 130
    symbols.append(
        place_symbol(
            lib_id="Connector:Conn_01x02",
            ref="J7",
            value="Speaker R (PUI AS04008PS, 4R 4W)",
            x=j7_x, y=j7_y,
            footprint="Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
            extra_props={"MPN": "AS04008PS-4W-WR-R", "LCSC": "TBD"},
            seed_suffix="J7",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(203.81, 128.73, 207, 128.73, seed_suffix="j7-p1"))
    labels.append(label(207, 128.73, "SPK_R+"))
    wires.append(wire(203.81, 131.27, 207, 131.27, seed_suffix="j7-p2"))
    labels.append(label(207, 131.27, "SPK_R-"))
    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 6: Audio (PCM5102A + PAM8403H)")\n'
        f'    (date "2026-05-14")\n'
        f'    (rev "0.6.3")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6.3 §8 + Errata-Fixes (PCM5102A TI, PAM8403H PDF verifiziert)")\n'
        f'    (comment 2 "PCM5102A pinout per TI SLAS859C: CPVDD=1, OUTL=6, AVDD=8, BCK=13, DIN=14, LRCK=15, DVDD=20")\n'
        f'    (comment 3 "PAM8403H pinout per Diodes Inc PAM8403H.PDF Rev 1-0 (LCSC C17337)")\n'
        f'    (comment 4 "v0.6.3 adds R_MUTE_PD + R_SHDN_PD 10k pull-downs - Amp default-OFF/muted during Pico boot"))\n'
        "  (lib_symbols\n"
        + LIB_SYMBOLS
        + "\n  )\n"
        + "".join(symbols)
        + "".join(wires)
        + "".join(junctions)
        + "".join(labels)
        + "".join(hlabels)
        + f'  (sheet_instances\n    (path "/" (page "6")))\n'
        ")\n"
    )
    return body


# ----------------------------------------------------------------------------
# Sheet 7 — Pi Zero 2 W GPIO-Header (J2) + UART-R1 + Power-Injection
# per SPEC v0.6 §1/§8 + BOM R1.
# Inputs: +5V (zu Pi pin 2/4), GND, PICO_TX_PI_RX (von Pico GP0 zu Pi pin 10 RX).
# Outputs: PI_TX_PICO_RX (von Pi pin 8 TX zu Pico GP1 via R1 1k), I2S_BCK/LRCK/DOUT.
# ----------------------------------------------------------------------------


def pi_sheet() -> str:
    sheet_uuid = det_uuid("sheet_pi")
    sus = "sheet_pi"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    def attach_gnd(x: float, y: float, ref: str, rot: int = 270) -> None:
        symbols.append(
            place_symbol(
                lib_id="Power:GND",
                ref=f"#PWR_{ref}",
                value="GND",
                x=x, y=y, rotation=rot,
                seed_suffix=f"gnd-{ref}",
                sheet_uuid_seed=sus,
            )
        )

    # J2 Conn_02x20 @ (130, 130). Pin local: links x=-5.08, rechts x=+5.08.
    # Pin 1 (top-left) abs (124.92, 105.87). Row R abs y = 105.87 + (R-1)*2.54.
    J2_X, J2_Y = 130.0, 130.0
    J2_LX = J2_X - 5.08   # 124.92 (odd pins, left side)
    J2_RX = J2_X + 5.08   # 135.08 (even pins, right side)

    def j2_y(pin_num: int) -> float:
        """Pi-Pin-Position. Pin 1/2 row 1 = oben (y=105.87). Pin 39/40 row 20 = unten."""
        row = (pin_num - 1) // 2  # 0..19
        return 105.87 + row * 2.54

    def j2_x(pin_num: int) -> float:
        return J2_LX if pin_num % 2 == 1 else J2_RX

    symbols.append(
        place_symbol(
            lib_id="Connector:Conn_02x20",
            ref="J2",
            value="Pi Zero 2 W GPIO Header 2x20 (2.54mm)",
            x=J2_X, y=J2_Y,
            footprint="Connector_PinHeader_2.54mm:PinHeader_2x20_P2.54mm_Vertical",
            datasheet="https://datasheets.raspberrypi.com/pizero/pi-zero-2-w-product-brief.pdf",
            extra_props={
                "MPN": "Standard 2x20 2.54mm Header",
                "LCSC": "TBD",
            },
            seed_suffix="J2",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Helper: Pin → Power-Symbol via Stub-Wire
    def pin_to_power(pin: int, lib_id: str, ref: str, value: str) -> None:
        py = j2_y(pin)
        px = j2_x(pin)
        if pin % 2 == 1:  # left side, extend left
            flag_x = px - 3
            rot = 90
        else:
            flag_x = px + 3
            rot = 270
        wires.append(wire(px, py, flag_x, py, seed_suffix=f"j2-p{pin}-power"))
        symbols.append(
            place_symbol(
                lib_id=lib_id,
                ref=ref,
                value=value,
                x=flag_x, y=py, rotation=rot,
                seed_suffix=f"j2-pwr-{pin}",
                sheet_uuid_seed=sus,
            )
        )

    # ---- Helper: Pin → NC-Label (gegen Dangling-Warnings)
    def pin_to_nc(pin: int, name: str = "") -> None:
        py = j2_y(pin)
        px = j2_x(pin)
        if pin % 2 == 1:  # left
            label_x = px - 5
        else:
            label_x = px + 5
        wires.append(wire(px, py, label_x, py, seed_suffix=f"j2-p{pin}-nc"))
        labels.append(label(label_x, py, name or f"NC_PI_{pin}"))

    # ---- Helper: Pin → Hier-Label
    def pin_to_hier(pin: int, net: str, shape: str = "output") -> None:
        py = j2_y(pin)
        px = j2_x(pin)
        if pin % 2 == 1:
            hier_x = px - 5
            rot = 0
        else:
            hier_x = px + 5
            rot = 180
        wires.append(wire(px, py, hier_x, py, seed_suffix=f"j2-p{pin}-hier"))
        hlabels.append(hier_label(hier_x, py, net, shape=shape, rotation=rot))

    # ---- Power Pins:
    # Pin 2, 4 = 5V Input (von unserer PCB-Rail)
    pin_to_power(2, "Power:+5V", "#PWR_PI_5V2", "+5V")
    pin_to_power(4, "Power:+5V", "#PWR_PI_5V4", "+5V")
    # Pin 1, 17 = Pi's 3V3 output (NICHT verwenden, da unsere 3V3 vom Pico kommt)
    pin_to_nc(1, "NC_PI_3V3_OUT")
    pin_to_nc(17, "NC_PI_3V3_OUT2")
    # Pin 6, 9, 14, 20, 25, 30, 34, 39 = GND
    for gnd_pin in (6, 9, 14, 20, 25, 30, 34, 39):
        py = j2_y(gnd_pin)
        px = j2_x(gnd_pin)
        if gnd_pin % 2 == 1:
            flag_x = px - 3
            rot = 90
        else:
            flag_x = px + 3
            rot = 270
        wires.append(wire(px, py, flag_x, py, seed_suffix=f"j2-p{gnd_pin}-gnd"))
        attach_gnd(flag_x, py, f"J2_GND{gnd_pin}", rot=rot)

    # ---- Audio I²S Outputs zum PCM5102A (Sheet 6) mit Series-Resistoren
    # für Signal-Integrity über lange Traces (320mm Board) — v0.6.3-r3 N2-Fix.
    # 33Ω 0603 Series direkt am Pi-Pin, dämpft Overshoot/Reflexionen.
    def pin_to_hier_via_r(pin: int, net: str, rref: str, rval: str = "33R 0603 (I2S series term)") -> None:
        """Pi-Pin → R series → hier-output. R rotation=90 horizontal: pin1 (left)
        connects to Pi pin via short stub, pin2 (right) goes to hier label."""
        py = j2_y(pin)
        px = j2_x(pin)
        # Pin 12 ist auf der LEFT (odd=1)... wait, pin 12 is even → right side.
        # j2_x(12) = J2_RX = 135.08. So Pi-pin is on right side.
        # Wire goes right from Pi pin to R, then continues right to hier label.
        if pin % 2 == 0:  # right side (even pin numbers)
            # R sym at px+5, rotation=90. pin1 abs (px+1.19, py) (close to Pi pin), pin2 abs (px+8.81, py).
            r_sx = px + 5
            wires.append(wire(px, py, r_sx - 3.81, py, seed_suffix=f"j2-p{pin}-to-r-{rref}"))
            symbols.append(
                place_symbol(
                    lib_id="Device:R", ref=rref, value=rval,
                    x=r_sx, y=py, rotation=90,
                    footprint="Resistor_SMD:R_0603_1608Metric",
                    extra_props={"MPN": "RC0603FR-0733RL", "LCSC": "C23138"},
                    seed_suffix=rref, sheet_uuid_seed=sus,
                )
            )
            wires.append(wire(r_sx + 3.81, py, r_sx + 8, py, seed_suffix=f"r-{rref}-to-hier"))
            hlabels.append(hier_label(r_sx + 8, py, net, shape="output", rotation=180))
        else:
            r_sx = px - 5
            wires.append(wire(px, py, r_sx + 3.81, py, seed_suffix=f"j2-p{pin}-to-r-{rref}"))
            symbols.append(
                place_symbol(
                    lib_id="Device:R", ref=rref, value=rval,
                    x=r_sx, y=py, rotation=90,
                    footprint="Resistor_SMD:R_0603_1608Metric",
                    extra_props={"MPN": "RC0603FR-0733RL", "LCSC": "C23138"},
                    seed_suffix=rref, sheet_uuid_seed=sus,
                )
            )
            wires.append(wire(r_sx - 3.81, py, r_sx - 8, py, seed_suffix=f"r-{rref}-to-hier"))
            hlabels.append(hier_label(r_sx - 8, py, net, shape="output", rotation=0))

    pin_to_hier_via_r(12, "I2S_BCK", "R_BCK")     # Pi GPIO18 = PCM_CLK
    pin_to_hier_via_r(35, "I2S_LRCK", "R_LRCK")   # Pi GPIO19 = PCM_FS
    pin_to_hier_via_r(40, "I2S_DOUT", "R_DOUT")   # Pi GPIO21 = PCM_DOUT

    # ---- UART (115200) zwischen Pi und Pico:
    # Pi GPIO14 (pin 8) = Pi TX → Pico GP1 RX (via R1 1k series, per SPEC v0.6 §5+BOM)
    # Place R1 horizontal, sym at (115, j2_y(8)) rotation=90: pin1 abs (111.19, py), pin2 abs (118.81, py)
    p8_y = j2_y(8)
    p8_x = j2_x(8)  # = J2_LX = 124.92
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R1",
            value="1k 0603 (UART RX series)",
            x=119, y=p8_y, rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "RC0603FR-071KL", "LCSC": "C22548"},
            seed_suffix="R1",
            sheet_uuid_seed=sus,
        )
    )
    # R1 pin2 (right side) at (122.81, p8_y) → connect to Pi pin 8 at (124.92, p8_y)
    wires.append(wire(122.81, p8_y, p8_x, p8_y, seed_suffix="r1-to-pi-p8"))
    # R1 pin1 (left side) at (115.19, p8_y) → hier-label PI_TX_PICO_RX
    wires.append(wire(115.19, p8_y, 112, p8_y, seed_suffix="r1-to-pi-tx-pico-rx"))
    hlabels.append(hier_label(112, p8_y, "PI_TX_PICO_RX", shape="output", rotation=0))

    # Pi GPIO15 (pin 10) = Pi RX ← Pico GP0 TX (direkter Anschluss, kein R)
    pin_to_hier(10, "PICO_TX_PI_RX", shape="input")

    # ---- Alle übrigen GPIOs → NC labels (Pi-Funktionen die wir nicht nutzen)
    unused = [
        (3, "NC_GPIO2_SDA1"),
        (5, "NC_GPIO3_SCL1"),
        (7, "NC_GPIO4"),
        (11, "NC_GPIO17"),
        (13, "NC_GPIO27"),
        (15, "NC_GPIO22"),
        (16, "NC_GPIO23"),
        (18, "NC_GPIO24"),
        (19, "NC_GPIO10_MOSI"),
        (21, "NC_GPIO9_MISO"),
        (22, "NC_GPIO25"),
        (23, "NC_GPIO11_SCLK"),
        (24, "NC_GPIO8_CE0"),
        (26, "NC_GPIO7_CE1"),
        (27, "NC_GPIO0_ID_SD"),
        (28, "NC_GPIO1_ID_SC"),
        (29, "NC_GPIO5"),
        (31, "NC_GPIO6"),
        (32, "NC_GPIO12"),
        (33, "NC_GPIO13"),
        (36, "NC_GPIO16"),
        (37, "NC_GPIO26"),
        (38, "NC_GPIO20_PCM_DIN"),
    ]
    for pin, name in unused:
        pin_to_nc(pin, name)

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 7: Pi Zero 2 W GPIO Header")\n'
        f'    (date "2026-05-12")\n'
        f'    (rev "0.6")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6 §1 + §8 + BOM R1")\n'
        f'    (comment 2 "+5V Power-Injection via Pin 2+4. 8x GND auf Pi-GND-Pins")\n'
        f'    (comment 3 "I2S Audio: Pi GPIO18/19/21 → PCM5102A BCK/LRCK/DOUT")\n'
        f'    (comment 4 "UART: Pi TX (GPIO14) → Pico GP1 via R1 1k series. Pi RX (GPIO15) ← Pico GP0 direkt"))\n'
        "  (lib_symbols\n"
        + LIB_SYMBOLS
        + "\n  )\n"
        + "".join(symbols)
        + "".join(wires)
        + "".join(junctions)
        + "".join(labels)
        + "".join(hlabels)
        + f'  (sheet_instances\n    (path "/" (page "7")))\n'
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
    enc_uuid = det_uuid("sheet_encoder")
    audio_uuid = det_uuid("sheet_audio")
    pi_uuid = det_uuid("sheet_pi")
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
        # ---- Sheet 2: Pico (extended height für Encoder-Pins) ----
        f'  (sheet (at 130 40) (size 60 130) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{pico_uuid}")\n'
        f'    (property "Sheetname" "Pico" (at 130 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "pico.kicad_sch" (at 130 170.5 0)\n'
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
        f'      (uuid "{det_uuid("picopin_mcpint")}"))\n'
        # ---- Encoder-Pins (12) ----
        f'    (pin "DRIVE_A" output (at 190 110 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_drv_a")}"))\n'
        f'    (pin "DRIVE_B" output (at 190 115 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_drv_b")}"))\n'
        f'    (pin "DRIVE_SW" output (at 190 120 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_drv_sw")}"))\n'
        f'    (pin "BRIGHT_A" output (at 190 125 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_brt_a")}"))\n'
        f'    (pin "BRIGHT_B" output (at 190 130 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_brt_b")}"))\n'
        f'    (pin "BRIGHT_SW" output (at 190 135 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_brt_sw")}"))\n'
        f'    (pin "DISPLAY_A" output (at 190 140 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_dsp_a")}"))\n'
        f'    (pin "DISPLAY_B" output (at 190 145 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_dsp_b")}"))\n'
        f'    (pin "DISPLAY_SW" output (at 190 150 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_dsp_sw")}"))\n'
        f'    (pin "VOL_A" output (at 190 155 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_vol_a")}"))\n'
        f'    (pin "VOL_B" output (at 190 160 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_vol_b")}"))\n'
        f'    (pin "VOL_SW" output (at 190 165 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_vol_sw")}"))\n'
        # ---- Audio control outputs ----
        f'    (pin "AMP_SHUTDOWN" output (at 130 95 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_amp_shdn")}"))\n'
        f'    (pin "AMP_MUTE" output (at 130 100 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_amp_mute")}"))\n'
        # ---- UART to/from Pi ----
        f'    (pin "PICO_TX_PI_RX" output (at 130 105 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_uart_tx")}"))\n'
        f'    (pin "PI_TX_PICO_RX" input (at 130 110 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_uart_rx")}")))\n'
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
        # ---- Sheet 4: MCP23017 (verschoben nach (130, 200) wegen Pico-Expansion) ----
        f'  (sheet (at 130 200) (size 60 30) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{mcp_uuid}")\n'
        f'    (property "Sheetname" "MCP23017" (at 130 199 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "mcp.kicad_sch" (at 130 230.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "I2C_SDA" bidirectional (at 130 210 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("mcppin_sda")}"))\n'
        f'    (pin "I2C_SCL" bidirectional (at 130 215 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("mcppin_scl")}"))\n'
        f'    (pin "MCP_INT" output (at 130 220 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("mcppin_int")}"))\n'
        # v0.6.3-r5 N1-Fix: PCM_XSMT output (MCP GPA5 drives PCM5102A XSMT)
        f'    (pin "PCM_XSMT" output (at 190 215 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("mcppin_xsmt")}")))\n'
        # ---- Sheet 5: Encoders ----
        f'  (sheet (at 230 105) (size 60 70) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{enc_uuid}")\n'
        f'    (property "Sheetname" "Encoders" (at 230 104 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "encoder.kicad_sch" (at 230 175.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "DRIVE_A" input (at 230 110 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_drv_a")}"))\n'
        f'    (pin "DRIVE_B" input (at 230 115 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_drv_b")}"))\n'
        f'    (pin "DRIVE_SW" input (at 230 120 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_drv_sw")}"))\n'
        f'    (pin "BRIGHT_A" input (at 230 125 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_brt_a")}"))\n'
        f'    (pin "BRIGHT_B" input (at 230 130 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_brt_b")}"))\n'
        f'    (pin "BRIGHT_SW" input (at 230 135 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_brt_sw")}"))\n'
        f'    (pin "DISPLAY_A" input (at 230 140 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_dsp_a")}"))\n'
        f'    (pin "DISPLAY_B" input (at 230 145 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_dsp_b")}"))\n'
        f'    (pin "DISPLAY_SW" input (at 230 150 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_dsp_sw")}"))\n'
        f'    (pin "VOL_A" input (at 230 155 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_vol_a")}"))\n'
        f'    (pin "VOL_B" input (at 230 160 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_vol_b")}"))\n'
        f'    (pin "VOL_SW" input (at 230 165 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("encpin_vol_sw")}")))\n'
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
        f'  (wire (pts (xy 125 210) (xy 130 210)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_sda_mcp")}"))\n'
        f'  (label "I2C_SDA" (at 125 210 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_sda_mcp")}"))\n'
        f'  (wire (pts (xy 125 215) (xy 130 215)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_scl_mcp")}"))\n'
        f'  (label "I2C_SCL" (at 125 215 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_scl_mcp")}"))\n'
        f'  (wire (pts (xy 125 220) (xy 130 220)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_mcpint_mcp")}"))\n'
        f'  (label "MCP_INT" (at 125 220 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_mcpint_mcp")}"))\n'
        # ---- Sheet 7: Pi Header (rechts unten) ----
        f'  (sheet (at 330 40) (size 60 130) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{pi_uuid}")\n'
        f'    (property "Sheetname" "PiHeader" (at 330 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "pi.kicad_sch" (at 330 170.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "PICO_TX_PI_RX" input (at 330 50 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("pipin_uart_tx")}"))\n'
        f'    (pin "PI_TX_PICO_RX" output (at 330 55 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("pipin_uart_rx")}"))\n'
        f'    (pin "I2S_BCK" output (at 330 65 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("pipin_i2sbck")}"))\n'
        f'    (pin "I2S_LRCK" output (at 330 70 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("pipin_i2slrck")}"))\n'
        f'    (pin "I2S_DOUT" output (at 330 75 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("pipin_i2sdout")}")))\n'
        # ---- Inter-sheet labels: Pico UART → Pi UART
        f'  (wire (pts (xy 125 105) (xy 130 105)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_uart_tx_pico")}"))\n'
        f'  (label "PICO_TX_PI_RX" (at 125 105 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_uart_tx_pico")}"))\n'
        f'  (wire (pts (xy 125 110) (xy 130 110)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_uart_rx_pico")}"))\n'
        f'  (label "PI_TX_PICO_RX" (at 125 110 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_uart_rx_pico")}"))\n'
        f'  (wire (pts (xy 325 50) (xy 330 50)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_uart_tx_pi")}"))\n'
        f'  (label "PICO_TX_PI_RX" (at 325 50 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_uart_tx_pi")}"))\n'
        f'  (wire (pts (xy 325 55) (xy 330 55)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_uart_rx_pi")}"))\n'
        f'  (label "PI_TX_PICO_RX" (at 325 55 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_uart_rx_pi")}"))\n'
        # ---- Inter-sheet labels: Pi I2S → Audio Sheet (matching labels already on Audio side)
        f'  (wire (pts (xy 325 65) (xy 330 65)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2sbck_pi")}"))\n'
        f'  (label "I2S_BCK" (at 325 65 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2sbck_pi")}"))\n'
        f'  (wire (pts (xy 325 70) (xy 330 70)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2slrck_pi")}"))\n'
        f'  (label "I2S_LRCK" (at 325 70 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2slrck_pi")}"))\n'
        f'  (wire (pts (xy 325 75) (xy 330 75)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2sdout_pi")}"))\n'
        f'  (label "I2S_DOUT" (at 325 75 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2sdout_pi")}"))\n'
        # ---- Sheet 6: Audio (rechts neben MCP, unter Encoder-Sheet) ----
        f'  (sheet (at 230 200) (size 60 55) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{audio_uuid}")\n'
        f'    (property "Sheetname" "Audio" (at 230 199 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "audio.kicad_sch" (at 230 250.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "I2S_BCK" input (at 230 210 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_bck")}"))\n'
        f'    (pin "I2S_LRCK" input (at 230 215 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_lrck")}"))\n'
        f'    (pin "I2S_DOUT" input (at 230 220 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_dout")}"))\n'
        f'    (pin "AMP_SHUTDOWN" input (at 230 230 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_shdn")}"))\n'
        f'    (pin "AMP_MUTE" input (at 230 235 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_mute")}"))\n'
        # v0.6.3-r5 N1-Fix: PCM_XSMT input (from MCP GPA5)
        f'    (pin "PCM_XSMT" input (at 230 240 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_xsmt")}")))\n'
        # ---- Labels für Audio: Pico AMP_SHUTDOWN/MUTE → Audio Sheet inputs
        f'  (wire (pts (xy 125 95) (xy 130 95)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ampshdn_pico")}"))\n'
        f'  (label "AMP_SHUTDOWN" (at 125 95 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_ampshdn_pico")}"))\n'
        f'  (wire (pts (xy 125 100) (xy 130 100)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ampmute_pico")}"))\n'
        f'  (label "AMP_MUTE" (at 125 100 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_ampmute_pico")}"))\n'
        f'  (wire (pts (xy 225 230) (xy 230 230)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ampshdn_audio")}"))\n'
        f'  (label "AMP_SHUTDOWN" (at 225 230 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_ampshdn_audio")}"))\n'
        f'  (wire (pts (xy 225 235) (xy 230 235)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ampmute_audio")}"))\n'
        f'  (label "AMP_MUTE" (at 225 235 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_ampmute_audio")}"))\n'
        # v0.6.3-r5 N1-Fix: PCM_XSMT label-bridge zwischen MCP-Sheet (rechte Seite, y=215) und Audio-Sheet (linke Seite, y=240)
        f'  (wire (pts (xy 190 215) (xy 195 215)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_xsmt_mcp")}"))\n'
        f'  (label "PCM_XSMT" (at 195 215 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_xsmt_mcp")}"))\n'
        f'  (wire (pts (xy 225 240) (xy 230 240)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_xsmt_audio")}"))\n'
        f'  (label "PCM_XSMT" (at 225 240 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_xsmt_audio")}"))\n'
        # ---- I2S labels (von Sheet 7 Pi-Header — Sheet 7 noch nicht implementiert,
        # I2S-Labels bleiben momentan dangling auf Audio-Sheet-Input-Seite)
        f'  (wire (pts (xy 225 210) (xy 230 210)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2sbck_audio")}"))\n'
        f'  (label "I2S_BCK" (at 225 210 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2sbck_audio")}"))\n'
        f'  (wire (pts (xy 225 215) (xy 230 215)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2slrck_audio")}"))\n'
        f'  (label "I2S_LRCK" (at 225 215 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2slrck_audio")}"))\n'
        f'  (wire (pts (xy 225 220) (xy 230 220)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2sdout_audio")}"))\n'
        f'  (label "I2S_DOUT" (at 225 220 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2sdout_audio")}"))\n'
        # ---- Inter-sheet wires Sheet 2 → Sheet 5 (Pico Encoder Bus → Encoder Sheet) ----
        f'  (wire (pts (xy 190 110) (xy 230 110)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_drv_a")}"))\n'
        f'  (wire (pts (xy 190 115) (xy 230 115)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_drv_b")}"))\n'
        f'  (wire (pts (xy 190 120) (xy 230 120)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_drv_sw")}"))\n'
        f'  (wire (pts (xy 190 125) (xy 230 125)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_brt_a")}"))\n'
        f'  (wire (pts (xy 190 130) (xy 230 130)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_brt_b")}"))\n'
        f'  (wire (pts (xy 190 135) (xy 230 135)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_brt_sw")}"))\n'
        f'  (wire (pts (xy 190 140) (xy 230 140)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_dsp_a")}"))\n'
        f'  (wire (pts (xy 190 145) (xy 230 145)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_dsp_b")}"))\n'
        f'  (wire (pts (xy 190 150) (xy 230 150)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_dsp_sw")}"))\n'
        f'  (wire (pts (xy 190 155) (xy 230 155)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_vol_a")}"))\n'
        f'  (wire (pts (xy 190 160) (xy 230 160)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_vol_b")}"))\n'
        f'  (wire (pts (xy 190 165) (xy 230 165)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_vol_sw")}"))\n'
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
    (OUT_DIR / "encoder.kicad_sch").write_text(encoder_sheet())
    (OUT_DIR / "audio.kicad_sch").write_text(audio_sheet())
    (OUT_DIR / "pi.kicad_sch").write_text(pi_sheet())
    print(f"Wrote KiCad project + Sheets 1+2+3+4+5+6+7 to {OUT_DIR}")


if __name__ == "__main__":
    main()
