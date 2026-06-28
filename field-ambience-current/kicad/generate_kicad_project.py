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
            [det_uuid("sheet_stm32"), "STM32H743"],
            [det_uuid("sheet_lcd"), "LCD"],
            [det_uuid("sheet_mcp"), "MCP23017"],
            [det_uuid("sheet_encoder"), "Encoders"],
            [det_uuid("sheet_audio"), "Audio"],
            [det_uuid("sheet_battery"), "Battery"],
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


def _stm32h743_lib_symbol() -> str:
    """STM32H743VIT6 LQFP-100 — Pin-Daten 1:1 aus der offiziellen KiCad-Lib
    (KiCad/kicad-symbols MCU_ST_STM32H7, Symbol STM32H743VITx; generiert aus
    ST-CubeMX-Daten) und gegen SPEC v0.7 §5 / DS12110 Rev 5 Table 8 doppelt
    verifiziert: alle 52 in der SPEC belegten Zuordnungen stimmen überein."""
    out = []
    out.append('    (symbol "STM32H743VIT6" (pin_names (offset 0.508)) (in_bom yes) (on_board yes)')
    out.append('      (property "Reference" "U" (at 0 0 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Value" "STM32H743VIT6" (at 0 -2.54 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "STM32H743VIT6_0_1"')
    out.append('        (rectangle (start -17.78 63.5) (end 17.78 -66.04) (stroke (width 0.254) (type default)) (fill (type background)))')
    out.append('      )')
    out.append('      (symbol "STM32H743VIT6_1_1"')
    out.append('        (pin bidirectional line (at -22.86 12.7 0) (length 5.08)')
    out.append('          (name "PE2" (effects (font (size 1.27 1.27))))')
    out.append('          (number "1" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 10.16 0) (length 5.08)')
    out.append('          (name "PE3" (effects (font (size 1.27 1.27))))')
    out.append('          (number "2" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 7.62 0) (length 5.08)')
    out.append('          (name "PE4" (effects (font (size 1.27 1.27))))')
    out.append('          (number "3" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 5.08 0) (length 5.08)')
    out.append('          (name "PE5" (effects (font (size 1.27 1.27))))')
    out.append('          (number "4" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 2.54 0) (length 5.08)')
    out.append('          (name "PE6" (effects (font (size 1.27 1.27))))')
    out.append('          (number "5" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -7.62 68.58 270) (length 5.08)')
    out.append('          (name "VBAT" (effects (font (size 1.27 1.27))))')
    out.append('          (number "6" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -58.42 180) (length 5.08)')
    out.append('          (name "PC13" (effects (font (size 1.27 1.27))))')
    out.append('          (number "7" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -60.96 180) (length 5.08)')
    out.append('          (name "PC14" (effects (font (size 1.27 1.27))))')
    out.append('          (number "8" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -63.5 180) (length 5.08)')
    out.append('          (name "PC15" (effects (font (size 1.27 1.27))))')
    out.append('          (number "9" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -7.62 -71.12 90) (length 5.08)')
    out.append('          (name "VSS" (effects (font (size 1.27 1.27))))')
    out.append('          (number "10" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -5.08 68.58 270) (length 5.08)')
    out.append('          (name "VDD" (effects (font (size 1.27 1.27))))')
    out.append('          (number "11" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin input line (at -22.86 25.4 0) (length 5.08)')
    out.append('          (name "PH0" (effects (font (size 1.27 1.27))))')
    out.append('          (number "12" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin input line (at -22.86 22.86 0) (length 5.08)')
    out.append('          (name "PH1" (effects (font (size 1.27 1.27))))')
    out.append('          (number "13" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin input line (at -22.86 60.96 0) (length 5.08)')
    out.append('          (name "NRST" (effects (font (size 1.27 1.27))))')
    out.append('          (number "14" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -25.4 180) (length 5.08)')
    out.append('          (name "PC0" (effects (font (size 1.27 1.27))))')
    out.append('          (number "15" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -27.94 180) (length 5.08)')
    out.append('          (name "PC1" (effects (font (size 1.27 1.27))))')
    out.append('          (number "16" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -30.48 180) (length 5.08)')
    out.append('          (name "PC2_C" (effects (font (size 1.27 1.27))))')
    out.append('          (number "17" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -33.02 180) (length 5.08)')
    out.append('          (name "PC3_C" (effects (font (size 1.27 1.27))))')
    out.append('          (number "18" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at 5.08 -71.12 90) (length 5.08)')
    out.append('          (name "VSSA" (effects (font (size 1.27 1.27))))')
    out.append('          (number "19" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 45.72 0) (length 5.08)')
    out.append('          (name "VREF+" (effects (font (size 1.27 1.27))))')
    out.append('          (number "20" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at 7.62 68.58 270) (length 5.08)')
    out.append('          (name "VDDA" (effects (font (size 1.27 1.27))))')
    out.append('          (number "21" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 60.96 180) (length 5.08)')
    out.append('          (name "PA0" (effects (font (size 1.27 1.27))))')
    out.append('          (number "22" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 58.42 180) (length 5.08)')
    out.append('          (name "PA1" (effects (font (size 1.27 1.27))))')
    out.append('          (number "23" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 55.88 180) (length 5.08)')
    out.append('          (name "PA2" (effects (font (size 1.27 1.27))))')
    out.append('          (number "24" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 53.34 180) (length 5.08)')
    out.append('          (name "PA3" (effects (font (size 1.27 1.27))))')
    out.append('          (number "25" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -5.08 -71.12 90) (length 5.08)')
    out.append('          (name "VSS" (effects (font (size 1.27 1.27))))')
    out.append('          (number "26" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -2.54 68.58 270) (length 5.08)')
    out.append('          (name "VDD" (effects (font (size 1.27 1.27))))')
    out.append('          (number "27" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 50.8 180) (length 5.08)')
    out.append('          (name "PA4" (effects (font (size 1.27 1.27))))')
    out.append('          (number "28" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 48.26 180) (length 5.08)')
    out.append('          (name "PA5" (effects (font (size 1.27 1.27))))')
    out.append('          (number "29" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 45.72 180) (length 5.08)')
    out.append('          (name "PA6" (effects (font (size 1.27 1.27))))')
    out.append('          (number "30" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 43.18 180) (length 5.08)')
    out.append('          (name "PA7" (effects (font (size 1.27 1.27))))')
    out.append('          (number "31" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -35.56 180) (length 5.08)')
    out.append('          (name "PC4" (effects (font (size 1.27 1.27))))')
    out.append('          (number "32" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -38.1 180) (length 5.08)')
    out.append('          (name "PC5" (effects (font (size 1.27 1.27))))')
    out.append('          (number "33" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 17.78 180) (length 5.08)')
    out.append('          (name "PB0" (effects (font (size 1.27 1.27))))')
    out.append('          (number "34" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 15.24 180) (length 5.08)')
    out.append('          (name "PB1" (effects (font (size 1.27 1.27))))')
    out.append('          (number "35" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 12.7 180) (length 5.08)')
    out.append('          (name "PB2" (effects (font (size 1.27 1.27))))')
    out.append('          (number "36" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 0.0 0) (length 5.08)')
    out.append('          (name "PE7" (effects (font (size 1.27 1.27))))')
    out.append('          (number "37" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -2.54 0) (length 5.08)')
    out.append('          (name "PE8" (effects (font (size 1.27 1.27))))')
    out.append('          (number "38" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -5.08 0) (length 5.08)')
    out.append('          (name "PE9" (effects (font (size 1.27 1.27))))')
    out.append('          (number "39" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -7.62 0) (length 5.08)')
    out.append('          (name "PE10" (effects (font (size 1.27 1.27))))')
    out.append('          (number "40" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -10.16 0) (length 5.08)')
    out.append('          (name "PE11" (effects (font (size 1.27 1.27))))')
    out.append('          (number "41" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -12.7 0) (length 5.08)')
    out.append('          (name "PE12" (effects (font (size 1.27 1.27))))')
    out.append('          (number "42" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -15.24 0) (length 5.08)')
    out.append('          (name "PE13" (effects (font (size 1.27 1.27))))')
    out.append('          (number "43" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -17.78 0) (length 5.08)')
    out.append('          (name "PE14" (effects (font (size 1.27 1.27))))')
    out.append('          (number "44" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -20.32 0) (length 5.08)')
    out.append('          (name "PE15" (effects (font (size 1.27 1.27))))')
    out.append('          (number "45" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -7.62 180) (length 5.08)')
    out.append('          (name "PB10" (effects (font (size 1.27 1.27))))')
    out.append('          (number "46" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -10.16 180) (length 5.08)')
    out.append('          (name "PB11" (effects (font (size 1.27 1.27))))')
    out.append('          (number "47" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -22.86 50.8 0) (length 5.08)')
    out.append('          (name "VCAP1" (effects (font (size 1.27 1.27))))')
    out.append('          (number "48" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -2.54 -71.12 90) (length 5.08)')
    out.append('          (name "VSS" (effects (font (size 1.27 1.27))))')
    out.append('          (number "49" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at 0.0 68.58 270) (length 5.08)')
    out.append('          (name "VDD" (effects (font (size 1.27 1.27))))')
    out.append('          (number "50" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -12.7 180) (length 5.08)')
    out.append('          (name "PB12" (effects (font (size 1.27 1.27))))')
    out.append('          (number "51" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -15.24 180) (length 5.08)')
    out.append('          (name "PB13" (effects (font (size 1.27 1.27))))')
    out.append('          (number "52" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -17.78 180) (length 5.08)')
    out.append('          (name "PB14" (effects (font (size 1.27 1.27))))')
    out.append('          (number "53" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -20.32 180) (length 5.08)')
    out.append('          (name "PB15" (effects (font (size 1.27 1.27))))')
    out.append('          (number "54" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -45.72 0) (length 5.08)')
    out.append('          (name "PD8" (effects (font (size 1.27 1.27))))')
    out.append('          (number "55" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -48.26 0) (length 5.08)')
    out.append('          (name "PD9" (effects (font (size 1.27 1.27))))')
    out.append('          (number "56" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -50.8 0) (length 5.08)')
    out.append('          (name "PD10" (effects (font (size 1.27 1.27))))')
    out.append('          (number "57" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -53.34 0) (length 5.08)')
    out.append('          (name "PD11" (effects (font (size 1.27 1.27))))')
    out.append('          (number "58" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -55.88 0) (length 5.08)')
    out.append('          (name "PD12" (effects (font (size 1.27 1.27))))')
    out.append('          (number "59" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -58.42 0) (length 5.08)')
    out.append('          (name "PD13" (effects (font (size 1.27 1.27))))')
    out.append('          (number "60" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -60.96 0) (length 5.08)')
    out.append('          (name "PD14" (effects (font (size 1.27 1.27))))')
    out.append('          (number "61" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -63.5 0) (length 5.08)')
    out.append('          (name "PD15" (effects (font (size 1.27 1.27))))')
    out.append('          (number "62" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -40.64 180) (length 5.08)')
    out.append('          (name "PC6" (effects (font (size 1.27 1.27))))')
    out.append('          (number "63" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -43.18 180) (length 5.08)')
    out.append('          (name "PC7" (effects (font (size 1.27 1.27))))')
    out.append('          (number "64" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -45.72 180) (length 5.08)')
    out.append('          (name "PC8" (effects (font (size 1.27 1.27))))')
    out.append('          (number "65" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -48.26 180) (length 5.08)')
    out.append('          (name "PC9" (effects (font (size 1.27 1.27))))')
    out.append('          (number "66" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 40.64 180) (length 5.08)')
    out.append('          (name "PA8" (effects (font (size 1.27 1.27))))')
    out.append('          (number "67" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 38.1 180) (length 5.08)')
    out.append('          (name "PA9" (effects (font (size 1.27 1.27))))')
    out.append('          (number "68" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 35.56 180) (length 5.08)')
    out.append('          (name "PA10" (effects (font (size 1.27 1.27))))')
    out.append('          (number "69" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 33.02 180) (length 5.08)')
    out.append('          (name "PA11" (effects (font (size 1.27 1.27))))')
    out.append('          (number "70" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 30.48 180) (length 5.08)')
    out.append('          (name "PA12" (effects (font (size 1.27 1.27))))')
    out.append('          (number "71" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 27.94 180) (length 5.08)')
    out.append('          (name "PA13" (effects (font (size 1.27 1.27))))')
    out.append('          (number "72" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -22.86 48.26 0) (length 5.08)')
    out.append('          (name "VCAP2" (effects (font (size 1.27 1.27))))')
    out.append('          (number "73" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at 0.0 -71.12 90) (length 5.08)')
    out.append('          (name "VSS" (effects (font (size 1.27 1.27))))')
    out.append('          (number "74" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at 2.54 68.58 270) (length 5.08)')
    out.append('          (name "VDD" (effects (font (size 1.27 1.27))))')
    out.append('          (number "75" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 25.4 180) (length 5.08)')
    out.append('          (name "PA14" (effects (font (size 1.27 1.27))))')
    out.append('          (number "76" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 22.86 180) (length 5.08)')
    out.append('          (name "PA15" (effects (font (size 1.27 1.27))))')
    out.append('          (number "77" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -50.8 180) (length 5.08)')
    out.append('          (name "PC10" (effects (font (size 1.27 1.27))))')
    out.append('          (number "78" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -53.34 180) (length 5.08)')
    out.append('          (name "PC11" (effects (font (size 1.27 1.27))))')
    out.append('          (number "79" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -55.88 180) (length 5.08)')
    out.append('          (name "PC12" (effects (font (size 1.27 1.27))))')
    out.append('          (number "80" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -25.4 0) (length 5.08)')
    out.append('          (name "PD0" (effects (font (size 1.27 1.27))))')
    out.append('          (number "81" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -27.94 0) (length 5.08)')
    out.append('          (name "PD1" (effects (font (size 1.27 1.27))))')
    out.append('          (number "82" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -30.48 0) (length 5.08)')
    out.append('          (name "PD2" (effects (font (size 1.27 1.27))))')
    out.append('          (number "83" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -33.02 0) (length 5.08)')
    out.append('          (name "PD3" (effects (font (size 1.27 1.27))))')
    out.append('          (number "84" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -35.56 0) (length 5.08)')
    out.append('          (name "PD4" (effects (font (size 1.27 1.27))))')
    out.append('          (number "85" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -38.1 0) (length 5.08)')
    out.append('          (name "PD5" (effects (font (size 1.27 1.27))))')
    out.append('          (number "86" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -40.64 0) (length 5.08)')
    out.append('          (name "PD6" (effects (font (size 1.27 1.27))))')
    out.append('          (number "87" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 -43.18 0) (length 5.08)')
    out.append('          (name "PD7" (effects (font (size 1.27 1.27))))')
    out.append('          (number "88" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 10.16 180) (length 5.08)')
    out.append('          (name "PB3" (effects (font (size 1.27 1.27))))')
    out.append('          (number "89" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 7.62 180) (length 5.08)')
    out.append('          (name "PB4" (effects (font (size 1.27 1.27))))')
    out.append('          (number "90" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 5.08 180) (length 5.08)')
    out.append('          (name "PB5" (effects (font (size 1.27 1.27))))')
    out.append('          (number "91" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 2.54 180) (length 5.08)')
    out.append('          (name "PB6" (effects (font (size 1.27 1.27))))')
    out.append('          (number "92" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 0.0 180) (length 5.08)')
    out.append('          (name "PB7" (effects (font (size 1.27 1.27))))')
    out.append('          (number "93" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin input line (at -22.86 55.88 0) (length 5.08)')
    out.append('          (name "BOOT0" (effects (font (size 1.27 1.27))))')
    out.append('          (number "94" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -2.54 180) (length 5.08)')
    out.append('          (name "PB8" (effects (font (size 1.27 1.27))))')
    out.append('          (number "95" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at 22.86 -5.08 180) (length 5.08)')
    out.append('          (name "PB9" (effects (font (size 1.27 1.27))))')
    out.append('          (number "96" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 17.78 0) (length 5.08)')
    out.append('          (name "PE0" (effects (font (size 1.27 1.27))))')
    out.append('          (number "97" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin bidirectional line (at -22.86 15.24 0) (length 5.08)')
    out.append('          (name "PE1" (effects (font (size 1.27 1.27))))')
    out.append('          (number "98" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at 2.54 -71.12 90) (length 5.08)')
    out.append('          (name "VSS" (effects (font (size 1.27 1.27))))')
    out.append('          (number "99" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at 5.08 68.58 270) (length 5.08)')
    out.append('          (name "VDD" (effects (font (size 1.27 1.27))))')
    out.append('          (number "100" (effects (font (size 1.27 1.27)))))')
    out.append('      )')
    out.append('    )')
    return '\n'.join(out)


# Pin-Kontaktpunkte des STM32H743VIT6-Symbols in Symbol-Lokalkoordinaten (mm,
# Y-UP wie lib_symbols). Verdrahtung im Sheet rechnet via pin_abs() um.
# orient: 0=Pin zeigt nach rechts (linke Symbolseite), 180=links (rechte Seite),
# 90=nach oben (Unterseite), 270=nach unten (Oberseite).
STM32_PIN_LOC = {
    1: (-22.86, 12.7, 0),  # PE2
    2: (-22.86, 10.16, 0),  # PE3
    3: (-22.86, 7.62, 0),  # PE4
    4: (-22.86, 5.08, 0),  # PE5
    5: (-22.86, 2.54, 0),  # PE6
    6: (-7.62, 68.58, 270),  # VBAT
    7: (22.86, -58.42, 180),  # PC13
    8: (22.86, -60.96, 180),  # PC14
    9: (22.86, -63.5, 180),  # PC15
    10: (-7.62, -71.12, 90),  # VSS
    11: (-5.08, 68.58, 270),  # VDD
    12: (-22.86, 25.4, 0),  # PH0
    13: (-22.86, 22.86, 0),  # PH1
    14: (-22.86, 60.96, 0),  # NRST
    15: (22.86, -25.4, 180),  # PC0
    16: (22.86, -27.94, 180),  # PC1
    17: (22.86, -30.48, 180),  # PC2_C
    18: (22.86, -33.02, 180),  # PC3_C
    19: (5.08, -71.12, 90),  # VSSA
    20: (-22.86, 45.72, 0),  # VREF+
    21: (7.62, 68.58, 270),  # VDDA
    22: (22.86, 60.96, 180),  # PA0
    23: (22.86, 58.42, 180),  # PA1
    24: (22.86, 55.88, 180),  # PA2
    25: (22.86, 53.34, 180),  # PA3
    26: (-5.08, -71.12, 90),  # VSS
    27: (-2.54, 68.58, 270),  # VDD
    28: (22.86, 50.8, 180),  # PA4
    29: (22.86, 48.26, 180),  # PA5
    30: (22.86, 45.72, 180),  # PA6
    31: (22.86, 43.18, 180),  # PA7
    32: (22.86, -35.56, 180),  # PC4
    33: (22.86, -38.1, 180),  # PC5
    34: (22.86, 17.78, 180),  # PB0
    35: (22.86, 15.24, 180),  # PB1
    36: (22.86, 12.7, 180),  # PB2
    37: (-22.86, 0.0, 0),  # PE7
    38: (-22.86, -2.54, 0),  # PE8
    39: (-22.86, -5.08, 0),  # PE9
    40: (-22.86, -7.62, 0),  # PE10
    41: (-22.86, -10.16, 0),  # PE11
    42: (-22.86, -12.7, 0),  # PE12
    43: (-22.86, -15.24, 0),  # PE13
    44: (-22.86, -17.78, 0),  # PE14
    45: (-22.86, -20.32, 0),  # PE15
    46: (22.86, -7.62, 180),  # PB10
    47: (22.86, -10.16, 180),  # PB11
    48: (-22.86, 50.8, 0),  # VCAP1
    49: (-2.54, -71.12, 90),  # VSS
    50: (0.0, 68.58, 270),  # VDD
    51: (22.86, -12.7, 180),  # PB12
    52: (22.86, -15.24, 180),  # PB13
    53: (22.86, -17.78, 180),  # PB14
    54: (22.86, -20.32, 180),  # PB15
    55: (-22.86, -45.72, 0),  # PD8
    56: (-22.86, -48.26, 0),  # PD9
    57: (-22.86, -50.8, 0),  # PD10
    58: (-22.86, -53.34, 0),  # PD11
    59: (-22.86, -55.88, 0),  # PD12
    60: (-22.86, -58.42, 0),  # PD13
    61: (-22.86, -60.96, 0),  # PD14
    62: (-22.86, -63.5, 0),  # PD15
    63: (22.86, -40.64, 180),  # PC6
    64: (22.86, -43.18, 180),  # PC7
    65: (22.86, -45.72, 180),  # PC8
    66: (22.86, -48.26, 180),  # PC9
    67: (22.86, 40.64, 180),  # PA8
    68: (22.86, 38.1, 180),  # PA9
    69: (22.86, 35.56, 180),  # PA10
    70: (22.86, 33.02, 180),  # PA11
    71: (22.86, 30.48, 180),  # PA12
    72: (22.86, 27.94, 180),  # PA13
    73: (-22.86, 48.26, 0),  # VCAP2
    74: (0.0, -71.12, 90),  # VSS
    75: (2.54, 68.58, 270),  # VDD
    76: (22.86, 25.4, 180),  # PA14
    77: (22.86, 22.86, 180),  # PA15
    78: (22.86, -50.8, 180),  # PC10
    79: (22.86, -53.34, 180),  # PC11
    80: (22.86, -55.88, 180),  # PC12
    81: (-22.86, -25.4, 0),  # PD0
    82: (-22.86, -27.94, 0),  # PD1
    83: (-22.86, -30.48, 0),  # PD2
    84: (-22.86, -33.02, 0),  # PD3
    85: (-22.86, -35.56, 0),  # PD4
    86: (-22.86, -38.1, 0),  # PD5
    87: (-22.86, -40.64, 0),  # PD6
    88: (-22.86, -43.18, 0),  # PD7
    89: (22.86, 10.16, 180),  # PB3
    90: (22.86, 7.62, 180),  # PB4
    91: (22.86, 5.08, 180),  # PB5
    92: (22.86, 2.54, 180),  # PB6
    93: (22.86, 0.0, 180),  # PB7
    94: (-22.86, 55.88, 0),  # BOOT0
    95: (22.86, -2.54, 180),  # PB8
    96: (22.86, -5.08, 180),  # PB9
    97: (-22.86, 17.78, 0),  # PE0
    98: (-22.86, 15.24, 0),  # PE1
    99: (2.54, -71.12, 90),  # VSS
    100: (5.08, 68.58, 270),  # VDD
}


# ============================================================================
# r18 (H7-Migration Phase 3): Hilfs-Symbole + Sheets für STM32H743
# ============================================================================


def no_connect(x: float, y: float) -> str:
    return (
        f'  (no_connect (at {x} {y})\n'
        f"    (uuid \"{det_uuid(f'nc/{x},{y}')}\"))\n"
    )


def _crystal_lib_symbol() -> str:
    """Device:Crystal-äquivalent (2-Pin, kein GND-Body) für Y1 HSE."""
    out = []
    out.append('    (symbol "Device:Crystal" (pin_numbers hide) (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)')
    out.append('      (property "Reference" "Y" (at 0 3.81 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Value" "Crystal" (at 0 -3.81 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Device:Crystal_0_1"')
    out.append('        (rectangle (start -1.143 2.54) (end 1.143 -2.54) (stroke (width 0.3048) (type default)) (fill (type none)))')
    out.append('        (polyline (pts (xy -2.032 1.27) (xy -2.032 -1.27)) (stroke (width 0.508) (type default)) (fill (type none)))')
    out.append('        (polyline (pts (xy 2.032 1.27) (xy 2.032 -1.27)) (stroke (width 0.508) (type default)) (fill (type none)))')
    out.append('      )')
    out.append('      (symbol "Device:Crystal_1_1"')
    out.append('        (pin passive line (at -5.08 0 0) (length 3.048)')
    out.append('          (name "1" (effects (font (size 1.27 1.27))))')
    out.append('          (number "1" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin passive line (at 5.08 0 180) (length 3.048)')
    out.append('          (name "2" (effects (font (size 1.27 1.27))))')
    out.append('          (number "2" (effects (font (size 1.27 1.27)))))')
    out.append('      )')
    out.append('    )')
    return "\n".join(out)


def _ap7361c_lib_symbol() -> str:
    """AP7361C 1A LDO, SOT-89-5 (Diodes Inc.).

    r18.6: Symbol auf AP7361C umgestellt. AP7361 ist laut Diodes-Datasheet
    NRND („Not Recommended For New Designs — Use AP7361C"). AP7361C nutzt für
    SOT-89-5 dasselbe Pin-Mapping.

    OFFIZIELLES PINOUT SOT-89-5 (Diodes AP7361 / AP7361C Datasheet,
    via Mouser-CDN, vom User verifiziert und im PR verlinkt):

        Pin 1 = EN
        Pin 2 = GND
        Pin 3 = ADJ/NC
        Pin 4 = IN
        Pin 5 = OUT

    Frühere r18.5-Annahme (1=ADJ/NC,2=OUT,3=IN,4=GND,5=EN) war FALSCH und
    hätte IN/OUT/EN vertauscht. Quelle: mouser.de/datasheet/3/175/1/AP7361.pdf

    Footprint: Package_TO_SOT_SMD:SOT-89-5. KEIN SOT-223 (AP7361-33E-13 /
    C150719 ist SOT-223 und passt NICHT zu diesem Symbol).
    """
    out = []
    out.append('    (symbol "Regulator_Linear:AP7361C" (pin_names (offset 0.508)) (in_bom yes) (on_board yes)')
    out.append('      (property "Reference" "U" (at 0 6.35 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Value" "AP7361C" (at 0 -8.89 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Regulator_Linear:AP7361C_0_1"')
    out.append('        (rectangle (start -6.35 5.08) (end 6.35 -6.35) (stroke (width 0.254) (type default)) (fill (type background)))')
    out.append('      )')
    out.append('      (symbol "Regulator_Linear:AP7361C_1_1"')
    # Linke Seite (Eingänge): EN oben, IN unten
    out.append('        (pin input line (at -8.89 2.54 0) (length 2.54)')
    out.append('          (name "EN" (effects (font (size 1.27 1.27))))')
    out.append('          (number "1" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin power_in line (at -8.89 -2.54 0) (length 2.54)')
    out.append('          (name "IN" (effects (font (size 1.27 1.27))))')
    out.append('          (number "4" (effects (font (size 1.27 1.27)))))')
    # Rechte Seite (Ausgänge): OUT oben, ADJ/NC unten
    out.append('        (pin power_out line (at 8.89 2.54 180) (length 2.54)')
    out.append('          (name "OUT" (effects (font (size 1.27 1.27))))')
    out.append('          (number "5" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin no_connect line (at 8.89 -2.54 180) (length 2.54)')
    out.append('          (name "ADJ/NC" (effects (font (size 1.27 1.27))))')
    out.append('          (number "3" (effects (font (size 1.27 1.27)))))')
    # Untere Seite: GND
    out.append('        (pin power_in line (at 0 -8.89 90) (length 2.54)')
    out.append('          (name "GND" (effects (font (size 1.27 1.27))))')
    out.append('          (number "2" (effects (font (size 1.27 1.27)))))')
    out.append('      )')
    out.append('    )')
    return "\n".join(out)


