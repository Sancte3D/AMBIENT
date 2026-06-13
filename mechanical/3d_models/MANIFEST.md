# 3D-Modell-Manifest — Field Ambience

**Stand: r18.14 (2026-06-12).** STEP-Dateien liegen in
`field-ambience-current/kicad/libraries/field_ambience.3dshapes/`.
Quelle: EasyEDA/LCSC-CAD-Daten, gezogen mit `easyeda2kicad --full --lcsc_id=<ID>`.

> Zweck: Enclosure-/Panel-/PCB-Abstimmung in CAD. Die Z-Höhen hier sind die
> verbindliche Quelle für ADR-0011 (Gehäuse-Dicke) und das Frontpanel-CAD.

## Im Repo (Z-/Panel-kritisch)

| LCSC | Bauteil | STEP-Datei | Höhe über PCB |
|---|---|---|---|
| C209762 | EC11J1525402 Encoder (SMD, r18.14: nur noch Referenz — ersetzt durch EC11E THT, ADR-0012) | `SW-SMD_EC11J1525402-L15.0-W18.0-H24.5-P2.5.step` | **24.5 mm** (Beleg: zu hoch) |
| C283540 | USB-C TYPE-C-31-M-17 (**r18.14: LCSC-Nr. korrigiert**, war C165935 = MOSFET!) | `TYPE-C-SMD_6P-L8.9-W6.8-H3.2-P1.00.step` | 3.2 mm |
| C114409 | STM32H743VIT6 LQFP-100 | `LQFP-100_L14.0-W14.0-H1.4-LS16.0-P0.50.step` | 1.4 mm |
| C720477 | SW_BOOT Taster (**r18.14: MPN korrigiert** — XUNPU TS-1088-AR02016, nicht TS-1185A) | `SW-SMD_L3.9-W2.9-H2.0-LS4.8.step` | 2.0 mm |
| C596838 | ABLS-8.000MHZ-B4-T Crystal HC-49/US-SMD | `CRYSTAL-SMD_L11.4-W4.7-LS12.7.step` | 4.2 mm |
| C150470 | SWPA6045S2R2MT Boost-Inductor | `IND-SMD_L6.0-W6.0-H4.5.step` | 4.5 mm |
| C431535 | PJ-320D 3.5mm-Klinke (Line-Out, Panel-Cutout!) | `AUDIO-SMD_PJ-320D-1.step` | 5.0 mm |
| C295747 | JST-PH S2B-PH-SM4-TB (Battery) | `CONN-SMD_P2.00_S2B-PH-SM4-TB-LF-SN.step` | 6.0 mm |
| C165129 | TPS61089RNR Boost VQFN-HR-11 | `VQFN-HR_L2.5-W2.0-H1.0-P0.50.step` | 1.0 mm |

## Bewusst NICHT im Repo (regenerierbar, Z-unkritisch)

0603/0805-Passives, LEDs, SOT-23-x, SOIC/TSSOP/SSOP-ICs (alle ≤ 2 mm),
SMA-Dioden, Sicherung. Regenerieren:

```bash
pip install easyeda2kicad
easyeda2kicad --full --lcsc_id=C<ID> --output <lib-pfad>
```

## Fehlende 3D-Modelle (externe Quelle nötig)

| Bauteil | Höhe (DS) | Quelle |
|---|---|---|
| HX 12×12×7.3 Taster (C36498966) | 7.3 mm | kein EasyEDA-3D; bei Bedarf aus DS extrudieren |
| Cloth-Cone Mini-Speaker (CMS-402811-28SP primär / AS04008PS sekundär) | **11.5 mm** Treiber-Tiefe, Footprint 40 × 28.3 mm | sameskydevices.com / puiaudio.com → CAD-Download |
| LCD-Modul ST7789 (Adafruit 5394) | 3.5 mm über Standoff | Adafruit GitHub (CAD-Repo) |
| EC11E THT Encoder-Familie (ADR-0012, neu) | Body 7 + Schaft n. Wahl | ALPS tech.alpsalpine.com → 3D-Download je Variante; LCSC-ID nach Sourcing-Verifikation fetchen |
| Gateron LP Magnetic Switch + Stabilizer (ADR-0013) | ~8 mm Stack | Gateron/NuPhy; Community-CAD (kbd-Ökosystem) |
| Encoder-Knöpfe (Alu, Ø19–20, Kick75-artig) | 8–10 mm | eigenes CAD (Industrial-Design-Sprint) |

## Befund-Log (r18.14-Verifikationslauf)

1. 🔴 **C165935 war STF18N65M5 MOSFET (TO-220F-3)** — nicht der USB-C-Connector.
   Im BOM seit r18.10. Korrekt: **C283540** = Korean Hroparts TYPE-C-31-M-17.
   Via EasyEDA-CAD-Abruf entdeckt → Generator + SPEC in r18.14 gefixt.
2. 🟠 **C720477 = XUNPU TS-1088-AR02016** (nicht TS-1185A-C-A wie in r18.10
   notiert). Footprint/Funktion identisch (3.9×2.9 SMD-Taster), MPN korrigiert.
3. 🟠 **EC11J-Draft-FP war falsch**: echtes Land-Pattern hat 2.54-mm-Pitch
   (nicht 2.5), Pad-Reihen bei Y=±7.0/7.3 (nicht ±5.0), 6 Anker-Pads + 2
   THT-Posts, Body 15×18 (nicht 12×13.4). Draft ersetzt durch
   EasyEDA-verifizierte Version; Encoder-Strategie wechselt ohnehin auf
   EC11E THT (ADR-0012).
