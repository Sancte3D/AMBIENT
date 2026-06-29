# ADR-0018: PCB-Layer-Stack — 4-Layer

**Status:** ACCEPTED (2026-06-29)
**Date:** 2026-06-22

> **Locked 2026-06-29.** 4-layer confirmed: a 2-layer board cannot give the
> STM32H743 (LQFP-100) + USB-D± + Class-D/boost switching + I²S audio a
> continuous GND reference, so EMI/USB-compliance/audio-noise would all suffer.
> The 4-layer planes also act as heat spreaders (see the thermal note —
> no ventilation slots needed). **One open pre-fab check:** verify the USB-D±
> differential impedance (~90 Ω) against JLCPCB's real 4-layer stack with their
> impedance calculator before ordering.

## Kontext

Das PCB-Layout ist noch nicht begonnen (`PROJECT_STATUS.md` §3 PCB/BOM).
Bevor Bauteile in KiCad platziert werden, muss der **Layer-Stack** entschieden
sein — er bestimmt was wo geroutet werden kann und welche Signal-Integrity-
Reserven da sind.

Eingangs-Constraints des Designs (aus Schematic-Walkthrough + ADRs):

- **STM32H743 LQFP-100** — 100 Pins auf 14×14 mm, 0,5 mm Pitch. Hohe Pin-
  Dichte → 4 Layer entlasten die Top-Layer-Routing-Komplexität enorm
- **USB-D± Differential** — 90 Ω, kontrollierte Impedanz, *braucht* eine
  durchgehende GND-Referenz-Plane darunter (USBLC6 + USB-C-Stecker)
- **SAI1 I²S an PCM5102A** — BCK 1,4 MHz × Oversampling → ~MHz-Edges,
  Audio-Analog daneben, EMI-Empfindlichkeit
- **PAM8403 Class-D-Switching** — Carrier ~250 kHz, EMI-Quelle. Ferrit +
  Bulk-Cap helfen, aber GND-Plane unter dem Switching-Knoten ist Pflicht
- **TPS61089 Boost** — Switching auf der Akku-Seite, ähnliche EMI-Sorge
- **ST7789 SPI 24–32 MHz** + **PCA9685 + MCP23017 I²C** — Mittel-Frequenz,
  brauchen sauberes Routing aber kein Highspeed-Layer
- **LCD-Backlight-PWM** über Q2 — Mittelfrequenz, gating-Switch

Mit **2 Layer** in einem solchen Design:
- Top-Routing wird dicht, Komponenten sitzen in Lücken
- GND ist ein Flächen-Mosaik durch die Signal-Bahnen, nicht eine Plane →
  Return-Currents verlaufen unkontrolliert → EMI hoch, USB-Compliance
  unwahrscheinlich, Audio knackt-anfällig
- Audio-Brumm-Risiko (Class-D switching koppelt durch GND-Schlitze in
  Analog-Audio)

Mit **4 Layer**:
- Layer 2 = durchgehende GND-Plane → saubere Return-Currents, USB-Diff-Pair
  kontrollierte Impedanz, EMI 10–20 dB besser als 2-Layer
- Layer 3 = Power-Plane (+3V3, +5V) + Signal-Inseln → kurze Power-Vias,
  weniger Spannungsabfall, weniger Last-Coupling über GND
- Top + Bottom frei für Komponenten + Signale ohne Plane-Frickelei

## Optionen

| Variante | Pro | Contra |
|---|---|---|
| A. **2 Layer (Top + Bottom)** | billig, ~$5/Board JLCPCB Standard | EMI-Risiko hoch, USB-Compliance fragwürdig, Audio-Brumm wahrscheinlich, Klasse-D-Switching koppelt |
| B. **4 Layer Standard (Top / GND / Power / Bottom)** ⭐ | saubere GND-Reference, kontrollierte USB-Impedanz, niedrigere EMI, Komponenten beidseitig möglich | +~$5/Board (5er-Run ~+25 $), nichts sonst |
| C. **6 Layer** | top-tier signal isolation | für 100-Pin-MCU + 3–4 mid-freq Schnittstellen Overkill; ~+15 $/Board |

## Entscheidung

**Option B: 4 Layer.**

### Stack