def _nmos_sot23_lib_symbol() -> str:
    """N-Kanal-MOSFET SOT-23 (2N7002) für LCD-Backlight-Low-Side-Switch.
    Pin-Zuordnung 1=G, 2=S, 3=D = JEDEC-TO-236-Standardbelegung des 2N7002
    (FP_VERIFY: gegen Hersteller-Datasheet der bestückten Marke prüfen)."""
    out = []
    out.append('    (symbol "Transistor_FET:Q_NMOS_GSD" (pin_names (offset 0.254) hide) (in_bom yes) (on_board yes)')
    out.append('      (property "Reference" "Q" (at 5.08 1.27 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Value" "Q_NMOS_GSD" (at 5.08 -1.27 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Transistor_FET:Q_NMOS_GSD_0_1"')
    out.append('        (polyline (pts (xy 0.254 1.778) (xy 0.254 -1.778)) (stroke (width 0.254) (type default)) (fill (type none)))')
    out.append('        (polyline (pts (xy 0.762 1.27) (xy 0.762 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))')
    out.append('        (polyline (pts (xy 0.762 0) (xy 2.54 0)) (stroke (width 0) (type default)) (fill (type none)))')
    out.append('        (polyline (pts (xy 2.54 1.27) (xy 0.762 1.27)) (stroke (width 0) (type default)) (fill (type none)))')
    out.append('        (polyline (pts (xy 2.54 -1.27) (xy 0.762 -1.27)) (stroke (width 0) (type default)) (fill (type none)))')
    out.append('      )')
    out.append('      (symbol "Transistor_FET:Q_NMOS_GSD_1_1"')
    out.append('        (pin input line (at -2.54 0 0) (length 2.794)')
    out.append('          (name "G" (effects (font (size 1.27 1.27))))')
    out.append('          (number "1" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin passive line (at 2.54 -3.81 90) (length 2.54)')
    out.append('          (name "S" (effects (font (size 1.27 1.27))))')
    out.append('          (number "2" (effects (font (size 1.27 1.27)))))')
    out.append('        (pin passive line (at 2.54 3.81 270) (length 2.54)')
    out.append('          (name "D" (effects (font (size 1.27 1.27))))')
    out.append('          (number "3" (effects (font (size 1.27 1.27)))))')
    out.append('      )')
    out.append('    )')
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


