# ADR-0011: Gehäuse-Dicke — Z-Budget gegen reale Bauteile

**Status:** ACCEPTED (Engineering-Berechnung 2026-06-11)
**Date:** 2026-06-11

## Kontext

User-Frage: „Wie dick wird das Gehäuse inklusive Lautsprecher?"

Bisher in `mechanical_coordinates.md` ohne harte Z-Berechnung. Jetzt fixiert: drei Stack-Up-Varianten gegeneinander gerechnet, die realistische gewählt.

## Echte Bauteil-Höhen (Datenblatt-verifiziert)

| Komponente | Höhe über PCB | Quelle |
|---|---|---|
| **Encoder EC11J Schaft + Body** | **19 mm** (Body 7 mm + Schaft 12 mm) | ALPSALPINE EC11J Datasheet |
| **C_BULK 1000 µF Alu-Elko** | **10.5 mm** | EEE-FK1A102P / JVJ16V1000M10x10 |
| **LCD-Header 2.54 mm** | **8.5 mm** (Buchse) | Pinheader 1×08 Standard |
| **USB-C TYPE-C-31-M-17** | **7.5 mm** Connector + 7 mm Mate | HRO M-17 Drawing |
| **SW6-10 HX 12×12×7.3** | **7.3 mm** | LCSC C36498966 |
| **J9 JST-PH 2.0 Battery** | **6.0 mm** | JST S2B-PH-SM4-TB |
| **PUI AS04008PS Speaker** | **5.0 mm** Treiber-Frame | PUI Datasheet |
| **HC-49/US-SMD Crystal Y1** | **4.2 mm** | ABRACON ABLS Drawing |
| **LCD-Modul ST7789 (Adafruit 5394)** | **3.5 mm** (über Header-Standoff) | Adafruit Drawing |
| **L1 Boost-Inductor 0630** | **3.0 mm** | SWPA6045S2R2MT |
| **D2 SMAJ5.0A SMA Diode** | **2.3 mm** | SMA-Standard |
| **MCU LQFP-100 / TSSOPs / SOIC** | **1.0–2.0 mm** | JEDEC-Standard |
| **R/C 0603 + LEDs 0603** | **0.45–0.95 mm** | Standard SMD |
| **FSR-Pad + Silicon-Cap (Cell)** | **~3.5 mm** (Pad 0.45 + Cap-Form 3 mm) | ADR-0006 |

**Höchster aktiver Komponentenstapel:**
- Encoder dominiert: 19 mm
- Encoder-Knopf darüber (Aluminium-Knopf): 8–12 mm außerhalb des Gehäuses

## Drei Stack-Up-Varianten

### Variante A — Encoder-Knopf vollständig im Gehäuse

```
       ┌─────────────┐
2.5 mm │  TOP-Panel  │   ← Frontpanel-Material (ABS/Aluminium)
       ├─────────────┤
   ?   │ Encoder-    │   ← Encoder-Schaft + Knopf intern
  mm   │ Knopf-      │     (Knopf 8 mm + Schaft-Restweg 4 mm)
       │ Bereich     │
       ├─────────────┤
 12 mm │ Schaft     │
  ↕    │             │
       ├─────────────┤
  7 mm │ Encoder-    │   ← Body + restliche Top-Komponenten
       │ Body-Zone   │
       ├═════════════┤   ← PCB 1.6 mm
 4.5mm │ PCB Standoff│
       ├─────────────┤
 2.5 mm│ BOTTOM Panel│
       └─────────────┘
TOTAL: ~ 40 mm — zu dick, OP-1-Field-Sprache verletzt
```

### Variante B — Encoder-Knopf ragt heraus (empfohlen)

```
        ┌─────┐
   8 mm │ Knopf│             ← Aluminium-Knopf außerhalb
        ├─────┤
        ┌─────────────┐
 2.5 mm │  TOP-Panel  │
        ├─────────────┤
 12 mm  │ Encoder-Schaft│   ← Frontpanel hat Bohrung Ø 7-8 mm
        │ (in Bohrung)  │
        ├─────────────┤
  7 mm  │ Encoder-Body+ │   ← Höchste Top-Komponenten
        │ C_BULK (10.5)+│     C_BULK seitlich vom LCD platziert
        │ USB-C (7.5)   │     (Layout-Constraint)
        ├═════════════┤    ← PCB 1.6 mm
  3 mm  │ PCB Standoff │
        ├─────────────┤
  2.5mm │ BOTTOM Panel│
        └─────────────┘
TOTAL (Gehäuse): 2.5 + 12 + 7 + 1.6 + 3 + 2.5 = ~ 28.6 mm
TOTAL inkl. Knopf-Erhebung: ~ 36-38 mm
```

### Variante C — Side-Mount-Speaker statt PCB-Speaker

