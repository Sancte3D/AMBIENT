# Project Quality — Rating Card

**Stand: 2026-06-13 (v0.7-r18.16)**

> User-Vorgabe: Alles hat eine Skala 1–10, am Ende soll alles **10/10** sein.
> Dieses File ist der ehrliche, aktuelle Zwischenstand pro Aspekt.

## Score auf einen Blick

| Aspekt | Score | Trend |
|---|---|---|
| **Audio-Engine (Sound)** | 9.5 / 10 | ↑ (r18.11: Buffer-Underrun-Schutz + SAI-PLL-Architektur dokumentiert) |
| **Display-Sim Web-UI** | 8 / 10 | ↑ (r18.15: Cell-Velocity sichtbar — Fill + 0–127-Readout, Firmware-Kurve gespiegelt) |
| **Industrial Design (Render)** | 9 / 10 | ↑ |
| **Bench Bring-Up (Pico 2)** | 8 / 10 | → |
| **Schematic (Korrektheit)** | 9.5 / 10 | ↑ (r18.10: SW_BOOT + USB-C-Upgrade + SPEC-BOM-Sync) |
| **Hardware-Symbole** | 9.5 / 10 | ↑ (BOOT-Button + R_BOOT_SW ergänzt) |
| **Footprint-Verifikation** | 10 / 10 | ✅ (r18.14: 0 offene Punkte — EC11J-Blocker via Teil-Retire gelöst, TS-1088-FP EasyEDA-verifiziert) |
| **BOM-Sourcing** | 9 / 10 | ↑ (r18.14: EC11E + Hall-Sensor LCSC-IDs verifiziert + DRV5056-Pinout DS-bestätigt; offen: Gateron-LP, Polymer-Cap, MLCC-220µF/10V) |
| **PCB Layout** | 0 / 10 | — (existiert nicht) |
| **DRC / Manufacturing** | 0 / 10 | — (Layout-abhängig) |
| **Mechanical / Enclosure** | 9 / 10 | ↑ (r18.16: `mechanical_coordinates.md` echt + geometrisch validiert, Power-Insel verortet, Speaker-Höhen-Constraint) |
| **Cell-Mechanik (Piano-Feel)** | 8 / 10 | ↑ (r18.15: Velocity-Modell in Firmware implementiert + host-getestet + Engine-Pfad + Sim; offen: Plate-CAD, Muster-Tuning) |
| **Speaker-Cover (Dust-Mesh)** | 3 / 10 | ↑ (ADR-0007 erstellt) |
| **LED-Logik (Cell + Modifier)** | 10 / 10 | ✅ (Schematic + Sim + ADR komplett) |
| **Doku / Onboarding** | 10 / 10 | ✅ (r18.13: alle Cross-Refs konsistent nach Phase-2-Moves) |
| **Test-Abdeckung (host)** | 9.5 / 10 | ↑ (r18.15: 13. Suite test_cells.c + Engine-Velocity-Check) |
| **Repo-Struktur** | 8 / 10 | ↑ (r18.13: Phase 2 done — Doc-Moves in `mechanical/`, `software/`, `archive/`; Phase 3-5 queued) |
| **CI / Auto-Validierung** | 8 / 10 | → |

**Gesamt-Manufacturing-Readiness: 6.5 / 10** (Gate 1.5 von 9). r18.16: echte
Mechanik-Koordinaten (Mechanical 7→9). Verbleibende Hauptlücken bleiben
hardware-/layout-seitig: Layout 0, DRC 0, Mesh 3.

## Was jeder Score bedeutet — und was auf 10 fehlt

### Audio-Engine — 9 / 10
- ✅ Läuft host-portabel, 13 Test-Suiten grün, 8-Min-Performance-Render hörbar
- ⏳ Fehlend für 10: STM32H743-Native-Build (Phase-Pfad-Migration, kein Acceptance-Gate mehr per ADR-0005)