def _pca9685pw_lib_symbol() -> str:
    """PCA9685PW TSSOP-28 16-channel I²C PWM LED-driver (NXP).

    Pin-Map per NXP Datasheet Rev 4 (16-April-2015) Table 3 + Fig. 2:
    Left (top→bottom, pins 1..14):
        1=A0  2=A1  3=A2  4=A3  5=A4  6=LED0  7=LED1  8=LED2  9=LED3 10=LED4
        11=LED5 12=LED6 13=LED7 14=VSS
    Right (top→bottom, pins 28..15):
        28=VDD 27=SDA 26=SCL 25=EXTCLK 24=A5 23=~OE 22=LED15 21=LED14
        20=LED13 19=LED12 18=LED11 17=LED10 16=LED9 15=LED8

    Pin-Body-Geometry mirrors MCP23017 (also 28-pin TSSOP/SSOP), 2.54 mm pitch.
    """
    pins_left = [
        (1, "A0", "input"),
        (2, "A1", "input"),
        (3, "A2", "input"),
        (4, "A3", "input"),
        (5, "A4", "input"),
        (6, "LED0", "output"),
        (7, "LED1", "output"),
        (8, "LED2", "output"),
        (9, "LED3", "output"),
        (10, "LED4", "output"),
        (11, "LED5", "output"),
        (12, "LED6", "output"),
        (13, "LED7", "output"),
        (14, "VSS", "power_in"),
    ]
    pins_right = [
        (28, "VDD", "power_in"),
        (27, "SDA", "bidirectional"),
        (26, "SCL", "input"),
        (25, "EXTCLK", "input"),
        (24, "A5", "input"),
        (23, "~OE", "input"),
        (22, "LED15", "output"),
        (21, "LED14", "output"),
        (20, "LED13", "output"),
        (19, "LED12", "output"),
        (18, "LED11", "output"),
        (17, "LED10", "output"),
        (16, "LED9", "output"),
        (15, "LED8", "output"),
    ]
    y_top = 16.51  # 14-Pin column → ((14-1)*2.54)/2 = 16.51
    rect_top = y_top + 2.54
    rect_bot = -y_top - 2.54
    out = ['    (symbol "Driver_LED:PCA9685PW" (in_bom yes) (on_board yes)']
    out.append(f'      (property "Reference" "U" (at 0 {rect_top + 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append(f'      (property "Value" "PCA9685PW" (at 0 {rect_bot - 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Driver_LED:PCA9685PW_0_1"')
    out.append(f'        (rectangle (start -10.16 {rect_top}) (end 10.16 {rect_bot})')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append('      (symbol "Driver_LED:PCA9685PW_1_1"')
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


def _mcp73831_lib_symbol() -> str:
    """MCP73831T-2ACI/OT — LiPo Charger, SOT-23-5, 5 Pins.
    Per Microchip DS20001984H (PCB-Pin-Map Page 2):
    Pin 1: STAT (output, open-drain status indicator)
    Pin 2: VSS (power_in)
    Pin 3: VBAT (output, charge current to battery)
    Pin 4: VDD (power_in, +5V from USB-C)
    Pin 5: ~PROG (input, programs Icharge via Rprog to GND)
    """
    pins = [
        (1, "STAT", "output", -7.62, 5.08),
        (2, "VSS", "power_in", -7.62, 0.0),
        (3, "VBAT", "output", 7.62, 5.08),
        (4, "VDD", "power_in", 7.62, 0.0),
        (5, "PROG", "input", 7.62, -5.08),
    ]
    out = ['    (symbol "Battery:MCP73831" (in_bom yes) (on_board yes)']
    out.append('      (property "Reference" "U" (at 0 7.62 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Value" "MCP73831T-2ACI/OT" (at 0 -7.62 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "https://ww1.microchip.com/downloads/en/DeviceDoc/MCP73831-Family-Data-Sheet-DS20001984H.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Battery:MCP73831_0_1"')
    out.append('        (rectangle (start -5.08 6.35) (end 5.08 -6.35)')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append('      (symbol "Battery:MCP73831_1_1"')
    for num, name, ptype, px, py in pins:
        rot = 0 if px < 0 else 180
        out.append(
            f'        (pin {ptype} line (at {px} {py:.3f} {rot}) (length 2.54)\n'
            f'          (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{num}" (effects (font (size 1.27 1.27)))))'
        )
    out.append('        )')
    out.append('      )')
    return "\n".join(out)


def _tps61089_lib_symbol() -> str:
    """TPS61089RNR — Boost Converter LiPo→5V, VQFN-11 HotRod 2×2.5mm + Thermal Pad.
    Per TI Datasheet SLVSD38C Table 6-1 (LCSC C165129):
    Pin 1: FSW    — switching-frequency programming resistor (to SW pin)
    Pin 2: VCC    — internal LDO output, requires ≥1µF cap to GND
    Pin 3: FB     — output voltage feedback
    Pin 4: COMP   — error amplifier output, compensation network to GND
    Pin 5: GND    — power-in / quiet ground
    Pin 6: VOUT   — boost converter output
    Pin 7: EN     — enable input (HIGH = on)
    Pin 8: ILIM   — current-limit programming resistor (to GND)
    Pin 9: VIN    — IC power supply input
    Pin 10: BOOT  — high-side gate driver bootstrap (cap to SW)
    Pin 11: SW    — switch node (to inductor)
    Plus thermal pad (treated as pin 12 = GND).

    HINWEIS r12-B11: SPEC-Wechsel von TPS61089RNSR (QFN-12 3×3) auf
    TPS61089RNR (VQFN-11 HotRod 2×2.5) wegen JLC-Stock-Verfügbarkeit
    (RNSR nicht bei LCSC). RNR-Variante braucht 5 zusätzliche externe
    Bauteile (C_VCC, R_FSW, R_ILIM, C_BOOT, R_COMP+C_COMP).
    """
    pins_left = [
        (1, "FSW", "input"),
        (2, "VCC", "power_out"),
        (3, "FB", "input"),
        (4, "COMP", "output"),
        (5, "GND", "power_in"),
        (6, "VOUT", "power_out"),
    ]
    pins_right = [
        (11, "SW", "passive"),
        (10, "BOOT", "output"),
        (9, "VIN", "power_in"),
        (8, "ILIM", "output"),
        (7, "EN", "input"),
    ]
    y_top = 6.35  # 6 pins per side max
    rect_top = y_top + 2.54
    rect_bot = -y_top - 2.54
    out = ['    (symbol "Regulator_Switching:TPS61089" (in_bom yes) (on_board yes)']
    out.append(f'      (property "Reference" "U" (at 0 {rect_top + 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append(f'      (property "Value" "TPS61089RNR" (at 0 {rect_bot - 1.27} 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "https://www.ti.com/lit/ds/symlink/tps61089.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Regulator_Switching:TPS61089_0_1"')
    out.append(f'        (rectangle (start -7.62 {rect_top}) (end 7.62 {rect_bot})')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append('      (symbol "Regulator_Switching:TPS61089_1_1"')
    for idx, (num, name, ptype) in enumerate(pins_left):
        ly = y_top - idx * 2.54
        out.append(
            f'        (pin {ptype} line (at -10.16 {ly:.3f} 0) (length 2.54)\n'
            f'          (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{num}" (effects (font (size 1.27 1.27)))))'
        )
    for idx, (num, name, ptype) in enumerate(pins_right):
        ly = y_top - idx * 2.54
        out.append(
            f'        (pin {ptype} line (at 10.16 {ly:.3f} 180) (length 2.54)\n'
            f'          (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{num}" (effects (font (size 1.27 1.27)))))'
        )
    # r18.7 CORRECTION: RNR0011A has NO separate exposed thermal pad.
    # The central large pad IS pin 11 (SW), per TI datasheet Pin Configuration
    # diagram. The old generator created a fake "pin 12 = ePAD = GND" — but
    # that pad doesn't exist in the package. SW (pin 11) carries both the
    # switching current and the thermal duty. GND is taken via pin 5 (one of
    # the enlarged side pads).
    out.append('        )')
    out.append('      )')
    return "\n".join(out)


def _dmg2305ux_lib_symbol() -> str:
    """DMG2305UX — P-Channel MOSFET, SOT-23, 3 Pins.
    Per Diodes Inc DS39061 Page 1:
    Pin 1: G (Gate, input)
    Pin 2: S (Source, passive)
    Pin 3: D (Drain, passive)
    """
    pins = [
        (1, "G", "input", -7.62, 0.0, 0),
        (2, "S", "passive", 7.62, 2.54, 180),
        (3, "D", "passive", 7.62, -2.54, 180),
    ]
    out = ['    (symbol "Transistor_FET:DMG2305UX" (in_bom yes) (on_board yes)']
    out.append('      (property "Reference" "Q" (at 0 5.08 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Value" "DMG2305UX" (at 0 -5.08 0) (effects (font (size 1.27 1.27))))')
    out.append('      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (property "Datasheet" "https://www.diodes.com/assets/Datasheets/DMG2305UX.pdf" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))')
    out.append('      (symbol "Transistor_FET:DMG2305UX_0_1"')
    out.append('        (rectangle (start -5.08 3.81) (end 5.08 -3.81)')
    out.append('          (stroke (width 0.254) (type default)) (fill (type none))))')
    out.append('      (symbol "Transistor_FET:DMG2305UX_1_1"')
    for num, name, ptype, px, py, rot in pins:
        out.append(
            f'        (pin {ptype} line (at {px} {py:.3f} {rot}) (length 2.54)\n'
            f'          (name "{name}" (effects (font (size 1.27 1.27))))\n'
            f'          (number "{num}" (effects (font (size 1.27 1.27)))))'
        )
    out.append('        )')
    out.append('      )')
    return "\n".join(out)


def _inductor_lib_symbol() -> str:
    """Simple 2-pin inductor symbol (Device:L)."""
    return r"""    (symbol "Device:L" (pin_numbers hide) (pin_names (offset 1.016)) (in_bom yes) (on_board yes)
      (property "Reference" "L" (at -2.032 0 90) (effects (font (size 1.27 1.27))))
      (property "Value" "L" (at 1.778 0 90) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:L_0_1"
        (arc (start 0 -2.54) (mid 0.6323 -1.905) (end 0 -1.27) (stroke (width 0) (type default)) (fill (type none)))
        (arc (start 0 -1.27) (mid 0.6323 -0.635) (end 0 0) (stroke (width 0) (type default)) (fill (type none)))
        (arc (start 0 0) (mid 0.6323 0.635) (end 0 1.27) (stroke (width 0) (type default)) (fill (type none)))
        (arc (start 0 1.27) (mid 0.6323 1.905) (end 0 2.54) (stroke (width 0) (type default)) (fill (type none))))
      (symbol "Device:L_1_1"
        (pin passive line (at 0 3.81 270) (length 1.27)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -3.81 90) (length 1.27)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
""".strip()


def _schottky_diode_lib_symbol() -> str:
    """Simple Schottky diode symbol (Device:D_Schottky). Pin 1 = K (Kathode), Pin 2 = A (Anode)."""
    return r"""    (symbol "Device:D_Schottky" (pin_numbers hide) (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)
      (property "Reference" "D" (at 0 2.54 0) (effects (font (size 1.27 1.27))))
      (property "Value" "D_Schottky" (at 0 -2.54 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Device:D_Schottky_0_1"
        (polyline (pts (xy -1.016 1.27) (xy -1.016 -1.27)) (stroke (width 0.2032) (type default)) (fill (type none)))
        (polyline (pts (xy 1.016 1.27) (xy 1.016 0.508) (xy 1.524 0.508)) (stroke (width 0.2032) (type default)) (fill (type none)))
        (polyline (pts (xy 1.016 -1.27) (xy 1.016 -0.508) (xy 0.508 -0.508)) (stroke (width 0.2032) (type default)) (fill (type none)))
        (polyline (pts (xy 1.016 1.27) (xy -1.016 0) (xy 1.016 -1.27) (xy 1.016 1.27)) (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Device:D_Schottky_1_1"
        (pin passive line (at -3.81 0 0) (length 2.794)
          (name "K" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 3.81 0 180) (length 2.794)
          (name "A" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))
""".strip()


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
      (property "Datasheet" "https://jlcpcb.com/partdetail/C209762" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
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


def _audiojack_lib_symbol() -> str:
    """3.5mm TRS stereo jack with insertion-detect switch (PJ-320 class).

    Pins:
      T   (1) Tip    = Left audio out
      R   (2) Ring   = Right audio out
      S   (3) Sleeve = GND
      DET (4) detect switch contact — shorts to S(GND) when NO plug,
              opens on insertion. With an MCP23017 pull-up: idle=LOW,
              inserted=HIGH. (Firmware can invert if the chosen jack's
              switch polarity differs.)
    """
    return r"""
    (symbol "Connector:AudioJack3_Switch" (in_bom yes) (on_board yes)
      (property "Reference" "J" (at 0 7.62 0) (effects (font (size 1.27 1.27))))
      (property "Value" "AudioJack3_Switch" (at 0 5.08 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Connector:AudioJack3_Switch_0_1"
        (rectangle (start -5.08 3.81) (end 5.08 -5.08)
          (stroke (width 0.254) (type default)) (fill (type none))))
      (symbol "Connector:AudioJack3_Switch_1_1"
        (pin passive line (at 7.62 2.54 180) (length 2.54)
          (name "T" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 7.62 0 180) (length 2.54)
          (name "R" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 7.62 -2.54 180) (length 2.54)
          (name "S" (effects (font (size 1.27 1.27))))
          (number "3" (effects (font (size 1.27 1.27)))))
        (pin passive line (at -7.62 0 0) (length 2.54)
          (name "DET" (effects (font (size 1.27 1.27))))
          (number "4" (effects (font (size 1.27 1.27)))))))
""".strip()


LIB_SYMBOLS = (
    LIB_SYMBOLS
    + "\n" + _pico2_lib_symbol()
    + "\n" + _conn_01xN_lib_symbol(16)
    + "\n" + _mcp23017_lib_symbol()
    + "\n" + _pca9685pw_lib_symbol()
    + "\n" + _mcp73831_lib_symbol()
    + "\n" + _tps61089_lib_symbol()
    + "\n" + _dmg2305ux_lib_symbol()
    + "\n" + _inductor_lib_symbol()
    + "\n" + _schottky_diode_lib_symbol()
    + "\n" + _rotary_encoder_switch_lib_symbol()
    + "\n" + _pcm5102a_lib_symbol()
    + "\n" + _pam8403_lib_symbol()
    + "\n" + _ferrite_bead_lib_symbol()
    + "\n" + _conn_01xN_lib_symbol(2)
    + "\n" + _conn_02xN_lib_symbol(20)
    + "\n" + _audiojack_lib_symbol()
    # r18 (H7-Migration):
    + "\n" + _stm32h743_lib_symbol()
    + "\n" + _crystal_lib_symbol()
    + "\n" + _ap7361c_lib_symbol()
    + "\n" + _nmos_sot23_lib_symbol()
    + "\n" + _conn_01xN_lib_symbol(8)
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
#   USB-C(VBUS) -> F1 (polyfuse 3A/6A) -> C_BULK (1000µF) -> +5V Rail
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
            value="USB_C_Receptacle (TYPE-C-31-M-12, C165948, 16-Pin mit D+/D-/CC, ~5k Insertion-Cycles)",
            x=J1_X,
            y=J1_Y,
            footprint="Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12",
            datasheet="https://datasheet.lcsc.com/lcsc/1903211732_Korean-Hroparts-Elec-TYPE-C-31-M-12_C165948.pdf",
            extra_props={"MPN": "TYPE-C-31-M-12", "LCSC": "C165948",
                         "Notes": "r18.19-REVERT auf das 16-Pin-Original: Die r18.10/r18.14-Variante C283540 (TYPE-C-31-M-17) wurde am LCSC-Produktseite als **6-Pin POWER-ONLY** identifiziert — keine D+/D-, kein CC. Damit waere USB-DFU-Flashen (PA11/PA12, ADR-0009) physikalisch unmoeglich gewesen und alle USB_DM/USB_DP-/CC-Pads des Schematic floateten. Zurueck auf C165948 (16-Pin volle USB-C-Belegung, 168k+ Stock). Insertion-Cycle-Trade-Off 10k -> ~5k bewusst akzeptiert (typischer Hobby-Gebrauch < 5k ueber Jahre)."},
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
    # v0.7: 3A-hold / 6A-trip (war 2A/4A — zu klein für 2.45A Worst-Case-Last).
    # Bei Gehäuse-Innentemp ~50°C derated 3A→~2.3A hold → deckt Typical-Audio
    # (1.35A) sicher, Worst-Case-Bass-Peaks (2.45A, <ms) reiten den Bulk-Cap.
    # Setzt 5V/3A-Netzteil voraus (siehe SPEC §3). Itrip 6A schützt bei Hard-Short.
    # Polyfuse pin1 (local -3.81, 0) → abs (41.19, 60). pin2 (local +3.81, 0) → abs (48.81, 60).
    symbols.append(
        place_symbol(
            lib_id="Device:Polyfuse",
            ref="F1",
            value="3A/6A 1812 (Littelfuse 1812L300, 16V)",
            x=45,
            y=RAIL_Y,
            footprint="Fuse:Fuse_1812_4532Metric",
            datasheet="https://www.littelfuse.com/media?resourcetype=datasheets&itemid=ce0d2bf7-3eb1-4cf6-9c8c-8d3d3a8b1f9a&filename=littelfuse-pptc-1812l-datasheet",
            extra_props={"MPN": "1812L300/16GR", "LCSC": "C18198349"},
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

    # ---- r12: VBUS_USBC hier-output (pre-fuse raw VBUS) für USB-Detect-Pfad in mcp_sheet
    # + Battery-Charger-Input (U7 MCP73831) in battery_sheet. Tap am rail-30-35-Segment.
    wires.append(wire(30.48, RAIL_Y, 30.48, RAIL_Y - 6, seed_suffix="vbus-usbc-tap-up"))
    hlabels.append(hier_label(30.48, RAIL_Y - 6, "VBUS_USBC", shape="output", rotation=90))
    junctions.append(junction(30.48, RAIL_Y))

    # ---- F1 pin2 → Rail nach rechts (48.81 → 110)
    # CBULK an der Rail @ (65, sy), D2 TVS @ (55, sy)
    # C1 (10µF) @ x=80, C2 (100nF) @ x=86 = +5V-Rail HF-Bypass (SPEC §3).
    # Rail-x-Positionen mit Junctions: 48.81, 55, 65, 80, 86, 110
    rail_xs = [48.81, 55, 65, 80, 86, 110]
    for xa, xb in zip(rail_xs, rail_xs[1:]):
        wires.append(wire(xa, RAIL_Y, xb, RAIL_Y, seed_suffix=f"rail-{xa}-{xb}"))
    # Junctions an allen Stützen außer Endpunkten
    for x in rail_xs[1:-1]:
        junctions.append(junction(x, RAIL_Y))

    # ---- C_BULK 470µF Polymer-Alu (low-profile) + C_BULK2 220µF MLCC parallel
    # r18.12: Wechsel von 1000µF Alu-Elko (10.5mm Höhe, ADR-0011-Konflikt)
    # auf 470µF Polymer (~2mm Höhe). Plus 220µF MLCC X5R parallel für HF-Bulk.
    # Effektive Bulk-Kapazität: ~690 µF (Polymer + MLCC), aber ESR ~10mΩ
    # statt ~25mΩ → Class-D-Bass-Transienten sind sauberer (Anti-Kratzig per
    # ADR-0010 Punkt 4). 1000→470µF ist OK weil Polymer kein Derating wie
    # Alu hat; effective rail-energy ist vergleichbar.
    #
    # Device:CP Symbol bleibt (Polymer ist polarisiert), Footprint wechselt
    # auf 7343-31 (Standard-D-Case SMD-Polymer-Tantal, 7.3×4.3×3.1mm).
    symbols.append(
        place_symbol(
            lib_id="Device:CP",
            ref="C_BULK",
            value="470uF 10V Polymer-Tantal (Case-E 7343-43, ESR 100mOhm)",
            x=65,
            y=63.81,
            footprint="Capacitor_SMD:CP_Tantalum_Case-E_EIA-7343-43_Reflow",
            extra_props={
                "MPN": "TPSE477K010R0100 (Kyocera AVX, Polymer-Tantal)",
                "LCSC": "TBD-VERIFY (JLC-Stock vor Layout pruefen)",
                "LCSC": "C444831", "Notes": "r18.20b: Sourcing final. 470uF/10V Polymer-Tantal Case-E (7343-43, 4.3mm H — passt in 8mm-Top-Zone), JLC Extended ~517 Stock. ESR 100mOhm (NICHT <25mOhm — die <25mOhm-Flachteile sind bei LCSC nicht lagernd; siehe ADR-0011). Die niedrige Transient-ESR liefert der parallele C_BULK2-MLCC (~5mOhm). 16V/8mOhm-Alternative C403795 (Panasonic 16SEPG470M) waere ein zylindrischer 8x8.9mm-Can mit ~43 Stock — verworfen wegen Bauhoehe + Stock.",
            },
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

    # ---- C_BULK2 (220µF MLCC parallel zu C_BULK Polymer, r18.12) + C1/C2 HF-Bypass
    # r18.20b: 100µF/10V MLCC X5R 1210 parallel zum 470µF-Polymer. WICHTIG:
    # 220µF/10V in 1210 EXISTIERT NICHT (220µF gibt es nur bis 6.3V im 1210-
    # Case). 100µF/10V (echter Headroom am 5V-Rail, minimales DC-Bias-Derating)
    # statt 220µF/6.3V (waere am 5V-Rail auf ~70-110µF derated). Effektiv-Bulk
    # vergleichbar, aber audit-sauberer Voltage-Headroom. Sehr niedrige ESR
    # (~5mOhm) deckt den Transient-Pfad (ADR-0010).
    # C1 (10µF) + C2 (100nF) bleiben als zusätzlicher HF-Bypass am Rail-Eintritt.
    for cref, cx, cval, cfp, cmpn, clcsc in (
        ("C_BULK2", 73, "100uF 10V X5R 1210 (MLCC parallel zu C_BULK)",
         "Capacitor_SMD:C_1210_3225Metric",
         "LMK325ABJ107MM-T (Taiyo Yuden, 100uF 10V X5R)", "C2880380"),
        ("C1", 80, "10uF X5R 0805 (+5V rail HF-bulk)",
         "Capacitor_SMD:C_0805_2012Metric",
         "CL21A106KAYNNNE (Samsung, 25V X5R)", "C15850"),
        ("C2", 86, "100nF X7R 0603 (+5V rail HF-decoupling)",
         "Capacitor_SMD:C_0603_1608Metric",
         "CC0603KRX7R9BB104 (Yageo, 50V X7R)", "C14663"),
    ):
        symbols.append(
            place_symbol(
                lib_id="Device:C",
                ref=cref,
                value=cval,
                x=cx,
                y=63.81,
                footprint=cfp,
                extra_props={"MPN": cmpn, "LCSC": clcsc},
                seed_suffix=cref,
            )
        )
        wires.append(wire(cx, 67.62, cx, GND_LABEL_Y, seed_suffix=f"{cref.lower()}-neg-to-gnd"))
        symbols.append(
            place_symbol(
                lib_id="Power:GND",
                ref=f"#PWR_{cref}",
                value="GND",
                x=cx,
                y=GND_LABEL_Y,
                seed_suffix=f"{cref.lower()}-gnd",
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
            extra_props={"MPN": "0603WAF5101T5E", "LCSC": "C23186"},
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
            extra_props={"MPN": "0603WAF5101T5E", "LCSC": "C23186"},
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
    # Pin 1 (upstream D+) ← Label USB_DP. Pin 6 (downstream D+) → USB_DP hier_label.
    # Pin 3 (upstream D-) ← Label USB_DN. Pin 4 (downstream D-) → USB_DM hier_label.
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
    # Pin 6 (I/O1 downstream D+) @ y=77.46 → USB_DP hier_label
    wires.append(wire(107.62, 77.46, 115, 77.46, seed_suffix="d1-p6-stub"))
    hlabels.append(hier_label(115, 77.46, "USB_DP", shape="output", rotation=0))
    # Pin 4 (I/O2 downstream D-) @ y=82.54 → USB_DM hier_label
    wires.append(wire(107.62, 82.54, 115, 82.54, seed_suffix="d1-p4-stub"))
    hlabels.append(hier_label(115, 82.54, "USB_DM", shape="output", rotation=0))

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

    # ---- r18.6: U5 AP7361C-33 LDO — +5V-Rail → +3V3 (SPEC §4 U5, war DNP).
    # AP7361 ist laut Diodes-Datasheet NRND ("Not Recommended For New Designs —
    # Use AP7361C"); für SOT-89-5 ist das relevante Pin-Mapping identisch.
    #
    # OFFIZIELLES PINOUT (Diodes-DS via mouser.de/datasheet/3/175/1/AP7361.pdf):
    #   Pin 1 = EN   (links oben, y=+2.54)
    #   Pin 2 = GND  (unten, gemeinsam mit Tab)
    #   Pin 3 = ADJ/NC (rechts unten, y=-2.54)
    #   Pin 4 = IN   (links unten, y=-2.54)
    #   Pin 5 = OUT  (rechts oben, y=+2.54)
    # (Die r18.5-Annahme war 1=ADJ/NC,2=OUT,3=IN,4=GND,5=EN — falsch.)
    #
    # EN ist immer-an: EN-Pin direkt an IN-Pin gebrueckt (Diodes-DS: "tie EN to
    # IN if always-on" — typische Anwendungsschaltung).
    LDO_X, LDO_Y = 120.0, 80.0
    PIN_EN_Y  = LDO_Y - 2.54    # Pin 1 EN
    PIN_IN_Y  = LDO_Y + 2.54    # Pin 4 IN  (KORRIGIERT: jetzt unten links)
    PIN_OUT_Y = LDO_Y - 2.54    # Pin 5 OUT
    # +5V-Rail droppt zur IN-Hoehe runter
    junctions.append(junction(110, RAIL_Y))
    wires.append(wire(110, RAIL_Y, 110, PIN_IN_Y, seed_suffix="ldo-drop"))
    wires.append(wire(110, PIN_IN_Y, LDO_X - 8.89, PIN_IN_Y, seed_suffix="ldo-in"))
    junctions.append(junction(110, PIN_IN_Y))
    # EN-Pin → IN-Pin (always-on); auf Hoehe PIN_EN_Y nach links bis x=104, dann
    # vertikal hoch zur Hoehe PIN_IN_Y, dort auf den IN-Wire einfaedeln.
    wires.append(wire(LDO_X - 8.89, PIN_EN_Y, 104, PIN_EN_Y, seed_suffix="ldo-en-left"))
    wires.append(wire(104, PIN_EN_Y, 104, PIN_IN_Y, seed_suffix="ldo-en-up"))
    wires.append(wire(104, PIN_IN_Y, 110, PIN_IN_Y, seed_suffix="ldo-en-into-in"))
    junctions.append(junction(104, PIN_IN_Y))
    symbols.append(place_symbol(lib_id="Regulator_Linear:AP7361C", ref="U5",
                                value="AP7361C-33 1A LDO (+5V->+3V3, SOT-89-5)",
                                x=LDO_X, y=LDO_Y,
                                footprint="Package_TO_SOT_SMD:SOT-89-5",
                                datasheet="https://www.mouser.de/datasheet/3/175/1/AP7361.pdf",
                                extra_props={
                                    "MPN": "AP7361C-33Y5-13",
                                    "LCSC": "C460397",
                                    "Package": "SOT-89-5 (NICHT SOT-223)",
                                    "PIN_SOURCE": "Diodes-DS via mouser.de/datasheet/3/175/1/AP7361.pdf (User-verifiziert): 1=EN,2=GND,3=ADJ/NC,4=IN,5=OUT",
                                    "NOTE": "AP7361 ist NRND; AP7361C ist der Nachfolger (gleicher Pinout am SOT-89-5)",
                                    "FP_NOTE": "KiCad-Standard SOT-89-5: 5 Pads, Pin 2 (Tab, 0.8x2mm @ origin) plus Pins 1,3,4,5 (1.5x0.7mm @ +-1.85 X, +-1.5 Y). Standard-JEDEC TO-243 = AP7361C-Package. Verifiziert gegen KiCad-Footprints-Repo.",
                                },
                                seed_suffix="U5", sheet_uuid_seed="sheet_power_tree"))
    # GND-Pin (unten)
    wires.append(wire(LDO_X, LDO_Y + 8.89, LDO_X, LDO_Y + 11, seed_suffix="ldo-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_U5", value="GND",
                                x=LDO_X, y=LDO_Y + 11, seed_suffix="ldo-gnd",
                                sheet_uuid_seed="sheet_power_tree"))
    # Cin 4.7µF Keramik am IN-Knoten (Diodes-DS: ≥1µF, empfohlen 4.7µF)
    symbols.append(place_symbol(lib_id="Device:C", ref="C_LDO_IN",
                                value="4.7uF X5R 0603 10V (LDO Cin, DS empfohlen)",
                                x=106, y=PIN_IN_Y + 3.81,
                                footprint="Capacitor_SMD:C_0603_1608Metric",
                                extra_props={"MPN": "GRM188R61A475KE15D", "LCSC": "C46653"},
                                seed_suffix="CLDOIN", sheet_uuid_seed="sheet_power_tree"))
    wires.append(wire(106, PIN_IN_Y, 106, PIN_IN_Y + 3.81 - 3.81, seed_suffix="cldoin-top"))
    junctions.append(junction(106, PIN_IN_Y))
    wires.append(wire(106, PIN_IN_Y + 3.81 + 3.81, 106, PIN_IN_Y + 10, seed_suffix="cldoin-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_CLDOIN", value="GND",
                                x=106, y=PIN_IN_Y + 10, seed_suffix="cldoin-gnd",
                                sheet_uuid_seed="sheet_power_tree"))
    # OUT (Pin 5, rechts oben) → +3V3-Flag + Cout + hier_label
    wires.append(wire(LDO_X + 8.89, PIN_OUT_Y, 140, PIN_OUT_Y, seed_suffix="ldo-out"))
    symbols.append(place_symbol(lib_id="Power:+3V3", ref="#PWR_LDO3V3", value="+3V3",
                                x=132, y=PIN_OUT_Y, rotation=270, seed_suffix="ldo-3v3",
                                sheet_uuid_seed="sheet_power_tree"))
    junctions.append(junction(132, PIN_OUT_Y))
    # Cout 4.7µF Keramik (Diodes-DS: ≥2.2µF, empfohlen 4.7µF; mit ESR-stabil)
    symbols.append(place_symbol(lib_id="Device:C", ref="C_LDO_OUT",
                                value="4.7uF X5R 0603 10V (LDO Cout, DS empfohlen)",
                                x=136, y=PIN_OUT_Y + 3.81,
                                footprint="Capacitor_SMD:C_0603_1608Metric",
                                extra_props={"MPN": "GRM188R61A475KE15D", "LCSC": "C46653"},
                                seed_suffix="CLDOOUT", sheet_uuid_seed="sheet_power_tree"))
    wires.append(wire(136, PIN_OUT_Y, 136, PIN_OUT_Y + 3.81 - 3.81, seed_suffix="cldoout-top"))
    junctions.append(junction(136, PIN_OUT_Y))
    wires.append(wire(136, PIN_OUT_Y + 3.81 + 3.81, 136, PIN_OUT_Y + 10, seed_suffix="cldoout-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_CLDOOUT", value="GND",
                                x=136, y=PIN_OUT_Y + 10, seed_suffix="cldoout-gnd",
                                sheet_uuid_seed="sheet_power_tree"))
    hlabels.append(hier_label(140, PIN_OUT_Y, "+3V3_OUT", shape="output", rotation=180))

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
        f'    (rev "0.7")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6.3 §1 + §3")\n'
        f'    (comment 2 "USB-C → F1(3A/6A) → C_BULK(470µF Polymer + 220µF MLCC) → +5V rail")\n'
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
# Pin-Allocation per SPEC v0.6 §5. Inputs: +5V_IN, GND, USB_DP/DN.
# Outputs: +3V3_OUT (Pico SMPS) + alle GP-Funktionsleitungen zu Sheets 3-6.
# ----------------------------------------------------------------------------


def pico_sheet() -> str:  # LEGACY r18: nicht mehr geschrieben (s. legacy_pico2/)
    """Sheet 2: Pico 2 (RP2350) per SPEC v0.6 §5. [LEGACY — durch stm32h743_sheet ersetzt]

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
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
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
            footprint="field_ambience:SW_TS1088_SMD",
            extra_props={"MPN": "TS-1088-AR02016", "LCSC": "C720477"},
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

    # ---- GP26 (Pin 31, y=98.73) → BAT_SENSE (ADC0, r12). VBAT-Spannungsteiler
    # 100k/100k bringt 0..4.2V Battery auf 0..2.1V am ADC (gut innerhalb 3.3V-Range).
    # C_BAT_FILT 10nF glättet S/H-Spikes + Boost-Switching-Noise.
    # STATUS_LED1 ist nach r12 auf PCA9685 LED10 gewandert (siehe mcp_sheet).
    p31_y = pico_right_pin_y(31)
    labels.append(label(120, p31_y, "BAT_SENSE"))
    wires.append(wire(PIN_R_X, p31_y, 130, p31_y, seed_suffix="bat-sense-stub"))
    # R_BAT_DIV_TOP rotation=90 horizontal: pin1 (sx-3.81, sy), pin2 (sx+3.81, sy).
    # Verbindet VBAT (hier-input rechts) → BAT_SENSE-Net (links).
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_BAT_DIV_TOP",
            value="100k 0603 (Battery-Divider Top, r12)",
            x=135,
            y=p31_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF1003T5E", "LCSC": "C25803"},
            seed_suffix="R_BAT_DIV_TOP",
            sheet_uuid_seed=sus,
        )
    )
    # R_BAT_DIV_TOP pin1 (131.19, p31_y), pin2 (138.81, p31_y).
    wires.append(wire(131.19, p31_y, 130, p31_y, seed_suffix="r-bat-top-stub-l"))
    junctions.append(junction(130, p31_y))
    wires.append(wire(138.81, p31_y, 142, p31_y, seed_suffix="r-bat-top-stub-r"))
    hlabels.append(hier_label(142, p31_y, "BAT_PLUS", shape="input", rotation=0))

    # R_BAT_DIV_BOT (BAT_SENSE → GND) rotation=0 vertical at (130, p31_y+10).
    # rotation=0: pin1 (top) abs (sx, sy-3.81), pin2 (bottom) abs (sx, sy+3.81).
    r_bat_bot_sy = p31_y + 10
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_BAT_DIV_BOT",
            value="100k 0603 (Battery-Divider Bottom, r12)",
            x=130,
            y=r_bat_bot_sy,
            rotation=0,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF1003T5E", "LCSC": "C25803"},
            seed_suffix="R_BAT_DIV_BOT",
            sheet_uuid_seed=sus,
        )
    )
    # pin1 (top) at (130, r_bat_bot_sy-3.81), pin2 (bottom) at (130, r_bat_bot_sy+3.81)
    wires.append(wire(130, p31_y, 130, r_bat_bot_sy - 3.81, seed_suffix="r-bat-bot-top"))
    wires.append(wire(130, r_bat_bot_sy + 3.81, 130, r_bat_bot_sy + 6, seed_suffix="r-bat-bot-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_R_BAT_DIV_BOT",
            value="GND",
            x=130,
            y=r_bat_bot_sy + 6,
            seed_suffix="r-bat-bot-gnd-pwr",
            sheet_uuid_seed=sus,
        )
    )

    # C_BAT_FILT 10nF (BAT_SENSE → GND) vertical at (123, r_bat_bot_sy).
    # Device:C pin1 (top) abs (sx, sy-3.81), pin2 (bottom) abs (sx, sy+3.81).
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_BAT_FILT",
            value="10nF X7R 0603 (ADC S/H filter, r12)",
            x=123,
            y=r_bat_bot_sy,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CC0603KRX7R9BB103", "LCSC": "C14858"},
            seed_suffix="C_BAT_FILT",
            sheet_uuid_seed=sus,
        )
    )
    # Connect cap top (123, r_bat_bot_sy-3.81) to BAT_SENSE net at (123, p31_y) then to (130, p31_y)
    wires.append(wire(123, r_bat_bot_sy - 3.81, 123, p31_y, seed_suffix="c-bat-filt-top-v"))
    wires.append(wire(123, p31_y, 130, p31_y, seed_suffix="c-bat-filt-top-h"))
    junctions.append(junction(123, p31_y))
    # Cap bottom to GND
    wires.append(wire(123, r_bat_bot_sy + 3.81, 123, r_bat_bot_sy + 6, seed_suffix="c-bat-filt-gnd-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_C_BAT_FILT",
            value="GND",
            x=123,
            y=r_bat_bot_sy + 6,
            seed_suffix="c-bat-filt-gnd-pwr",
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
            footprint="field_ambience:SW_TS1088_SMD",
            extra_props={"MPN": "TS-1088-AR02016", "LCSC": "C720477", "DNP": "true (THT-Pico)"},
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
    hlabels.append(hier_label(75, 65, "USB_DP", shape="input", rotation=0))
    hlabels.append(hier_label(75, 68, "USB_DM", shape="input", rotation=0))

    # ---- Funktionale GP-Pins → Hierarchical Outputs per SPEC v0.6 §5
    # v0.9 (Pi-frei): GP0/GP1/GP4 sind jetzt der I²S-Master zum PCM5102A
    # (BCK/LRCK/DIN) statt UART-zu-Pi + ungenutzter OLED-MISO. Der RP2350
    # erzeugt die I²S-Clocks per PIO — kein Raspberry Pi mehr im Audio-Pfad.
    left_signals = {
        1: "I2S_BCK",      # pin1=GP0 — PIO I²S bit clock → PCM5102A pin 13
        2: "I2S_LRCK",     # pin2=GP1 — PIO I²S word clock → PCM5102A pin 15
        4: "I2C_SDA",      # pin4=GP2
        5: "I2C_SCL",      # pin5=GP3
        6: "I2S_DOUT",     # pin6=GP4 — PIO I²S data → PCM5102A pin 14 (war OLED_MISO_NC)
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
        32: "AMP_nSHDN",# GP27
        34: "AMP_nMUTE",    # GP28
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
        f'    (rev "0.7")\n'
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




def stm32h743_sheet() -> str:
    """Sheet 2 (r18, H7-Migration Phase 3): STM32H743VIT6 per SPEC v0.7 §5.

    Ersetzt pico_sheet(). Alle Pin-Zuordnungen aus SPEC v0.7-r18 §5
    (verifiziert gegen DS12110 Rev 5 Table 8 + offizielle KiCad-Lib —
    beide Quellen stimmen für alle 52 belegten Pins überein).

    Support-Schaltung:
      - HSE: Y1 ABLS-8.000MHZ-B4-T + 2× 22 pF C0G (§5.9; GM-Analyse in
        docs/component_reviews/Y1_alternatives.md)
      - VCAP1/2: je 2.2 µF X5R (DS12110 Table 24)
      - VDD-Decoupling: 5× (4.7 µF + 100 nF) als Bank (§5.10)
      - VDDA: BLM18AG601 + 1 µF + 100 nF; VREF+ an VDDA (§5.10)
      - BOOT0 10k-Pulldown, NRST 10k-Pullup + 100 nF (§5.8)
      - SWD J4 (3-Pin, bestehend) auf PA13/PA14/GND (§5.8)
      - BAT_SENSE-Teiler 100k/100k + 10 nF an PA3 (§5.6, aus pico_sheet)
      - STATUS_LED an PD8 (§5.6)
      - MIDI_TX (PD5) nur als Netz: 🔴 TRS-Buchse fehlt im Design
        (PCB_TODO B-MIDI) — Buchse + Serien-R nach MIDI-1.0-3V3-Spec
        sind in SPEC §4 noch nicht spezifiziert.
    """
    sheet_uuid = det_uuid("sheet_stm32")
    sus = "sheet_stm32"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []
    ncs: list[str] = []
    texts: list[str] = []

    SYM_X, SYM_Y = 140.0, 130.0

    def pa(num: int) -> tuple[float, float, int]:
        lx, ly, orient = STM32_PIN_LOC[num]
        x, y = pin_abs(SYM_X, SYM_Y, lx, ly, 0)
        return (round(x, 2), round(y, 2), orient)

    # ---- U1 STM32H743VIT6
    symbols.append(
        place_symbol(
            lib_id="MCU:STM32H743VIT6",
            ref="U1",
            value="STM32H743VIT6 (480MHz M7, LQFP-100)",
            x=SYM_X,
            y=SYM_Y,
            footprint="Package_QFP:LQFP-100_14x14mm_P0.5mm",
            datasheet="https://www.st.com/resource/en/datasheet/stm32h743vi.pdf",
            extra_props={
                "MPN": "STM32H743VIT6",
                "LCSC": "C114409",
                "FP_NOTE": "KiCad-Standard LQFP-100_14x14mm_P0.5mm: 100 Pads 1.6x0.3mm @ 0.5mm Pitch (JEDEC MS-026, IPC-7351). Pin 1 (-7.675,-6); Pin 26 (-6,7.675). Verifiziert gegen KiCad-Footprints-Repo (kicad/libraries/kicad-footprints, gitlab.com/kicad/libraries).",
            },
            seed_suffix="U1",
            sheet_uuid_seed=sus,
        )
    )

    # ---- Funktions-Pins: Stub + Label (Netnamen aus SPEC §5).
    # hier == True → zusätzlich hierarchical_label (Sheet-Interface zum Root).
    NETS: dict[int, tuple[str, bool, str]] = {
        # pin: (net, hier?, hier_shape)
        3: ("I2S_LRCK", True, "output"),
        4: ("I2S_BCK", True, "output"),
        5: ("I2S_DOUT", True, "output"),
        29: ("LCD_SCK", True, "output"),
        31: ("LCD_MOSI", True, "output"),
        30: ("LCD_CS", True, "output"),
        32: ("LCD_DC", True, "output"),
        33: ("LCD_RES", True, "output"),
        92: ("I2C_SCL", True, "bidirectional"),
        93: ("I2C_SDA", True, "bidirectional"),
        7: ("MCP_INT", True, "input"),
        22: ("DRIVE_A", True, "output"),
        23: ("DRIVE_B", True, "output"),
        97: ("DRIVE_SW", True, "output"),
        63: ("BRIGHT_A", True, "output"),
        64: ("BRIGHT_B", True, "output"),
        98: ("BRIGHT_SW", True, "output"),
        59: ("DISPLAY_A", True, "output"),
        60: ("DISPLAY_B", True, "output"),
        2: ("DISPLAY_SW", True, "output"),
        67: ("VOL_A", True, "output"),
        68: ("VOL_B", True, "output"),
        86: ("MIDI_TX", False, ""),
        25: ("BAT_SENSE", False, ""),
        53: ("AMP_nSHDN", True, "output"),
        54: ("AMP_nMUTE", True, "output"),
        55: ("STATUS_LED", False, ""),
        70: ("USB_DM", True, "bidirectional"),
        71: ("USB_DP", True, "bidirectional"),
        72: ("SWDIO", False, ""),
        76: ("SWCLK", False, ""),
        89: ("SWO", False, ""),
        # r18.9 (ADR-0006) / r18.14 (ADR-0013): Cell-Velocity-Sense — lineare
        # Hall-Sensoren (unter Gateron-LP-Magnetic-Switches) an ADC-faehigen
        # Pins. INP-Kanal-Zuordnung verifiziert Phase 4 gegen DS12110 Table 8
        # ANA-Spalte (PC0/PC1/PA4/PB0/PB1 sind Standard-ADC12-Pins).
        # Pins/Nets UNVERAENDERT gegenueber FSR-Design — nur die Quelle der
        # Analogspannung wechselt (FSR-Teiler -> Hall-OUT + Serien-RC).
        15: ("CELL1_SENSE", False, ""),
        16: ("CELL2_SENSE", False, ""),
        28: ("CELL3_SENSE", False, ""),
        34: ("CELL4_SENSE", False, ""),
        35: ("CELL5_SENSE", False, ""),
        12: ("HSE_IN", False, ""),
        13: ("HSE_OUT", False, ""),
        14: ("NRST", False, ""),
        94: ("BOOT0_PIN", False, ""),
    }
    STUB = 5.0
    for num, (net, is_hier, shape) in sorted(NETS.items()):
        x, y, orient = pa(num)
        if orient == 0:        # linke Symbolseite → Stub nach links
            xe = x - STUB
            wires.append(wire(x, y, xe, y, seed_suffix=f"u1-p{num}"))
            labels.append(label(xe, y, net))
            if is_hier:
                hlabels.append(hier_label(xe - 8, y, net, shape=shape, rotation=0))
                wires.append(wire(xe, y, xe - 8, y, seed_suffix=f"u1-p{num}-h"))
        elif orient == 180:    # rechte Seite → Stub nach rechts
            xe = x + STUB
            wires.append(wire(x, y, xe, y, seed_suffix=f"u1-p{num}"))
            labels.append(label(xe, y, net))
            if is_hier:
                hlabels.append(hier_label(xe + 8, y, net, shape=shape, rotation=180))
                wires.append(wire(xe, y, xe + 8, y, seed_suffix=f"u1-p{num}-h"))
        elif orient == 90:     # Unterseite → Stub nach unten
            ye = y + STUB
            wires.append(wire(x, y, x, ye, seed_suffix=f"u1-p{num}"))
            labels.append(label(x, ye, net, rotation=270))
        else:                  # Oberseite → Stub nach oben
            ye = y - STUB
            wires.append(wire(x, y, x, ye, seed_suffix=f"u1-p{num}"))
            labels.append(label(x, ye, net, rotation=90))

    # ---- Power-Pins: VSS→GND, VDD/VBAT→+3V3 (SPEC §5.10)
    VSS_PINS = [10, 26, 49, 74]
    VDD_PINS = [11, 27, 50, 75, 100]
    for num in VSS_PINS:
        x, y, orient = pa(num)
        if orient == 90:   # Unterseite
            ye = y + 4.0
            wires.append(wire(x, y, x, ye, seed_suffix=f"u1-vss{num}"))
            symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_U1_VSS{num}", value="GND",
                                        x=x, y=ye, seed_suffix=f"u1-vss{num}", sheet_uuid_seed=sus))
        else:
            ye = y - 4.0
            wires.append(wire(x, y, x, ye, seed_suffix=f"u1-vss{num}"))
            symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_U1_VSS{num}", value="GND",
                                        x=x, y=ye, rotation=180, seed_suffix=f"u1-vss{num}", sheet_uuid_seed=sus))
    for num in VDD_PINS + [6]:  # 6 = VBAT → +3V3 (SPEC §5.10)
        x, y, orient = pa(num)
        if orient == 270:  # Oberseite
            ye = y - 4.0
            wires.append(wire(x, y, x, ye, seed_suffix=f"u1-vdd{num}"))
            symbols.append(place_symbol(lib_id="Power:+3V3", ref=f"#PWR_U1_VDD{num}", value="+3V3",
                                        x=x, y=ye, seed_suffix=f"u1-vdd{num}", sheet_uuid_seed=sus))
        elif orient == 0:  # linke Seite (VBAT pin 6)
            xe = x - 4.0
            wires.append(wire(x, y, xe, y, seed_suffix=f"u1-vdd{num}"))
            symbols.append(place_symbol(lib_id="Power:+3V3", ref=f"#PWR_U1_VDD{num}", value="+3V3",
                                        x=xe, y=y, rotation=90, seed_suffix=f"u1-vdd{num}", sheet_uuid_seed=sus))
        else:
            ye = y + 4.0
            wires.append(wire(x, y, x, ye, seed_suffix=f"u1-vdd{num}"))
            symbols.append(place_symbol(lib_id="Power:+3V3", ref=f"#PWR_U1_VDD{num}", value="+3V3",
                                        x=x, y=ye, rotation=180, seed_suffix=f"u1-vdd{num}", sheet_uuid_seed=sus))

    # ---- VSSA (19) → GND, VREF+ (20) → VDDA-Netz, VDDA (21) → Ferrit-Filter
    x19, y19, _ = pa(19)
    wires.append(wire(x19, y19, x19 - 5, y19, seed_suffix="u1-vssa"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_U1_VSSA", value="GND",
                                x=x19 - 5, y=y19, rotation=90, seed_suffix="u1-vssa", sheet_uuid_seed=sus))
    # VREF+ (20): an VDDA_3V3 + DEDIZIERTES lokales Decoupling am Pin (r18.24,
    # Audit-Punkt 5). ST DS12110 / AN: VREF+ braucht 1µF + 100nF nah am Pin für
    # rauscharmen ADC-Ref. Für die Hall-Velocity (ratiometrisch gegen denselben
    # 3V3-Rail) reduziert das v.a. Sample-Jitter. C_VREF1 (1µF) + C_VREF2 (100nF)
    # VREF+ → GND.
    x20, y20, _ = pa(20)
    wires.append(wire(x20, y20, x20 - 5, y20, seed_suffix="u1-vref"))
    labels.append(label(x20 - 5, y20, "VDDA_3V3"))
    for i, (val, mpn, lcsc) in enumerate([
        ("1uF X5R 0603 (VREF+ ref-decoupling)", "CL10A105KB8NNNC", "C15849"),
        ("100nF X7R 0603 (VREF+ HF)", "CC0603KRX7R9BB104", "C14663"),
    ]):
        vcx = x20 - 12 - i * 6
        wires.append(wire(x20 - 5, y20, vcx, y20, seed_suffix=f"vref-c{i}-rail"))
        junctions.append(junction(vcx, y20)) if i == 0 else None
        symbols.append(place_symbol(lib_id="Device:C", ref=f"C_VREF{i+1}", value=val,
                                    x=vcx, y=y20 + 3.81, rotation=0,
                                    footprint="Capacitor_SMD:C_0603_1608Metric",
                                    extra_props={"MPN": mpn, "LCSC": lcsc},
                                    seed_suffix=f"CVREF{i+1}", sheet_uuid_seed=sus))
        wires.append(wire(vcx, y20 + 7.62, vcx, y20 + 10, seed_suffix=f"vref-c{i}-gnd"))
        symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_CVREF{i+1}", value="GND",
                                    x=vcx, y=y20 + 10, seed_suffix=f"cvref{i}-gnd", sheet_uuid_seed=sus))
    x21, y21, _ = pa(21)
    wires.append(wire(x21, y21, x21 - 5, y21, seed_suffix="u1-vdda"))
    labels.append(label(x21 - 5, y21, "VDDA_3V3"))

    # VDDA-Filter-Gruppe (links unten, frei): +3V3 → FB → VDDA_3V3; 1µF + 100nF an VDDA_3V3
    fbx, fby = 96.0, 218.0
    symbols.append(place_symbol(lib_id="Device:Ferrite_Bead", ref="FB2",
                                value="BLM18AG601 600R@100MHz (VDDA-Filter)",
                                x=fbx, y=fby, rotation=90,
                                footprint="Inductor_SMD:L_0603_1608Metric",
                                extra_props={"MPN": "BLM18AG601SN1D", "LCSC": "C84094"},
                                seed_suffix="FB2", sheet_uuid_seed=sus))
    symbols.append(place_symbol(lib_id="Power:+3V3", ref="#PWR_FB2", value="+3V3",
                                x=fbx - 7.81, y=fby, rotation=90, seed_suffix="fb2-3v3", sheet_uuid_seed=sus))
    wires.append(wire(fbx - 7.81, fby, fbx - 3.81, fby, seed_suffix="fb2-in"))
    wires.append(wire(fbx + 3.81, fby, fbx + 9, fby, seed_suffix="fb2-out"))
    labels.append(label(fbx + 9, fby, "VDDA_3V3"))
    junctions.append(junction(fbx + 6, fby))
    for i, (val, mpn, lcsc) in enumerate([
        ("1uF X5R 0603 (VDDA)", "CL10A105KB8NNNC", "C15849"),
        ("100nF X7R 0603 (VDDA)", "CC0603KRX7R9BB104", "C14663"),
    ]):
        cx = fbx + 6 + i * 6
        cy = fby + 3.81
        if i == 1:
            wires.append(wire(fbx + 6, fby, cx, fby, seed_suffix=f"vdda-c{i}-rail"))
            junctions.append(junction(cx, fby))
        symbols.append(place_symbol(lib_id="Device:C", ref=f"C_VDDA{i+1}", value=val,
                                    x=cx, y=cy,
                                    footprint="Capacitor_SMD:C_0603_1608Metric",
                                    extra_props={"MPN": mpn, "LCSC": lcsc},
                                    seed_suffix=f"CVDDA{i+1}", sheet_uuid_seed=sus))
        wires.append(wire(cx, cy + 3.81, cx, cy + 6, seed_suffix=f"vdda-c{i}-gnd"))
        symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_CVDDA{i+1}", value="GND",
                                    x=cx, y=cy + 6, seed_suffix=f"vdda-c{i}-gnd", sheet_uuid_seed=sus))

    # ---- VCAP1 (48) / VCAP2 (73): 2.2 µF X5R an VSS (DS12110 Table 24)
    for num in (48, 73):
        x, y, orient = pa(num)
        dx = -5.0 if orient == 0 else 5.0
        xe = x + dx
        wires.append(wire(x, y, xe, y, seed_suffix=f"u1-vcap{num}"))
        cy = y + 3.81
        symbols.append(place_symbol(lib_id="Device:C", ref=f"C_VCAP{1 if num == 48 else 2}",
                                    value="2.2uF X5R 0603, ESR<100mOhm (VCAP, DS12110 Tab.24)",
                                    x=xe, y=cy,
                                    footprint="Capacitor_SMD:C_0603_1608Metric",
                                    extra_props={"MPN": "CL10A225KP8NNNC", "Manufacturer": "Samsung", "LCSC": "C24539", "VERIFY-STOCK": "JLC Extended 2.2uF/10V X5R 0603 — falls out-of-stock alternative LCSC-Nr. waehlen"},
                                    seed_suffix=f"CVCAP{num}", sheet_uuid_seed=sus))
        wires.append(wire(xe, cy + 3.81, xe, cy + 6, seed_suffix=f"u1-vcap{num}-gnd"))
        symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_VCAP{num}", value="GND",
                                    x=xe, y=cy + 6, seed_suffix=f"u1-vcap{num}-gnd", sheet_uuid_seed=sus))

    # ---- VDD-Decoupling-Bank: 5× (4.7 µF + 100 nF) — Platzierung am Pin ist
    # Layout-Aufgabe; im Schematic als Bank an +3V3/GND (SPEC §5.10).
    bx, by = 96.0, 238.0
    for i in range(5):
        for j, (val, mpn, lcsc, fp) in enumerate([
            ("4.7uF X5R 0805 10V (VDD bulk)", "CL21A475KQFNNNE", "C45783", "Capacitor_SMD:C_0805_2012Metric"),
            ("100nF X7R 0603 (VDD HF)", "CC0603KRX7R9BB104", "C14663", "Capacitor_SMD:C_0603_1608Metric"),
        ]):
            cx = bx + i * 12 + j * 5
            symbols.append(place_symbol(lib_id="Device:C",
                                        ref=f"C_VDD{i+1}{'A' if j == 0 else 'B'}",
                                        value=val, x=cx, y=by, footprint=fp,
                                        extra_props={"MPN": mpn, "LCSC": lcsc},
                                        seed_suffix=f"CVDD{i}{j}", sheet_uuid_seed=sus))
            symbols.append(place_symbol(lib_id="Power:+3V3", ref=f"#PWR_CVDD{i}{j}T", value="+3V3",
                                        x=cx, y=by - 6, seed_suffix=f"cvdd{i}{j}t", sheet_uuid_seed=sus))
            wires.append(wire(cx, by - 6, cx, by - 3.81, seed_suffix=f"cvdd{i}{j}-top"))
            symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_CVDD{i}{j}B", value="GND",
                                        x=cx, y=by + 6, seed_suffix=f"cvdd{i}{j}b", sheet_uuid_seed=sus))
            wires.append(wire(cx, by + 3.81, cx, by + 6, seed_suffix=f"cvdd{i}{j}-bot"))

    # ---- HSE: Y1 + 2× 27 pF (SPEC §5.9). Nets HSE_IN/HSE_OUT per Label.
    # r18.20 (Audit-HIGH-Fix): Load-Cap 22 -> 27 pF. CL=18pF, C_ext = 2*(CL -
    # C_stray); 22pF implizierte C_stray=7pF (zu hoch fuer 4-Lagen-Layout nahe
    # MCU). Mit realistischem C_stray~4.5pF -> C_ext = 2*(18-4.5) = 27pF.
    # Finaler Wert ist layout-abhaengig: am realen Board Frequenz mit 10x-Probe
    # am OSC_OUT messen, Cs ggf. nachjustieren (beide C_HSE gleich halten).
    yx, yy = 100.0, 196.0
    symbols.append(place_symbol(lib_id="Device:Crystal", ref="Y1",
                                value="8MHz ABLS-8.000MHZ-B4-T (CL=18pF, ESR<=80R)",
                                x=yx, y=yy,
                                footprint="field_ambience:Crystal_HC49-US-SMD_ABLS",
                                datasheet="https://abracon.com/Resonators/ABLS.pdf",
                                extra_props={
                                    "MPN": "ABLS-8.000MHZ-B4-T",
                                    "LCSC": "C596838",
                                    "FP_NOTE": "Custom-FP field_ambience:Crystal_HC49-US-SMD_ABLS — 5.6x2.1mm Pads, 9.5mm Pitch per ABLS-DS Page 3 (KiCad-Standard Crystal_SMD_HC49-SD hat nur 4.5x2.0/8.5mm).",
                                    "NOTE": "Gain Margin 2.97 Worst-Case / ~5-6 real, bewusst akzeptiert — docs/component_reviews/Y1_alternatives.md",
                                },
                                seed_suffix="Y1", sheet_uuid_seed=sus))
    wires.append(wire(yx - 5.08, yy, yx - 9, yy, seed_suffix="y1-l"))
    labels.append(label(yx - 9, yy, "HSE_IN"))
    wires.append(wire(yx + 5.08, yy, yx + 9, yy, seed_suffix="y1-r"))
    labels.append(label(yx + 9, yy, "HSE_OUT"))
    for k, side in ((1, -5.08 - 1.92), (2, 5.08 + 1.92)):
        cx = yx + side
        cy = yy + 6.35
        symbols.append(place_symbol(lib_id="Device:C", ref=f"C_HSE{k}",
                                    value="27pF C0G/NP0 0603 (HSE load, berechnet §5.9, bench-final)",
                                    x=cx, y=cy,
                                    footprint="Capacitor_SMD:C_0603_1608Metric",
                                    extra_props={"MPN": "CC0603JRNPO9BN270", "Manufacturer": "Yageo", "LCSC": "C107045", "NOTE": "27pF C0G/NP0 50V 0603, JLC Basic, ~834k Stock (r18.20b verifiziert). Vorher 22pF/C1804 (zu klein, implizierte C_stray=7pF). Final bench-tuned am OSC_OUT."},
                                    seed_suffix=f"CHSE{k}", sheet_uuid_seed=sus))
        wires.append(wire(cx, cy - 3.81, cx, yy, seed_suffix=f"chse{k}-top"))
        wires.append(wire(cx, cy + 3.81, cx, cy + 6, seed_suffix=f"chse{k}-gnd"))
        symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_CHSE{k}", value="GND",
                                    x=cx, y=cy + 6, seed_suffix=f"chse{k}-gnd", sheet_uuid_seed=sus))
        junctions.append(junction(cx, yy))

    # ---- NRST: 10k Pullup + 100nF (SPEC §5.8)
    nx, ny = 100.0, 172.0
    labels.append(label(nx, ny, "NRST"))
    wires.append(wire(nx, ny, nx + 6, ny, seed_suffix="nrst-rail"))
    symbols.append(place_symbol(lib_id="Device:R", ref="R_NRST", value="10k 0603 (NRST pull-up)",
                                x=nx + 2, y=ny - 3.81 - 1.27 + 1.27,
                                footprint="Resistor_SMD:R_0603_1608Metric",
                                extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
                                seed_suffix="RNRST", sheet_uuid_seed=sus))
    symbols.append(place_symbol(lib_id="Power:+3V3", ref="#PWR_RNRST", value="+3V3",
                                x=nx + 2, y=ny - 3.81 - 1.27 - 3.81 + 1.27, seed_suffix="rnrst-3v3", sheet_uuid_seed=sus))
    symbols.append(place_symbol(lib_id="Device:C", ref="C_NRST", value="100nF X7R 0603 (NRST debounce)",
                                x=nx + 6, y=ny + 3.81,
                                footprint="Capacitor_SMD:C_0603_1608Metric",
                                extra_props={"MPN": "CC0603KRX7R9BB104", "LCSC": "C14663"},
                                seed_suffix="CNRST", sheet_uuid_seed=sus))
    wires.append(wire(nx + 6, ny + 3.81 + 3.81, nx + 6, ny + 9.62, seed_suffix="cnrst-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_CNRST", value="GND",
                                x=nx + 6, y=ny + 9.62, seed_suffix="cnrst-gnd", sheet_uuid_seed=sus))
    junctions.append(junction(nx + 2, ny))
    junctions.append(junction(nx + 6, ny))

    # ---- BOOT0: 10k Pulldown + SW_BOOT-Tactile zu +3V3 (SPEC §5.8 + ADR-0009)
    # Ohne SW_BOOT ist USB-DFU-Flash physikalisch nicht möglich (BOOT0 ist
    # dauernd LOW). SW_BOOT zieht BOOT0 für DFU-Sequenz temporär nach +3V3.
    # Bedienung: SW_BOOT halten → NRST tippen → loslassen → USB-Mode-Flash.
    box_, boy = 100.0, 184.0
    labels.append(label(box_, boy, "BOOT0_PIN"))
    wires.append(wire(box_, boy, box_ + 2, boy, seed_suffix="boot0-rail"))
    symbols.append(place_symbol(lib_id="Device:R", ref="R_BOOT0", value="10k 0603 (BOOT0 pull-down)",
                                x=box_ + 2, y=boy + 3.81 + 1.27 - 1.27,
                                footprint="Resistor_SMD:R_0603_1608Metric",
                                extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
                                seed_suffix="RBOOT0", sheet_uuid_seed=sus))
    wires.append(wire(box_ + 2, boy + 7.62, box_ + 2, boy + 10, seed_suffix="rboot0-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_RBOOT0", value="GND",
                                x=box_ + 2, y=boy + 10, seed_suffix="rboot0-gnd", sheet_uuid_seed=sus))
    # SW_BOOT links neben dem BOOT0-Label (pin1=BOOT0_PIN, pin2=+3V3 über
    # R_BOOT_SW 1k Strombegrenzung — direkter Pull nach +3V3 würde bei
    # gleichzeitig betätigtem SW_RESET einen kurzzeitigen Querstrom-Konflikt
    # über den 10k-Pulldown erzeugen, deshalb der serielle 1k).
    bsx, bsy = box_ - 14, boy
    symbols.append(place_symbol(lib_id="Switch:SW_Push", ref="SW_BOOT",
                                value="BOOT0 Tactile (USB-DFU)",
                                x=bsx, y=bsy,
                                footprint="field_ambience:SW_TS1088_SMD",
                                extra_props={"MPN": "TS-1088-AR02016", "LCSC": "C720477",
                                             "FP_NOTE": "Mini-SMD-Tactile (kleinere Bauform als HX 12x12), nur fuer Service-Use bei Flash. r18.14: MPN korrigiert (war TS-1185A-C-A — C720477 ist XUNPU TS-1088-AR02016) + FP auf EasyEDA-verifiziertes Land-Pattern (3.9x2.9, P4.45) gewechselt; TL3342-FP passte nicht."},
                                seed_suffix="SWBOOT", sheet_uuid_seed=sus))
    # SW_Push pin1 (-5.08, 0) → BOOT0_PIN-Rail, pin2 (+5.08, 0) → R_BOOT_SW → +3V3
    wires.append(wire(bsx - 5.08, bsy, box_, bsy, seed_suffix="swboot-to-boot0"))
    symbols.append(place_symbol(lib_id="Device:R", ref="R_BOOT_SW",
                                value="1k 0603 (BOOT0-SW Strombegrenzung)",
                                x=bsx + 9, y=bsy, rotation=90,
                                footprint="Resistor_SMD:R_0603_1608Metric",
                                extra_props={"MPN": "0603WAF1001T5E", "LCSC": "C21190"},
                                seed_suffix="RBOOTSW", sheet_uuid_seed=sus))
    wires.append(wire(bsx + 5.08, bsy, bsx + 9 - 3.81, bsy, seed_suffix="swboot-to-r"))
    wires.append(wire(bsx + 9 + 3.81, bsy, bsx + 14, bsy, seed_suffix="r-to-3v3"))
    symbols.append(place_symbol(lib_id="Power:+3V3", ref="#PWR_SWBOOT", value="+3V3",
                                x=bsx + 14, y=bsy, rotation=90, seed_suffix="swboot-3v3",
                                sheet_uuid_seed=sus))

    # ---- SWD J4 (3-Pin 1.27mm, bestehend aus v0.6): SWCLK / GND / SWDIO (§5.8)
    jx, jy = 196.0, 208.0
    symbols.append(place_symbol(lib_id="Connector:Conn_01x03", ref="J4",
                                value="SWD 1.27mm (SWCLK/GND/SWDIO)",
                                x=jx, y=jy,
                                footprint="Connector_PinHeader_1.27mm:PinHeader_1x03_P1.27mm_Vertical",
                                extra_props={"MPN": "TC2030-IDC", "LCSC": "TBD"},
                                seed_suffix="J4", sheet_uuid_seed=sus))
    for off, net in ((-2.54, "SWCLK"), (2.54, "SWDIO")):
        wires.append(wire(jx - 5.08, jy + off, jx - 9, jy + off, seed_suffix=f"j4-{net}"))
        labels.append(label(jx - 9, jy + off, net))
    wires.append(wire(jx - 5.08, jy, jx - 9, jy, seed_suffix="j4-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_J4", value="GND",
                                x=jx - 9, y=jy, rotation=90, seed_suffix="j4-gnd", sheet_uuid_seed=sus))

    # ---- BAT_SENSE: BAT_PLUS → 100k/100k + 10nF → PA3 (SPEC §5.6, wie r12)
    bsx, bsy = 196.0, 226.0
    hlabels.append(hier_label(bsx - 14, bsy - 7.62, "BAT_PLUS", shape="input", rotation=0))
    wires.append(wire(bsx - 14, bsy - 7.62, bsx, bsy - 7.62, seed_suffix="bat-in"))
    symbols.append(place_symbol(lib_id="Device:R", ref="R_BAT_DIV_TOP",
                                value="100k 0603 (BAT divider top)",
                                x=bsx, y=bsy - 3.81,
                                footprint="Resistor_SMD:R_0603_1608Metric",
                                extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
                                seed_suffix="RBATT", sheet_uuid_seed=sus))
    junctions.append(junction(bsx, bsy))
    labels.append(label(bsx + 2, bsy, "BAT_SENSE"))
    wires.append(wire(bsx, bsy, bsx + 2, bsy, seed_suffix="bat-tap"))
    symbols.append(place_symbol(lib_id="Device:R", ref="R_BAT_DIV_BOT",
                                value="100k 0603 (BAT divider bottom)",
                                x=bsx, y=bsy + 3.81,
                                footprint="Resistor_SMD:R_0603_1608Metric",
                                extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
                                seed_suffix="RBATB", sheet_uuid_seed=sus))
    wires.append(wire(bsx, bsy + 7.62, bsx, bsy + 10, seed_suffix="rbatb-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_RBATB", value="GND",
                                x=bsx, y=bsy + 10, seed_suffix="rbatb-gnd", sheet_uuid_seed=sus))
    symbols.append(place_symbol(lib_id="Device:C", ref="C_BAT_FILT",
                                value="10nF X7R 0603 (ADC S/H filter)",
                                x=bsx + 8, y=bsy + 3.81,
                                footprint="Capacitor_SMD:C_0603_1608Metric",
                                extra_props={"MPN": "0603B103K500NT", "LCSC": "C57112"},
                                seed_suffix="CBATF", sheet_uuid_seed=sus))
    wires.append(wire(bsx, bsy, bsx + 8, bsy, seed_suffix="cbatf-top-rail"))
    wires.append(wire(bsx + 8, bsy, bsx + 8, bsy + 3.81 - 3.81, seed_suffix="cbatf-top"))
    wires.append(wire(bsx + 8, bsy + 7.62, bsx + 8, bsy + 10, seed_suffix="cbatf-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_CBATF", value="GND",
                                x=bsx + 8, y=bsy + 10, seed_suffix="cbatf-gnd", sheet_uuid_seed=sus))

    # ---- STATUS_LED an PD8 (SPEC §5.6): R 820R → LED → GND.
    # 820R an 3V3-GPIO ≈ 0.4–0.7 mA (warm-white Vf≈2.8V) — bewusst dezenter
    # Heartbeat; vorhandene BOM-Position wiederverwendet. Phase 5: ggf. anpassen.
    sx, sy = 230.0, 226.0
    labels.append(label(sx - 4, sy - 7.62, "STATUS_LED"))
    wires.append(wire(sx - 4, sy - 7.62, sx, sy - 7.62, seed_suffix="sled-in"))
    symbols.append(place_symbol(lib_id="Device:R", ref="R_SLED", value="820R 0603 (status LED)",
                                x=sx, y=sy - 3.81,
                                footprint="Resistor_SMD:R_0603_1608Metric",
                                extra_props={"MPN": "0603WAF8200T5E", "LCSC": "C23253"},
                                seed_suffix="RSLED", sheet_uuid_seed=sus))
    symbols.append(place_symbol(lib_id="Device:LED", ref="LED_HB",
                                value="0603 warm-white (heartbeat)",
                                x=sx, y=sy + 3.81, rotation=90,
                                footprint="LED_SMD:LED_0603_1608Metric",
                                extra_props={"MPN": "XL-1608UWC-04", "LCSC": "C965808"},
                                seed_suffix="LEDHB", sheet_uuid_seed=sus))
    wires.append(wire(sx, sy + 7.62, sx, sy + 10, seed_suffix="ledhb-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_LEDHB", value="GND",
                                x=sx, y=sy + 10, seed_suffix="ledhb-gnd", sheet_uuid_seed=sus))

    # ---- r18.14 (ADR-0013, ersetzt FSR aus ADR-0006): 5x Hall-Velocity-Sense.
    # Cells werden Low-Profile-MAGNETIC-Switches (Gateron LP Magnetic Jade
    # Klasse, HiChord-artiges Tastengefuehl, lange Caps + LP-Stabilizer).
    # Der Switch selbst ist pin-los — auf dem PCB sitzt pro Cell ein linearer
    # Hall-Sensor unter dem Magnet-Stem: Position analog -> Velocity = dPos/dt.
    # ADC-Interface UNVERAENDERT (PC0/PC1/PA4/PB0/PB1, Labels CELLn_SENSE).
    # Pro Cell: DRV5056A4 SOT-23 (VCC/OUT/GND) + R 1k Serien-RC mit dem
    # bestehenden 10nF (fc ~ 16kHz) vor dem ADC.
    # r18.20: Footprint von 1x3-Header-Platzhalter auf echtes SOT-23 umgestellt
    # (Audit-HIGH-Fix). 3-Pin-Symbol Conn_01x03 Pin 1/2/3 -> SOT-23 Pad 1/2/3
    # = DRV5056 VCC/OUT/GND (TI-DS Table 4-1, r18.14 verifiziert). Site-Wiring
    # Pin1->+3V3, Pin2->CELLn_SENSE, Pin3->GND matched 1:1.
    for ci in range(5):
        jx = 100.0 + ci * 24.0
        jy = 262.0
        symbols.append(place_symbol(lib_id="Connector:Conn_01x03", ref=f"J_CELL{ci+1}",
                                    value=f"DRV5056A4 Hall Cell {ci+1} SOT-23 (Velocity, ADR-0013)",
                                    x=jx, y=jy,
                                    footprint="Package_TO_SOT_SMD:SOT-23",
                                    extra_props={
                                        "MPN": "DRV5056A4QDBZR (TI, SOT-23, 3.3V ratiometrisch, unipolar, 0.6V@B=0, magnet temp comp)",
                                        "Manufacturer": "Texas Instruments",
                                        "LCSC": "C2152902",
                                        "Datasheet": "https://www.ti.com/lit/ds/symlink/drv5056.pdf",
                                        "PINOUT-VERIFIED": "TI DRV5056 DS Table 4-1 (r18.14): SOT-23 Pin 1=VCC, Pin 2=OUT, Pin 3=GND. Symbol-Conn_01x03-Pin 1/2/3 -> SOT-23-Pad 1/2/3 = VCC/OUT/GND. Site-Wiring 1=+3V3 / 2=CELLn_SENSE / 3=GND matched 1:1.",
                                        "FP_NOTE": "r18.20: SOT-23 final (Package_TO_SOT_SMD:SOT-23, KiCad-Standard). Vorher 1x3-Header-Platzhalter (Audit-Fix). JLC-bestueckbar (Extended). Pin-1-Marker im GUI gegen TI-DS Pin Configuration pruefen.",
                                        "Mechanik": "Sensor sitzt unter dem Magnet-Stem des Gateron-LP-Magnetic-Switch (plate-mounted, ADR-0013). Lange Caps (>=2u): LP-Stabilizer links/rechts wie Spacebar.",
                                    },
                                    seed_suffix=f"JCELL{ci}", sheet_uuid_seed=sus))
        # Pin1 (oben, jy-2.54) -> +3V3
        wires.append(wire(jx - 5.08, jy - 2.54, jx - 9.08, jy - 2.54, seed_suffix=f"jcell{ci}-3v3"))
        symbols.append(place_symbol(lib_id="Power:+3V3", ref=f"#PWR_JCELL{ci}", value="+3V3",
                                    x=jx - 9.08, y=jy - 2.54, rotation=90,
                                    seed_suffix=f"jcell{ci}-3v3", sheet_uuid_seed=sus))
        # Pin2 (mitte, jy) -> R_CELL 1k Serie -> Knoten K -> Label CELLn_SENSE
        # R horizontal (rotation=90): Pins bei sym_x +/- 3.81
        rx = jx - 12.89
        kx, ky = jx - 16.70, jy
        wires.append(wire(jx - 5.08, jy, rx + 3.81, jy, seed_suffix=f"jcell{ci}-k"))
        symbols.append(place_symbol(lib_id="Device:R", ref=f"R_CELL{ci+1}",
                                    value="1k 0603 (Hall-OUT Serien-R, RC mit C_CELL)",
                                    x=rx, y=jy, rotation=90,
                                    footprint="Resistor_SMD:R_0603_1608Metric",
                                    extra_props={"MPN": "0603WAF1001T5E", "LCSC": "C21190"},
                                    seed_suffix=f"RCELL{ci}", sheet_uuid_seed=sus))
        labels.append(label(kx, ky, f"CELL{ci+1}_SENSE"))
        junctions.append(junction(kx, ky))
        # Pin3 (unten, jy+2.54) -> GND
        wires.append(wire(jx - 5.08, jy + 2.54, jx - 7.5, jy + 2.54, seed_suffix=f"jcell{ci}-gnd"))
        symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_JCELL{ci}G", value="GND",
                                    x=jx - 7.5, y=jy + 2.54, rotation=90,
                                    seed_suffix=f"jcell{ci}-gnd", sheet_uuid_seed=sus))
        # C_CELL 10nF am Knoten K -> GND (RC-Filter, fc ~ 16kHz mit 1k)
        wires.append(wire(rx - 3.81, ky, kx, ky, seed_suffix=f"ccell{ci}-rail"))
        symbols.append(place_symbol(lib_id="Device:C", ref=f"C_CELL{ci+1}",
                                    value="10nF X7R 0603 (RC-Filter mit R_CELL 1k)",
                                    x=kx, y=ky + 3.81,
                                    footprint="Capacitor_SMD:C_0603_1608Metric",
                                    extra_props={"MPN": "0603B103K500NT", "LCSC": "C57112"},
                                    seed_suffix=f"CCELL{ci}", sheet_uuid_seed=sus))
        wires.append(wire(kx, ky + 7.62, kx, ky + 10, seed_suffix=f"ccell{ci}-gnd"))
        symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_CCELL{ci}", value="GND",
                                    x=kx, y=ky + 10, seed_suffix=f"ccell{ci}-gnd", sheet_uuid_seed=sus))

    # ---- MIDI-Out (J10) — ENTSCHIEDEN r18.67 (ADR-0004 / User): TRS **Type A**,
    # OUT-only, 3.3V direkt (MIDI-Spec CA-033, kein Level-Shifter). Verschaltung:
    #   MIDI_TX (PD5) --R_MIDI_TX 220R--> Tip ("cold/data")
    #   +3V3        --R_MIDI_REF 220R--> Ring ("hot/ref")
    #   Sleeve --> GND ;  DET-Pin = no_connect (MIDI nutzt keinen Insert-Detect)
    # Eigener Refdes **J10** (J9 ist der Akku-Stecker, NICHT MIDI!). Verbindung via
    # Net-Labels (MIDI_TX/MIDI_TIP/MIDI_RING) = robust, keine langen Wires.
    # 220R-LCSC = NEEDS-VERIFY (Anti-Guess) → NO-LCSC-Liste.
    symbols.append(place_symbol(
        lib_id="Connector:AudioJack3_Switch", ref="J10",
        value="3.5mm TRS MIDI-Out (Type A, PJ-320 class)",
        x=150, y=258, footprint="field_ambience:Jack_3.5mm_PJ-320D_SMT",
        extra_props={"MPN": "PJ-320D (3.5mm TRS w/ switch)", "LCSC": "C431535"},
        seed_suffix="J10", sheet_uuid_seed=sus))
    # J10 Tip (157.62, 255.46) -> MIDI_TIP
    wires.append(wire(157.62, 255.46, 161, 255.46, seed_suffix="j10-tip"))
    labels.append(label(161, 255.46, "MIDI_TIP"))
    # J10 Ring (157.62, 258) -> MIDI_RING
    wires.append(wire(157.62, 258, 161, 258, seed_suffix="j10-ring"))
    labels.append(label(161, 258, "MIDI_RING"))
    # J10 Sleeve (157.62, 260.54) -> GND
    wires.append(wire(157.62, 260.54, 161, 260.54, seed_suffix="j10-sleeve"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_J10_S", value="GND",
        x=161, y=260.54, rotation=270, seed_suffix="j10-s-gnd", sheet_uuid_seed=sus))
    # J10 DET (142.38, 258) -> no_connect
    ncs.append(no_connect(142.38, 258))
    # R_MIDI_TX 220R: MIDI_TX -> Tip
    symbols.append(place_symbol(lib_id="Device:R", ref="R_MIDI_TX",
        value="220R 0603 (MIDI TX series, Type-A Tip)", x=130, y=255.46, rotation=90,
        footprint="Resistor_SMD:R_0603_1608Metric",
        extra_props={"MPN": "0603WAF2200T5E (220R 0603)", "LCSC": "C22962"},
        seed_suffix="R_MIDI_TX", sheet_uuid_seed=sus))
    wires.append(wire(126.19, 255.46, 123, 255.46, seed_suffix="rmiditx-tx"))
    labels.append(label(123, 255.46, "MIDI_TX"))
    wires.append(wire(133.81, 255.46, 137, 255.46, seed_suffix="rmiditx-tip"))
    labels.append(label(137, 255.46, "MIDI_TIP"))
    # R_MIDI_REF 220R: +3V3 -> Ring
    symbols.append(place_symbol(lib_id="Device:R", ref="R_MIDI_REF",
        value="220R 0603 (MIDI ref, Type-A Ring)", x=130, y=258, rotation=90,
        footprint="Resistor_SMD:R_0603_1608Metric",
        extra_props={"MPN": "0603WAF2200T5E (220R 0603)", "LCSC": "C22962"},
        seed_suffix="R_MIDI_REF", sheet_uuid_seed=sus))
    wires.append(wire(126.19, 258, 123, 258, seed_suffix="rmidiref-3v3"))
    symbols.append(place_symbol(lib_id="Power:+3V3", ref="#PWR_R_MIDI_REF", value="+3V3",
        x=123, y=258, rotation=90, seed_suffix="rmidiref-3v3-pwr", sheet_uuid_seed=sus))
    wires.append(wire(133.81, 258, 137, 258, seed_suffix="rmidiref-ring"))
    labels.append(label(137, 258, "MIDI_RING"))
    texts.append(
        f'  (text "MIDI-Out J10 (TRS Type A, OUT-only, 3.3V/CA-033). MIDI_TX=PD5. J9=Akku!" (at 100 250 0)\n'
        f'    (effects (font (size 1.27 1.27)) (justify left bottom))\n'
        f'    (uuid "{det_uuid("txt-midi")}"))\n'
    )
    texts.append(
        f'  (text "SWO (PB3): optionaler Trace-Pin, bewusst nur Label (ERC-Info erwartet)." (at 100 252 0)\n'
        f'    (effects (font (size 1.27 1.27)) (justify left bottom))\n'
        f'    (uuid "{det_uuid("txt-swo")}"))\n'
    )

    # ---- Unbelegte GPIOs: no_connect (Reserve, SPEC §5 „~50 GPIOs frei")
    used = set(NETS) | set(VSS_PINS) | set(VDD_PINS) | {6, 19, 20, 21, 48, 73}
    for num in range(1, 101):
        if num in used:
            continue
        x, y, _ = pa(num)
        ncs.append(no_connect(x, y))

    header = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience — Sheet 2: STM32H743 (r18 H7-Migration)")\n'
        f'    (date "2026-06-11")\n'
        f'    (rev "0.7-r18")\n'
        f'    (company "Field Ambience Project"))\n'
        '  (lib_symbols\n' + LIB_SYMBOLS + '\n  )\n'
    )
    return (header + "".join(symbols) + "".join(wires) + "".join(junctions)
            + "".join(labels) + "".join(hlabels) + "".join(ncs) + "".join(texts)
            + '  (sheet_instances\n    (path "/" (page "2")))\n' + ")\n")


def lcd_sheet() -> str:
    """Sheet 3 (r16/r18): 1.9-Zoll ST7789 320×170 IPS-LCD, 8-Pin-SPI-Header
    per SPEC v0.7 §6 (ersetzt das 16-Pin-SSD1322-OLED aus v0.6).

    Netnamen LCD_* folgen SPEC §5.2 (r18) — §6 nannte historisch OLED_*;
    die SPEC bekommt dazu eine r18.5-Fußnote.

    Backlight: BLK (Header-Pin 8) über Q2 2N7002 Low-Side, Gate an
    PCA9685-Kanal 12 (mcp_sheet, Net LCD_BLK_PWM), 100k Gate-Pulldown
    (Default: Backlight aus bis I²C-Init — §12.5-Boot-Default macht die
    Firmware über PCA9685-Init).
    """
    sheet_uuid = det_uuid("sheet_lcd")
    sus = "sheet_lcd"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    JX, JY = 140.0, 100.0   # J3 Conn_01x08, Pin 1 oben; Pins links (x-3.81? generisch)
    symbols.append(place_symbol(lib_id="Connector:Conn_01x08", ref="J3",
                                value="LCD 1.9in ST7789 320x170 SPI-Header (8-pin)",
                                x=JX, y=JY,
                                footprint="Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical",
                                extra_props={
                                    "MPN": "TBD (LCD-Modul separat: Adafruit 5394-Klasse / ER-TFT019)",
                                    "LCSC": "TBD (Modul, separat bestellen)",
                                    "FP_NOTE": "Standard 2.54mm 1x8-Header (PinHeader_1x08_P2.54mm_Vertical, KiCad-Standard, JEDEC). Adafruit 5394-Pinraster auf 2.54mm — passt 1:1.",
                                },
                                seed_suffix="J3", sheet_uuid_seed=sus))
    # Conn_01xN: Pin k at local (-5.08? ) — wie oled_sheet: Pins links, y = top + (k-1)*2.54
    # _conn_01xN_lib_symbol: pin at (-5.08, ...) → abs x = JX-5.08.
    PINX = JX - 5.08
    def jpy(k: int) -> float:
        return JY + (k - 1) * 2.54 - (8 - 1) * 2.54 / 2  # zentriert

    # Pin 1 GND
    wires.append(wire(PINX, jpy(1), PINX - 5, jpy(1), seed_suffix="j3-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_J3_GND", value="GND",
                                x=PINX - 5, y=jpy(1), rotation=90, seed_suffix="j3-gnd", sheet_uuid_seed=sus))
    # Pin 2 VCC +3V3 + lokale C6b/C6c
    wires.append(wire(PINX, jpy(2), PINX - 5, jpy(2), seed_suffix="j3-vcc"))
    symbols.append(place_symbol(lib_id="Power:+3V3", ref="#PWR_J3_VCC", value="+3V3",
                                x=PINX - 5, y=jpy(2), rotation=90, seed_suffix="j3-vcc", sheet_uuid_seed=sus))
    cbx = PINX - 14
    labels.append(label(cbx + 2, jpy(2), "+3V3"))
    for k, (ref, val, mpn, lcsc, fp) in enumerate([
        ("C6b", "10uF X5R 0805 (LCD VCC bulk)", "CL21A106KOQNNNE", "C15850", "Capacitor_SMD:C_0805_2012Metric"),
        ("C6c", "100nF X7R 0603 (LCD VCC HF)", "CC0603KRX7R9BB104", "C14663", "Capacitor_SMD:C_0603_1608Metric"),
    ]):
        cx = cbx - k * 6
        symbols.append(place_symbol(lib_id="Device:C", ref=ref, value=val, x=cx, y=jpy(2) + 3.81,
                                    footprint=fp, extra_props={"MPN": mpn, "LCSC": lcsc},
                                    seed_suffix=ref, sheet_uuid_seed=sus))
        symbols.append(place_symbol(lib_id="Power:+3V3", ref=f"#PWR_{ref}T", value="+3V3",
                                    x=cx, y=jpy(2) - 2, seed_suffix=f"{ref}t", sheet_uuid_seed=sus))
        wires.append(wire(cx, jpy(2) - 2, cx, jpy(2), seed_suffix=f"{ref}-top"))
        wires.append(wire(cx, jpy(2) + 7.62, cx, jpy(2) + 10, seed_suffix=f"{ref}-gnd"))
        symbols.append(place_symbol(lib_id="Power:GND", ref=f"#PWR_{ref}B", value="GND",
                                    x=cx, y=jpy(2) + 10, seed_suffix=f"{ref}b", sheet_uuid_seed=sus))
    # Pins 3-7: SCL/SDA/RES/DC/CS ← hier_labels (vom STM32-Sheet via Root)
    for k, net in ((3, "LCD_SCK"), (4, "LCD_MOSI"), (5, "LCD_RES"), (6, "LCD_DC"), (7, "LCD_CS")):
        wires.append(wire(PINX, jpy(k), PINX - 8, jpy(k), seed_suffix=f"j3-{net}"))
        hlabels.append(hier_label(PINX - 8, jpy(k), net, shape="input", rotation=0))
    # Pin 8 BLK ← Q2 Drain (Low-Side-Switch)
    qx, qy = PINX - 16, jpy(8) + 8
    wires.append(wire(PINX, jpy(8), qx + 2.54, jpy(8), seed_suffix="j3-blk"))
    wires.append(wire(qx + 2.54, jpy(8), qx + 2.54, qy - 3.81, seed_suffix="q2-drain"))
    symbols.append(place_symbol(lib_id="Transistor_FET:Q_NMOS_GSD", ref="Q2",
                                value="2N7002 (LCD-Backlight Low-Side)",
                                x=qx, y=qy,
                                footprint="Package_TO_SOT_SMD:SOT-23",
                                extra_props={"MPN": "2N7002,215", "Manufacturer": "Nexperia", "LCSC": "C8545", "Package": "SOT-23 (TO-236), pin 1=G, 2=S, 3=D — JEDEC-Standard", "VERIFY-STOCK": "JLC Basic 2N7002, alle Marken haben G/S/D=1/2/3"},
                                seed_suffix="Q2", sheet_uuid_seed=sus))
    wires.append(wire(qx + 2.54, qy + 3.81, qx + 2.54, qy + 7, seed_suffix="q2-src-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_Q2", value="GND",
                                x=qx + 2.54, y=qy + 7, seed_suffix="q2-gnd", sheet_uuid_seed=sus))
    # Gate: hier_label LCD_BLK_PWM + 100k Pulldown
    wires.append(wire(qx - 2.54, qy, qx - 8, qy, seed_suffix="q2-gate"))
    junctions.append(junction(qx - 6, qy))
    hlabels.append(hier_label(qx - 14, qy, "LCD_BLK_PWM", shape="input", rotation=0))
    wires.append(wire(qx - 8, qy, qx - 14, qy, seed_suffix="q2-gate-h"))
    symbols.append(place_symbol(lib_id="Device:R", ref="R_BLK_PD",
                                value="100k 0603 (gate pull-down, BL default off)",
                                x=qx - 6, y=qy + 3.81,
                                footprint="Resistor_SMD:R_0603_1608Metric",
                                extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
                                seed_suffix="RBLKPD", sheet_uuid_seed=sus))
    wires.append(wire(qx - 6, qy + 7.62, qx - 6, qy + 10, seed_suffix="rblkpd-gnd"))
    symbols.append(place_symbol(lib_id="Power:GND", ref="#PWR_RBLKPD", value="GND",
                                x=qx - 6, y=qy + 10, seed_suffix="rblkpd-gnd", sheet_uuid_seed=sus))

    header = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A4")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience — Sheet 3: LCD ST7789 (r16/r18)")\n'
        f'    (date "2026-06-11")\n'
        f'    (rev "0.7-r18")\n'
        f'    (company "Field Ambience Project"))\n'
        '  (lib_symbols\n' + LIB_SYMBOLS + '\n  )\n'
    )
    return (header + "".join(symbols) + "".join(wires) + "".join(junctions)
            + "".join(labels) + "".join(hlabels)
            + '  (sheet_instances\n    (path "/" (page "3")))\n' + ")\n")


def oled_sheet() -> str:  # LEGACY r18: nicht mehr geschrieben (durch lcd_sheet ersetzt)
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
        f'    (rev "0.7")\n'
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
            extra_props={"MPN": "0603B103K500NT", "LCSC": "C57112"},
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
            extra_props={"MPN": "0603WAF4701T5E", "LCSC": "C23162"},
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
            extra_props={"MPN": "0603WAF4701T5E", "LCSC": "C23162"},
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
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
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
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
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
    # ---- r18.9 (ADR-0006): Cells sind KEINE MCP-Switches mehr. Die 5 Cell-
    # Inputs sind jetzt FSR-Velocity-Pads am STM32-ADC (stm32h743_sheet,
    # Netze CELL1..5_SENSE an PC0/PC1/PA4/PB0/PB1). Die Kailh-Choc-Hotswap-
    # Sockets + Stabilizer entfallen aus der BOM. GPA0-4 frei (NC, Rev-B-
    # Reserve).
    for mcp_pin in (21, 22, 23, 24, 25):
        py = mcp_right_pin_y(mcp_pin)
        wires.append(wire(PIN_R_X, py, 145, py, seed_suffix=f"u2-cell-stub-{mcp_pin}"))
        labels.append(label(145, py, f"NC_GPA{mcp_pin - 21}"))

    # ---- Switches SW6-SW10 (Modifier) auf GPB0-GPB4 (pins 1-5). Links vom MCP.
    # SW_Push reversed: pin2 (right) connects to MCP via pin1 to GND.
    # Or: simpler: place SW left of MCP, SW pin2 → MCP-pin, SW pin1 → GND.
    # SW at (sx, sy) — pin1 abs (sx-5.08, sy), pin2 abs (sx+5.08, sy).
    # We want pin2 at x=PIN_L_X - 5 (left of left-stub). For pin2 abs x = 90: sx = 84.92. pin1 abs x = 79.84.
    # r18.68: SW6-10 = TC-1212-7.3-260G THT-Tactile (C2845240, eckiger Kopf fuer Caps), KEINE integrierte LED.
    #      Status-LEDs LED6-LED10 sind separate SMD-0603 unter PCA9685-Kontrolle (siehe PCA9685-Block weiter unten).
    mod_pins = [(1, "MOD_SHIFT", 6), (2, "MOD_HOLD", 7), (3, "MOD_DRONE", 8), (4, "MOD_GENERATE", 9), (5, "MOD_CLEAR", 10)]
    for mcp_pin, netname, sw_num in mod_pins:
        py = mcp_left_pin_y(mcp_pin)
        wires.append(wire(PIN_L_X, py, 90, py, seed_suffix=f"u2-mod-stub-{mcp_pin}"))
        labels.append(label(115, py, netname))
        symbols.append(
            place_symbol(
                lib_id="Switch:SW_Push",
                ref=f"SW{sw_num}",
                value=f"Modifier {netname.replace('MOD_','')} (TC-1212-7.3 THT-Tactile, eckiger Kopf fuer Caps, r18.68)",
                x=84.92,
                y=py,
                footprint="field_ambience:SW_TC1212-7.3_THT_4P",
                datasheet="https://jlcpcb.com/partdetail/C2845240",
                extra_props={
                    "MPN": "TC-1212-7.3-260G",
                    "Manufacturer": "HCTL",
                    "LCSC": "C2845240",
                    "Package": "THT 4-pin 11.8x11.8mm, square-head plunger 7.3mm (clip-on caps), SPST 2.6N, 100k cycles, 12V/50mA",
                    "FP_NOTE": "field_ambience:SW_TC1212-7.3_THT_4P (easyeda2kicad C2845240, +STEP). 4 THT-Pads; SPST 2-Terminal: Top-Kante (y=-2.54)=Pin1, Bottom-Kante (y=+2.54)=Pin2 (per User-verifizierter HX-Konvention) — bei GUI-ERC bestaetigen.",
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
    # r18: GPB5 (Pin 6) = EN4-Volume-Switch (SPEC §5.4 — VOL_SW bleibt am MCP;
    # das Schematic hatte VOL_SW bis r17 faelschlich am Pico-GPIO, mcp-GPB5 war NC).
    py6 = mcp_left_pin_y(6)
    wires.append(wire(PIN_L_X, py6, 115, py6, seed_suffix="u2-gpb-nc-6"))
    labels.append(label(115, py6, "VOL_SW"))
    wires.append(wire(115, py6, 122, py6, seed_suffix="u2-gpb5-hier"))
    hlabels.append(hier_label(122, py6, "VOL_SW", shape="input", rotation=180))
    for pin in (7, 8):
        py = mcp_left_pin_y(pin)
        wires.append(wire(PIN_L_X, py, 115, py, seed_suffix=f"u2-gpb-nc-{pin}"))
        labels.append(label(115, py, f"NC_GPB{pin-1}"))
    # GPA5 (Pin 26): v0.6.3-r5 N1-Fix — drives PCM5102A XSMT via hier-output
    py = mcp_right_pin_y(26)
    wires.append(wire(PIN_R_X, py, 152, py, seed_suffix="u2-gpa5-xsmt"))
    labels.append(label(145, py, "GPA5/XSMT"))
    hlabels.append(hier_label(152, py, "PCM_XSMT", shape="output", rotation=180))
    # GPA6 (Pin 27): v0.7 — jack-detect input vom Line-Out (J8 DET switch).
    # MCP-interner Pull-Up + IRQ-on-change (Firmware-config). Idle (kein Plug,
    # Switch closed) = LOW; Plug eingesteckt (Switch open) = HIGH.
    py = mcp_right_pin_y(27)
    wires.append(wire(PIN_R_X, py, 152, py, seed_suffix="u2-gpa6-jackdet"))
    labels.append(label(145, py, "GPA6/JACKDET"))
    hlabels.append(hier_label(152, py, "JACK_DETECT", shape="input", rotation=180))
    # GPA7 (Pin 28) — r12: USB-C-VBUS-Detect via 10k Series + 100k Pull-Down.
    # VBUS=5V (USB-C verbunden) → MCP liest HIGH; VBUS=0V → LOW (Battery-Mode).
    # MCP I/O 5.5V-tolerant per Datasheet → 4.55V am Pin safe.
    py = mcp_right_pin_y(28)
    # MCP-Pin → R_VBUS_SENSE pin1 (Wire endet exakt am Pin, sonst 0.19mm-Gap = dangling)
    wires.append(wire(PIN_R_X, py, 152.19, py, seed_suffix="u2-gpa7-stub"))
    labels.append(label(149, py, "USB_VBUS_SENSE"))
    # R_VBUS_SENSE 10k rotation=90 horizontal at (156, py): pin1 (152.19), pin2 (159.81)
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_VBUS_SENSE",
            value="10k 0603 (USB-VBUS Series, r12)",
            x=156,
            y=py,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
            seed_suffix="R_VBUS_SENSE",
            sheet_uuid_seed=sus,
        )
    )
    # R_VBUS_SENSE pin2 → hier-input VBUS_USBC (raw USB-C VBUS pre-fuse)
    wires.append(wire(159.81, py, 165, py, seed_suffix="r-vbus-sense-to-hier"))
    hlabels.append(hier_label(165, py, "VBUS_USBC", shape="input", rotation=180))
    # R_VBUS_PD 100k pull-down: rotation=0 vertical at (149, py+10).
    r_vbus_pd_sy = py + 10
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_VBUS_PD",
            value="100k 0603 (USB-VBUS Pull-Down, r12)",
            x=149,
            y=r_vbus_pd_sy,
            rotation=0,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF1003T5E", "LCSC": "C25803"},
            seed_suffix="R_VBUS_PD",
            sheet_uuid_seed=sus,
        )
    )
    # R_VBUS_PD pin1 (top, sy-3.81) → join the GPA7-net at (149, py); pin2 (bottom, sy+3.81) → GND
    wires.append(wire(149, py, 149, r_vbus_pd_sy - 3.81, seed_suffix="r-vbus-pd-top"))
    junctions.append(junction(149, py))
    wires.append(wire(149, r_vbus_pd_sy + 3.81, 149, r_vbus_pd_sy + 6, seed_suffix="r-vbus-pd-gnd"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_R_VBUS_PD",
            value="GND",
            x=149,
            y=r_vbus_pd_sy + 6,
            seed_suffix="r-vbus-pd-gnd-pwr",
            sheet_uuid_seed=sus,
        )
    )

    # ========================================================================
    # r10-LED — U6 PCA9685PW LED-Driver + 10× SMD-0603 Status-LEDs
    # ========================================================================
    # U6 sitzt UNTER U2 (eigener Koord-Space, ~40 mm Gap). I²C teilt sich Bus mit
    # MCP via existierende I2C_SDA/I2C_SCL Nets (Same-Name-Label-Matching).
    # Adresse: A0-A5 = GND → 0x40 (Default).
    # 10 LEDs in 5×2 Grid weiter unten (y=200..215). Channels:
    #   LED0-LED4 → LED6-LED10 (Modifier-Buttons SHIFT/HOLD/DRONE/GENERATE/CLEAR)
    #   LED5-LED9 → LED11-LED15 (Cell-HOLD-Status CELL1..CELL5)
    #   LED10-LED15 reserve (LED10 wird r12 = System-Status, ersetzt GP26-LED1)

    U6_X, U6_Y = 130.0, 165.0
    PCA_PIN_L_X = U6_X - 12.7  # 117.3
    PCA_PIN_R_X = U6_X + 12.7  # 142.7

    def pca_left_pin_y(pin: int) -> float:
        """PCA9685 left pin (1..14, top→bottom). Pin 1 at U6_Y - 16.51."""
        return U6_Y - 16.51 + (pin - 1) * 2.54

    def pca_right_pin_y(pin: int) -> float:
        """PCA9685 right pin (28..15, top→bottom). Pin 28 at U6_Y - 16.51."""
        idx = 28 - pin
        return U6_Y - 16.51 + idx * 2.54

    symbols.append(
        place_symbol(
            lib_id="Driver_LED:PCA9685PW",
            ref="U6",
            value="PCA9685PW (16-Ch I²C PWM LED-Driver)",
            x=U6_X,
            y=U6_Y,
            footprint="Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm",
            datasheet="https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf",
            extra_props={
                "MPN": "PCA9685PW,118",
                "LCSC": "C2678753",
            },
            seed_suffix="U6",
            sheet_uuid_seed=sus,
        )
    )

    # ---- PCA-A0..A4 (Pins 1-5, left side) → GND. Address 0x40.
    for pin in (1, 2, 3, 4, 5):
        py = pca_left_pin_y(pin)
        wires.append(wire(PCA_PIN_L_X, py, 113, py, seed_suffix=f"u6-a-{pin}"))
        symbols.append(
            place_symbol(
                lib_id="Power:GND",
                ref=f"#PWR_U6_A{pin-1}",
                value="GND",
                x=113,
                y=py,
                rotation=90,
                seed_suffix=f"u6-a-{pin}-gnd",
                sheet_uuid_seed=sus,
            )
        )

    # ---- PCA-A5 (Pin 24, right side) → GND
    py = pca_right_pin_y(24)
    wires.append(wire(PCA_PIN_R_X, py, 147, py, seed_suffix="u6-a5"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_U6_A5",
            value="GND",
            x=147,
            y=py,
            rotation=270,
            seed_suffix="u6-a5-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- VSS (Pin 14, left, bottommost) → GND
    p14_y = pca_left_pin_y(14)
    wires.append(wire(PCA_PIN_L_X, p14_y, 113, p14_y, seed_suffix="u6-vss"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_U6_VSS",
            value="GND",
            x=113,
            y=p14_y,
            rotation=90,
            seed_suffix="u6-vss-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- VDD (Pin 28, right, topmost) → +3V3 + C_PCA_VDD 10µF + C_PCA_VDD_HF 100nF
    p28_y = pca_right_pin_y(28)
    wires.append(wire(PCA_PIN_R_X, p28_y, 152, p28_y, seed_suffix="u6-vdd-stub"))
    symbols.append(
        place_symbol(
            lib_id="Power:+3V3",
            ref="#PWR_U6_VDD",
            value="+3V3",
            x=152,
            y=p28_y,
            rotation=0,
            seed_suffix="u6-vdd-3v3",
            sheet_uuid_seed=sus,
        )
    )
    # C_PCA_VDD bulk 10µF at (158, p28_y+5)
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_PCA_VDD",
            value="10uF X5R 0805 (PCA VDD bulk, r7)",
            x=158,
            y=p28_y + 5,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A106KOQNNNG-class", "LCSC": "C15850"},
            seed_suffix="C_PCA_VDD",
            sheet_uuid_seed=sus,
        )
    )
    # Cap pin1 (top) at (158, p28_y+5-3.81), pin2 (bottom) at (158, p28_y+5+3.81)
    wires.append(wire(158, p28_y + 5 - 3.81, 158, p28_y, seed_suffix="c-pca-vdd-top"))
    wires.append(wire(152, p28_y, 158, p28_y, seed_suffix="c-pca-vdd-tee"))
    junctions.append(junction(158, p28_y))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_C_PCA_VDD",
            value="GND",
            x=158,
            y=p28_y + 5 + 5,
            rotation=0,
            seed_suffix="c-pca-vdd-gnd",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(158, p28_y + 5 + 3.81, 158, p28_y + 5 + 5, seed_suffix="c-pca-vdd-bot"))
    # C_PCA_VDD_HF 100nF at (164, p28_y+5)
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_PCA_VDD_HF",
            value="100nF X7R 0603 (PCA VDD HF, r7)",
            x=164,
            y=p28_y + 5,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10B104KO8NNNC-class", "LCSC": "C14663"},
            seed_suffix="C_PCA_VDD_HF",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(164, p28_y + 5 - 3.81, 164, p28_y, seed_suffix="c-pca-vddhf-top"))
    wires.append(wire(158, p28_y, 164, p28_y, seed_suffix="c-pca-vddhf-tee"))
    junctions.append(junction(164, p28_y))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_C_PCA_VDD_HF",
            value="GND",
            x=164,
            y=p28_y + 5 + 5,
            rotation=0,
            seed_suffix="c-pca-vddhf-gnd",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(164, p28_y + 5 + 3.81, 164, p28_y + 5 + 5, seed_suffix="c-pca-vddhf-bot"))

    # ---- SDA (Pin 27, right) → I2C_SDA hier-label (gemeinsamer Bus mit MCP23017-Pin)
    p27_y = pca_right_pin_y(27)
    wires.append(wire(PCA_PIN_R_X, p27_y, 152, p27_y, seed_suffix="u6-sda"))
    hlabels.append(hier_label(152, p27_y, "I2C_SDA", shape="input", rotation=0))

    # ---- SCL (Pin 26, right) → I2C_SCL hier-label
    p26_y = pca_right_pin_y(26)
    wires.append(wire(PCA_PIN_R_X, p26_y, 152, p26_y, seed_suffix="u6-scl"))
    hlabels.append(hier_label(152, p26_y, "I2C_SCL", shape="input", rotation=0))

    # ---- EXTCLK (Pin 25, right) → GND. WICHTIG: per NXP-Datasheet Rev 4 S.7
    # Footnote [2] MUSS dieser Pin auf GND wenn EXTCLK ungenutzt — sonst undefined.
    p25_y = pca_right_pin_y(25)
    wires.append(wire(PCA_PIN_R_X, p25_y, 147, p25_y, seed_suffix="u6-extclk"))
    symbols.append(
        place_symbol(
            lib_id="Power:GND",
            ref="#PWR_U6_EXTCLK",
            value="GND",
            x=147,
            y=p25_y,
            rotation=270,
            seed_suffix="u6-extclk-gnd",
            sheet_uuid_seed=sus,
        )
    )

    # ---- ~OE (Pin 23, right, active LOW) → R_OE 10kΩ pull-up zu +3V3.
    # Default disabled (HIGH = LEDs off). Firmware zieht /OE LOW erst nach PWM-Init.
    # R_OE rotation=90 horizontal: pin1 abs (sx-3.81), pin2 abs (sx+3.81).
    # Wire MUSS exakt am Pin enden, sonst dangling.
    p23_y = pca_right_pin_y(23)
    wires.append(wire(PCA_PIN_R_X, p23_y, 152.19, p23_y, seed_suffix="u6-oe-stub"))
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_OE",
            value="10k 0603 (PCA9685 /OE pull-up, r7)",
            x=156,
            y=p23_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
            seed_suffix="R_OE",
            sheet_uuid_seed=sus,
        )
    )
    # R_OE pin2 (159.81, p23_y) → +3V3
    wires.append(wire(159.81, p23_y, 163, p23_y, seed_suffix="r-oe-to-3v3"))
    symbols.append(
        place_symbol(
            lib_id="Power:+3V3",
            ref="#PWR_R_OE",
            value="+3V3",
            x=163,
            y=p23_y,
            rotation=90,
            seed_suffix="r-oe-3v3",
            sheet_uuid_seed=sus,
        )
    )

    # ---- 10× LED+R pairs in 5×2 grid at y=200/215, x=80..184
    # Row 1 (y=200): LED6-LED10 (Modifier-Indicators per PCA-Ch 0-4)
    # Row 2 (y=215): LED11-LED15 (Cell-HOLD-Indicators per PCA-Ch 5-9)
    # Per pair: R (rot=90) at (sx, sy) → LED (rot=180) at (sx+7.62, sy)
    #   +5V flag at (sx-6, sy), Kathode-label at (sx+13.43, sy)
    # r18.9 (ADR-0008): 15 LEDs — 5 Modifier (farbcodiert: Shift=Gruen,
    # Hold=Gelb, Drone/Generate/Clear=Weiss) + 5x2 Cell-LEDs (Gelb=Hold@Basis,
    # Gruen=Hold@Shift, XOR — nie beide). Kanal 15 = LCD_BLK_PWM (s.u.).
    # System-Status-LED ist seit r18.5 auf MCU-PD8 (stm32h743_sheet LED_HB) —
    # der r12-PCA-Kanal-10-Status entfaellt.
    # R einheitlich 390R an +5V-Anode: Gelb/Gruen Vf~2.1V -> ~7.4mA,
    # Weiss Vf~3.0V -> ~5.1mA. Beide < 8mA PCA-Sink-Budget; Helligkeits-
    # Matching macht die Firmware per PWM-Duty (Gelb/Gruen ggf. ~70%).
    LED_W = ("XL-1608UWC-04 (warm-white 0603)", "C965808")
    LED_Y = ("KT-0603Y (Hubei KENTO, yellow 0603, Vf 2.4V)", "C2287")
    LED_G = ("KT-0603G (Hubei KENTO, pure green 525nm 0603, Vf 3.1V)", "C12624")
    led_array = [
        # (pca_channel, led_ref(str), sx, sy, label_name, descr, (mpn, lcsc))
        (0,  "6",   80, 200, "LED6_K",   "SHIFT-Modifier (gelb)",    LED_Y),
        (1,  "7",  105, 200, "LED7_K",   "HOLD-Mode (gruen)",        LED_G),
        (2,  "8",  130, 200, "LED8_K",   "DRONE-Mode (weiss)",       LED_W),
        (3,  "9",  155, 200, "LED9_K",   "GENERATE-Mode (weiss)",    LED_W),
        (4,  "10", 180, 200, "LED10_K",  "CLEAR-Confirm (weiss)",    LED_W),
        (5,  "11Y",  80, 215, "LED11Y_K", "CELL1 Hold@Basis (gelb)",  LED_Y),
        (6,  "11G",  80, 228, "LED11G_K", "CELL1 Hold@Shift (gruen)", LED_G),
        (7,  "12Y", 105, 215, "LED12Y_K", "CELL2 Hold@Basis (gelb)",  LED_Y),
        (8,  "12G", 105, 228, "LED12G_K", "CELL2 Hold@Shift (gruen)", LED_G),
        (9,  "13Y", 130, 215, "LED13Y_K", "CELL3 Hold@Basis (gelb)",  LED_Y),
        (10, "13G", 130, 228, "LED13G_K", "CELL3 Hold@Shift (gruen)", LED_G),
        (11, "14Y", 155, 215, "LED14Y_K", "CELL4 Hold@Basis (gelb)",  LED_Y),
        (12, "14G", 155, 228, "LED14G_K", "CELL4 Hold@Shift (gruen)", LED_G),
        (13, "15Y", 180, 215, "LED15Y_K", "CELL5 Hold@Basis (gelb)",  LED_Y),
        (14, "15G", 180, 228, "LED15G_K", "CELL5 Hold@Shift (gruen)", LED_G),
    ]
    for pca_ch, led_ref, sx, sy, lname, descr, (led_mpn, led_lcsc) in led_array:
        # R_LEDn at (sx, sy), rotation=90 horizontal: pin1 (sx-3.81, sy), pin2 (sx+3.81, sy)
        symbols.append(
            place_symbol(
                lib_id="Device:R",
                ref=f"R_LED{led_ref}",
                value=f"390R 0603 ({descr} series)",
                x=sx,
                y=sy,
                rotation=90,
                footprint="Resistor_SMD:R_0603_1608Metric",
                extra_props={"MPN": "0603WAF3900T5E", "LCSC": "C23151"},
                seed_suffix=f"R_LED{led_ref}",
                sheet_uuid_seed=sus,
            )
        )
        # +5V flag at (sx-6, sy)
        wires.append(wire(sx - 3.81, sy, sx - 6, sy, seed_suffix=f"r-led{led_ref}-5v"))
        symbols.append(
            place_symbol(
                lib_id="Power:+5V",
                ref=f"#PWR_R_LED{led_ref}",
                value="+5V",
                x=sx - 6,
                y=sy,
                rotation=270,
                seed_suffix=f"r-led{led_ref}-5v-pwr",
                sheet_uuid_seed=sus,
            )
        )
        # LED at (sx+7.62, sy), rotation=180: pin1 K abs (sx+11.43, sy), pin2 A abs (sx+3.81, sy)
        symbols.append(
            place_symbol(
                lib_id="Device:LED",
                ref=f"LED{led_ref}",
                value=f"SMD 0603 indicator ({descr})",
                x=sx + 7.62,
                y=sy,
                rotation=180,
                footprint="LED_SMD:LED_0603_1608Metric",
                extra_props={"MPN": led_mpn, "LCSC": led_lcsc},
                seed_suffix=f"LED{led_ref}",
                sheet_uuid_seed=sus,
            )
        )
        # R-pin2 (sx+3.81) ↔ LED-Anode (sx+3.81): same x, same y → no wire needed (touching).
        # But emit a tiny zero-length wire for explicit-connect (KiCad ERC heuristic):
        # actually if pins overlap, KiCad treats as connected. Skip wire.
        # LED-Kathode → label
        wires.append(wire(sx + 11.43, sy, sx + 13.43, sy, seed_suffix=f"led{led_ref}-k-stub"))
        labels.append(label(sx + 13.43, sy, lname))

        # Corresponding PCA-LED-pin label (same net name = automatic connection)
        # Label MUSS auf dem Wire-Endpunkt sitzen (sonst hängt es in der Luft → ERC single-pin-net).
        if pca_ch < 8:
            # PCA-LED0..7 on LEFT side, pins 6..13
            pca_pin = 6 + pca_ch
            ppy = pca_left_pin_y(pca_pin)
            wires.append(wire(PCA_PIN_L_X, ppy, PCA_PIN_L_X - 5, ppy, seed_suffix=f"u6-led{pca_ch}-stub"))
            labels.append(label(PCA_PIN_L_X - 5, ppy, lname))
        else:
            # PCA-LED8..9 on RIGHT side, pins 15..16
            pca_pin = 15 + (pca_ch - 8)
            ppy = pca_right_pin_y(pca_pin)
            wires.append(wire(PCA_PIN_R_X, ppy, PCA_PIN_R_X + 5, ppy, seed_suffix=f"u6-led{pca_ch}-stub"))
            labels.append(label(PCA_PIN_R_X + 5, ppy, lname))

    # ---- r18.9: Kanal 15 (Pin 22) = LCD-Backlight-PWM (war ch12 in r18.5;
    # ch5-14 sind jetzt die 10 Cell-LEDs). Kein Reserve-Kanal mehr — 16/16
    # exakt belegt (SPEC §7.2-Tabelle).
    blk_py = pca_right_pin_y(22)
    wires.append(wire(PCA_PIN_R_X, blk_py, PCA_PIN_R_X + 5, blk_py, seed_suffix="u6-nc-led15"))
    labels.append(label(PCA_PIN_R_X + 5, blk_py, "LCD_BLK_PWM"))
    wires.append(wire(PCA_PIN_R_X + 5, blk_py, PCA_PIN_R_X + 12, blk_py, seed_suffix="u6-led15-hier"))
    hlabels.append(hier_label(PCA_PIN_R_X + 12, blk_py, "LCD_BLK_PWM", shape="output", rotation=180))

    # ---- r18.9: LED1/R_LED_STATUS (PCA-ch10-System-Status, r12) ENTFERNT.
    # System-Status/Heartbeat ist seit r18.5 die LED_HB an MCU-PD8
    # (stm32h743_sheet); PCA-Kanal 10 ist jetzt LED13G (Cell-3 Hold@Shift).

    # ========================================================================
    # r18.66 — U10 PCA9685PW #2 (I²C 0x41) → 8× Live-Level-Meter-LEDs (VU)
    # ========================================================================
    # OP-1-Field-Stil: wenige LEDs, alle **weiss** (r18.68 User). Die Firmware
    # rechnet RMS/Peak aus dem Audio-Buffer und treibt 8 Segmente per PWM (echtes
    # Live-Meter, nicht nur Knopfstellung). 2. PCA9685 weil U6 voll ist (16/16).
    # Teilt sich denselben I²C-Bus: U10-SDA/SCL via LOKALE Labels I2C_SDA/I2C_SCL
    # am selben Netz wie U6. KEINE neuen MCU-Pins, KEINE Root-/PINMAP-Aenderung.
    # Adresse 0x41: A0=+3V3, A1-A5=GND. 8 LEDs auf Channels 0-7 (Pins 6-13, links),
    # alle weiss (C965808 = wie Heartbeat). Position der Reihe im PCB = Layout-TBD.
    U10_X, U10_Y = 300.0, 120.0
    P10_L_X = U10_X - 12.7
    P10_R_X = U10_X + 12.7

    def p10_left_y(pin: int) -> float:
        return U10_Y - 16.51 + (pin - 1) * 2.54

    def p10_right_y(pin: int) -> float:
        return U10_Y - 16.51 + (28 - pin) * 2.54

    symbols.append(
        place_symbol(
            lib_id="Driver_LED:PCA9685PW",
            ref="U10",
            value="PCA9685PW #2 (16-Ch I²C PWM, 0x41 — Level-Meter)",
            x=U10_X,
            y=U10_Y,
            footprint="Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm",
            datasheet="https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf",
            extra_props={"MPN": "PCA9685PW,118", "LCSC": "C2678753"},
            seed_suffix="U10",
            sheet_uuid_seed=sus,
        )
    )
    # ---- A0 (Pin 1, left) → +3V3  => Adresse 0x41
    a0y = p10_left_y(1)
    wires.append(wire(P10_L_X, a0y, P10_L_X - 7, a0y, seed_suffix="u10-a0"))
    symbols.append(
        place_symbol(lib_id="Power:+3V3", ref="#PWR_U10_A0", value="+3V3",
                     x=P10_L_X - 7, y=a0y, rotation=270,
                     seed_suffix="u10-a0-3v3", sheet_uuid_seed=sus)
    )
    # ---- A1..A4 (Pins 2-5, left) → GND
    for pin in (2, 3, 4, 5):
        py = p10_left_y(pin)
        wires.append(wire(P10_L_X, py, P10_L_X - 7, py, seed_suffix=f"u10-a-{pin}"))
        symbols.append(
            place_symbol(lib_id="Power:GND", ref=f"#PWR_U10_A{pin-1}", value="GND",
                         x=P10_L_X - 7, y=py, rotation=90,
                         seed_suffix=f"u10-a-{pin}-gnd", sheet_uuid_seed=sus)
        )
    # ---- A5 (Pin 24, right) → GND
    a5y = p10_right_y(24)
    wires.append(wire(P10_R_X, a5y, P10_R_X + 7, a5y, seed_suffix="u10-a5"))
    symbols.append(
        place_symbol(lib_id="Power:GND", ref="#PWR_U10_A5", value="GND",
                     x=P10_R_X + 7, y=a5y, rotation=270,
                     seed_suffix="u10-a5-gnd", sheet_uuid_seed=sus)
    )
    # ---- VSS (Pin 14, left, bottommost) → GND
    vss10 = p10_left_y(14)
    wires.append(wire(P10_L_X, vss10, P10_L_X - 7, vss10, seed_suffix="u10-vss"))
    symbols.append(
        place_symbol(lib_id="Power:GND", ref="#PWR_U10_VSS", value="GND",
                     x=P10_L_X - 7, y=vss10, rotation=90,
                     seed_suffix="u10-vss-gnd", sheet_uuid_seed=sus)
    )
    # ---- VDD (Pin 28, right, topmost) → +3V3 + C_PCA2_VDD 10µF + C_PCA2_VDD_HF 100nF
    vdd10 = p10_right_y(28)
    wires.append(wire(P10_R_X, vdd10, P10_R_X + 9, vdd10, seed_suffix="u10-vdd-stub"))
    symbols.append(
        place_symbol(lib_id="Power:+3V3", ref="#PWR_U10_VDD", value="+3V3",
                     x=P10_R_X + 9, y=vdd10, rotation=0,
                     seed_suffix="u10-vdd-3v3", sheet_uuid_seed=sus)
    )
    cx1 = P10_R_X + 15
    symbols.append(
        place_symbol(lib_id="Device:C", ref="C_PCA2_VDD",
                     value="10uF X5R 0805 (PCA2 VDD bulk)",
                     x=cx1, y=vdd10 + 5, footprint="Capacitor_SMD:C_0805_2012Metric",
                     extra_props={"MPN": "CL21A106KOQNNNG-class", "LCSC": "C15850"},
                     seed_suffix="C_PCA2_VDD", sheet_uuid_seed=sus)
    )
    wires.append(wire(cx1, vdd10 + 5 - 3.81, cx1, vdd10, seed_suffix="c-pca2-vdd-top"))
    wires.append(wire(P10_R_X + 9, vdd10, cx1, vdd10, seed_suffix="c-pca2-vdd-tee"))
    junctions.append(junction(cx1, vdd10))
    symbols.append(
        place_symbol(lib_id="Power:GND", ref="#PWR_C_PCA2_VDD", value="GND",
                     x=cx1, y=vdd10 + 5 + 5, rotation=0,
                     seed_suffix="c-pca2-vdd-gnd", sheet_uuid_seed=sus)
    )
    wires.append(wire(cx1, vdd10 + 5 + 3.81, cx1, vdd10 + 5 + 5, seed_suffix="c-pca2-vdd-bot"))
    cx2 = P10_R_X + 21
    symbols.append(
        place_symbol(lib_id="Device:C", ref="C_PCA2_VDD_HF",
                     value="100nF X7R 0603 (PCA2 VDD HF)",
                     x=cx2, y=vdd10 + 5, footprint="Capacitor_SMD:C_0603_1608Metric",
                     extra_props={"MPN": "CL10B104KO8NNNC-class", "LCSC": "C14663"},
                     seed_suffix="C_PCA2_VDD_HF", sheet_uuid_seed=sus)
    )
    wires.append(wire(cx2, vdd10 + 5 - 3.81, cx2, vdd10, seed_suffix="c-pca2-vddhf-top"))
    wires.append(wire(cx1, vdd10, cx2, vdd10, seed_suffix="c-pca2-vddhf-tee"))
    junctions.append(junction(cx2, vdd10))
    symbols.append(
        place_symbol(lib_id="Power:GND", ref="#PWR_C_PCA2_VDD_HF", value="GND",
                     x=cx2, y=vdd10 + 5 + 5, rotation=0,
                     seed_suffix="c-pca2-vddhf-gnd", sheet_uuid_seed=sus)
    )
    wires.append(wire(cx2, vdd10 + 5 + 3.81, cx2, vdd10 + 5 + 5, seed_suffix="c-pca2-vddhf-bot"))
    # ---- SDA (Pin 27, right) → lokales Label I2C_SDA (selber Bus wie U6)
    sda10 = p10_right_y(27)
    wires.append(wire(P10_R_X, sda10, P10_R_X + 7, sda10, seed_suffix="u10-sda"))
    labels.append(label(P10_R_X + 7, sda10, "I2C_SDA"))
    # ---- SCL (Pin 26, right) → lokales Label I2C_SCL
    scl10 = p10_right_y(26)
    wires.append(wire(P10_R_X, scl10, P10_R_X + 7, scl10, seed_suffix="u10-scl"))
    labels.append(label(P10_R_X + 7, scl10, "I2C_SCL"))
    # ---- EXTCLK (Pin 25, right) → GND (NXP DS Rev4 S.7 Footnote [2], wenn unused)
    ext10 = p10_right_y(25)
    wires.append(wire(P10_R_X, ext10, P10_R_X + 7, ext10, seed_suffix="u10-extclk"))
    symbols.append(
        place_symbol(lib_id="Power:GND", ref="#PWR_U10_EXTCLK", value="GND",
                     x=P10_R_X + 7, y=ext10, rotation=270,
                     seed_suffix="u10-extclk-gnd", sheet_uuid_seed=sus)
    )
    # ---- ~OE (Pin 23, right, active LOW) → R_OE2 10k pull-up zu +3V3 (default off)
    oe10 = p10_right_y(23)
    wires.append(wire(P10_R_X, oe10, P10_R_X + 3.19, oe10, seed_suffix="u10-oe-stub"))
    symbols.append(
        place_symbol(lib_id="Device:R", ref="R_OE2",
                     value="10k 0603 (PCA9685 #2 /OE pull-up)",
                     x=P10_R_X + 7, y=oe10, rotation=90,
                     footprint="Resistor_SMD:R_0603_1608Metric",
                     extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
                     seed_suffix="R_OE2", sheet_uuid_seed=sus)
    )
    wires.append(wire(P10_R_X + 10.81, oe10, P10_R_X + 14, oe10, seed_suffix="r-oe2-to-3v3"))
    symbols.append(
        place_symbol(lib_id="Power:+3V3", ref="#PWR_R_OE2", value="+3V3",
                     x=P10_R_X + 14, y=oe10, rotation=90,
                     seed_suffix="r-oe2-3v3", sheet_uuid_seed=sus)
    )
    # ---- 8× VU-Meter LED+R (Channels 0-7 = Pins 6-13, links). Anode +5V via 390R,
    # Kathode → PCA-Channel-Sink. Eigene Reihe (y=255), x-Position im PCB = TBD.
    # r18.68 (User): VU-Meter ist **weiss** (gleiche LED wie Heartbeat/Status,
    # C965808) — keine blaue Extra-LED noetig (loest das No-LCSC-Problem).
    vu_array = [
        # (pca_channel, vu_ref, sx, sy, (mpn,lcsc), descr)
        (0, "1",  70, 255, LED_W, "Level seg 1 (weiss)"),
        (1, "2",  95, 255, LED_W, "Level seg 2 (weiss)"),
        (2, "3", 120, 255, LED_W, "Level seg 3 (weiss)"),
        (3, "4", 145, 255, LED_W, "Level seg 4 (weiss)"),
        (4, "5", 170, 255, LED_W, "Level seg 5 (weiss)"),
        (5, "6", 195, 255, LED_W, "Level seg 6 (weiss)"),
        (6, "7", 220, 255, LED_W, "Peak seg 7 (weiss)"),
        (7, "8", 245, 255, LED_W, "Peak seg 8 (weiss)"),
    ]
    for ch, vref, sx, sy, (vmpn, vlcsc), vdescr in vu_array:
        lname = f"VU{vref}_K"
        symbols.append(
            place_symbol(lib_id="Device:R", ref=f"R_VU{vref}",
                         value=f"390R 0603 ({vdescr} series)",
                         x=sx, y=sy, rotation=90,
                         footprint="Resistor_SMD:R_0603_1608Metric",
                         extra_props={"MPN": "0603WAF3900T5E", "LCSC": "C23151"},
                         seed_suffix=f"R_VU{vref}", sheet_uuid_seed=sus)
        )
        wires.append(wire(sx - 3.81, sy, sx - 6, sy, seed_suffix=f"r-vu{vref}-5v"))
        symbols.append(
            place_symbol(lib_id="Power:+5V", ref=f"#PWR_R_VU{vref}", value="+5V",
                         x=sx - 6, y=sy, rotation=270,
                         seed_suffix=f"r-vu{vref}-5v-pwr", sheet_uuid_seed=sus)
        )
        symbols.append(
            place_symbol(lib_id="Device:LED", ref=f"LED_VU{vref}",
                         value=f"SMD 0603 level-meter ({vdescr})",
                         x=sx + 7.62, y=sy, rotation=180,
                         footprint="LED_SMD:LED_0603_1608Metric",
                         extra_props={"MPN": vmpn, "LCSC": vlcsc},
                         seed_suffix=f"LED_VU{vref}", sheet_uuid_seed=sus)
        )
        wires.append(wire(sx + 11.43, sy, sx + 13.43, sy, seed_suffix=f"vu{vref}-k-stub"))
        labels.append(label(sx + 13.43, sy, lname))
        # matching label on U10 left LED-pin (channel 0-7 → pins 6-13)
        pca_pin = 6 + ch
        ppy = p10_left_y(pca_pin)
        wires.append(wire(P10_L_X, ppy, P10_L_X - 5, ppy, seed_suffix=f"u10-led{ch}-stub"))
        labels.append(label(P10_L_X - 5, ppy, lname))

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 4: MCP23017 + PCA9685 + 10 Switches + 10 LEDs")\n'
        f'    (date "2026-05-31")\n'
        f'    (rev "0.6.3-r10-LED")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6 §7 (MCP) + §7.2 (PCA9685)")\n'
        f'    (comment 2 "MCP 0x20 / U6 PCA 0x40 / U10 PCA 0x41 (A0=+3V3) - shared I²C bus")\n'
        f'    (comment 3 "r18.68: SW6-SW10 = TC-1212-7.3-260G THT-Tactile (C2845240, square head)")\n'
        f'    (comment 4 "r10: 10× LEDs (LED6-LED15); U10 PCA9685 #2 -> 8 VU-Level-LEDs (weiss)"))\n'
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
                          c_a: int, c_b: int,
                          variant: str = "smooth") -> None:
        """Platziere einen EC11 + Pull-Ups + Debounce-Caps + Hier-Labels.

        variant (ADR-0012, r18.14; r18.22 vereinfacht):
          "push"   — Display-Encoder: ALPS EC11E18244AU, 36 Detents/U, mit
                     Push-Switch (Menue-Navigation: 1 Rastung = 1 Schritt).
          "smooth" — Parameter-Encoder. **r18.22-Pivot:** physisch jetzt das
                     GLEICHE Teil (EC11E18244AU) wegen NRND-Status der frueheren
                     Wahl EC11E183440C UND des Folge-Kandidaten EC11E1834403
                     (ALPS hat die gesamte "0-Detent+Push"-Familie phased out).
                     UX bleibt identisch zum frueheren Smooth, weil die Firmware-
                     Acceleration langsam = 1 %/Klick, schnell = x8/Klick macht
                     (Originalanforderung „2 Klicks fuer 1%" gefixt). Die
                     variant-Unterscheidung dient nur noch der internen
                     Sortierung — beide Aeste platzieren dasselbe Bauteil.
        Beide Varianten: gleiches EC11E-THT-Land-Pattern + identisches Teil.
        EC11J SMD (C209762) retired: NRND + 24.5mm Gesamthöhe + Half-Step-
        Detent-Mismatch.
        """
        sx = 140.0
        # Pin Absolutpositionen (sym (sx, sy), KiCad Y-DOWN abs = sym - local_y):
        a_x, a_y = sx - 5.08, sy - 5.08     # Pin 1 A
        b_x, b_y = sx - 5.08, sy            # Pin 2 B
        c_x, c_y = sx - 5.08, sy + 5.08     # Pin 3 C → GND
        sw1_x, sw1_y = sx + 5.08, sy - 2.54  # Pin 4 SW1 → Signal
        sw2_x, sw2_y = sx + 5.08, sy + 2.54  # Pin 5 SW2 → GND

        if variant == "push":
            enc_value = f"EC11E18244AU push+detent ({net_prefix})"
            enc_props = {
                "MPN": "EC11E18244AU (ALPS EC11E, 18 Pulse, 36 Detents, mit Push-Switch, 0.5mm Travel)",
                "Manufacturer": "ALPSALPINE",
                "LCSC": "C202365",
                "Package": "THT EC11E vertikal, Shaft 20mm flat — Display-Encoder: 1 Rastung = 1 Schritt (ADR-0012)",
                "Datasheet": "https://datasheet.lcsc.com/lcsc/1809200034_ALPSALPINE-EC11E18244AU_C202365.pdf",
                "Variant": "DISPLAY-Encoder = einziger mit Push + Rastung",
            }
        else:
            # r18.22 (Audit-NRND-Fix): EC11E183440C ist NRND, der Kandidat
            # EC11E1834403 ebenfalls. Komplette ALPS-"0-Detent+Push"-Familie
            # ist phased out. Daher: alle 4 Encoder = active EC11E18244AU
            # (36 Detents). UX-funktional gleich, weil Firmware-Acceleration
            # langsam = 1 %/Klick, schnell = x8/Klick macht (genau die ur-
            # spruengliche User-Anforderung). Kein Smooth/Detent-Unterschied
            # mehr — Stueckzahl-Bestellung einfacher.
            enc_value = f"EC11E18244AU push+detent ({net_prefix}, r18.22 NRND-Pivot)"
            enc_props = {
                "MPN": "EC11E18244AU (ALPS EC11E, 18 Pulse, 36 Detents, mit Push-Switch, Flat-Shaft 20 mm)",
                "Manufacturer": "ALPSALPINE",
                "LCSC": "C202365",
                "Package": "THT EC11E vertikal, Shaft 20mm flat — Parameter-Encoder (r18.22): 36 Detents/U, mit Firmware-Acceleration langsam=1%/Klick, schnell=x8 (NATIVE_PORT_PLAN encoders.c)",
                "Datasheet": "https://datasheet.lcsc.com/lcsc/1809200034_ALPSALPINE-EC11E18244AU_C202365.pdf",
                "Variant": "Parameter-Encoder = identisch zu Display-Encoder (r18.22 Vereinfachung wegen NRND-Pivot der Smooth-Variante)",
                "Lifecycle-VERIFIED": "ALPS-Hersteller-Seite: Standard / for new designs (r18.22 verifiziert tech.alpsalpine.com). Die frühere Wahl EC11E183440C (C370986) und der Folge-Kandidat EC11E1834403 (C361165) sind beide NRND laut Octopart.",
            }
        enc_props["Height"] = (
            "Beide Varianten gleiche Bauform: Body 11.7×12.0×4.5 mm + 20 mm "
            "Flat-Shaft → garantiert gleich hoch (ADR-0012). Knopf Ø19-20 x "
            "8-10 mm Alu (Kick75-flach)"
        )
        enc_props["FP_NOTE"] = (
            "Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm — "
            "KiCad-Standard-Lib, fab-erprobt. Identisches Land-Pattern für "
            "beide Varianten (Push-Pins immer da, im Smooth-Fall mechanisch "
            "ungenutzt). r18.14: LCSC-IDs verifiziert."
        )

        # ---- EC11E Symbol (THT, ADR-0012)
        symbols.append(
            place_symbol(
                lib_id="Encoder:Rotary_Encoder_Switch",
                ref=f"EN{en_num}",
                value=enc_value,
                x=sx,
                y=sy,
                footprint="Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm",
                datasheet="https://tech.alpsalpine.com/e/products/category/encorder/sub/01/series/ec11e/",
                extra_props=enc_props,
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
                extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
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
                extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
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
                extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
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
    # ADR-0012: nur DISPLAY (EN3) hat Push + Detents; EN1/2/4 drehen glatt
    # ohne Rastung und ohne Switch (gleiche Bauhöhe, gleiches Footprint).
    place_one_encoder(en_num=1, sy=70, net_prefix="DRIVE",
                      r_a=7, r_b=8, r_sw=15, c_a=10, c_b=11, variant="smooth")
    place_one_encoder(en_num=2, sy=105, net_prefix="BRIGHT",
                      r_a=9, r_b=10, r_sw=16, c_a=12, c_b=13, variant="smooth")
    place_one_encoder(en_num=3, sy=140, net_prefix="DISPLAY",
                      r_a=11, r_b=12, r_sw=17, c_a=14, c_b=15, variant="push")
    place_one_encoder(en_num=4, sy=175, net_prefix="VOL",
                      r_a=13, r_b=14, r_sw=18, c_a=16, c_b=17, variant="smooth")

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 5: 4x EC11 Encoder")\n'
        f'    (date "2026-05-12")\n'
        f'    (rev "0.7")\n'
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
# Ferrite Bead, AMP_nSHDN+AMP_nMUTE GPIOs).
# Inputs: +5V, +3V3, GND, I2S_BCK, I2S_LRCK, I2S_DOUT (von Pi/Sheet 7),
#         AMP_nSHDN, AMP_nMUTE (von Pico/Sheet 2).
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
            extra_props={"MPN": "CL10A105KB8NNNC", "LCSC": "C15849"},
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
            extra_props={"MPN": "CL10A105KB8NNNC", "LCSC": "C15849"},
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
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
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
            extra_props={"MPN": "PAM8403DR-H", "LCSC": "C17337"},
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

    # ---- Pin 5 MUTE ← AMP_nMUTE hier input (ACTIVE LOW per datasheet)
    p5uy = u4_left(5)
    wires.append(wire(U4_LX, p5uy, 138, p5uy, seed_suffix="u4-mute-stub"))
    hlabels.append(hier_label(138, p5uy, "AMP_nMUTE", shape="input", rotation=0))
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
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
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
    # PAM8403H Datasheet decoupling: "1.0µF ceramic close to VDD" (HF) +
    # "20µF or greater" (bulk). v0.6.3-r6: upgraded from 100nF/10µF.
    # This left-side set serves VDD (pin 6) + PVDD-L (pin 4, adjacent).
    c9b_y = p6uy + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C9b",
            value="1uF X7R 0603 (VDD/PVDD-L HF, PAM8403H datasheet)",
            x=140, y=c9b_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10A105KB8NNNC", "LCSC": "C15849"},
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
            value="22uF X5R 0805 (VDD/PVDD-L bulk, PAM8403H datasheet >=20uF)",
            x=136, y=c9b_y,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A226MAQNNNE", "LCSC": "C45783"},
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
            extra_props={"MPN": "0603WAF2002T5E", "LCSC": "C4184"},
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
            extra_props={"MPN": "CL10A105KB8NNNC", "LCSC": "C15849"},
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
            extra_props={"MPN": "CL10A105KB8NNNC", "LCSC": "C15849"},
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
            extra_props={"MPN": "0603WAF2002T5E", "LCSC": "C4184"},
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
            extra_props={"MPN": "CL10A105KB8NNNC", "LCSC": "C15849"},
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

    # ---- Pin 12 SHDN ← AMP_nSHDN hier input (ACTIVE LOW per datasheet)
    p12uy = u4_right(12)
    wires.append(wire(U4_RX, p12uy, 180, p12uy, seed_suffix="u4-shdn-stub"))
    hlabels.append(hier_label(180, p12uy, "AMP_nSHDN", shape="input", rotation=180))
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
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
            seed_suffix="RSHDN_PD",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(180, p12uy, 176, p12uy, seed_suffix="rshdn-to-shdn-line"))
    junctions.append(junction(180, p12uy))
    wires.append(wire(176, rshdn_y + 3.81, 176, rshdn_y + 6, seed_suffix="rshdn-to-gnd"))
    attach_gnd(176, rshdn_y + 6, "RSHDN_PD", rot=270)

    # ---- Pin 13 PVDD (right) → +5V + lokales Decoupling (v0.6.3-r6)
    # PVDD-R ist auf der rechten Chip-Seite, weit weg vom VDD/PVDD-L
    # Decoupling-Set links. Class-D-Switching (~250kHz) braucht eigenes
    # low-impedance Decoupling nah am Pin 13. Datasheet: 1µF HF + >=20µF bulk.
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
    junctions.append(junction(U4_RX + 3, p13uy))
    # C_PVDDR_BULK 22µF + C_PVDDR_HF 1µF, vertikal unterhalb des PVDD-R-Pins
    cpvddr_y = p13uy + 3.81
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_PVDDR",
            value="22uF X5R 0805 (PVDD-R bulk, PAM8403H datasheet >=20uF)",
            x=181, y=cpvddr_y,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "CL21A226MAQNNNE", "LCSC": "C45783"},
            seed_suffix="CPVDDR",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(181, p13uy, U4_RX + 3, p13uy, seed_suffix="cpvddr-to-pvdd"))
    wires.append(wire(181, cpvddr_y + 3.81, 181, cpvddr_y + 6, seed_suffix="cpvddr-gnd"))
    attach_gnd(181, cpvddr_y + 6, "CPVDDR", rot=270)
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_PVDDR_HF",
            value="1uF X7R 0603 (PVDD-R HF, PAM8403H datasheet)",
            x=185, y=cpvddr_y,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10A105KB8NNNC", "LCSC": "C15849"},
            seed_suffix="CPVDDR_HF",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(185, p13uy, 181, p13uy, seed_suffix="cpvddr-hf-to-bulk"))
    junctions.append(junction(181, p13uy))
    wires.append(wire(185, cpvddr_y + 3.81, 185, cpvddr_y + 6, seed_suffix="cpvddr-hf-gnd"))
    attach_gnd(185, cpvddr_y + 6, "CPVDDR_HF", rot=270)

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
            value="Speaker L (Same Sky CMS-402811-28SP, Cloth-Cone, 8R 2W)",
            x=j6_x, y=j6_y,
            footprint="Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
            extra_props={"MPN": "Same Sky CMS-402811-28SP (Cloth-Cone primaer; PUI AS04008PS-4W-WR-R sekundaer)", "LCSC": "TBD (kein LCSC-Stock fuer 40mm-Treiber; DigiKey 102-CMS-402811-28SP-ND)", "Notes": "Treiber haengt vom Top-Panel (Top-Firing, ADR-0007/ADR-0011), kein PCB-Mount. J6/J7 sind nur 2-Pin-Loetpads; Draht zum Treiber-Eyelet manuell. r18.18: Cloth-Cone-Wechsel."},
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
            value="Speaker R (Same Sky CMS-402811-28SP, Cloth-Cone, 8R 2W)",
            x=j7_x, y=j7_y,
            footprint="Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
            extra_props={"MPN": "Same Sky CMS-402811-28SP (Cloth-Cone primaer; PUI AS04008PS-4W-WR-R sekundaer)", "LCSC": "TBD (kein LCSC-Stock fuer 40mm-Treiber; DigiKey 102-CMS-402811-28SP-ND)", "Notes": "Treiber haengt vom Top-Panel (Top-Firing, ADR-0007/ADR-0011), kein PCB-Mount. J6/J7 sind nur 2-Pin-Loetpads; Draht zum Treiber-Eyelet manuell. r18.18: Cloth-Cone-Wechsel."},
            seed_suffix="J7",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(203.81, 128.73, 207, 128.73, seed_suffix="j7-p1"))
    labels.append(label(207, 128.73, "SPK_R+"))
    wires.append(wire(203.81, 131.27, 207, 131.27, seed_suffix="j7-p2"))
    labels.append(label(207, 131.27, "SPK_R-"))

    # ====================================================================
    # LINE-OUT / KOPFHÖRER (v0.7) — passiver Tap an PCM5102A VOUTL/R, VOR
    # dem PAM8403. PCM5102A-Output ist ground-centered (kein DC-Offset durch
    # interne Charge-Pump) → keine Koppel-Caps nötig, nur Serien-R zum Schutz.
    # J8 TRS-Buchse mit Detect-Switch: Einstecken → JACK_DETECT → MCP GPA6
    # → Firmware mutet NUR den PAM8403 (Speaker), Line-Out bleibt live.
    # Damit wird der tiefe Charakter über externe Boxen/Kopfhörer hörbar
    # (40mm-Onboard-Speaker können den 30-60Hz SubBass nicht).
    # ====================================================================
    # R_LO_L 22Ω series an VOUTL (Tap vom PCM_VOUTL-Netz)
    lo_y = 150.0
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_LO_L",
            value="22R 0603 (line-out L series/protect)",
            x=160, y=lo_y, rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF220JT5E", "LCSC": "C23345"},
            seed_suffix="RLOL",
            sheet_uuid_seed=sus,
        )
    )
    # R rot=90: pin1 abs (156.19, lo_y) ← PCM_VOUTL label, pin2 abs (163.81, lo_y) → jack T
    wires.append(wire(156.19, lo_y, 152, lo_y, seed_suffix="rlol-to-voutl"))
    labels.append(label(152, lo_y, "PCM_VOUTL"))

    lo_y2 = lo_y + 6
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_LO_R",
            value="22R 0603 (line-out R series/protect)",
            x=160, y=lo_y2, rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF220JT5E", "LCSC": "C23345"},
            seed_suffix="RLOR",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(156.19, lo_y2, 152, lo_y2, seed_suffix="rlor-to-voutr"))
    labels.append(label(152, lo_y2, "PCM_VOUTR"))

    # J8 TRS jack @ (175, 152). Pins: T(1) @ (182.62,149.46), R(2) @ (182.62,152),
    # S(3) @ (182.62,154.54), DET(4) @ (167.38,152).
    j8_x, j8_y = 175.0, 152.0
    symbols.append(
        place_symbol(
            lib_id="Connector:AudioJack3_Switch",
            ref="J8",
            value="3.5mm TRS Line/Headphone Out (PJ-320 class, w/ detect)",
            x=j8_x, y=j8_y,
            footprint="field_ambience:Jack_3.5mm_PJ-320D_SMT",
            # v0.8: C2884109 existierte nicht in der JLC-Teile-DB → ersetzt durch
            # PJ-320D (C431535, SMD 3.5mm TRS mit Schaltkontakt, lagernd). Footprint
            # gegen PJ-320D-Pads im GUI verifizieren (TODO B0b) + Detect-Polarität.
            extra_props={"MPN": "PJ-320D (3.5mm TRS w/ switch)", "LCSC": "C431535"},
            seed_suffix="J8",
            sheet_uuid_seed=sus,
        )
    )
    # T (tip, left) @ (j8_x+7.62, j8_y-2.54) = (182.62, 149.46) ← R_LO_L pin2 (163.81, lo_y=150)
    wires.append(wire(163.81, lo_y, 182.62, lo_y, seed_suffix="rlol-to-j8t"))
    wires.append(wire(182.62, lo_y, 182.62, 149.46, seed_suffix="j8t-up"))
    # R (ring, right) @ (182.62, 152) ← R_LO_R pin2 (163.81, lo_y2=156)
    wires.append(wire(163.81, lo_y2, 178, lo_y2, seed_suffix="rlor-to-j8r"))
    wires.append(wire(178, lo_y2, 178, 152, seed_suffix="j8r-corner"))
    wires.append(wire(178, 152, 182.62, 152, seed_suffix="j8r-in"))
    # S (sleeve) @ (182.62, 154.54) → GND
    wires.append(wire(182.62, 154.54, 186, 154.54, seed_suffix="j8s-gnd"))
    attach_gnd(186, 154.54, "J8_S", rot=270)
    # DET (4) @ (j8_x-7.62, j8_y) = (167.38, 152) → JACK_DETECT hier-output to MCP GPA6
    wires.append(wire(167.38, 152, 162, 152, seed_suffix="j8-det"))
    hlabels.append(hier_label(162, 152, "JACK_DETECT", shape="output", rotation=0))

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 6: Audio (PCM5102A + PAM8403H)")\n'
        f'    (date "2026-05-14")\n'
        f'    (rev "0.7")\n'
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
                    extra_props={"MPN": "0603WAF330JT5E", "LCSC": "C23140"},
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
                    extra_props={"MPN": "0603WAF330JT5E", "LCSC": "C23140"},
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
            # v0.8: was Yageo RC0603FR-071KL (C22548, Extended). Swapped to the
            # Uni Royal 0603WAF Basic family used by every other R in this design
            # → no JLC extended-part setup fee, 15.8M stock, identical 1k 0603.
            extra_props={"MPN": "0603WAF1001T5E", "LCSC": "C21190"},
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
        f'    (rev "0.7")\n'
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
# Sheet 7 — Battery & Power-Path (NEU r9 + r12)
# ----------------------------------------------------------------------------
# Components per SPEC §2.2:
#   U7 MCP73831 LiPo-Charger (500mA via R21=2kΩ)
#   U8 TPS61089 Boost LiPo→5V (FB-Divider R23/R24)
#   Q1 DMG2305UX P-MOS Power-Path-Selector
#   L1 2.2µH, D3 SS34, J9 JST-PH-2P, R21-R24, 4× Caps, LED_CHRG + R_CHRG
#
# Hier-I/O:
#   VBUS_USBC (input)   ← raw USB-C VBUS pre-fuse, from power_tree
#   +5V_OUT (output)    → system rail (merges with power_tree's +5V_OUT net)
#   BAT_PLUS (output)   → Battery+ for BAT_SENSE-Divider in pico_sheet
# ----------------------------------------------------------------------------


def battery_sheet() -> str:
    sheet_uuid = det_uuid("sheet_battery")
    sus = "sheet_battery"
    symbols: list[str] = []
    wires: list[str] = []
    junctions: list[str] = []
    labels: list[str] = []
    hlabels: list[str] = []

    def attach_gnd(x: float, y: float, ref: str, rotation: int = 270) -> None:
        symbols.append(
            place_symbol(
                lib_id="Power:GND",
                ref=f"#PWR_{ref}",
                value="GND",
                x=x, y=y,
                rotation=rotation,
                seed_suffix=f"gnd-{ref}",
                sheet_uuid_seed=sus,
            )
        )

    # ====================================================================
    # J9 JST-PH 2-pin Battery-Connector @ (50, 80). Pin1=BAT_PLUS, Pin2=GND.
    # ====================================================================
    symbols.append(
        place_symbol(
            lib_id="Connector:Conn_01x02",
            ref="J9",
            value="JST PH 2.0 2-pin (LiPo Battery, r9)",
            x=50, y=80,
            footprint="Connector_JST:JST_PH_S2B-PH-SM4-TB_1x02-1MP_P2.00mm_Horizontal",
            extra_props={"MPN": "S2B-PH-SM4-TB(LF)(SN)", "LCSC": "C295747"},
            seed_suffix="J9",
            sheet_uuid_seed=sus,
        )
    )
    # Conn_01x02 pins at local (3.81, +1.27) and (3.81, -1.27), angle 180.
    # Abs: pin1 (sx+3.81, sy-1.27), pin2 (sx+3.81, sy+1.27).
    # sy=80: pin1 (53.81, 78.73), pin2 (53.81, 81.27)
    wires.append(wire(53.81, 78.73, 60, 78.73, seed_suffix="j9-bat-plus"))
    hlabels.append(hier_label(60, 78.73, "BAT_PLUS", shape="output", rotation=0))
    wires.append(wire(53.81, 81.27, 60, 81.27, seed_suffix="j9-gnd"))
    attach_gnd(60, 81.27, "J9", rotation=270)

    # ====================================================================
    # U7 MCP73831 LiPo-Charger @ (90, 80). 5-Pin SOT-23-5.
    # Pin layout (from _mcp73831_lib_symbol): 1=STAT(-7.62,5.08), 2=VSS(-7.62,0),
    # 3=VBAT(+7.62,5.08), 4=VDD(+7.62,0), 5=PROG(+7.62,-5.08).
    # ====================================================================
    U7_X, U7_Y = 90.0, 80.0
    symbols.append(
        place_symbol(
            lib_id="Battery:MCP73831",
            ref="U7",
            value="MCP73831T-2ACI/OT (LiPo Charger, 500mA via R21=2k)",
            x=U7_X, y=U7_Y,
            footprint="Package_TO_SOT_SMD:SOT-23-5",
            datasheet="https://ww1.microchip.com/downloads/en/DeviceDoc/MCP73831-Family-Data-Sheet-DS20001984H.pdf",
            extra_props={"MPN": "MCP73831T-2ACI/OT", "LCSC": "C424093"},
            seed_suffix="U7",
            sheet_uuid_seed=sus,
        )
    )
    # ---- U7 VSS (Pin 2, left, y=80) → GND
    wires.append(wire(U7_X - 7.62, U7_Y, U7_X - 12, U7_Y, seed_suffix="u7-vss"))
    attach_gnd(U7_X - 12, U7_Y, "U7_VSS", rotation=90)

    # ---- U7 STAT (Pin 1, left, y=75) → R_CHRG (1kΩ) → LED_CHRG kathode; LED_CHRG anode → +5V
    # Topology: STAT ist open-drain LOW=charging. Strom-Pfad +5V → LED_A → LED_K → R_CHRG → STAT-pin
    # (LOW). Wire-Endpunkte alle exakt an Pin-Positionen (sonst dangling).
    p_stat_y = U7_Y - 5.08  # = 74.92
    # R_CHRG rotation=90 horizontal at (U7_X-16, p_stat_y): pin1 (U7_X-19.81), pin2 (U7_X-12.19)
    # U7-STAT-pin abs = (U7_X-7.62, p_stat_y). Wire muss U7-STAT → R_CHRG-pin2 exakt verbinden.
    wires.append(wire(U7_X - 7.62, p_stat_y, U7_X - 12.19, p_stat_y, seed_suffix="u7-stat-to-rchrg"))
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_CHRG",
            value="1k 0603 (LED_CHRG series)",
            x=U7_X - 16, y=p_stat_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF1001T5E", "LCSC": "C21190"},
            seed_suffix="R_CHRG",
            sheet_uuid_seed=sus,
        )
    )
    # LED_CHRG at (U7_X-23, p_stat_y), rotation=180: pin1 K abs (sx+3.81)=U7_X-19.19 RIGHT (zu R_CHRG),
    # pin2 A abs (sx-3.81)=U7_X-26.81 LEFT (zu +5V). Forward-biased: +5V → A → K → R_CHRG → STAT-LOW.
    symbols.append(
        place_symbol(
            lib_id="Device:LED",
            ref="LED_CHRG",
            value="0603 amber (LiPo Charging indicator)",
            x=U7_X - 23, y=p_stat_y,
            rotation=180,
            footprint="LED_SMD:LED_0603_1608Metric",
            extra_props={"MPN": "Generic amber 0603", "LCSC": "C72041"},
            seed_suffix="LED_CHRG",
            sheet_uuid_seed=sus,
        )
    )
    # R_CHRG pin1 (U7_X-19.81) → LED_CHRG kathode pin1 (U7_X-19.19): wire bridge 0.62mm
    wires.append(wire(U7_X - 19.81, p_stat_y, U7_X - 19.19, p_stat_y, seed_suffix="r-chrg-to-led"))
    # LED_CHRG Anode pin2 (U7_X-26.81) → +5V flag at (U7_X-30)
    wires.append(wire(U7_X - 26.81, p_stat_y, U7_X - 30, p_stat_y, seed_suffix="led-chrg-to-5v"))
    symbols.append(
        place_symbol(
            lib_id="Power:+5V",
            ref="#PWR_LED_CHRG",
            value="+5V",
            x=U7_X - 30, y=p_stat_y,
            rotation=270,
            seed_suffix="led-chrg-5v",
            sheet_uuid_seed=sus,
        )
    )

    # ---- U7 VDD (Pin 4, right, y=80) → VBUS_USBC hier-input (matches Q1-S + sheet-edge)
    p_vdd_y = U7_Y  # 80
    wires.append(wire(U7_X + 7.62, p_vdd_y, U7_X + 14, p_vdd_y, seed_suffix="u7-vdd"))
    hlabels.append(hier_label(U7_X + 14, p_vdd_y, "VBUS_USBC", shape="input", rotation=0))

    # ---- U7 VBAT (Pin 3, right, y=75) → BAT_PLUS hier-output (zur Pico via root_sheet)
    p_vbat_y = U7_Y - 5.08  # 74.92
    wires.append(wire(U7_X + 7.62, p_vbat_y, U7_X + 14, p_vbat_y, seed_suffix="u7-vbat"))
    hlabels.append(hier_label(U7_X + 14, p_vbat_y, "BAT_PLUS", shape="output", rotation=0))

    # ---- U7 PROG (Pin 5, right, y=85) → R21 (2k) → GND. Sets Icharge=1000/Rprog=500mA.
    # Wire-Endpunkte exakt an Pin-Positionen (R rotation=90: pin1 sx-3.81, pin2 sx+3.81).
    p_prog_y = U7_Y + 5.08  # 85.08
    # R21 at (U7_X+18, p_prog_y): pin1=U7_X+14.19, pin2=U7_X+21.81
    wires.append(wire(U7_X + 7.62, p_prog_y, U7_X + 14.19, p_prog_y, seed_suffix="u7-prog-to-r21"))
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R21",
            value="2k 0603 (MCP73831 R_PROG → 500mA Icharge)",
            x=U7_X + 18, y=p_prog_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF2001T5E", "LCSC": "C22975"},
            seed_suffix="R21",
            sheet_uuid_seed=sus,
        )
    )
    # R21 pin2 (U7_X+21.81, p_prog_y) → GND
    wires.append(wire(U7_X + 21.81, p_prog_y, U7_X + 25, p_prog_y, seed_suffix="r21-to-gnd"))
    attach_gnd(U7_X + 25, p_prog_y, "R21", rotation=270)

    # ====================================================================
    # BAT_PLUS bus — Decoupling C_BAT_IN (22µF) + C_BAT_HF (100nF) at (75, 90).
    # ====================================================================
    # C_BAT_IN at (75, 90) vertical. pin1 top (75, 86.19), pin2 bottom (75, 93.81)
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_BAT_IN",
            value="22uF X5R 0805 (Battery-Input Bulk)",
            x=75, y=90,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "GRM21BR61E226ME44", "LCSC": "C45783"},
            seed_suffix="C_BAT_IN",
            sheet_uuid_seed=sus,
        )
    )
    # BAT_PLUS-bus runs horizontally at y=86.19 from x=70 to x=130 (to U8-VIN later)
    wires.append(wire(75, 86.19, 75, 80, seed_suffix="c-bat-in-top-up"))
    wires.append(wire(70, 80, 90, 80, seed_suffix="bat-bus-from-j9"))  # but J9-pin1 is at y=78.73
    # Reroute: BAT_PLUS-net via label. Just attach a label at C_BAT_IN top + at J9 + at U7-VBAT + at U8-VIN.
    # Cleaner via name-matching with "BAT_PLUS" label everywhere.
    # Remove the misplaced wire
    wires.pop()  # remove "bat-bus-from-j9"
    wires.append(wire(75, 86.19, 79, 86.19, seed_suffix="c-bat-in-top-stub"))
    hlabels.append(hier_label(79, 86.19, "BAT_PLUS", shape="output", rotation=0))
    # C_BAT_IN bottom → GND
    wires.append(wire(75, 93.81, 75, 96, seed_suffix="c-bat-in-gnd"))
    attach_gnd(75, 96, "C_BAT_IN", rotation=0)

    # C_BAT_HF at (82, 90) vertical
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_BAT_HF",
            value="100nF X7R 0603 (Battery-Input HF)",
            x=82, y=90,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10B104KO8NNNC", "LCSC": "C14663"},
            seed_suffix="C_BAT_HF",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(82, 86.19, 79, 86.19, seed_suffix="c-bat-hf-top-stub"))
    junctions.append(junction(79, 86.19))
    wires.append(wire(82, 93.81, 82, 96, seed_suffix="c-bat-hf-gnd"))
    attach_gnd(82, 96, "C_BAT_HF", rotation=0)

    # ====================================================================
    # U8 TPS61089RNR Boost @ (140, 90). VQFN-11 HotRod 2x2.5mm + Thermal Pad.
    # Decision r12-B11: Wechsel auf RNR-Variante (C165129) statt RNSR, weil RNSR
    # nicht bei LCSC verfügbar — JLC-soviel-wie-möglich-Strategie. Komplett anderer
    # Pinout + benötigt 6 zusätzliche externe Bauteile (C_VCC, R_FSW, R_ILIM,
    # C_BOOT, R_COMP+C_COMP).
    # Left pins (1..6, top→bottom): 1=FSW, 2=VCC, 3=FB, 4=COMP, 5=GND, 6=VOUT
    # Right pins (11..7, top→bottom): 11=SW, 10=BOOT, 9=VIN, 8=ILIM, 7=EN
    # ====================================================================
    U8_X, U8_Y = 140.0, 90.0
    U8_LX = U8_X - 10.16  # 129.84
    U8_RX = U8_X + 10.16  # 150.16
    symbols.append(
        place_symbol(
            lib_id="Regulator_Switching:TPS61089",
            ref="U8",
            value="TPS61089RNR (Boost LiPo→5V, 2A, programmable Fsw)",
            x=U8_X, y=U8_Y,
            footprint="field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A",
            datasheet="https://www.ti.com/lit/ds/symlink/tps61089.pdf",
            extra_props={
                "MPN": "TPS61089RNR",
                "Manufacturer": "Texas Instruments",
                "LCSC": "C165129",
                "Package": "VQFN-HR 11-pin 2.0 x 2.5 mm (RNR0011A)",
                "PIN_SOURCE": "TI-Datenblatt TPS61089 Rev C, Pin Functions Tabelle 6-1: 1=FSW, 2=VCC, 3=FB, 4=COMP, 5=GND, 6=VOUT, 7=EN, 8=ILIM, 9=VIN, 10=BOOT, 11=SW (Symbol entspricht dem)",
                "FP_NOTE": "Custom-FP field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A — TI-RNR0011A Land Pattern (Datasheet Mechanical Drawing 4222143/A, 08/2015) nachgebaut: 8x Side-Pads 0.55x0.25mm @ 0.5mm Pitch, EP-Pads 5+6 je 0.7x1.0mm, EP 11 (Thermal Pad) zentral 2.35x0.7mm. Alternative: TPS61089RNRR-FP von UltraLibrarian direkt importieren.",
            },
            seed_suffix="U8",
            sheet_uuid_seed=sus,
        )
    )

    # U8 pin Y positions per RNR lib_symbol (left pins 1..6, right pins 11..7).
    def u8_left_py(n: int) -> float:
        return U8_Y - 6.35 + (n - 1) * 2.54
    pins_right_order = [11, 10, 9, 8, 7]
    def u8_right_py(n: int) -> float:
        idx = pins_right_order.index(n)
        return U8_Y - 6.35 + idx * 2.54

    fsw_y  = u8_left_py(1)   # 83.65
    vcc_y  = u8_left_py(2)   # 86.19
    fb_y   = u8_left_py(3)   # 88.73
    comp_y = u8_left_py(4)   # 91.27
    gnd_y  = u8_left_py(5)   # 93.81
    vout_y = u8_left_py(6)   # 96.35
    sw_y   = u8_right_py(11) # 83.65
    boot_y = u8_right_py(10) # 86.19
    vin_y  = u8_right_py(9)  # 88.73
    ilim_y = u8_right_py(8)  # 91.27
    en_y   = u8_right_py(7)  # 93.81

    # ---- U8 SW (Pin 11, right) → L1 + R_FSW + C_BOOT shared switch-node
    # Label "U8_SW_NODE" auf SW-Pin, gleicher Label-Name an L1-Bottom, R_FSW-pin2, C_BOOT-pin2.
    wires.append(wire(U8_RX, sw_y, 156, sw_y, seed_suffix="u8-sw-stub"))
    labels.append(label(154, sw_y, "U8_SW_NODE"))

    # L1 above U8 at (140, 75) vertical. pin1 top (140, 71.19) → BAT_PLUS, pin2 bottom (140, 78.81) → U8_SW_NODE.
    symbols.append(
        place_symbol(
            lib_id="Device:L",
            ref="L1",
            value="2.2uH 5A Shielded 0630 (TPS61089 Boost Inductor)",
            x=140, y=75,
            rotation=0,
            footprint="field_ambience:L_Sunlord_SWPA6045",
            extra_props={"MPN": "SWPA6045S2R2MT", "LCSC": "C83455"},
            seed_suffix="L1",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(140, 71.19, 140, 68, seed_suffix="l1-top-stub"))
    hlabels.append(hier_label(140, 68, "BAT_PLUS", shape="output", rotation=90))
    wires.append(wire(140, 78.81, 140, sw_y, seed_suffix="l1-bottom-to-sw"))
    wires.append(wire(140, sw_y, 156, sw_y, seed_suffix="l1-to-sw-h"))
    junctions.append(junction(140, sw_y))
    # Place a second "U8_SW_NODE" label at L1-Bottom-Wire-Path to confirm net
    labels.append(label(141, sw_y, "U8_SW_NODE"))

    # ---- U8 VIN (Pin 9, right) → BAT_PLUS hier-output
    wires.append(wire(U8_RX, vin_y, 156, vin_y, seed_suffix="u8-vin-stub"))
    hlabels.append(hier_label(156, vin_y, "BAT_PLUS", shape="output", rotation=0))

    # ---- U8 EN (Pin 7, right) → BAT_PLUS (always-on when battery present)
    wires.append(wire(U8_RX, en_y, 156, en_y, seed_suffix="u8-en-stub"))
    hlabels.append(hier_label(156, en_y, "BAT_PLUS", shape="output", rotation=0))

    # ---- U8 BOOT (Pin 10, right) → C_BOOT 100nF → U8_SW_NODE
    wires.append(wire(U8_RX, boot_y, 154, boot_y, seed_suffix="u8-boot-stub"))
    # C_BOOT placed vertically at (158, boot_y+3) so pin1 top connects to BOOT-line, pin2 bottom to SW-line
    # Device:C pin1 (top) at (sx, sy-3.81), pin2 (bottom) at (sx, sy+3.81)
    # We want pin1 at y=boot_y → sy=boot_y+3.81. Pin2 at sy+3.81 = boot_y+7.62.
    # Hmm — pin2 needs to land at sw_y for natural routing. boot_y=86.19, sw_y=83.65 (UP from boot). Not natural vertical.
    # Use horizontal C_BOOT instead: rotation=90 horizontal. pin1 left (sx-3.81, sy), pin2 right (sx+3.81, sy).
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_BOOT",
            value="100nF X7R 0603 (TPS61089 BOOT-SW cap)",
            x=158, y=(boot_y + sw_y) / 2,  # midpoint between BOOT and SW
            rotation=0,  # vertical
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10B104KO8NNNC", "LCSC": "C14663"},
            seed_suffix="C_BOOT",
            sheet_uuid_seed=sus,
        )
    )
    # C_BOOT vertical at (158, (boot_y+sw_y)/2=84.92). pin1 top (158, 81.11), pin2 bottom (158, 88.73)
    # That's not at sw_y or boot_y exactly. Need slight adjustment.
    # Simpler: place C_BOOT vertically with explicit endpoints aligned.
    # Replace last symbol with explicit positioning:
    symbols.pop()  # remove
    c_boot_sy = (boot_y + sw_y) / 2 + 0.585  # tune so pin1 at sw_y exactly
    # Actually pin1 = sy - 3.81. Want pin1 = sw_y = 83.65 → sy = 87.46
    # Then pin2 = 87.46 + 3.81 = 91.27. Hmm that's at COMP-pin-y.
    # Switch to clean horizontal layout. C_BOOT-position at (158, 76) horizontal between BOOT-up-wire and SW-up-wire.
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_BOOT",
            value="100nF X7R 0603 (TPS61089 BOOT-SW cap)",
            x=158, y=80,
            rotation=90,  # horizontal: pin1 left (154.19, 80), pin2 right (161.81, 80)
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10B104KO8NNNC", "LCSC": "C14663"},
            seed_suffix="C_BOOT",
            sheet_uuid_seed=sus,
        )
    )
    # Wire BOOT-pin via 154-stub up to (154, 80) → right to C_BOOT pin1 (154.19, 80). Then bridge.
    wires.append(wire(154, boot_y, 154, 80, seed_suffix="boot-up"))
    wires.append(wire(154, 80, 154.19, 80, seed_suffix="boot-to-cboot"))
    # C_BOOT pin2 (161.81, 80) → up to (161.81, 76) → left to SW-node label at (162, sw_y)
    # Cleaner: just label both ends to share net.
    labels.append(label(154, 80, "U8_BOOT_NODE"))
    wires.append(wire(161.81, 80, 165, 80, seed_suffix="cboot-to-swlabel"))
    labels.append(label(165, 80, "U8_SW_NODE"))

    # ---- U8 FSW (Pin 1, left) → R_FSW 360k → U8_SW_NODE
    wires.append(wire(U8_LX, fsw_y, 124, fsw_y, seed_suffix="u8-fsw-stub"))
    # R_FSW at (118, fsw_y) rotation=90 horizontal. pin1 (114.19, fsw_y), pin2 (121.81, fsw_y)
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_FSW",
            value="360k 0603 1% (TPS61089 Fsw-Set ~1.21MHz, r12-B11)",
            x=118, y=fsw_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF3603T5E", "LCSC": "C23146"},
            seed_suffix="R_FSW",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(121.81, fsw_y, 124, fsw_y, seed_suffix="rfsw-to-u8"))
    # R_FSW pin1 (114.19, fsw_y) → wire left, label "U8_SW_NODE"
    wires.append(wire(114.19, fsw_y, 112, fsw_y, seed_suffix="rfsw-to-sw-label"))
    labels.append(label(112, fsw_y, "U8_SW_NODE"))

    # ---- U8 VCC (Pin 2, left) → C_VCC 1µF → GND
    wires.append(wire(U8_LX, vcc_y, 124, vcc_y, seed_suffix="u8-vcc-stub"))
    # C_VCC vertical at (124, vcc_y+5). pin1 top (124, vcc_y+1.19), pin2 bottom (124, vcc_y+8.81)
    # Want pin1 at vcc_y → sy = vcc_y + 3.81 = 90.0. pin2 at 93.81.
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_VCC",
            value="1uF X7R 0603 (TPS61089 internal LDO decoupling, datasheet>=1uF)",
            x=124, y=vcc_y + 3.81,
            rotation=0,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10A105KB8NNNC", "LCSC": "C15849"},
            seed_suffix="C_VCC",
            sheet_uuid_seed=sus,
        )
    )
    # pin1 (124, vcc_y), pin2 (124, vcc_y+7.62)
    wires.append(wire(124, vcc_y + 7.62, 124, vcc_y + 10, seed_suffix="cvcc-to-gnd"))
    attach_gnd(124, vcc_y + 10, "C_VCC", rotation=0)

    # ---- U8 FB (Pin 3, left) → divider R23/R24 (BOOST_OUT → FB → GND)
    wires.append(wire(U8_LX, fb_y, 124, fb_y, seed_suffix="u8-fb-stub"))
    labels.append(label(124, fb_y, "BOOST_FB"))
    # R23 (200k) FB-Divider top, vertical at (170, 75).
    # pin1 top (170, 71.19) → label BOOST_OUT (Same-Name-Match mit U8-VOUT-Label).
    # pin2 bottom (170, 78.81) → R24 pin1 top + FB-tap label BOOST_FB
    # WICHTIG: KEINE durchgehende Wire vertikal über R23+R24 — würde Divider kurzschließen!
    r23_sy = 75
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R23",
            value="200k 0603 (TPS61089 FB-Divider Top, Vout=5V target)",
            x=170, y=r23_sy,
            rotation=0,  # vertical
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF2003T5E", "LCSC": "C25811"},
            seed_suffix="R23",
            sheet_uuid_seed=sus,
        )
    )
    # R23 pin1 (top) — short stub + Label
    wires.append(wire(170, r23_sy - 3.81, 170, r23_sy - 6, seed_suffix="r23-top-stub"))
    labels.append(label(170, r23_sy - 6, "BOOST_OUT"))
    # R23 pin2 (bottom) → R24 pin1 (top) — kurze Brücke + FB-tap
    r24_sy = r23_sy + 7.62  # 82.62. R24 pin1 top at r24_sy-3.81=78.81 → exakt R23 pin2.
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R24",
            value="39k 0603 (TPS61089 FB-Divider Bottom)",
            x=170, y=r24_sy,
            rotation=0,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF3902T5E", "LCSC": "C23153"},
            seed_suffix="R24",
            sheet_uuid_seed=sus,
        )
    )
    # FB-tap exactly at R23-pin2 / R24-pin1 (both at y=78.81 → already touching).
    # Add a small horizontal stub + Label "BOOST_FB" at the junction.
    fb_tap_y = r23_sy + 3.81  # = 78.81 = R24-pin1-top abs
    wires.append(wire(170, fb_tap_y, 175, fb_tap_y, seed_suffix="fb-tap-stub"))
    labels.append(label(175, fb_tap_y, "BOOST_FB"))
    # R24 pin2 (bottom) → GND
    wires.append(wire(170, r24_sy + 3.81, 170, r24_sy + 6, seed_suffix="r24-gnd"))
    attach_gnd(170, r24_sy + 6, "R24", rotation=0)

    # ---- U8 COMP (Pin 4, left) → R_COMP + C_COMP series → GND (Type-II compensation)
    wires.append(wire(U8_LX, comp_y, 124, comp_y, seed_suffix="u8-comp-stub"))
    # R_COMP at (118, comp_y) horizontal rotation=90. pin1 (114.19, comp_y), pin2 (121.81, comp_y)
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_COMP",
            value="22k 0603 (TPS61089 loop-compensation)",
            x=118, y=comp_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF2202T5E", "LCSC": "C31850"},
            seed_suffix="R_COMP",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(121.81, comp_y, 124, comp_y, seed_suffix="rcomp-to-u8"))
    # R_COMP pin1 (114.19) → C_COMP at (108, comp_y) horizontal. pin1 (104.19, comp_y), pin2 (111.81, comp_y)
    wires.append(wire(114.19, comp_y, 111.81, comp_y, seed_suffix="rcomp-to-ccomp"))
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_COMP",
            value="1nF X7R 0603 (TPS61089 loop-compensation series)",
            x=108, y=comp_y,
            rotation=90,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10B102KB8NNNC", "LCSC": "C1588"},
            seed_suffix="C_COMP",
            sheet_uuid_seed=sus,
        )
    )
    # C_COMP pin1 (104.19) → GND
    wires.append(wire(104.19, comp_y, 102, comp_y, seed_suffix="ccomp-to-gnd"))
    attach_gnd(102, comp_y, "C_COMP", rotation=90)

    # ---- U8 GND (Pin 5, left) → GND
    wires.append(wire(U8_LX, gnd_y, 124, gnd_y, seed_suffix="u8-gnd-stub"))
    attach_gnd(124, gnd_y, "U8_GND", rotation=90)

    # ---- U8 VOUT (Pin 6, left) → BOOST_OUT
    wires.append(wire(U8_LX, vout_y, 124, vout_y, seed_suffix="u8-vout-stub"))
    labels.append(label(124, vout_y, "BOOST_OUT"))

    # ---- U8 ILIM (Pin 8, right) → R_ILIM 20k → GND (sets ~4A peak inductor current)
    wires.append(wire(U8_RX, ilim_y, 156, ilim_y, seed_suffix="u8-ilim-stub"))
    # R_ILIM at (160, ilim_y+4) vertical rotation=0. pin1 (160, ilim_y+0.19), pin2 (160, ilim_y+7.81)
    # Cleaner: horizontal at (160, ilim_y)
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R_ILIM",
            value="20k 0603 1% (TPS61089 current-limit ~4A peak)",
            x=160, y=ilim_y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF2002T5E", "LCSC": "C4184"},
            seed_suffix="R_ILIM",
            sheet_uuid_seed=sus,
        )
    )
    # R_ILIM pin1 (156.19, ilim_y), pin2 (163.81, ilim_y)
    wires.append(wire(156, ilim_y, 156.19, ilim_y, seed_suffix="rilim-to-u8"))
    wires.append(wire(163.81, ilim_y, 166, ilim_y, seed_suffix="rilim-to-gnd"))
    attach_gnd(166, ilim_y, "R_ILIM", rotation=270)

    # ---- r18.7 CORRECTION: TPS61089-RNR has NO separate exposed pad.
    # The old code had "pin 12 = ePAD → GND" which is wrong per TI
    # Mechanical Drawing 4222143/A: the central thermal pad IS pin 11 (SW),
    # carrying both switching current and thermal duty. GND is taken via
    # pin 5 (one of the enlarged side pads, wired above at U8 line 6331).
    # The "pin 12" stub has been removed — was a fake artifact from r12-B11
    # when the symbol was inherited from the older RNSR variant.

    # ====================================================================
    # BOOST_OUT bus → C_BOOST_OUT (22µF) + C_BOOST_HF (100nF) + D3 → +5V_OUT
    # ====================================================================
    # C_BOOST_OUT 22µF vertical at (180, 105). Connected via BOOST_OUT label-match.
    # pin1 top (180, 101.19), pin2 bottom (180, 108.81). Top → BOOST_OUT, bottom → GND.
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_BOOST_OUT",
            value="22uF X5R 0805 (Boost-Output Bulk)",
            x=180, y=105,
            footprint="Capacitor_SMD:C_0805_2012Metric",
            extra_props={"MPN": "GRM21BR61E226ME44", "LCSC": "C45783"},
            seed_suffix="C_BOOST_OUT",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(180, 101.19, 180, 98, seed_suffix="c-boost-out-top"))
    labels.append(label(180, 98, "BOOST_OUT"))
    wires.append(wire(180, 108.81, 180, 111, seed_suffix="c-boost-out-gnd"))
    attach_gnd(180, 111, "C_BOOST_OUT", rotation=0)

    # C_BOOST_HF 100nF vertical at (188, 105). Same topology.
    symbols.append(
        place_symbol(
            lib_id="Device:C",
            ref="C_BOOST_HF",
            value="100nF X7R 0603 (Boost-Output HF)",
            x=188, y=105,
            footprint="Capacitor_SMD:C_0603_1608Metric",
            extra_props={"MPN": "CL10B104KO8NNNC", "LCSC": "C14663"},
            seed_suffix="C_BOOST_HF",
            sheet_uuid_seed=sus,
        )
    )
    wires.append(wire(188, 101.19, 188, 98, seed_suffix="c-boost-hf-top"))
    labels.append(label(188, 98, "BOOST_OUT"))
    wires.append(wire(188, 108.81, 188, 111, seed_suffix="c-boost-hf-gnd"))
    attach_gnd(188, 111, "C_BOOST_HF", rotation=0)

    # D3 Schottky from BOOST_OUT (Anode) to +5V_OUT (Kathode). Horizontal at (200, 96).
    # rotation=180: pin1 K abs (sx+3.81=203.81), pin2 A abs (sx-3.81=196.19). A links, K rechts.
    symbols.append(
        place_symbol(
            lib_id="Device:D_Schottky",
            ref="D3",
            value="SS34 40V 3A Schottky (Boost-Out Reverse-Protect)",
            x=200, y=96,
            rotation=180,
            footprint="Diode_SMD:D_SMA",
            extra_props={"MPN": "SS34", "LCSC": "C8678"},
            seed_suffix="D3",
            sheet_uuid_seed=sus,
        )
    )
    # D3 Anode (left, 196.19) → BOOST_OUT label
    wires.append(wire(196.19, 96, 193, 96, seed_suffix="d3-anode-stub"))
    labels.append(label(193, 96, "BOOST_OUT"))
    # D3 Kathode (right, 203.81) → +5V_OUT hier-output (Boost-Pfad to system rail)
    wires.append(wire(203.81, 96, 210, 96, seed_suffix="d3-k-to-5vout"))
    hlabels.append(hier_label(210, 96, "+5V_OUT", shape="output", rotation=0))

    # ====================================================================
    # Q1 DMG2305UX P-MOS Power-Path @ (210, 70). 3-Pin SOT-23.
    # Pin layout (from _dmg2305ux_lib_symbol): 1=G(-7.62,0), 2=S(+7.62,+2.54), 3=D(+7.62,-2.54).
    # SPEC: S=VBUS_USBC, D=+5V_OUT, G=R22→GND (Default-OFF pull-down).
    # ====================================================================
    Q1_X, Q1_Y = 210.0, 70.0
    symbols.append(
        place_symbol(
            lib_id="Transistor_FET:DMG2305UX",
            ref="Q1",
            value="DMG2305UX P-MOS SOT-23 (USB-C Power-Path-Switch)",
            x=Q1_X, y=Q1_Y,
            footprint="Package_TO_SOT_SMD:SOT-23",
            datasheet="https://www.diodes.com/assets/Datasheets/DMG2305UX.pdf",
            extra_props={"MPN": "DMG2305UX-7", "LCSC": "C150470"},
            seed_suffix="Q1",
            sheet_uuid_seed=sus,
        )
    )
    # Q1 G (Pin 1, left, abs (Q1_X-7.62, Q1_Y))
    q1_g_x = Q1_X - 7.62
    wires.append(wire(q1_g_x, Q1_Y, q1_g_x - 5, Q1_Y, seed_suffix="q1-g-stub"))
    # R22 (10k pull-down G to GND) at (q1_g_x-9, Q1_Y) rotation=90 horizontal
    symbols.append(
        place_symbol(
            lib_id="Device:R",
            ref="R22",
            value="10k 0603 (Q1 Gate Pull-Down, Default-OFF)",
            x=q1_g_x - 9, y=Q1_Y,
            rotation=90,
            footprint="Resistor_SMD:R_0603_1608Metric",
            extra_props={"MPN": "0603WAF1002T5E", "LCSC": "C25804"},
            seed_suffix="R22",
            sheet_uuid_seed=sus,
        )
    )
    # R22 pin1 (q1_g_x-12.81, Q1_Y) → GND
    wires.append(wire(q1_g_x - 12.81, Q1_Y, q1_g_x - 16, Q1_Y, seed_suffix="r22-to-gnd"))
    attach_gnd(q1_g_x - 16, Q1_Y, "R22", rotation=90)
    # R22 pin2 (q1_g_x-5.19, Q1_Y) connects to G-stub at (q1_g_x-5, Q1_Y) — close enough but add bridging wire
    wires.append(wire(q1_g_x - 5.19, Q1_Y, q1_g_x - 5, Q1_Y, seed_suffix="r22-to-g"))

    # Q1 S (Pin 2, abs Y inverted vs Symbol-local: local +2.54 → abs Q1_Y - 2.54) → VBUS_USBC
    q1_s_y = Q1_Y - 2.54  # 67.46
    wires.append(wire(Q1_X + 7.62, q1_s_y, Q1_X + 14, q1_s_y, seed_suffix="q1-s-stub"))
    hlabels.append(hier_label(Q1_X + 14, q1_s_y, "VBUS_USBC", shape="input", rotation=180))

    # Q1 D (Pin 3, local -2.54 → abs Q1_Y + 2.54) → +5V_OUT hier-output (USB-Pfad to system rail)
    q1_d_y = Q1_Y + 2.54  # 72.54
    wires.append(wire(Q1_X + 7.62, q1_d_y, Q1_X + 14, q1_d_y, seed_suffix="q1-d-stub"))
    hlabels.append(hier_label(Q1_X + 14, q1_d_y, "+5V_OUT", shape="output", rotation=0))

    # ====================================================================
    # Hier-Labels für externe Nets sind inline an U7/U8/Q1/J9/C_BAT_IN platziert
    # (Same-Name-Match mit root_sheet's Sheet-Pins). Keine separaten Edge-Labels
    # nötig — KiCad merged alle gleichnamigen hier-labels innerhalb des Sheets.
    # ====================================================================

    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{sheet_uuid}")\n'
        f'  (paper "A3")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Sheet 7: Battery & Power-Path (r9 + r12)")\n'
        f'    (date "2026-05-31")\n'
        f'    (rev "0.6.3-r12")\n'
        f'    (company "Field Ambience Project")\n'
        f'    (comment 1 "Per SPEC v0.6 §2.2 (Battery-Add r9 + Battery-Sense r12)")\n'
        f'    (comment 2 "U7 MCP73831 LiPo-Charger 500mA + U8 TPS61089 Boost LiPo→5V")\n'
        f'    (comment 3 "Q1 DMG2305UX P-MOS USB-Path-Selector + D3 SS34 Boost-Reverse-Protect")\n'
        f'    (comment 4 "Hier-I/O: VBUS_USBC← +5V_OUT→ BAT_PLUS→"))\n'
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
    stm32_uuid = det_uuid("sheet_stm32")
    lcd_uuid = det_uuid("sheet_lcd")
    mcp_uuid = det_uuid("sheet_mcp")
    enc_uuid = det_uuid("sheet_encoder")
    audio_uuid = det_uuid("sheet_audio")
    body = (
        f'(kicad_sch (version {KICAD_VERSION_TAG}) {GENERATOR}\n'
        f'  (uuid "{root_uuid}")\n'
        f'  (paper "A4")\n'
        f'  (title_block\n'
        f'    (title "Field Ambience PCB — Root")\n'
        f'    (date "2026-05-12")\n'
        f'    (rev "0.7")\n'
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
        f'    (pin "+3V3_OUT" output (at 90 60 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_3v3")}"))\n'
        f'    (pin "USB_DP" output (at 90 75 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_dp")}"))\n'
        f'    (pin "USB_DM" output (at 90 80 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_dn")}"))\n'
        # NEU r12: VBUS_USBC (raw USB-C VBUS pre-fuse) für battery_sheet U7-Charger + mcp-GPA7-Detect
        f'    (pin "VBUS_USBC" output (at 90 95 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("rootpin_vbus_usbc")}")))\n'
        # ---- Sheet 2: Pico (extended height für Encoder-Pins) ----
        f'  (sheet (at 130 40) (size 60 130) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{stm32_uuid}")\n'
        f'    (property "Sheetname" "STM32H743" (at 130 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "stm32h743.kicad_sch" (at 130 170.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "USB_DP" input (at 130 75 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_dp")}"))\n'
        f'    (pin "USB_DM" input (at 130 80 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_dn")}"))\n'
        f'    (pin "LCD_SCK" output (at 190 60 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_osck")}"))\n'
        f'    (pin "LCD_MOSI" output (at 190 65 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_omosi")}"))\n'
        f'    (pin "LCD_CS" output (at 190 70 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_ocs")}"))\n'
        f'    (pin "LCD_DC" output (at 190 75 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("picopin_odc")}"))\n'
        f'    (pin "LCD_RES" output (at 190 80 0)\n'
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
        # ---- Audio control outputs ----
        f'    (pin "AMP_nSHDN" output (at 130 95 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_amp_shdn")}"))\n'
        f'    (pin "AMP_nMUTE" output (at 130 100 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_amp_mute")}"))\n'
        # ---- v0.9: I²S master out → PCM5102A (was UART to Pi) ----
        f'    (pin "I2S_BCK" output (at 130 105 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_i2sbck")}"))\n'
        f'    (pin "I2S_LRCK" output (at 130 110 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_i2slrck")}"))\n'
        f'    (pin "I2S_DOUT" output (at 130 115 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_i2sdout")}"))\n'
        # NEU r12: BAT_PLUS input (Battery+ für BAT_SENSE-Divider an GP26/ADC0)
        f'    (pin "BAT_PLUS" input (at 130 165 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("picopin_bat_plus")}")))\n'
        # ---- Sheet 3: OLED ----
        f'  (sheet (at 230 40) (size 60 60) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{lcd_uuid}")\n'
        f'    (property "Sheetname" "LCD" (at 230 39 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "lcd.kicad_sch" (at 230 100.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "LCD_SCK" input (at 230 60 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_sck")}"))\n'
        f'    (pin "LCD_MOSI" input (at 230 65 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_mosi")}"))\n'
        f'    (pin "LCD_CS" input (at 230 70 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_cs")}"))\n'
        f'    (pin "LCD_DC" input (at 230 75 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_dc")}"))\n'
        f'    (pin "LCD_RES" input (at 230 80 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("oledpin_res")}"))\n'
        f'    (pin "LCD_BLK_PWM" input (at 230 85 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("lcdpin_blk")}")))\n'
        f'  (wire (pts (xy 225 85) (xy 230 85)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_blk_lcd")}"))\n'
        f'  (label "LCD_BLK_PWM" (at 225 85 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_blk_lcd")}"))\n'
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
        f'      (uuid "{det_uuid("mcppin_xsmt")}"))\n'
        # v0.7: JACK_DETECT input (MCP GPA6 reads line-out jack switch)
        f'    (pin "JACK_DETECT" input (at 190 210 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("mcppin_jackdet")}"))\n'
        # NEU r12: VBUS_USBC input (raw USB-C VBUS pre-fuse für GPA7-Detect via R_VBUS_SENSE)
        f'    (pin "VBUS_USBC" input (at 130 225 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("mcppin_vbus_usbc")}"))\n'
        f'    (pin "VOL_SW" input (at 190 225 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("mcppin_vol_sw")}"))\n'
        f'    (pin "LCD_BLK_PWM" output (at 190 220 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("mcppin_blk_pwm")}")))\n'
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
        f'  (wire (pts (xy 90 50) (xy 112 50)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_5v")}"))\n'
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
        # ---- v0.9: Pi-Header-Sheet (Sheet 7) entfernt. Der RP2350 erzeugt I²S
        # selbst (GP0/GP1/GP4), kein Raspberry Pi mehr. Pico-I²S-Outputs werden
        # weiter unten per Label an das Audio-Sheet gebrückt.
        # ---- Inter-sheet labels: Pico I²S out → Audio Sheet (gleiche Labels
        # liegen schon auf der Audio-Eingangsseite, s.u.) ----
        f'  (wire (pts (xy 125 105) (xy 130 105)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2sbck_pico")}"))\n'
        f'  (label "I2S_BCK" (at 125 105 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2sbck_pico")}"))\n'
        f'  (wire (pts (xy 125 110) (xy 130 110)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2slrck_pico")}"))\n'
        f'  (label "I2S_LRCK" (at 125 110 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2slrck_pico")}"))\n'
        f'  (wire (pts (xy 125 115) (xy 130 115)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_i2sdout_pico")}"))\n'
        f'  (label "I2S_DOUT" (at 125 115 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_i2sdout_pico")}"))\n'
        # ---- Sheet 6: Audio (rechts neben MCP, unter Encoder-Sheet) ----
        f'  (sheet (at 230 200) (size 60 52) (fields_autoplaced)\n'
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
        f'    (pin "AMP_nSHDN" input (at 230 230 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_shdn")}"))\n'
        f'    (pin "AMP_nMUTE" input (at 230 235 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_mute")}"))\n'
        # v0.6.3-r5 N1-Fix: PCM_XSMT input (from MCP GPA5)
        f'    (pin "PCM_XSMT" input (at 230 240 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_xsmt")}"))\n'
        # v0.7: JACK_DETECT output (J8 line-out jack switch → MCP GPA6)
        f'    (pin "JACK_DETECT" output (at 230 245 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("audiopin_jackdet")}")))\n'
        # ---- Sheet 7: Battery & Power-Path (NEU r9 + r12) ----
        # Platziert links unten (unter Power Tree), Verbindung via Label-Bridges
        f'  (sheet (at 30 180) (size 60 40) (fields_autoplaced)\n'
        f'    (stroke (width 0.1524) (type solid))\n'
        f'    (fill (color 0 0 0 0.0000))\n'
        f'    (uuid "{det_uuid("sheet_battery")}")\n'
        f'    (property "Sheetname" "Battery" (at 30 179 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left bottom)))\n'
        f'    (property "Sheetfile" "battery.kicad_sch" (at 30 220.5 0)\n'
        f'      (effects (font (size 1.27 1.27)) (justify left top)))\n'
        f'    (pin "VBUS_USBC" input (at 30 190 180)\n'
        f'      (effects (font (size 1.524 1.524)) (justify left))\n'
        f'      (uuid "{det_uuid("batpin_vbus_usbc")}"))\n'
        f'    (pin "BAT_PLUS" output (at 90 195 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("batpin_bat_plus")}"))\n'
        f'    (pin "+5V_OUT" output (at 90 200 0)\n'
        f'      (effects (font (size 1.524 1.524)) (justify right))\n'
        f'      (uuid "{det_uuid("batpin_5v_out")}")))\n'
        # ---- Inter-sheet labels for r9 + r12 (Battery / Sense / Power-Path) ----
        # VBUS_USBC (raw USB-C VBUS pre-fuse): Power-Tree → Battery + MCP
        f'  (wire (pts (xy 90 95) (xy 95 95)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_vbususbc_pt")}"))\n'
        f'  (label "VBUS_USBC" (at 95 95 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_vbususbc_pt")}"))\n'
        f'  (wire (pts (xy 25 190) (xy 30 190)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_vbususbc_bat")}"))\n'
        f'  (label "VBUS_USBC" (at 25 190 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_vbususbc_bat")}"))\n'
        f'  (wire (pts (xy 125 225) (xy 130 225)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_vbususbc_mcp")}"))\n'
        f'  (label "VBUS_USBC" (at 125 225 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_vbususbc_mcp")}"))\n'
        # BAT_PLUS: Battery → Pico (BAT_SENSE-Divider)
        f'  (wire (pts (xy 90 195) (xy 95 195)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_batplus_bat")}"))\n'
        f'  (label "BAT_PLUS" (at 95 195 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_batplus_bat")}"))\n'
        f'  (wire (pts (xy 125 165) (xy 130 165)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_batplus_pico")}"))\n'
        f'  (label "BAT_PLUS" (at 125 165 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_batplus_pico")}"))\n'
        # +5V_OUT: Battery merges into Power-Tree → Pico +5V rail. Beide Labels matchen am
        # gleichen Net-Name. Existing direct-wire (90,50)→(130,50) bekommt eine Label-Bridge.
        f'  (wire (pts (xy 90 200) (xy 95 200)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_5vout_bat")}"))\n'
        f'  (label "+5V_OUT" (at 95 200 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_5vout_bat")}"))\n'
        f'  (label "+5V_OUT" (at 110 50 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_5vout_pt")}"))\n'
        # ---- Labels für Audio: Pico AMP_nSHDN/MUTE → Audio Sheet inputs
        f'  (wire (pts (xy 125 95) (xy 130 95)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ampshdn_pico")}"))\n'
        f'  (label "AMP_nSHDN" (at 125 95 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_ampshdn_pico")}"))\n'
        f'  (wire (pts (xy 125 100) (xy 130 100)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ampmute_pico")}"))\n'
        f'  (label "AMP_nMUTE" (at 125 100 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_ampmute_pico")}"))\n'
        f'  (wire (pts (xy 225 230) (xy 230 230)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ampshdn_audio")}"))\n'
        f'  (label "AMP_nSHDN" (at 225 230 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_ampshdn_audio")}"))\n'
        f'  (wire (pts (xy 225 235) (xy 230 235)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_ampmute_audio")}"))\n'
        f'  (label "AMP_nMUTE" (at 225 235 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_ampmute_audio")}"))\n'
        # v0.6.3-r5 N1-Fix: PCM_XSMT label-bridge zwischen MCP-Sheet (rechte Seite, y=215) und Audio-Sheet (linke Seite, y=240)
        f'  (wire (pts (xy 190 215) (xy 195 215)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_xsmt_mcp")}"))\n'
        f'  (label "PCM_XSMT" (at 195 215 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_xsmt_mcp")}"))\n'
        f'  (wire (pts (xy 225 240) (xy 230 240)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_xsmt_audio")}"))\n'
        f'  (label "PCM_XSMT" (at 225 240 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_xsmt_audio")}"))\n'
        # v0.7: JACK_DETECT label-bridge zwischen Audio-Sheet (y=245) und MCP-Sheet (y=210)
        f'  (wire (pts (xy 190 210) (xy 195 210)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_jackdet_mcp")}"))\n'
        f'  (label "JACK_DETECT" (at 195 210 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_jackdet_mcp")}"))\n'
        f'  (wire (pts (xy 225 245) (xy 230 245)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_jackdet_audio")}"))\n'
        f'  (label "JACK_DETECT" (at 225 245 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_jackdet_audio")}"))\n'
        # ---- I2S labels: jetzt vom Pico (GP0/GP1/GP4 I²S-Master) getrieben,
        # via die gleichnamigen Labels an der Pico-Brücke oben (v0.9, Pi entfernt)
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
        f'  (wire (pts (xy 225 165) (xy 230 165)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_vol_sw")}"))\n'
        f'  (label "VOL_SW" (at 225 165 0) (effects (font (size 1.524 1.524)) (justify right bottom)) (uuid "{det_uuid("rootlbl_volsw_enc")}"))\n'
        f'  (wire (pts (xy 190 225) (xy 195 225)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_volsw_mcp")}"))\n'
        f'  (label "VOL_SW" (at 195 225 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_volsw_mcp")}"))\n'
        f'  (wire (pts (xy 190 220) (xy 195 220)) (stroke (width 0) (type default)) (uuid "{det_uuid("rootw_blk_mcp")}"))\n'
        f'  (label "LCD_BLK_PWM" (at 195 220 0) (effects (font (size 1.524 1.524)) (justify left bottom)) (uuid "{det_uuid("rootlbl_blk_mcp")}"))\n'

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
    (OUT_DIR / "stm32h743.kicad_sch").write_text(stm32h743_sheet())  # r18 H7
    (OUT_DIR / "lcd.kicad_sch").write_text(lcd_sheet())  # r16/r18 ST7789
    (OUT_DIR / "mcp.kicad_sch").write_text(mcp_sheet())
    (OUT_DIR / "encoder.kicad_sch").write_text(encoder_sheet())
    (OUT_DIR / "audio.kicad_sch").write_text(audio_sheet())
    (OUT_DIR / "battery.kicad_sch").write_text(battery_sheet())  # NEU r9 + r12
    # v0.9: Pi-Header-Sheet (pi.kicad_sch) entfernt — RP2350 ist Pi-frei.
    print(f"Wrote KiCad project (r18: STM32H743 + LCD) to {OUT_DIR}")


if __name__ == "__main__":
    main()
