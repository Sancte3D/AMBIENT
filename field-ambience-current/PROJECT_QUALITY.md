# Project Quality — Rating Card

**Stand: 2026-06-11 (v0.7-r18.9)**

> User-Vorgabe: Alles hat eine Skala 1–10, am Ende soll alles **10/10** sein.
> Dieses File ist der ehrliche, aktuelle Zwischenstand pro Aspekt.

## Score auf einen Blick

| Aspekt | Score | Trend |
|---|---|---|
| **Audio-Engine (Sound)** | 9 / 10 | → |
| **Display-Sim Web-UI** | 7 / 10 | ↑ (von 5/10 vor IMG_9713-Redesign) |
| **Industrial Design (Render)** | 9 / 10 | ↑ |
| **Bench Bring-Up (Pico 2)** | 8 / 10 | → |
| **Schematic (Korrektheit)** | 9 / 10 | ↑ (r18.9: Cell-ADC + LED-Topologie implementiert) |
| **Hardware-Symbole** | 9 / 10 | → |
| **Footprint-Verifikation** | 8 / 10 | ↑ (6 von 9 closed in r18.7) |
| **BOM-Sourcing** | 7.5 / 10 | ↑ (Choc raus, FSR-Interface rein; FSR-MPN + 2 LED-Farben VERIFY) |
| **PCB Layout** | 0 / 10 | — (existiert nicht) |
| **DRC / Manufacturing** | 0 / 10 | — (Layout-abhängig) |
| **Mechanical / Enclosure** | 2 / 10 | → (Pico-Ära-Koordinaten) |
| **Cell-Mechanik (Piano-Feel)** | 5 / 10 | ↑ (Schematic-Seite done; FSR-Wahl + Silicon-Cap offen) |
| **Speaker-Cover (Dust-Mesh)** | 3 / 10 | ↑ (ADR-0007 erstellt) |
| **LED-Logik (Cell + Modifier)** | 10 / 10 | ✅ (Schematic + Sim + ADR komplett) |
| **Doku / Onboarding** | 9 / 10 | → |
| **Test-Abdeckung (host)** | 9 / 10 | → |
| **Repo-Struktur** | 6 / 10 | ↑ (Phase 1 done, 2-5 queued) |
| **CI / Auto-Validierung** | 8 / 10 | → |

**Gesamt-Manufacturing-Readiness: 5.5 / 10** (Gate 1.5 von 9).

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

### Footprint-Verifikation — 8 / 10
- ✅ 6 von 9 closed, 3 Custom-FPs nach Hersteller-Daten in `libraries/field_ambience.pretty/`
- ⏳ Fehlend für 10: EC11J Custom-FP (ALPS-Drawing nicht öffentlich; aus EasyEDA-Daten ableiten)

### BOM-Sourcing — 7 / 10
- ✅ Alle Haupt-ICs verifiziert, LCSC-Nummern auf JLC-Stock geprüft, Custom-FPs für Edge-Cases
- ⏳ Fehlend für 10: 5× FSR-Auswahl (ADR-0006), 4× Modifier-Standard-LEDs (3 Farben), 10× Cell-LEDs Stock-Check, Mesh-Hersteller-Entscheidung (ADR-0007)

### PCB Layout — 0 / 10
- ⏳ Existiert nicht. Pfad zu 10: Stack-Up → Placement → Routing → DRC → Gerber-Export
- Es lohnt sich erst nach r18.8-Schematic-Update (Velocity-Inputs + LED-Topologie)

### DRC / Manufacturing — 0 / 10
- ⏳ Layout-abhängig

### Mechanical / Enclosure — 2 / 10
- ✅ `mechanical_coordinates.md` existiert
- ⏳ Fehlend für 10: Update auf STM32-LQFP-100 + neue Komponenten (Cell-Velocity-Pads, Dust-Mesh-Aussparungen, 4-Corner-Encoder-Positionen). Enclosure-CAD existiert nicht

### Cell-Mechanik (Piano-Feel) — 3 / 10
- ✅ Entscheidung dokumentiert (ADR-0006)
- ⏳ Fehlend für 10: FSR-Komponenten-Review, Silicon-Cap-Tooling-Design, ADC-Pin-Allocation im Schematic (r18.8), Velocity-Curve-Tuning auf realer Hardware

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

### Repo-Struktur — 6 / 10
- ✅ Phase 1 done (Navigation, Onboarding, ADRs)
- ⏳ Fehlend für 10: Phasen 2 (Doc-Moves) + 3 (Firmware-Split atomar mit CI) + 4 (KiCad-Reorg) + 5 (Manufacturing-Scaffold sobald `.kicad_pcb` existiert)

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
5. **Repo-Refactor Phasen 2–5** — Repo 6 → 10
6. **Firmware-Migration STM32H743** parallel — Audio 9 → 10
7. **Prototype-Spin** — Bring-Up 8 → 10