### Display-Sim Web-UI — 8 / 10
- ✅ Mirror des `menu.c`, animiert, Acceleration, Shift-Modus, Backlight
- ✅ IMG_9713-Render (r18.8) + Cell-Velocity-Anzeige (r18.15: grüner Fill + 0–127-Readout, exakt die `cells.h`-Kurve)
- ⏳ Fehlend für 10: Drone/Generate transient-Animationen, Battery-Akku-Simulation

### Industrial Design (Render) — 9 / 10
- ✅ IMG_9713-Stand ist klar, kohärent, OP-1-Field-Sprache konsequent
- ⏳ Fehlend für 10: physisches Mockup; Material-Decision Aluminum vs ABS vs Polycarbonate

### Bench Bring-Up — 8 / 10
- ✅ Pico 2 + ST7789 + Encoder + Button real, läuft, Animationen + Backlight + SHIFT-Modus funktional
- ⏳ Fehlend für 10: Velocity-Cells nicht testbar auf Bench (braucht FSR-Hardware)

### Schematic (Korrektheit) — 8 / 10
- ✅ 100/100 STM32-Pins angebunden, Hier↔Root 7/7 PASS, Symbol gegen DS verifiziert (52/52)
- ✅ TPS61089-Symbol-Bug (Phantom-Pin-12) korrigiert in r18.7
- ⏳ Fehlend für 10: GUI-ERC-Lauf in KiCad 9; Cell-Velocity-Inputs (ADR-0006, kommt in r18.8)

### Hardware-Symbole — 9 / 10
- ✅ Alle Custom-Symbole nach offiziellen Datasheets gebaut
- ⏳ Fehlend für 10: User-Velocity-Sense-FSR-Symbol noch nicht (r18.8)

### Footprint-Verifikation — 10 / 10 ✅
- ✅ Alle Punkte closed: 5 Custom-FPs (2 davon EasyEDA-CAD-verifiziert in r18.14), Rest KiCad-Standard mit Quelle
- ✅ EC11J-Blocker gegenstandslos (Teil retired, ADR-0012); echtes Pattern liegt als Referenz in der Lib
- Haltebedingung: neue Bauteile nur noch mit CAD-Export-FP oder Standard-Lib aufnehmen

### BOM-Sourcing — 9 / 10
- ✅ Alle Haupt-ICs verifiziert; r18.14: 2 kritische ID/MPN-Fixes (USB-C C283540, SW_BOOT TS-1088) via 3D-CAD-Abruf + EC11E (C202365/C370986) + Hall-Sensor (DRV5056 C2152902) LCSC-IDs + DRV5056-Pinout DS-bestätigt
- ⏳ Fehlend für 10: Gateron-LP-Switch/Stabilizer-Beschaffungskanal (Keyboard-Markt, kein LCSC), Polymer-Cap-IDs (r18.12), 220µF/10V-MLCC, Cell/Modifier-LED-Stock-Check, Mesh-Hersteller (ADR-0007)

### PCB Layout — 0 / 10
- ⏳ Existiert nicht. Pfad zu 10: Stack-Up → Placement → Routing → DRC → Gerber-Export
- Es lohnt sich erst nach r18.8-Schematic-Update (Velocity-Inputs + LED-Topologie)

### DRC / Manufacturing — 0 / 10
- ⏳ Layout-abhängig

### Mechanical / Enclosure — 9 / 10
- ✅ Z-Budget berechnet (ADR-0011), C_BULK-Konflikt gelöst (r18.12)
- ✅ r18.14: 3D-STEP-Lib für Z-/Panel-kritische Teile + `mechanical/3d_models/MANIFEST.md` mit Höhen-Tabelle
- ✅ r18.16: `mechanical_coordinates.md` echt — Outline 252×102, alle X/Y/Z, Power-/Audio-Insel im 17-mm-Y-Streifen, Speaker-Treiber-Zone-Höhen-Constraint, geometrisch validiert (Python-Sanity-Check, 0 Konflikte)
- ⏳ Fehlend für 10: Enclosure-CAD (FreeCAD/Fusion-Modell), physisches Mockup, Knopf-CAD

