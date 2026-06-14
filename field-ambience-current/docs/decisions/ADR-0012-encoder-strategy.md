# ADR-0012: Encoder-Strategie — 1× Push+Detent, 3× Smooth, alle gleich hoch

**Status:** ACCEPTED (User-Direktive + Engineering-Analyse, 2026-06-12)
**Date:** 2026-06-12

## Kontext (User-Anforderung, wörtlich destilliert)

1. Nur der **Display-Encoder** ist Rotary+Push; die anderen drei sind reine
   Dreh-Encoder **ohne Anschlag** (360°-endlos), **smooth und angenehm**.
2. Langsames Drehen soll **1 % pro Schritt** entsprechen. Beim aktuellen
   Bench-Billo-Encoder braucht es **zwei Klick-Rastungen für 1 %** —
   „komische User Experience".
3. **Alle 4 Encoder gleich hoch.**
4. Referenz-Feel: **NuPhy Kick75**-Drehknopf — „nicht zu hoch, angenehmer".

## Analyse

### Der „2 Klicks = 1 %"-Bug ist ein Decoder-Mismatch, kein Hardware-Defekt

EC11-Encoder gibt es in zwei Rastungs-Geometrien:

| Typ | Detents/Umdr. | Pulse/Umdr. | Rastung sitzt bei |
|---|---|---|---|
| Full-Cycle (15/15) | 15 | 15 | jeder vollen Quadratur-Periode (Ruhelage 11) |
| **Half-Step (30/15)** | **30** | **15** | **jeder HALBEN Periode (Ruhelage 00 UND 11)** |

Der Bench-Encoder (KY-040-Klasse) ist Half-Step. Die Firmware zählte 1 Schritt
pro **voller** Periode → genau 2 Rastungen pro Schritt. **Fix (r18.14):**
`ENC_HALF_STEP` Default = 1 im Bench-Tool (`display_hw_test.c`); der
Produkt-Treiber (`encoders.c`) hat jetzt `substeps` **pro Encoder**
konfigurierbar.

### Kick75-Referenz (analysiert)

Der Kick75-Knob sitzt flach am Deck (Low-Profile-Board, Gateron LP 3.0),
**hat Rastungen** und dreht endlos. Übersetzung auf unser Gerät:
- **Flach** = kurzer Schaft + flacher Knopf (Ø19–20 mm, 8–10 mm hoch, Alu)
- **Detents am Display-Encoder gewollt** (Menü: 1 Rastung = 1 Schritt,
  taktiles Feedback beim Navigieren)
- **Parameter-Encoder dagegen ohne Rastung**: kontinuierliche Größen
  (Drive/Brightness/Volume in %) fühlen sich mit glattem Lauf besser an;
  Auflösung und Beschleunigung macht die Firmware, nicht die Mechanik.

### Bauteil-Entscheidung

**Familie: ALPS EC11E, THT, vertikal — alle 4 aus derselben Familie:**

| Position | Variante | Eigenschaften |
|---|---|---|
| EN3 (Display) | **EC11E18244AU** — 18 Puls / 36 Detents, mit Push-Switch (active, C202365) | Menü-Navigation + Klick |
| EN1/2/4 (Drive/Bright/Vol) | **EC11E18244AU (gleiches Teil)** — r18.22-Pivot wegen NRND-Status der früheren EC11E183440C-Wahl | Detents werden vom Firmware-Acceleration-Layer unsichtbar: langsam = 1 %/Klick (UX-Ziel), schnell = ×8/Klick |

> **r18.22-NRND-Pivot:** Die ursprüngliche r18.14-Wahl EC11E183440C
> (0-Detent + Push, C370986) wurde als NRND bestätigt. Der obvious-Kandidat
> EC11E1834403 (C361165) ist **ebenfalls NRND** — ALPS hat die gesamte
> „EC11E 0-Detent + Push-Switch"-SKU-Familie phased out. Kein aktives
> Drop-in-Equivalent existiert. Pivot auf **alle 4 = EC11E18244AU** (active).
> Der „smooth"-UX-Wunsch („1 % pro langsamem Klick") wird ohnehin von der
> Firmware-Acceleration erfüllt — die 36 Detents stören dabei nicht; bei
> schneller Drehung kommen sie kaum ins Bewusstsein, und langsam ist jeder
> Klick = 1 %. Bonus: alle 4 Encoder identisch → Lagerhaltung simpel.
>
> Lehre: **Lifecycle-Status muss aktiv verifiziert werden, nicht nur Stock**.
> Die r18.14-Sourcing-Recherche prüfte Stock, nicht NRND. Das ist ab jetzt
> Pflichtteil jeder Komponenten-Auswahl (analog zu Pin-Count nach r18.19).

