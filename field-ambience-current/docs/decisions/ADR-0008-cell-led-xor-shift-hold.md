# ADR-0008: Cell-LED-Logik — Hold/Shift+Hold (Independent Latches, r2)

**Status:** SUPERSEDED → AMENDED r2 (2026-06-14): Independent Latches statt XOR
**Date (Original r1):** 2026-06-11
**Date (Amendment r2):** 2026-06-14

---

## Amendment r2 (2026-06-14) — XOR → Independent Latches

> **User-Einwand:** „Es können eigentlich sowohl grün als auch gelb gleichzeitig
> erkennbar sein. Funktional sinnvoller. Im Shift Mode, dann im Hold Mode. Es
> kann ja beide gleichzeitig geben, rein logisch."

**Entscheidung:** Berechtigt. Die XOR-Wahl von r18.9 war eine Annahme, dass
„eine Cell nur eine Pitch sustainen kann". Tatsächlich sind „Cell hält
Grund-Oktave" und „Cell hält Shift-Oktave" **logisch unabhängige Zustände**:
beide gleichzeitig aktiv = Cell sustainet beide Oktaven simultan =
**Oktav-Stack** (klassischer fetter Ambient-Drone-Effekt).

### Neue Logik (verbindlich, ersetzt die XOR-Regel im Original-Abschnitt unten)

| `hold_base[cell]` | `hold_shift[cell]` | Gelb-LED | Grün-LED | Voicing |
|---|---|---|---|---|
| false | false | aus | aus | Cell sustainet nichts |
| **true** | false | **an** | aus | Cell sustainet Grund-Oktave |
| false | **true** | aus | **an** | Cell sustainet Shift-Oktave |
| **true** | **true** | **an** | **an** | Cell sustainet BEIDE Oktaven (Stack) |

**Tap-Logik:** Hold-Modifier latched + Cell-Tap toggelt nur den vom **Shift-
State** bestimmten Branch (Shift gedrückt → `hold_shift`; sonst → `hold_base`).
Der jeweils ANDERE Branch bleibt unverändert. `Clear` setzt beide Bits auf 0.

### Hardware-Folgen: KEINE

Beide LEDs hingen schon vorher an unabhängigen PCA9685-Kanälen — XOR war reine
Firmware-Logik. Kein Schaltplan-, kein Footprint-, kein BOM-Change.

### Firmware-Folgen — IMPLEMENTIERT (r18.28) ✅

`src/controls.c` + `include/controls.h` (host-getestet, `test/test_controls.c`,
19 Checks):
- State-Modell `hold_base[5]` + `hold_shift[5]` (zwei unabhängige Bit-Arrays)
- Voice-Routing: Cell `i` base → source `i` (0..4); Cell `i` shift → source
  `i+9` (9..13) — kollidiert nicht mit dem Gen-Bed-Source 8
- **`PAD_MAX` 8 → 12** in `include/pad.h` (r18.25, Worst-Case 5 Cells × 2 = 10
  Voices + 2 Headroom); `MAX_SOURCES = 16` reicht
- Tap-Logik: Hold latched + Shift-State wählt Branch, anderer Branch bleibt
- Modifier-Handler (Shift/Hold/Drone/Generate/Clear), Drone+Generate
  forwarden an `engine_set_drone` / `engine_set_generative`
- Momentary-Modus (ohne Hold): Tap = Note an, Release = Note aus
- Der STM32-Button-Handler (`src/hal_h743/main_h743.c`) muss nur noch
  MCP23017-Edges in `controls_modifier()` / `controls_cell_press/release()`
  einspeisen — die ganze Logik liegt schon hardware-unabhängig vor.

### Sim-Folgen

Sofort umgesetzt in `firmware-c-next/tools/display_sim.html` (r18.23): State
ist `{base: bool, shift: bool}` pro Cell, Click toggelt nur den aktiven Branch,
LED-Render zeigt beide unabhängig.

### Audio-Folgen

Bei voller Stack-Belegung (alle 5 Cells × 2 = 10 Pad-Voices simultan) wäre
~6 dB Pegel-Anstieg möglich. Der Master-Soft-Clip in `engine.c` fängt das
schon — kein zusätzlicher Eingriff nötig.

---

## Original (r1, 2026-06-11) — XOR-Wahl, durch r2 SUPERSEDED

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
