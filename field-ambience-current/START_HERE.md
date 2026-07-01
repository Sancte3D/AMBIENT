# Field Ambience — Start Here

Übersicht über das Projekt und wo was liegt. **Aktueller Stand:** Audio-Engine
+ UI laufen nativ in C, Steps 1–11 + 12a + 12b fertig.

> ⚠️ **MCU-Migration aktiv: Pico 2 (RP2350) → STM32H743VIT6 Bare-Chip.**
> SPEC v0.7-r18, siehe `NATIVE_PORT_PLAN.md` Step 13 + `CHANGELOG.md` r18.
> Phase 1 (Doku) ✓, Phase 2–5 ausstehend. **PCB-Layout darf parallel zur Firmware starten**
> das fruehere Phase-5-Profiling-Gate ist per ADR-0005 (r18.7) GESTRICHEN. Echte Gates vor Layout: GUI-ERC + Pinmap/Power-Konsistenz (siehe PCB_LAYOUT_STATUS.md).
> Pico-Stand bleibt im Repo als Fallback unter `kicad/legacy_pico2/` (nach
> Phase 3).

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
- `../software/supercollider_reference/field_ambience_v29o.scd` — kanonische SuperCollider-Quelle (Cross-Check)
- `../software/webapp/field_ambience_webapp.html` — Web-Audio-Port, war Port-Vorlage für `firmware-c/`

## PCB
- `field_ambience_pcb_SPEC_v0.7.md` — komplette, aktuelle Spec (v0.7-r18, H7-Migration)
- `kicad/` — KiCad-Projekt (Schaltplan fertig + validiert, generiert aus
  `generate_kicad_project.py`)
- `kicad/libraries/` — Custom-Footprints (`field_ambience.pretty`, inkl. der
  Kailh-Choc-V1-Cell-Keyswitch-FP von LCSC/EasyEDA) + 3D-STEPs.
  (Cells sind seit r18.73 **digital am MCP23017**; seit r18.75 mit einem
  **echten Kailh-Choc-V1-Keyswitch, direkt gelötet** (kein Hotswap-Socket
  mehr, r18.74 hatte einen versucht) statt des kleinen Modifier-Tactile
  — bewusster UX-Unterschied. ADR-0013 Gateron-Magnetic+Hall abgelöst.)
- `kicad/datasheets/` — Datasheets der verbauten Bauteile
- `../mechanical/coordinates/mechanical_coordinates.md` — Platzierungs-Koordinaten fürs Layout
- `CHANGELOG.md` — Entscheidungs- und Änderungshistorie
- `PCB_LAYOUT_STATUS.md` — aktuelle Blocker · `docs/onboarding/HARDWARE_ENGINEER_START.md` — echte HW-Übersicht
  (alter `PCB_TODO.md` → `../archive/PCB_TODO_historical.md`, nur History)

## Pre-Step-6 (archiviert)
- `../archive/legacy_pre_native/` — die alte Pi+SuperCollider+Browser-Architektur. Inhalte sind
  obsolet aber zur Nachvollziehbarkeit eingefroren. Siehe `../archive/legacy_pre_native/README.md`.

## Design-Philosophie
- `../archive/old_specs/pitch_pre_step6.md` / `../archive/old_specs/roadmap_pre_step6.md` —
  beide aus der Pi-Phase, **archiviert in v0.7-r18**. Klang-Vision (Sound-
  Constitution) gilt weiter, technische Hardware-Details sind überholt.
  Ein neuer Pitch wird nach abgeschlossener H7-Migration geschrieben.
