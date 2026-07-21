# Cost Estimate — Field Ambience

**Stand: v0.7-r18.87 (2026-07-04).** Ehrliche Kostenschätzung für einen
**5-Geräte-Prototyp-Run** über JLCPCB-PCBA + Hand-Supply-Teile. Preise der
teuren Positionen am 2026-07-02 **live von den LCSC-Produktseiten geholt**
(10+-Staffel); Rest gebändert. ±20 % — die exakte Zahl liefert das
JLCPCB-Quote-Tool, sobald `.kicad_pcb` + Gerber/CPL existieren.
BOM-Basis: `kicad/jlc_bom.csv` r18.87 = **57 unique LCSC-Parts,
188 Placements** (davon ~17 THT-Hand: 10 Switches, 4 Encoder, 3 Header).
r18.87: U10-PCA9685 + 8 VU-LEDs + Heartbeat-LED entfernt (−22 Placements,
−~$3.20 SMD-BOM/Board).

> ⚠️ **Assembly-Größenlimit (NEU r18.84, für Aron):** Die PCB-Outline ist
> **252 × 102 mm** — JLCs *Economic*-PCBA endet bei **250 × 250 mm**. 2 mm
> drüber ⇒ Zwangsupgrade auf *Standard*-PCBA (Setup ~$25 statt $8, teurerer
> Stencil). **Outline um ≥2 mm auf ≤250 mm kürzen spart ~$40–60 pro Run** —
> beim Layout prüfen, ob die 4-mm-Bezel das hergibt (mech. Spec §1 hat
> 4 mm Bezel je Seite; 1 mm opfern reicht).

## 1. JLCPCB-Anteil (5 Boards, SMD vollbestückt)

| Posten | Economic (≤250 mm) | Standard (252 mm) | Notiz |
|---|---|---|---|
| PCB-Fab 4-Layer 252×102, 1.6 mm, ×5 | ~$50 | ~$50 | 2.57 dm²/Board |
| Stencil | ~$8 | ~$38 | |
| PCBA-Setup | ~$8 | ~$25 | pro Design |
| Feeder/Extended-Gebühren | ~$8–15 | ~$8–15 | Große ICs sind „Extended (Preferred)“ = gebührenfrei (r18.22-Erkenntnis); plain-Extended: MCP23017, BQ24074, TPA6132A2, TPS22918, MST-12D18G3, PJ-320D, JST, Polyfuse u. a. à $1.50 |
| Placement (~610 SMD-Joints × 5) | ~$5 | ~$5 | $0.0017/Joint |
| SMD-BOM-Teile ×5 (§3) | ~$110 | ~$110 | ~$21.3/Board + Attrition |
| Versand DHL EU | ~$30–40 | ~$30–40 | 5× 252-mm-Boards sind sperrig |
| **JLC-Zwischensumme** | **~$225** | **~$275** | **~$45 bzw. ~$55/Board** |

## 2. Hand-Supply (separat)

| Teil | pro Gerät | × 5 | Quelle |
|---|---|---|---|
| 4× ALPS EC11E18244AU (THT, hand) | ~$7.60 | $38 | LCSC C202365, **$1.91 @10+ live 2026-07-02** (2.5k Stock) |
| 5× Kailh Choc V1 CPG135001D01 (THT, hand) | ~$5 | $25 | LCSC C400229 = 0 Stock → Kailh-Händler/AliExpress ~$1/St. |
| 5× HX B3F-4055 Modifier (THT, hand) | ~$0.30 | $2 | C36498965 (kann alternativ JLC-THT) |
| 2× CMS-402811-28SP Speaker | ~$6 | $30 | DigiKey |
| Waveshare 1.9" ST7789 | ~$12 | $60 | PiHut/Waveshare |
| 2000 mAh LiPo (mit Schutz-PCM!) | ~$8 | $40 | PiHut — **beim Kauf prüfen: Pouch MUSS eigenes Protection-PCM haben** (BQ24074 hat Charge-Timer + TS, aber KEINEN Discharge-/UVLO-Schutz; F2-PTC ist nur Hard-Short-Backup — ADR-0023) |
| Dust-Mesh + M2.5-Schrauben/Standoffs | ~$2 | $10 | AliExpress/Reichelt |
| Tag-Connect TC2030-IDC (Tool, einmalig) | — | $15 | Tag-Connect |
| **Hand-Supply-Zwischensumme** | **~$41** | **~$220** | |