| Layer | Funktion | Begründung |
|---|---|---|
| 1 (Top) | Signal + Bauteile | Standard-Platzierung; hochfrequente Signale (USB, SPI, I²S) primär hier |
| 2 | **GND solid plane** (durchgehend) | Return-Current-Reference für ALLE Signale auf Layer 1 + Layer 3. Niemals splitten. |
| 3 | Power-Plane (+3V3 / +5V) + sekundäre Signale | +3V3 als grosse Fläche, +5V-Insel um PAM8403 + Boost-Output. Frei verbleibend für unkritische Signale (LED-PWM, Button-Lines). |
| 4 (Bottom) | Signal + ggf. Bauteile (Q2, Hall-Sensoren, kleine Passive) | Frei für Crossings + sekundäre Routes |

### Material + Dicke

- **Standard FR-4**, 1,6 mm Gesamt (JLCPCB-Default)
- 1 oz Cu (0,035 mm) inner + outer Layer (JLCPCB-Default für 4-L)
- Impedance-Control nur für USB-D± nötig — JLCPCB-Default-Stack ergibt
  laut deren Impedance-Calculator ~90 Ω differential bei 0,2 mm Trace-Width
  + 0,2 mm Gap auf Top mit GND auf L2 (verifizieren mit JLCPCB-Tool vor
  Bestellung)

### Disziplin-Regeln (Routing-Vorgaben für Schritt 9)

1. **GND-Plane auf Layer 2 NIEMALS schneiden.** Vias überall durchstossbar.
2. **USB-D±** zueinander parallel + gleiche Länge, *direkt über GND-Plane*,
   keine Power-Plane darunter. Kürzest möglich von `J1` zum MCU.
3. **Class-D-Speaker-Ausgänge** (PAM8403 Outputs) am unteren Boardrand,
   *weit weg* von Analog-Audio-Pfad (PCM5102A → J8). Bulk-Cap < 5 mm von
   PVDD-Pin (ADR-0010 §4).
4. **Crystal Y1 + Caps** direkt am MCU, < 3 mm Trace, kein
   schnelles Signal unterm Crystal, lokale GND-Pour.
5. **HSE / SAI / USB / SPI** auf Top-Layer, sodass die Layer-2-GND
   ungebrochen darunter sitzt.
6. **Power-Plane-Übergang** über breite Vias (≥ 0,5 mm Drill) — pro
   100 mA mindestens ein Via.
7. **Audio-Ground vs. Digital-Ground**: NICHT splitten. Eine einzige
   GND-Plane; Analog-Bereiche sitzen über einer ungestörten Section der
   Plane, weit weg von Switchern. Ferrite + lokale Bulk-Caps machen die
   AC-Trennung, nicht ein Plane-Split.

## Consequences

**Positiv:**
- USB-Compliance erreichbar
- Audio sauber (Hall der gleichen Klasse wie Eurorack-Standard)
- Layout-Iteration einfacher (mehr Routing-Reserve)
- EMI-Reserve für FCC/CE falls jemals nötig

**Negativ:**
- **+~$5/Board** bei JLCPCB 5er-Run → +25 $ Materialkosten gesamt. Im
  Verhältnis zur Audio-Endhardware (PCM5102A + PAM8403 + Speaker + Akku)
  vernachlässigbar.
- Reflow/Rework leicht aufwendiger (innere Layer kann man nicht "fixen"
  ohne Re-Spin) — gleichzeitig Grund mehr für saubere Pre-Layout-Reviews

**Bewusst nicht entschieden:**
- Genaue PCB-Außenmaße (siehe MECHANICAL_REQUIREMENTS-Doc, kommt separat)
- Komponenten-Platzierung (Schritt 7 des PCB-Workflows)
- Trace-Widths für Power (kommt aus Schritt 9 / DRC)

## Related

- ADR-0010 — Audio Buffer / SAI-PLL / Class-D-EMI-Disziplin
- ADR-0016 — Power & Sleep (`U9` Load-Switch wird auf dieser Plane geroutet)
- `field-ambience-current/docs/hardware/SCHEMATIC_WALKTHROUGH.md` — was wo
  funktional verbunden ist
- `field-ambience-current/docs/hardware/ERC_DRC_CHECKLIST.md` — wird um
  4-Layer-spezifische Checks erweitert (Plane-Cuts, Via-Stitching)
