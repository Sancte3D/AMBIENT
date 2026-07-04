# Hardware — Echte Übersicht (Engineer Start)

> **Eine Seite, aktueller Stand (r18.65).** Quelle der Wahrheit ist der
> Generator `kicad/generate_kicad_project.py`. Diese Seite **verweist** auf die
> kanonischen Detail-Docs — sie dupliziert sie nicht. Wenn etwas widerspricht:
> Generator gewinnt, dann regenerieren.

## Was es ist
Handheld Ambient-Instrument. MCU **STM32H743VIT6** (LQFP-100). Schaltplan ist
generator-basiert: 8 hierarchische Sheets (power_tree, stm32h743, lcd, mcp,
encoder, audio, battery + root), **100/100 Pins verdrahtet**, hier↔root-Crossref
clean. **Es existiert noch KEIN `.kicad_pcb`** — Layout ist der nächste große
Schritt.

## Module + Komponenten (kein Doppel — Detail in BOM_MASTER)

| Modul | Kernteile | Anz. | Detail |
|---|---|---|---|
| MCU + Clock | STM32H743VIT6 + 8 MHz Crystal | — | [BOM §1](../../../BOM_MASTER.md) |
| Power | USB-C, LiPo→Boost (TPS61089), LDO (AP7361C), Charger (MCP73831) | — | [BOM §2](../../../BOM_MASTER.md) |
| Audio | PCM5102A DAC → PAM8403 Amp → 2 Speaker + PJ-320D Line-out | 2 Spk | [BOM §3](../../../BOM_MASTER.md) |
| I/O + LED | MCP23017 (GPIO) + PCA9685 U6 (PWM) → 15 Mono-Status-LEDs | 15 LED | [BOM §4/§9](../../../BOM_MASTER.md) |
| Display | Waveshare 1.9″ ST7789 (Steckmodul, 8-Pin J3) | 1 | [BOM §5](../../../BOM_MASTER.md) |
| **Encoder** | **4× ALPS EC11E18244AU — alle Push-Encoder.** Alle 4 Push-Switches verdrahtet: DISPLAY (PE3), VOL (MCP-GPB5), DRIVE (PE0), BRIGHT (PE1) | 4 | [BOM §6](../../../BOM_MASTER.md) · [ADR-0012](../decisions/ADR-0012-encoder-strategy.md) |
| Cells | 5× Kailh Choc V1, direkt gelötet (SW1–5, CPG135001D01, LCSC C400229), **digital** auf MCP23017 GPA0–4, echter Keyswitch (~3mm Hub) (r18.75, ADR-0013 abgelöst) | 5 | [BOM §7](../../../BOM_MASTER.md) |
| Buttons | 5× HX B3F-4055 Modifier (SW6–10, bewusst kleineres/simpleres Gefühl als die Cells) + 2× TS-1088 Service (Reset/BOOT) | 7 | [BOM §8](../../../BOM_MASTER.md) |

> Footprints **komplett** (6 Custom + Rest KiCad-Standard, keine Leichen). 3D
> für Enclosure-CAD: 7 STEP im Repo + Rest Standard-Lib + **1 dokumentierte
> Lücke** (HX-Modifier-Taster, Envelope 11,8×11,8×7,3) + Off-Board-Bodies.

## Die 4 echten Referenzen (alles andere ist Detail/History)
- **Jeder Pin / jedes Netz, pro Modul** → [`docs/hardware/PINMAP.md`](../hardware/PINMAP.md)
- **3D / Mechanik / Gehäuse-Maße** → [`mechanical/3d_models/MANIFEST.md`](../../../mechanical/3d_models/MANIFEST.md) + [`docs/hardware/MECHANICAL_REQUIREMENTS.md`](../hardware/MECHANICAL_REQUIREMENTS.md)
- **Bestell-BOM** → [`BOM_MASTER.md`](../../../BOM_MASTER.md) + [`kicad/jlc_bom.csv`](../../kicad/jlc_bom.csv) (57 LCSC-Teile, generiert via `kicad/export_jlc_bom.py`)
- **Was blockt vor Fab** → [`PCB_LAYOUT_STATUS.md`](../../PCB_LAYOUT_STATUS.md)

## Die echten offenen Blocker (Reihenfolge)
1. **KiCad-GUI-ERC** — `kicad-cli` ist nicht in dieser Umgebung; einmal in
   KiCad 9 öffnen, ERC laufen lassen, 0 Errors (erlaubte Warnungen:
   [`ERC_DRC_CHECKLIST.md`](../hardware/ERC_DRC_CHECKLIST.md)).
2. **PCB-Layout + Routing** — greenfield, kein `.kicad_pcb`.
3. **PCB-Außenmaße + Mechanik-Koordinaten** — hängt am Gehäuse-CAD.
4. **Gerber + CPL** Export → JLCPCB.

> ⚠️ **Beim (Neu-)Aufbau des Schaltbilds 2 Dinge nicht vergessen** — sie stehen
> in BOM + ADR, sind aber **noch nicht im Generator** (`PCB_LAYOUT_STATUS.md`
> SCH-1/SCH-2): **(a)** Power-Aus `U_PWR`/`SW_PWR` (ADR-0016, BOM §2); **(b)** für
> die Modifier-Buttons einen **THT**-12×12-Footprint (Teil C2845240 ist THT, der
> Generator-FP ist noch SMD). MIDI (J10) ist schon im Schaltbild.

Schritt-für-Schritt-Anleitung fürs Layout: [`KICAD_BLUEPRINT.md`](../hardware/KICAD_BLUEPRINT.md).

## Generator-Workflow
```bash
cd field-ambience-current/kicad
python3 generate_kicad_project.py     # → 8 .kicad_sch + .kicad_pro
```
**Nie** `.kicad_sch` von Hand editieren — immer den Generator ändern und
re-generieren, dann den Diff prüfen. Custom-Footprints: 6 in
`kicad/libraries/field_ambience.pretty/`. Skript-Validierung (kein GUI-ERC):
paren-balance, 100/100-Pin-Connectivity, hier↔root-Crossref.

## Detail-/Arbeits-Layer (für tiefer — nicht nötig für die Übersicht)
- [`field_ambience_pcb_SPEC_v0.7.md`](../../field_ambience_pcb_SPEC_v0.7.md) — Design-Rationale + Power-Budget + Pin-Allocation §5
- [`docs/hardware/SCHEMATIC_WALKTHROUGH.md`](../hardware/SCHEMATIC_WALKTHROUGH.md) — Sheet-für-Sheet-Prosa
- [`CHANGELOG.md`](../../CHANGELOG.md) — Entscheidungs-/Änderungshistorie
- [`docs/decisions/`](../decisions/) — 20 ADRs (verworfene sind als SUPERSEDED markiert)
- [`archive/PCB_TODO_historical.md`](../../../archive/PCB_TODO_historical.md) — alte Pre-H743-Issues (nur History)
