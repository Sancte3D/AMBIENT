# Field Ambience — Start Here

Übersicht über das Projekt und wo was liegt.

## Mein nächster Schritt
→ **`MEINE_TODO.md`** — konkrete Checkliste (Footprints prüfen, ERC, Layout, Bestellung)

## PCB
- `field_ambience_pcb_SPEC_v0.6.md` — komplette, aktuelle Spec
- `kicad/` — KiCad-Projekt (Schaltplan fertig + validiert, generiert aus `generate_kicad_project.py`)
- `kicad/libraries/` — eingebundene kiswitch-Footprints (Choc V2)
- `mechanical_coordinates.md` — Platzierungs-Koordinaten fürs Layout
- `CHANGELOG.md` — Entscheidungs- und Änderungshistorie
- `PCB_TODO.md` — detaillierter Engineering-Status

## Audio Engine
- `field_ambience_v29o.scd` — SuperCollider Engine
- `field_ambience_bridge.py` — WS↔OSC Bridge zur UI
- `field_ambience_panel.html` — UI-Mockup

## Firmware (Pico)
- `firmware/` — MicroPython für den RP2350 (Buttons, Encoder, OLED, Amp-Power)

## Design-Philosophie & Roadmap
- `field_ambience_skill.md` — Sound Constitution / Architekturprinzipien
- `ROADMAP.md` — was als nächstes ansteht
