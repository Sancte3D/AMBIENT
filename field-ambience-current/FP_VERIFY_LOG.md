# Footprint-Verifikations-Log

**Stand: r18.7 (2026-06-11)**

Stand der `FP_VERIFY`/`FP_NOTE`-Properties an den im Schematic platzierten Symbolen. Die Quelle für jeden Eintrag ist im Generator-Code dokumentiert; bei Custom-Footprints liegt das `.kicad_mod` in `kicad/libraries/field_ambience.pretty/`.

## Zusammenfassung

| Status | Anzahl |
|---|---|
| ✅ Verifiziert (KiCad-Standard, Source quoted) | 4 |
| ✅ Custom-FP nach Hersteller-Daten erstellt | 3 |
| 🟡 Offen (echte FP-Verify nötig) | 1 |

## Verifikations-Details

### ✅ U1 STM32H743VIT6 — LQFP-100
- **FP:** `Package_QFP:LQFP-100_14x14mm_P0.5mm` (KiCad-Standard)
- **Quelle:** kicad-footprints@master Package_QFP.pretty/LQFP-100_14x14mm_P0.5mm.kicad_mod
- **Verifiziert:** 100 Pads, Side-Pads 1.6 × 0.3 mm @ 0.5 mm Pitch, JEDEC MS-026 / IPC-7351-konform. Pin 1 (-7.675, -6); Pin 26 (-6, 7.675). Courtyard ±8.73 mm.
- **2026-06-11**: über gitlab.com/kicad/libraries/kicad-footprints offizieller Raw-Inhalt geprüft.

### ✅ U5 AP7361C-33Y5-13 — SOT-89-5
- **FP:** `Package_TO_SOT_SMD:SOT-89-5` (KiCad-Standard)
- **Quelle:** kicad-footprints@master Package_TO_SOT_SMD.pretty/SOT-89-5.kicad_mod
- **Verifiziert:** 5 Pads, Pin 2 (Tab, 0.8 × 2 mm @ origin), Pins 1/3/4/5 (1.5 × 0.7 mm @ ±1.85 X, ±1.5 Y). JEDEC TO-243.
- **Pinout (vom User aus Diodes-DS bestätigt):** 1=EN, 2=GND/Tab, 3=ADJ/NC, 4=IN, 5=OUT. Symbol matched.
- **2026-06-11**: offizieller Raw-Inhalt geprüft.

### ✅ U8 TPS61089RNR — Texas RNR0011A VQFN-HR-11
- **FP:** `field_ambience:Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A` (Custom)
- **Quelle:** TI Mechanical Drawing 4222143/A 08/2015 (eingebettet in TPS61089 Rev C Datasheet)
- **Custom-FP-Daten:**
  - 8× Side-Pads 0.25 × 0.55 mm @ 0.5 mm Pitch (Pins 1-4, 7-10)
  - 2× Enlarged EP-Side-Pads 0.55 × 0.7 mm (Pins 5 GND, 6 VOUT)
  - 1× Central Thermal Pad 1.5 × 0.7 mm (Pin 11 SW — **kein separates ePAD**, das war ein Symbol-Bug aus r12-B11; in r18.7 korrigiert)
- **2026-06-11**: TI-PDF-Stream-Extraktion (PACKAGE OUTLINE + LAND PATTERN EXAMPLE)
- **Hinweis:** Pin-1-Index-Orientierung im Custom-FP folgt TI-Standard (upper-left); vor Layout in KiCad-GUI visuell gegen Datasheet Pin Configuration Diagram prüfen.

### ✅ Y1 ABLS-8.000MHZ-B4-T — HC-49/US SMD
- **FP:** `field_ambience:Crystal_HC49-US-SMD_ABLS` (Custom)
- **Quelle:** ABRACON ABLS Datasheet Drawing 450669 Rev AD Page 3
- **Custom-FP-Daten:** 2× Pads 5.6 × 2.1 mm @ 9.5 mm Pitch (ABRACON-Empfehlung; KiCad-Standard `Crystal_SMD_HC49-SD` hat nur 4.5×2.0 mm @ 8.5 mm)
- **Body:** 11.4 × 4.7 mm
- **2026-06-11**: Custom-FP nach ABLS-Spec erstellt. KiCad-Standard war konservativer, hätte funktioniert, aber unter ABRACON-Empfehlung.

