# Project Quality — Rating Card

**Stand: 2026-06-12 (v0.7-r18.14)**

> User-Vorgabe: Alles hat eine Skala 1–10, am Ende soll alles **10/10** sein.
> Dieses File ist der ehrliche, aktuelle Zwischenstand pro Aspekt.

## Score auf einen Blick

| Aspekt | Score | Trend |
|---|---|---|
| **Audio-Engine (Sound)** | 9.5 / 10 | ↑ (r18.11: Buffer-Underrun-Schutz + SAI-PLL-Architektur dokumentiert) |
| **Display-Sim Web-UI** | 7 / 10 | ↑ (von 5/10 vor IMG_9713-Redesign) |
| **Industrial Design (Render)** | 9 / 10 | ↑ |
| **Bench Bring-Up (Pico 2)** | 8 / 10 | → |
| **Schematic (Korrektheit)** | 9.5 / 10 | ↑ (r18.10: SW_BOOT + USB-C-Upgrade + SPEC-BOM-Sync) |
| **Hardware-Symbole** | 9.5 / 10 | ↑ (BOOT-Button + R_BOOT_SW ergänzt) |
| **Footprint-Verifikation** | 10 / 10 | ✅ (r18.14: 0 offene Punkte — EC11J-Blocker via Teil-Retire gelöst, TS-1088-FP EasyEDA-verifiziert) |
| **BOM-Sourcing** | 9 / 10 | ↑ (r18.14: EC11E + Hall-Sensor LCSC-IDs verifiziert + DRV5056-Pinout DS-bestätigt; offen: Gateron-LP, Polymer-Cap, MLCC-220µF/10V) |
| **PCB Layout** | 0 / 10 | — (existiert nicht) |
| **DRC / Manufacturing** | 0 / 10 | — (Layout-abhängig) |
| **Mechanical / Enclosure** | 7 / 10 | ↑ (r18.14: 3D-STEP-Lib für Z-/Panel-kritische Teile + MANIFEST mit Höhen-Tabelle) |
| **Cell-Mechanik (Piano-Feel)** | 7 / 10 | ↑ (r18.14/ADR-0013: Magnetic-Hall-Architektur final, Schematic umgestellt; offen: Plate-CAD, Sensor-Pinout-Verify, Muster) |
| **Speaker-Cover (Dust-Mesh)** | 3 / 10 | ↑ (ADR-0007 erstellt) |
| **LED-Logik (Cell + Modifier)** | 10 / 10 | ✅ (Schematic + Sim + ADR komplett) |
| **Doku / Onboarding** | 10 / 10 | ✅ (r18.13: alle Cross-Refs konsistent nach Phase-2-Moves) |
| **Test-Abdeckung (host)** | 9 / 10 | → |
| **Repo-Struktur** | 8 / 10 | ↑ (r18.13: Phase 2 done — Doc-Moves in `mechanical/`, `software/`, `archive/`; Phase 3-5 queued) |
| **CI / Auto-Validierung** | 8 / 10 | → |

**Gesamt-Manufacturing-Readiness: 6 / 10** (Gate 1.5 von 9). r18.14-Anstieg:
FP-Verify 9→10, BOM 8→8.5, Mechanical 6→7 (3D-Lib), Cell-Mechanik 5→7
(Magnetic-Hall final). Verbleibende Hauptlücken: Layout 0, DRC 0, Mesh 3.

## Was jeder Score bedeutet — und was auf 10 fehlt

### Audio-Engine — 9 / 10
- ✅ Läuft host-portabel, 12 Test-Suiten grün, 8-Min-Performance-Render hörbar
- ⏳ Fehlend für 10: STM32H743-Native-Build (Phase-Pfad-Migration, kein Acceptance-Gate mehr per ADR-0005)

### Display-Sim Web-UI — 7 / 10
- ✅ Mirror des `menu.c`, animiert, Acceleration, Shift-Modus, Backlight
- ✅ NEU (r18.8): IMG_9713-Render umgesetzt — kleines Display, 4 Encoder in Ecken, Modifier mit LEDs, 5 Cell-Pillen mit XOR-LEDs, Dust-Mesh-Speakers
- ⏳ Fehlend für 10: Velocity-Visualisierung der Cell-Pads (ADR-0006), Drone/Generate transient-Animationen, Battery-Akku-Simulation (USB-Stecker-Toggle entfernt — könnte zurückkommen)

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

### BOM-Sourcing — 8.5 / 10
- ✅ Alle Haupt-ICs verifiziert; r18.14: 2 kritische ID/MPN-Fixes (USB-C C283540, SW_BOOT TS-1088) via 3D-CAD-Abruf — der Abruf ist jetzt Teil der Verifikations-Routine
- ⏳ Fehlend für 10: EC11E-Varianten-LCSC/Mouser-IDs (ADR-0012), Hall-Sensor-ID + Pinout-Verify (ADR-0013), Gateron-LP-Switch/Stabilizer-Beschaffungskanal, Polymer-Cap-IDs (r18.12), Cell/Modifier-LED-Stock-Check, Mesh-Hersteller (ADR-0007)

### PCB Layout — 0 / 10
- ⏳ Existiert nicht. Pfad zu 10: Stack-Up → Placement → Routing → DRC → Gerber-Export
- Es lohnt sich erst nach r18.8-Schematic-Update (Velocity-Inputs + LED-Topologie)

### DRC / Manufacturing — 0 / 10
- ⏳ Layout-abhängig

### Mechanical / Enclosure — 7 / 10
- ✅ Z-Budget berechnet (ADR-0011), C_BULK-Konflikt gelöst (r18.12)
- ✅ r18.14: 3D-STEP-Lib für Z-/Panel-kritische Teile + `mechanical/3d_models/MANIFEST.md` mit Höhen-Tabelle — CAD-Abstimmung kann starten
- ⏳ Fehlend für 10: `mechanical_coordinates.md`-Rewrite (IMG_9713 + EC11E-Höhen + Plate), Enclosure-CAD, Speaker/LCD/Switch-CAD von extern holen

### Cell-Mechanik (Piano-Feel) — 7 / 10
- ✅ Architektur final (ADR-0013): Gateron-LP-Magnetic + Hall-Sensor, Velocity = dPos/dt, lange Caps + LP-Stabilizer (Spacebar-Prinzip)
- ✅ Schematic umgestellt (r18.14): 1×3-Hall-Sites + Serien-RC an unveränderten ADC-Pins
- ⏳ Fehlend für 10: Sensor-Pinout-Verify (DRV5056-DS), Plate-Cutout-CAD, Magnet-/Abstands-Messung am Muster, Velocity-Curve-Tuning auf realer Hardware

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

### Test-Abdeckung (host) — 9 / 10
- ✅ 12+ Suiten grün; Bench-Test mit 50+ Checks für Display-Logik
- ⏳ Fehlend für 10: Velocity-Sampling-Test (kommt mit Cell-Velocity-Implementation)

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
