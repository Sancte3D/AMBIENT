# Footprint-Verifikations-Log

**Stand: r18.14 (2026-06-12)**

Stand der `FP_VERIFY`/`FP_NOTE`-Properties an den im Schematic platzierten Symbolen. Die Quelle für jeden Eintrag ist im Generator-Code dokumentiert; bei Custom-Footprints liegt das `.kicad_mod` in `kicad/libraries/field_ambience.pretty/`, 3D-STEP-Modelle in `kicad/libraries/field_ambience.3dshapes/` (siehe `mechanical/3d_models/MANIFEST.md`).

## Zusammenfassung

| Status | Anzahl |
|---|---|
| ✅ Verifiziert (KiCad-Standard, Source quoted) | 5 |
| ✅ Custom-FP nach Hersteller-/EasyEDA-Daten erstellt | 5 |
| 🟡 Offen (echte FP-Verify nötig) | 0 — EC11J-Blocker in r18.14 aufgelöst (Teil retired, ADR-0012) |

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

### ✅ EN1-EN4 — ALPS EC11E THT (r18.14, ADR-0012; ersetzt EC11J SMD)
- **FP:** `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` (KiCad-Standard) für **alle 4** — jetzt passend, weil die Teile selbst auf EC11**E** THT gewechselt sind
- **Varianten:** EN3 = EC11E mit Push+Detents (Suffix TBD-VERIFY); EN1/2/4 = EC11E183440C (smooth, ohne Switch — S1/S2-Löcher bleiben leer, gleiche Land-Pattern-Familie)
- **Der frühere 🟡-Blocker (EC11J-SMD-Land-Pattern) ist damit GEGENSTANDSLOS.** Zusätzlich wurde das echte EC11J-Pattern via EasyEDA-Export (C209762) doch noch beschafft und als Referenz in die Project-Lib gelegt — der r18.12-Hand-Draft war nachweislich falsch (Pitch 2.54 statt 2.5, Pad-Reihen ±7.0/7.3 statt ±5.0, Body 15×18 statt 12×13.4). Lehre: Custom-FPs nur noch aus CAD-Exporten, nicht aus Prosa-Maßen.
- **3D:** `SW-SMD_EC11J1525402-...-H24.5-P2.5.step` belegt 24.5 mm Gesamthöhe → einer der drei Retire-Gründe (ADR-0012).

### ✅ SW11/SW_BOOT TS-1088-AR02016 — Mini-SMD-Tactile (r18.14)
- **FP:** `field_ambience:SW_TS1088_SMD` (Custom, aus EasyEDA-Export C720477)
- **Korrektur:** C720477 ist XUNPU **TS-1088-AR02016** (SW_BOOT-Property sagte fälschlich TS-1185A-C-A). Vorher generisches `SW_SPST_TL3342`-FP — Land-Pattern passte nicht (TS-1088 ist 3.9×2.9, 2-Pad @ 4.36 mm).

### ✅ J1 TYPE-C-31-M-17 — LCSC-Nr.-Korrektur (r18.14)
- **FP:** `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12` (unverändert, drop-in)
- **Befund:** BOM-LCSC-Nr. C165935 war ein STF18N65M5-MOSFET — beim 3D-Abruf entdeckt (STEP war ein TO-220F-3!). Korrigiert auf **C283540** (LCSC-verifiziert). 3D: `TYPE-C-SMD_6P-...-H3.2-P1.00.step`.

## Was im Code geändert wurde

In `generate_kicad_project.py` werden `FP_VERIFY`-Properties durch `FP_NOTE`-Properties ersetzt, sobald die Verifikation positiv ist. Im Schematic sieht man:
- `FP_VERIFY` = noch nicht verifiziert, vor Layout zu schließen
- `FP_NOTE` = verifiziert, mit Quelle dokumentiert
- `FP_MISMATCH` = nicht mehr verwendet (war für HX 12×12, durch Custom-FP gelöst)

## Custom-Footprints in der Project-Lib

Alle 6 in `kicad/libraries/field_ambience.pretty/`:

```
SW_HX_12x12x7.3_SMD-4P.kicad_mod                  HX 12×12 Tactile (5 Plätze)
Crystal_HC49-US-SMD_ABLS.kicad_mod                Y1 HSE-Crystal (1 Platz)
Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A.kicad_mod  TPS61089RNR Boost (1 Platz)
SW_TS1088_SMD.kicad_mod                           SW11 + SW_BOOT (EasyEDA-verifiziert, r18.14)
Jack_3.5mm_PJ-320D_SMT.kicad_mod                  J8/J9 Audio+MIDI-Klinke (EasyEDA-verifiziert, r18.19)
L_Sunlord_SWPA6045.kicad_mod                      L1 Boost-Inductor 2.2µH (EasyEDA-verifiziert r18.20c — Phantom-Name L_0630 ersetzt)
```

3D-STEP-Modelle (Z-/Panel-kritische Teile) in
`kicad/libraries/field_ambience.3dshapes/` — 7 STEPs (U1, Y1, U8, L1, J_BAT,
J8, SW11/SW_BOOT). Z-Höhen-Tabelle + Regenerier-Kommando in
`mechanical/3d_models/MANIFEST.md` (Repo-Root, nicht unter
`field-ambience-current/`).

r18.36: Footprint `RotaryEncoder_ALPS_EC11J_SMD.kicad_mod` entfernt
(unreferenzierte Leiche — EN1–4 nutzen seit ADR-0012 KiCad-Standard
`Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm`).

`fp-lib-table` referenziert `${KIPRJMOD}/libraries/field_ambience.pretty`.