## 3. SMD-BOM pro Board — ~$21.30 (Preise @10+, live 2026-07-02 wo fett)

**STM32H743VIT6 $8.68** · **PCA9685 $2.93 (r18.87: nur noch U6)** · **C_BULK Tantal 470µ $1.78** ·
**MCP23017 $1.62** · **PCM5102A $0.99** · **BQ24074 $2.24** ·
**TPS61089 $0.50** · PAM8403 ~$0.44 · TPS22918 $0.25 (live r18.81) ·
L1 SWPA6045 ~$0.25 · C_BULK2 ~$0.25 · AP7361C ~$0.20 · USBLC6 ~$0.20 ·
**USB-C $0.17** · Polyfuse ~$0.15 · Crystal ~$0.12 · 2× PJ-320D ~$0.24 · **TPA6132A2 $1.35** ·
JST ~$0.10 · MST-12D18G3 $0.08 (live r18.81) · 2× SS34 + TVS ~$0.16 ·
2× TS-1088 ~$0.04 · 2N7002 ~$0.01 · 16× LED ~$0.40 · 3× Header ~$0.35 ·
~110 R/C-Passives ~$0.55

Kostentreiber-Ranking: STM32 (41 %), PCA9685 (14 %), Tantal-Bulk (8 %),
MCP23017 (8 %). Die 4 Encoder ($7.60, THT-hand) sind der größte
Nicht-JLC-Posten pro Gerät.

## 4. Summen

| | 5er-Run | pro Gerät |
|---|---|---|
| JLC (Economic-Fall, Outline ≤250 mm) | ~$225 | ~$45 |
| JLC (Standard-Fall, 252 mm) | ~$275 | ~$55 |
| Hand-Supply | ~$220 | ~$41–44 |
| **Gesamt (Economic)** | **~$445** | **~$89** |
| **Gesamt (Standard)** | **~$495** | **~$99** |

10er-Run: PCB ~$75, Gebühren identisch, Teile ~×2 → grob **~$72–82/Gerät**.

## 5. Stock-Warnungen (live 2026-07-02)

- **C444831 (Tantal 470 µF): nur 121 St. LCSC / 161 JLC** — für 5–10 Boards
  ok, aber VOR dem Order-Klick prüfen; Alternative in BOM_MASTER §2.
- **C114409 (STM32H743VIT6): LCSC 6 St., JLC-Assembly ~392** — über
  JLC-Assembly bestücken lassen (nicht LCSC-Einzelkauf).
- **C400229 (Choc): 0 Stock** — extern beschaffen (jede Choc-V1-Farbe passt).
- C165129 (TPS61089): 2.1k ✓ · C2678753 (PCA9685): 1.4k ✓ ·
  C202365 (EC11): 2.5k ✓.

## 6. Historie

- r18.87: U10-PCA9685 + 8 VU-LEDs + R_VU + R_OE2 + 2 Caps + Heartbeat-LED
  entfernt (User: nur Cell-/Modifier-LEDs bleiben) → SMD-BOM $24.50 → ~$21.30,
  57 Parts / 188 Placements.
- r18.22: Erststand ($38/Board JLC, $18 SMD-BOM). r18.84 ersetzt: +U10-PCA
  (+$2.93), +Power-Off-Block (+$0.35), +Dioden-OR/3×22µ/Comp (+$0.15),
  Cells auf direkt-gelötete Choc (LCSC-Anteil 0), Encoder von „~$5“ auf
  live $7.60/Gerät, STM32 live $8.68, plus Economic-vs-Standard-Split
  wegen der 252-mm-Outline.
