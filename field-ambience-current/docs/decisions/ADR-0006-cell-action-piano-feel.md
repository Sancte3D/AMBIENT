# ADR-0006: Cell-Action — Piano-Feel statt Tactile-Switches

**Status:** ACCEPTED (User-Entscheidung 2026-06-11, IMG_9713 + Brief)
**Date:** 2026-06-11

## Context

Bis r18.7 waren die 5 Cell-Inputs als Standard-Tactile-Switches modelliert
(plain SMD-Tactile, HX 12×12 oder ähnlich). Damit fühlen sie sich an wie
gewöhnliche Modifier-Buttons — also genau so wie Shift/Hold/Drone/Generate
direkt darüber.

Das ist falsch. Im neuen Industrial-Design-Stand (IMG_9713) sind die Cells
**lange vertikale Pillen** — visuell und mechanisch als „Tasten" gedacht,
**nicht** als Push-Buttons. Vergleichbarer Referenzpunkt: **HiChord** und das
HVD/Pieces-OS-Field-Gerät — Tasten mit einer kurzen, weichen, klavierähnlichen
Auslösung, nicht ein harter Klick.

## Decision

Cell-Inputs werden als **Silikon-Pad-Tasten mit Velocity-Sense** designt,
nicht als Tactile-Switches.

Implementierung am realistischsten als **Force-Sensitive Resistor (FSR) +
Silicon Dome**:

- mechanisch: Silicon-Pad-Cap auf einem Plastik-Frame, niedriger Travel
  (~1-2 mm), weiche Federung, **keine Click-Mechanik**
- elektrisch: FSR (Force-Sensitive Resistor) liefert kontinuierlichen
  Widerstandswert; MCU sampled via ADC → mappt auf Velocity 0-127
  (MIDI-konform)
- alternativ (low-cost-Pfad für Prototype-1): **2 Tactile-Switches pro Cell**
  übereinander (Quiet+Loud), Firmware misst Time-Diff → grobe Velocity in
  3 Stufen. Wie beim Roli-Lightpad-Vorgänger.
- Cell-LED-Hold-Anzeige bleibt: 1 gelb + 1 grün pro Cell, XOR (siehe
  ADR-0008)

## Consequences

**Positive:**
- Spielbarkeit drastisch höher — der User kann anschwellen lassen, leise
  starten, hart attackieren. Das Generative-Bed wird **spielbar**, nicht nur
  scheduled
- Visual-Mechanic-Kongruenz: lange Pille sieht aus wie Taste, fühlt sich an
  wie Taste
- Vergleich zu HiChord: gleiche Klasse von Interaktion

**Negative:**
- BOM-Komplexität steigt: FSR (~$2-3/Stk × 5 = $10-15) statt einfache
  Tactile (~$0.10 × 5 = $0.50)
- Mechanik-Aufwand: Silicon-Cap-Tooling, Frame-Design — gehört in Phase 6
  (Layout + Enclosure)
- ADC-Channels: 5 zusätzliche ADC-Inputs am STM32H743 — vorhanden (LQFP100
  hat 16 ADC1-Kanäle plus ADC2/ADC3; siehe SPEC §5)

**Trade-Off-Entscheidung für Prototype-1:**
Beide Pfade dokumentieren. Wenn FSR + Silicon-Cap die Mechanik-Komplexität
für Spin 1 nicht trägt, fallback auf **Dual-Tactile-Switch** (2 Switches,
3-Level-Velocity). Final-Entscheidung beim Layout/Enclosure-Schritt.

## Implementation Plan

| Phase | Was |
|---|---|
| Schematic r18.8 | 5× ADC-Input ergänzen (PA0, PA1, PA2, PA4, PA6 — alle frei laut SPEC §5.4 wenn Encoder auf TIM-QEI bleiben) für FSR-Pfad. Tactile-Pfad bleibt parallel als DNP optional |
| Component Review | FSR-Kandidaten: Interlink FSR 400 (~$3, 5 mm), Tekscan FlexiForce A201 — gegen Verfügbarkeit + Lebensdauer prüfen |
| Mechanical CAD | Silicon-Cap-Profil + Frame-Tooling — gehört in mechanical_coordinates Update |
| Firmware | ADC-Sampling 1 kHz + Velocity-Curve (log-mapping) + Note-On-Trigger an Generative-Bed/Voicing |
| Phase 5 (Bring-Up) | Real-Board-Test: passt Velocity-Curve zum musikalischen Gefühl? |

## Related

- ADR-0008 — LED-XOR-Logik pro Cell (gelb/grün)
- ADR-0007 — Dust-Mesh-Speaker (gleiches Mechanik-/Industrial-Design-Update)
- SPEC §5.4 — ADC-Pins frei für 5× FSR