```
        ┌─────────────┐
 2.5 mm │ TOP-Panel   │
        ├─────────────┤
 12 mm  │ Encoder-Schaft│
        ├─────────────┤
        │     │     │ │
        │ PCB │     │S│   ← Speaker seitlich in Kammer
        │     │     │P│     (Bass-Reflex-Volumen ≥ 20 mm hoch)
  7 mm  │     │     │K│
        ├═════├     ├─┤
  1.6mm │ PCB │     │ │
        ├─────┤     │ │
  3 mm  │Stand│     │ │
        ├─────┘     └─┤
  2.5mm │  BOTTOM    │
        └─────────────┘
TOTAL: gleiches Profil wie B, Speaker eingeschwenkt
```

## Entscheidung

**Variante B**, mit folgenden Detail-Maßen:

| Zone | Dicke | Notiz |
|---|---|---|
| Bottom-Panel | 2.5 mm | ABS oder Aluminium |
| PCB-Standoff (Schraube/Boss zum Bottom) | 3.0 mm | Standoff M2.5 oder integrierter Plastik-Boss |
| PCB | 1.6 mm | 4-Layer FR4 |
| Über-PCB-Bereich (Encoder-Body, USB-C, C_BULK, Speaker) | **8.0 mm** (Layout muss alle Komponenten in dieser Höhe halten **außer** Encoder-Schaft) | C_BULK 10.5 mm passt nicht — Workaround: liegender Elko (SMD-Polymer-Alternative oder Side-Mount) |
| Encoder-Schaft (durch Frontpanel-Bohrung) | 12 mm | passt durch 2.5 mm Frontpanel-Aussparung |
| Top-Panel (Frontpanel) | 2.5 mm | mit Aussparungen für Display + Encoder-Bohrungen + Speaker-Mesh + Modifier-Buttons |
| **Gehäuse-Innenhöhe** | **14.6 mm** | (Standoff + PCB + Top-Komponenten) |
| **Gehäuse-Außenhöhe (ohne Knöpfe)** | **19.6 mm** | (+ Top + Bottom) |
| **Gesamthöhe (mit Encoder-Knöpfen)** | **~ 30 mm** | (+ 10 mm Knopf-Erhebung) |

## Konkretes Layout-Constraint (ADR-Konsequenz)

**C_BULK 1000 µF (10.5 mm)** ist die einzige Komponente, die über die 8 mm-Top-Zone hinausragt. Drei Optionen, in absteigender Sympathie:

1. **Polymer-Tantal-Cap statt Alu-Elko** — 6×8 mm Footprint, **2.5-mm-Höhe**. Beispiel: Panasonic 25SVPF1000M oder Kemet T520. Trade-Off: höhere Kosten (~$0.50 statt $0.10), bessere ESR (10 mΩ vs. 25 mΩ → besser für Bass-Transienten), passt in 8-mm-Höhenzone.
2. **Side-Mount-Alu-Elko** — Bauform liegend (8×16 mm vertikales Footprint, 6 mm Höhe). Footprint-Änderung nötig.
3. **Beibehalten** und LCD-Modul/Speaker so anordnen, dass C_BULK in einer freien Tasche steht. Layout-Constraint, nicht BOM-Änderung.

**Empfehlung: Option 1** (Polymer-Cap). Bessere Audio-Eigenschaften UND Höhenproblem gelöst. Wird in r18.12 oder vor Layout-Phase fixiert.

## Speaker-Anschluss-Kammer

PUI AS04008PS (40-mm-Treiber) brauchen Innenvolumen für Mid-Range-Performance:

- **Closed-Box**: ~5-8 cm³ minimal, sonst zu dumpf
- **Bass-Reflex**: 15-25 cm³ + Reflex-Port

Bei einem ~33 × 14.3 cm Gehäuse mit 19.6 mm Innenhöhe ist seitliche Speaker-Kammer machbar:

- Speaker-Kammer L+R: je ~3 × 7 × 1.5 cm = ~31 cm³ pro Seite — passt für Closed-Box-Mid-Range-Sound
- Dust-Mesh-Aussparung im Top-Panel: 36 × 70 mm oval, 0.3-mm-Mesh-Inset (ADR-0007)

## Consequences

**Positive:**
- Z-Budget realistisch berechnet, keine Layout-Überraschungen
- C_BULK-Konflikt früh erkannt → Lösungs-Optionen dokumentiert, bevor Layout startet
- Gehäuse-Dicke **~ 20 mm + 10 mm Encoder-Knöpfe** ist OP-1-Field-mäßig (OP-1 ist 18.5 mm, Field 19 mm)

**Negative:**
- Encoder ragt heraus → Knopf-Material muss zum Industrial-Design passen
- C_BULK-Wechsel auf Polymer = $0.40 Mehrkosten pro Gerät (bei Option 1)

## Open Points für r18.12

- C_BULK-Variante final entscheiden (Polymer-Cap-Komponentenwahl + LCSC-Verifikation)
- Speaker-Kammer-CAD-Modell (parallel zu Frontpanel-CAD)
- Knopf-Material + Knopf-Maße (Industrial-Design-Sprint)

## Related

- ADR-0006 — Cell-Mechanik (Silicon-Cap = +3.5 mm Top-Zone)
- ADR-0007 — Speaker-Mesh (Frontpanel-Aussparung)
- mechanical_coordinates.md §0 (IMG_9713-Layout)
