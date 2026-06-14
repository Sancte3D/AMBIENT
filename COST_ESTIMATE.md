# Cost Estimate — Field Ambience

**Stand: v0.7-r18.21 (2026-06-14).** Ehrliche Kostenschätzung für einen
**5-Geräte-Prototyp-Run** über JLCPCB-PCBA + Hand-Supply-Teile, mit den
r18.21-Kostensenkungen. Erfahrungsbasiert ±20 % — die exakte Zahl liefert das
JLCPCB-Quote-Tool, sobald `.kicad_pcb` + Gerber/BOM/CPL existieren.

> **Wichtige Korrektur ggü. der ersten Schätzung:** Die „$3-Extended-Setup-
> Gebühr pro Teil" war zu hoch angesetzt. JLCPCB (a) hat die Feeder-Gebühr Ende
> 2025 auf $1.50 gesenkt und (b) stuft unsere großen ICs (STM32H743, PCM5102A,
> PAM8403, PCA9685, AP7361C, Crystal) als **„Extended (Preferred)" = gebührenfrei**
> ein. Real zahlen nur 3 plain-Extended-Teile (MCP23017, MCP73831, DRV5056A4)
> je $1.50 → **~$4.50 Feeder-Gebühr für den ganzen Run**, nicht ~$96.

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
| 4× ALPS EC11E THT-Encoder | ~$5 | $25 | LCSC/Mouser |
| 5× Gateron LP Magnetic Jade | ~$4 | $20 | Gateron/NuPhy/Ukeebs |
| ~~Stabilizer~~ | **$0** | **$0** | **gestrichen (1u-Caps, r18.21)** |
| 4× Encoder-Knöpfe | **$0** | **$0** | **selbst 3D-gedruckt** |
| 5× Cell-Caps | **$0** | **$0** | **selbst 3D-gedruckt (1u)** |
| 2× CMS-Speaker | ~$6 | $30 | DigiKey |
| 1× **bare ST7789V**-Display | ~$4 | $20 | AliExpress (war Adafruit $75!) |
| 1× **2000 mAh** LiPo | ~$8 | $40 | (war 5000 mAh ~$60) |
| Tag-Connect TC2030-IDC (Tool, **einmalig**) | — | $15 | Tag-Connect |
| Dust-Mesh-Sticker | ~$1 | $5 | AliExpress |
| M2.5 Standoffs + Schrauben | ~$1 | $5 | Reichelt |
| **Hand-Supply-Zwischensumme** | | **~$185** | inkl. einmaligem $15-Tool |

## 3. SMD-BOM pro Board (~$18)

STM32H743 ~$8.78 · TPS61089 ~$0.80 · PCM5102A ~$1.11 · PAM8403 ~$0.44 ·
PCA9685 ~$3.33 · MCP23017 ~$1.79 · AP7361C ~$0.34 · MCP73831 ~$0.81 ·
5× DRV5056A4 ~$2.90 · C_BULK Polymer ~$0.60 · 100µF MLCC ~$0.30 ·
17× LEDs ~$0.40 · USB-C ~$0.15 · Crystal ~$0.27 · 2× Klinke ~$0.50 ·
~120 Passives ~$3 · Rest ~$1 → **~$27** (Mengenrabatt ×5 drückt auf ~$18).

## 4. Gehäuse

Für 5 Prototypen **3D-gedruckt** statt Spritzguss-Tool (4-stellig):
- **SLS PA12** (JLC3DP / Shapeways): ~$25–35/Gehäuse (Top + Bottom)
- **FDM** (selbst, PETG): ~$3–8 Material/Gehäuse
- → $15–175 für 5, je nach Verfahren. Annahme **FDM selbst: ~$30 für 5**.

## 5. Gesamt

| Block | 5 Geräte |
|---|---|
| JLCPCB (Fab + PCBA + SMD-BOM + Versand) | ~$190 |
| Hand-Supply (mit Kostensenkung) | ~$185 |
| Gehäuse (FDM selbst) | ~$30 |
| **GESAMT** | **~$405** |
| **pro Stück** | **~$81** |

Mit SLS-Gehäuse + etwas Puffer: **~$450–520 / ~$90–104 pro Stück**.

## 6. Was die r18.21-Kostensenkung gebracht hat

| Maßnahme | Ersparnis (5er-Run) |
|---|---|
| Display Adafruit → bare ST7789V | ~$55 |
| Knöpfe + Cell-Caps selbst 3D-drucken | ~$50–200 |
| Stabilizer gestrichen (1u-Caps) | ~$25–75 |
| Akku 5000 → 2000 mAh | ~$20 |
| **Summe** | **~$150–350** |

Vorher (erste Schätzung): ~$750. Jetzt: **~$405–520**. Der Rest ist
hauptsächlich der STM32H743 (Kern, gebührenfrei „Preferred", nicht ersetzbar
ohne Audio-Risiko) + die Magnetic-Switches.

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