### ✅ SW6-SW10 HX 12×12×7.3 TPFT-B — SMD-4P
- **FP:** `field_ambience:SW_HX_12x12x7.3_SMD-4P` (Custom)
- **Quelle:** LCSC C36498966 / EasyEDA-User-verifizierte Datenblätter
- **Custom-FP-Daten:** 4× Pads 2.5 × 1.5 mm auf 7 mm-Pitch (X+Y), SPST (gegenüberliegende Pads verbunden 1↔1, 2↔2)
- **Body:** 11.8 × 11.8 mm, Höhe 7.3 mm
- **2026-06-11**: nach LCSC-Datenblatt erstellt (r18.6).

### ✅ J3 ST7789 LCD-Header — PinHeader_1×08 2.54 mm
- **FP:** `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` (KiCad-Standard)
- **Verifiziert:** trivialer Standard, alle 2.54 mm-Single-Row-Header sind identisch. Adafruit 5394 (ST7789 1.9″) hat 2.54 mm-Pinraster auf 8 Pins (Vin/GND/SCK/MOSI/TCS/DC/RES/Lite — alle anderen Pins sind über separaten SD-Header).

### ✅ Q2 2N7002 — SOT-23
- **FP:** `Package_TO_SOT_SMD:SOT-23` (KiCad-Standard)
- **Pinout (JEDEC TO-236):** Pin 1=G, Pin 2=S, Pin 3=D — alle Hersteller-Marken (Nexperia, Diodes, On Semi) identisch. Verbaut: Nexperia 2N7002,215 / LCSC C8545.

### 🟡 EN1-EN4 EC11J1525402 — ALPS SMD Rotary Encoder
- **FP:** `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` (KiCad-Standard, **NICHT exakt passend**)
- **Problem:** KiCad-Standard ist EC11**E** (THT-Stems). Verbaut: EC11**J** (SMD-Bauform). Land-Pattern unterscheidet sich (KiCad ist THT-bohrungsbasiert; SMD ist Surface-Pads).
- **Quelle für Auflösung:** ALPS-Drawing für EC11J1525402 — nicht öffentlich verfügbar. Alternative: JLCPCB-EasyEDA-Drawing für C209762 herunterladen und Custom-FP erstellen.
- **Status:** **Echter offener Blocker.** Empfehlung: Wenn JLCPCB-EasyEDA-Drawing in der KiCad-GUI geöffnet wird, Custom-FP erstellen nach demselben Muster wie SW6-10 (HX 12×12) und Crystal_HC49-US-SMD_ABLS.
- **Bewertung:** Production-Risk auch ohne Custom-FP — ALPS markiert EC11J1525402 als NRND ("Not Recommended for New Designs"). Für Serie sowieso Ersatztyp wählen.

## Was im Code geändert wurde

In `generate_kicad_project.py` werden `FP_VERIFY`-Properties durch `FP_NOTE`-Properties ersetzt, sobald die Verifikation positiv ist. Im Schematic sieht man:
- `FP_VERIFY` = noch nicht verifiziert, vor Layout zu schließen
- `FP_NOTE` = verifiziert, mit Quelle dokumentiert
- `FP_MISMATCH` = nicht mehr verwendet (war für HX 12×12, durch Custom-FP gelöst)

## Custom-Footprints in der Project-Lib

Alle 3 in `kicad/libraries/field_ambience.pretty/`:

```
SW_HX_12x12x7.3_SMD-4P.kicad_mod                  HX 12×12 Tactile (5 Plätze)
Crystal_HC49-US-SMD_ABLS.kicad_mod                Y1 HSE-Crystal (1 Platz)
Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A.kicad_mod  TPS61089RNR Boost (1 Platz)
```

`fp-lib-table` referenziert `${KIPRJMOD}/libraries/field_ambience.pretty`.