Warum THT statt des bisherigen EC11J SMD (C209762):
1. **EC11J ist NRND** (seit r18.6 bekannt).
2. **3D-verifiziert zu hoch**: EasyEDA-STEP zeigt H = **24.5 mm** gesamt —
   verfehlt das Kick75-flache Ziel deutlich. EC11E mit kurzer Schaftwahl
   (einheitlich für alle 4, Ziel ~15 mm-Klasse) → Gesamt ≈ 20–22 mm.
3. **THT = mechanische Robustheit**: Encoder sehen tägliche Dreh- und
   Druckkräfte; THT-Anker schlagen SMD-Pads.
4. **Gleiche Höhe garantiert**: gleiche Body-Geometrie + gleiche Schaftlänge
   über die ganze Familie. Die Knöpfe (Ø19–20 × 8–10 mm) sind identisch.

Footprint: **KiCad-Standard-Lib** `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm`
für ALLE vier (fab-erprobt; Land-Pattern ist für alle Schafthöhen identisch,
H20mm betrifft nur das 3D-Modell). Die Smooth-Variante hat physisch keine
Switch-Pins — die S1/S2-Löcher bleiben leer; das SW-Net bleibt im Schematic
verdrahtet (Pull-up = idle high, Firmware pollt nicht: `has_sw = false`).

### UX-Gesetz (Firmware, verbindlich)

```
Display (Detents):   1 Rastung   = 1 Menü-Schritt          (substeps = 4 full-cycle / 2 half-step)
Smooth (18 PPR):     substeps=2  → 36 Ticks/Umdrehung      (~1 % pro 10° bei langsamem Drehen)
Acceleration-Tiers:  dt < 28 ms → ×8 | < 60 ms → ×5 | < 120 ms → ×3 | < 240 ms → ×2
                     (identisch in Sim, Bench und Produkt — SPEC §5)
```

Langsam = exakt 1 %-Schritte (Feinkontrolle), schneller Spin = 0→100 % in
unter einer Dreiviertel-Umdrehung. Auf STM32 übernehmen TIM2–TIM5 im
Hardware-Encoder-Mode das Zählen; `substeps`-Semantik wandert in den
HAL-Layer (NATIVE_PORT_PLAN Phase 2).

## Konsequenzen

**Positiv:** UX-Bug an der Wurzel gefixt; Höhen-Gleichheit by design; flacheres
Gerät (−2.5 bis −4.5 mm gegenüber EC11J); NRND-Teil eliminiert; Standard-Lib-FP
statt Custom-Draft (ein FP_VERIFY-Punkt entfällt).

**Negativ / offen:**
- LCSC führt Original-ALPS nur sporadisch → ggf. Mouser/Digikey + THT-Handbestückung
  (5 Minuten pro Board, akzeptabel für Prototyp-Serie). LCSC-IDs: TBD-VERIFY.
- Exakte Display-Encoder-Suffix-Variante (Detent-Kraft 0.5–1.5 N wählbar) nach
  Muster-Test festlegen — Detent-Kraft ist Feel-Entscheidung.
- Knopf-CAD (Ø19–20 × 8–10 mm, D-Shaft/Knurled passend zur Schaft-Wahl) =
  Industrial-Design-Sprint.

## Related

- ADR-0011 — Gehäuse-Z-Budget (Encoder dominiert Bauhöhe; dieses ADR senkt sie)
- `mechanical/3d_models/MANIFEST.md` — EC11J-STEP als Beleg der 24.5 mm
- `firmware-c-next/src/encoders.c` — per-Encoder `substeps` + `has_sw`
- `firmware-c-next/tools/display_hw_test.c` — `ENC_HALF_STEP` Default-Fix