### Cell-Mechanik (Piano-Feel) — 8 / 10
- ✅ Architektur final (ADR-0013): Gateron-LP-Magnetic + Hall-Sensor, Velocity = Banddurchlaufzeit, lange Caps + LP-Stabilizer (Spacebar-Prinzip)
- ✅ Schematic (r18.14) + Sensor-Pinout DS-verifiziert (DRV5056 C2152902)
- ✅ Velocity-Modell IMPLEMENTIERT (r18.15): `cells.{h,c}` + Engine-Pfad + 25-Check-Test + Sim-Spiegelung
- ⏳ Fehlend für 10: Plate-Cutout-CAD, Magnet-/Abstands-Messung am Muster, Velocity-Curve-Feintuning auf realer Hardware

### Speaker-Cover (Dust-Mesh) — 3 / 10
- ✅ Entscheidung dokumentiert (ADR-0007)
- ⏳ Fehlend für 10: Mesh-Hersteller-Auswahl, Akustik-Charakterisierung, Tooling-Design

### LED-Logik (Cell + Modifier) — 9 / 10
- ✅ XOR-Entscheidung dokumentiert (ADR-0008)
- ✅ Web-Sim implementiert die Logik
- ✅ PCA9685-Budget passt exakt (16/16 Kanäle)
- ⏳ Fehlend für 10: Schematic-Update auf 5×2 Cell-LEDs (r18.8); Firmware-State-Machine

### Doku / Onboarding — 9 / 10
- ✅ Real-README, PROJECT_MAP, PROJECT_STATUS, REPO_STRUCTURE, CONTRIBUTING, 4 Onboarding-Docs, 8 ADRs, ERC/DRC-Checklist, FP_VERIFY-Log
- ⏳ Fehlend für 10: SPEC v0.7 ist auf r18.7 — r18.8 muss alle Cell/LED/Speaker-Updates reflektieren

### Test-Abdeckung (host) — 9.5 / 10
- ✅ 13 Suiten grün; Bench-Test mit 50+ Checks; r18.15: `test_cells.c` (25 Checks) + End-to-End-Velocity-Check in der Engine-Suite
- ⏳ Fehlend für 10: ADC-Normalisierungs-/Driver-Layer-Test (kommt mit STM32-HAL)

### Repo-Struktur — 8 / 10
- ✅ Phase 1 done (Navigation, Onboarding, ADRs)
- ✅ Phase 2 done (r18.13: Doc-Moves nach `mechanical/`, `software/`, `archive/`)
- ⏳ Fehlend für 10: Phasen 3 (Firmware-Split atomar mit CI) + 4 (KiCad-Reorg) + 5 (Manufacturing-Scaffold sobald `.kicad_pcb` existiert)

### CI / Auto-Validierung — 8 / 10
- ✅ firmware-c-Pipeline grün, Bench-Test in CI, Pages-Pipeline für Web-Sim
- ⏳ Fehlend für 10: kicad-cli-CI-Stage (sobald in der Umgebung verfügbar) für headless-ERC

## Reihenfolge zum 10/10-Gesamt

1. **r18.8 Schematic-Update** (Cell-FSR + LED-Topologie + SPEC) — viele Punkte
   gehen gleichzeitig auf 9
2. **Mechanical Coordinates Update** für STM32 + Cells + Dust-Mesh — Mechanical
   3 → 7
3. **Enclosure-CAD-Start** (separates Industrial-Designer-Arbeitspaket) — IDE
   9 → 10, Mechanical 7 → 10
4. **PCB Layout** (Phase 6, jetzt ohne Gate-Wartezeit) — Layout 0 → 8, DRC
   0 → 8
5. **Repo-Refactor Phasen 3–5** — Repo 8 → 10 (Phase 2 done in r18.13)
6. **Firmware-Migration STM32H743** parallel — Audio 9 → 10
7. **Prototype-Spin** — Bring-Up 8 → 10
