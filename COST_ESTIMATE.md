# Cost Estimate — Field Ambience

**Stand: v0.7-r18.22 (2026-06-14).** Ehrliche Kostenschätzung für einen
**5-Geräte-Prototyp-Run** über JLCPCB-PCBA + Hand-Supply-Teile, mit den
r18.21-Kostensenkungen. Erfahrungsbasiert ±20 % — die exakte Zahl liefert das
JLCPCB-Quote-Tool, sobald `.kicad_pcb` + Gerber/BOM/CPL existieren.

> **Wichtige Korrektur ggü. der ersten Schätzung:** Die „$3-Extended-Setup-
> Gebühr pro Teil" war zu hoch angesetzt. JLCPCB (a) hat die Feeder-Gebühr Ende
> 2025 auf $1.50 gesenkt und (b) stuft unsere großen ICs (STM32H743, PCM5102A,
> PAM8403, PCA9685, AP7361C, Crystal) als **„Extended (Preferred)" = gebührenfrei**
> ein. Real zahlen nur 2 plain-Extended-Teile (MCP23017, MCP73831)
> je $1.50 → **~$3.00 Feeder-Gebühr für den ganzen Run**, nicht ~$96.
> (r18.73: DRV5056A4 entfällt — Cells sind jetzt digitale Switches am MCP23017.)

## 1. JLCPCB-Anteil (5 Boards, vollbestückt SMD)

| Posten | Schätzung (5er-Run) | Notiz |
|---|---|---|
| PCB-Fab 4-Layer 252×102 mm, 1.6 mm | ~$45 | 257 cm², 4-Lagen |
| Stencil (framed) | ~$8 | SMT-Pflicht |
| PCBA-Setup | ~$8 | pro Design |
| Feeder-Gebühr | ~$4.50 | nur 3 plain-Extended-Teile |
| Placement (Joints) | ~$5 | ~600 Joints × 5 × $0.0017 |
| SMD-BOM-Teile (×5) | ~$90 | ~$18/Board, siehe §3 |
| Versand DHL EU | ~$30 | typisch |
| **JLC-Zwischensumme** | **~$190** | **~$38/Board** |

## 2. Hand-Supply-Anteil (separat, mit r18.21-Kostensenkung)

| Teil | pro Gerät | × 5 | Quelle |
|---|---|---|---|
| 4× ALPS EC11E18244AU (alle 4 gleich, r18.22 NRND-Pivot) | ~$5 | $25 | LCSC C202365 (~3.052 Stock, active) |
| ~~Stabilizer~~ | **$0** | **$0** | **gestrichen (1u-Caps, r18.21)** |
| 4× Encoder-Knöpfe | **$0** | **$0** | **selbst 3D-gedruckt** |
| 5× Cell-Caps | **$0** | **$0** | **selbst 3D-gedruckt (1u)** |
| 2× CMS-Speaker | ~$6 | $30 | DigiKey |
| 1× **Waveshare 1.9in ST7789V2**-Display (r18.22 von bare-AliExpress; QC + Level-Shifter) | ~$12 | $60 | PiHut £11.60 / Waveshare direkt |
| 1× **2000 mAh** LiPo | ~$8 | $40 | (war 5000 mAh ~$60) |
| Tag-Connect TC2030-IDC (Tool, **einmalig**) | — | $15 | Tag-Connect |
| Dust-Mesh-Sticker | ~$1 | $5 | AliExpress |
| M2.5 Standoffs + Schrauben | ~$1 | $5 | Reichelt |
| **Hand-Supply-Zwischensumme** | | **~$165** | inkl. einmaligem $15-Tool (r18.73 strich Gateron -$20; r18.75: Cell-Switch ist jetzt direkt-gelötetes THT in §3, nicht mehr hier — siehe unten) |
> **r18.75:** die 5× Kailh-Choc-V1-Switches (CPG135001D01, LCSC C400229)
> sind jetzt direkt auf die Platine gelötet, nicht mehr ein separater
> Hotswap-Socket + Klick-Switch — siehe §3 SMD-BOM.

## 3. SMD-BOM pro Board (~$18)

