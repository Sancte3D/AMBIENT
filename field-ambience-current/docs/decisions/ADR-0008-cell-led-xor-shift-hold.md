# ADR-0008: Cell-LED-Logik — Hold/Shift+Hold als XOR (gelb/grün)

**Status:** ACCEPTED (User-Entscheidung 2026-06-11)
**Date:** 2026-06-11

## Context

Pro Cell sollen 2 LEDs den Hold-Zustand der Cell visualisieren — gelb und
grün. Es gibt zwei mögliche Interpretationen, die ein User-Frage waren:

**Variante A (Overlay):**
- Gelbe LED = Hold-Modus aktiv für diese Cell
- Grüne LED = Shift-Modus globalbedingt aktiv
- Beide gleichzeitig an, wenn Shift + Hold beide aktiv

**Variante B (XOR):**
- Gelbe LED = Cell ist gehalten **bei Basis-Oktave**
- Grüne LED = Cell ist gehalten **bei Shifted-Oktave** (oder Voicing-Variante)
- Pro Cell immer nur eine Farbe gleichzeitig

## Decision

**Variante B (XOR).**

Gelbe LED = Hold @ Basis-Oktave.
Grüne LED = Hold @ Shift-Oktave (oder andere Voicing-Variante).
Niemals beide gleichzeitig an einer Cell.

## Begründung

1. **Eindeutigkeit pro Cell.** Eine LED-Farbe = eine eindeutige Zustand-
   Aussage über genau diese Cell. Der User sieht auf einen Blick „Cell 3
   hält die Shifted-Oktave" — kein Quer-Lesen mit Shift-Modifier-LED nötig.

2. **„Shift aktiv" ist globaler Zustand.** Das gehört auf die Shift-Modifier-
   LED selbst (oben in der Modifier-Reihe), nicht repliziert auf jede Cell.
   Sonst hätten 5 Cells × redundante Shift-Anzeige = visuelle Redundanz.

3. **Spielbarkeit.** Beim Live-Spielen will der User schnell sehen, **welche
   Cells gerade auf welcher Oktave halten**. XOR-Anzeige liefert genau das,
   ohne Overlay-Lesen.

4. **Beide-an-Risiko vermieden.** Gelb + Grün gleichzeitig sieht aus wie
   kaputt (oder wie Status-LED-Spam). XOR vermeidet das per Design.

5. **Hardware-Synergie:** 2 LEDs pro Cell × 5 = 10 PCA9685-Kanäle. Plus
   5 Modifier-LEDs + 1 LCD-Backlight = exakt 16/16 — passt **perfekt** auf
   den vorhandenen PCA9685-Treiber. **Kein Hardware-Change nötig.**

## Interaction-Modell

Cell-Tap-Tabelle:

| Zustand davor | Hold-Modifier | Shift-Modifier | Cell-Tap-Aktion | LED danach |
|---|---|---|---|---|
| Off | aus | aus | momentaner Note-On, kein Latch | aus |
| Off | **an** | aus | latch @ Basis-Oktave | **gelb** |
| Off | **an** | **an** | latch @ Shift-Oktave | **grün** |
| Gelb | an | aus | toggle off | aus |
| Gelb | an | **an** | wechseln auf Shift-Oktave | grün |
| Grün | an | aus | wechseln auf Basis-Oktave | gelb |
| Grün | an | **an** | toggle off | aus |
| Beliebig | Clear gedrückt | — | alle Cells off | alle aus |

## Consequences

**Positive:**
- LED-Sprache bleibt monochrom-tauglich (Gelb/Grün als gut unterscheidbare
  Standard-Farben, gut bei Sonnenlicht/Indoor lesbar)
- PCA9685 reicht exakt — keine zusätzliche I/O-Expander-Bestückung
- Firmware-Logik trivial (Cell-State-Enum: off/yellow/green)

**Negative:**
- Vom User-Wunsch „2 LEDs pro Cell" werden in jedem Moment nur 1 von 2 LEDs
  tatsächlich genutzt → halbe LED-Effizienz im strengen Sinne. Akzeptiert:
  die zwei LEDs sind die zwei _möglichen Zustände_, der Anwender liest die
  Farbe, nicht „wieviele leuchten".

**Alternative für die ferne Zukunft:** Bi-Color-LED (Gelb-Grün Common-
Cathode 3-Pin in einem Gehäuse) — würde 5 Kanäle einsparen. Aktuell **nicht**
nötig, weil das Budget exakt passt.

## Implementation

- Generator (`generate_kicad_project.py`): PCA9685-Kanal-Map aktualisieren
  auf 5×2 Cell-LEDs + 5×Modifier-LED + 1×Backlight = 16 Kanäle
- BOM:
  - 5× gelbe 0603 SMD-LED (Standard JLC-Basic)
  - 5× grüne 0603 SMD-LED (Standard JLC-Basic)
  - 1× grüne 0603 SMD-LED für Shift-Modifier
  - 1× gelbe 0603 SMD-LED für Hold-Modifier
  - 3× weiße 0603 SMD-LED für Drone/Generate/Clear
- Firmware: Cell-State `enum { OFF, HOLD_BASE, HOLD_SHIFT }` + Tap-Handler
  laut Tabelle oben

## Related

- ADR-0006 — Cell-Piano-Feel (das ist der Mechanik-Teil; LED ist Visual-Teil)
- ADR-0007 — Dust-Mesh-Speakers (gleicher Industrial-Design-Sprint)
- SPEC §7.2 — PCA9685-Kanal-Zuordnung wird mit dem ADR aktualisiert
