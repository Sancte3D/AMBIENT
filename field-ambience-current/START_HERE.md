# Field Ambience — Start Here

Übersicht über das Projekt und wo was liegt. **Aktueller Stand:** Audio-Engine
+ UI laufen nativ in C auf dem RP2350 (Pico 2). Steps 1–11 + 12a des Native-
Ports sind fertig; Step 12b (Menü + USB-MIDI) steht aus.

## Mein nächster Schritt
→ **`firmware-c/HOERTEST.md`** bzw. **`firmware-c/HOERTEST.html`** — On-Device-
  Hörtest mit Pico 2 + DAC + optional Tasten, Schritt für Schritt.
→ **`MEINE_TODO.md`** — Engineering-Checkliste (Footprints, ERC, Layout,
  Bestellung) für die finale Platine.

## Firmware (RP2350 nativ)
- `firmware-c/` — **Hörtest-Snapshot** (Steps 1–11 + 12a, eingefroren) für den
  On-Device-Test. Build-/Test-Anleitung in `firmware-c/README.md`.
- `firmware-c/hoertest/HOERTEST.md` + `.html` — Aufbau-Anleitung für den
  Hörtest (Pico + DAC + optional MCP/Tasten).
- `firmware-c-next/` — **aktive Entwicklung** (Step 12b: Reverb-Presets,
  Drone, Live-Parameter-Regel, TRS-MIDI Out, OLED-Menü, Encoder-Bindings).
- `firmware-c/test/run_tests.sh` und `firmware-c-next/test/run_tests.sh` —
  Host-Unit-Tests (kein Pico-SDK nötig).
- `NATIVE_PORT_PLAN.md` — Steps + aktueller Fortschritt.

## Audio-Engine-Referenzen (nicht-Firmware-Quellen, aktiv genutzt)
- `field_ambience_v29o.scd` — kanonische SuperCollider-Quelle (Cross-Check)
- `field_ambience_webapp.html` — Web-Audio-Port, war Port-Vorlage für `firmware-c/`

## PCB
- `field_ambience_pcb_SPEC_v0.6.md` — komplette, aktuelle Spec
- `kicad/` — KiCad-Projekt (Schaltplan fertig + validiert, generiert aus
  `generate_kicad_project.py`)
- `kicad/libraries/` — eingebundene kiswitch-Footprints (Choc V2)
- `kicad/datasheets/` — Datasheets der verbauten Bauteile
- `mechanical_coordinates.md` — Platzierungs-Koordinaten fürs Layout
- `CHANGELOG.md` — Entscheidungs- und Änderungshistorie
- `PCB_TODO.md` — detaillierter Engineering-Status

## Pre-Step-6 (archiviert)
- `legacy/` — die alte Pi+SuperCollider+Browser-Architektur. Inhalte sind
  obsolet aber zur Nachvollziehbarkeit eingefroren. Siehe `legacy/README.md`.

## Design-Philosophie
- `PITCH.md` / `ROADMAP.md` — beide tragen einen Stand-Hinweis: textuell aus
  der Pre-Step-6-Phase, Klang-Vision (Sound-Constitution-Charakter) gilt
  weiter, technische Details darin sind überholt.