STM32H743 ~$8.78 · TPS61089 ~$0.80 · PCM5102A ~$1.11 · PAM8403 ~$0.44 ·
PCA9685 ~$3.33 · MCP23017 ~$1.79 · AP7361C ~$0.34 · MCP73831 ~$0.81 ·
5× HX B3F-4055 Modifier-Switches (C36498965) ~$0.30 · 5× Kailh Choc V1 cell keyswitches, direct-solder (CPG135001D01, LCSC C400229, r18.75) ~$2 (⚠ market estimate — no confirmed unit quote yet) · C_BULK Polymer ~$0.60 · 100µF MLCC ~$0.30 ·
17× LEDs ~$0.40 · USB-C ~$0.15 · Crystal ~$0.27 · 2× Klinke ~$0.50 ·
~120 Passives ~$3 · Rest ~$1 → **~$29** (Mengenrabatt ×5 drückt auf ~$19).

## 4. Gehäuse

Für 5 Prototypen **3D-gedruckt** statt Spritzguss-Tool (4-stellig):
- **SLS PA12** (JLC3DP / Shapeways): ~$25–35/Gehäuse (Top + Bottom)
- **FDM** (selbst, PETG): ~$3–8 Material/Gehäuse
- → $15–175 für 5, je nach Verfahren. Annahme **FDM selbst: ~$30 für 5**.

## 5. Gesamt

| Block | 5 Geräte |
|---|---|
| JLCPCB (Fab + PCBA + SMD-BOM + Versand) | ~$190 |
| Hand-Supply (r18.22: Waveshare-Display statt bare) | ~$225 |
| Gehäuse (FDM selbst) | ~$30 |
| **GESAMT** | **~$445** |
| **pro Stück** | **~$89** |

Mit SLS-Gehäuse + etwas Puffer: **~$490–560 / ~$98–112 pro Stück**.

## 6. Was die r18.21-Kostensenkung gebracht hat

| Maßnahme | Ersparnis (5er-Run) |
|---|---|
| Display Adafruit → **Waveshare** (r18.22 von bare zurück) | ~$15 |
| Knöpfe + Cell-Caps selbst 3D-drucken | ~$50–200 |
| Stabilizer gestrichen (1u-Caps) | ~$25–75 |
| Akku 5000 → 2000 mAh | ~$20 |
| Encoder vereinheitlicht (r18.22: keine NRND-Variante mehr, gleicher Stückpreis) | 0 (Sicherheit) |
| **Summe** | **~$110–310** |

Vorher (erste Schätzung): ~$750. Jetzt: **~$440–560**. Der Rest ist
hauptsächlich der STM32H743 (Kern, gebührenfrei „Preferred", nicht ersetzbar
ohne Audio-Risiko). (r18.73 strich die teuren Gateron-Magnetic-Switches;
r18.74 versuchte kurz einen Hotswap-Socket, r18.75 vereinfachte auf direkt
gelötete Kailh-Choc-V1-Switches (C400229) — echtes Tastengefühl für einen
Bruchteil der Gateron-Kosten, elektrisch weiterhin digital am MCP23017,
ohne unverifiziertes Zwischenteil.)

> **r18.22-Korrektur:** Die r18.21-„bare ST7789V"-Senkung war zu aggressiv.
> Display ist jetzt Waveshare (~$11 statt $3 bare statt $15 Adafruit) — gleiche
> Panel-Qualität wie Adafruit + branded QC + Level-Shifter. $4 Ersparnis statt
> $12, dafür kein DOA-Risiko. Und alle 4 Encoder sind jetzt das gleiche active
> ALPS-Teil (statt 3× NRND + 1× active) — Sicherheits-Pivot, kein Mehrpreis.

## 7. Weitere Spar-Optionen (nicht umgesetzt — Trade-Off)

- **STM32H750VBT6** statt H743: ~$2.35 statt $8.78 (–$32/5er-Run), ABER nur
  128 KB Flash → externes QSPI-Flash + XIP-Boot = Firmware-Re-Architektur.
  **Nicht empfohlen** — Audio-Engine-Risiko.
- **TP4056** statt MCP73831-Charger: $0.19 statt $0.81, aber auch Extended +
  Footprint-Wechsel. ~$3/5er-Run — Rework lohnt nicht bei 5 Stück.
- **Größerer Mengen-Run** (z. B. 30 statt 5): PCB-Fab + Setup amortisieren sich,
  pro-Stück-Kosten fallen Richtung ~$55–65.

## Exakte Zahl

Sobald das `.kicad_pcb` existiert → Gerber + BOM-CSV + CPL exportieren → ins
JLCPCB-Quote-Tool hochladen = echte Zahl. Bis dahin gilt obige Schätzung.
