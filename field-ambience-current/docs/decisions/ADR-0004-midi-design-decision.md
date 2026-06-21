# ADR-0004: MIDI-Out — DEFERRED (r18.30)

**Status:** DEFERRED → out of scope für den 5er-Prototyp-Run (User-Entscheidung 2026-06-15)
**Datum (Original):** 2026-06-11 (r18.6)
**Datum (Deferral):** 2026-06-15 (r18.30)

---

## Deferral r18.30 — „vielleicht brauchen wir gar kein MIDI"

User-Entscheidung: MIDI fällt aus dem aktiven Scope. Begründung: das Gerät
ist ein **Standalone-Ambient-Instrument** — der Klang lebt im Gerät selbst,
nicht als „denkender Controller" für externe Synths. Für den 5er-Prototyp-Run
nicht nötig; bei späterem Interesse jederzeit nachrüstbar, weil die Hardware-
Vorarbeit auf der PCB-Seite folgendes konserviert hat:

- **J9 (PJ-320D)**: bleibt im Schaltplan als **DNP** (Do Not Populate).
  Edge-Cutout + Footprint vorhanden — nur die Buchse + die 2× 220 Ω werden
  beim 5er-Run nicht bestückt.
- **PD5 (STM32 USART2_TX) bleibt für MIDI reserviert** (kein anderer Verbraucher).
- **`src/midi.c`** (Logic) + **`src/hal_h743/midi_uart_h743.c`** (Skelett)
  bleiben im Code, werden aber im STM32-Build nicht initialisiert
  (`midi_tx_init()` wird im `main_h743.c` auskommentiert).
- **`engine.c`** sendet keine `midi_send_note_on/off` mehr (waren ohnehin
  optional, hängen am gleichen FIFO).

**Konsequenzen für r18.30:**
- BOM_MASTER §3: J9-Zeile bleibt mit Hinweis „DNP für 5er-Run (ADR-0004
  deferred); Footprint + Cutout konserviert"
- COST_ESTIMATE: 1× PJ-320D weniger pro Board ≈ $0.30 × 5 = $1.50 (Pfennig-
  Ersparnis, real wert wegen reduzierter Bestück-Komplexität)
- 5 Achsen unten bleiben als Future-Entscheidung dokumentiert für den Tag,
  an dem MIDI doch reaktiviert wird.

**Reaktivierung:** Nur ein Re-Spin nötig wenn die 220-Ω-Schaltung anders sein
soll (TRS-Type-A/B, OUT-only, 3V3-direkt — alles im Original-Body unten).
Sonst nur: Buchse + 2× 220 Ω einlöten, `midi_tx_init()` in `main_h743.c`
einkommentieren, fertig.

---

## Original (r1, 2026-06-11) — Design-Entscheidung offen
**Kontext:** Schaltplan hat MIDI_TX-Netz auf PD5 (STM32-USART2_TX, SPEC §5.5). Buchse + Serien-R wurden bewusst NICHT instanziiert.

## Warum diese ADR existiert

Frühere Vorschläge (10R/33R nach MIDI-1.0-3V3-Spec, "TRS Type A") wurden als generisches Audit-Item behandelt. Das ist falsch — das ist eine **echte Produkt-Entscheidung** mit fünf voneinander unabhängigen Achsen, von denen jede die anderen beeinflusst:

| Achse | Optionen | Folgen |
|---|---|---|
| **Topologie** | (a) OUT-only · (b) IN+OUT | (b) belegt USART2_RX (PA3 oder PD6) und braucht 6N137-Klasse-Optokoppler + 220R-Strombegrenzung am Sender. PA3 ist aktuell als ADC1_INP15 für BAT_SENSE belegt — Konflikt; PD6 wäre frei |
| **TRS-Polarität** | Type A · Type B | Inkompatibilität mit Drittgeräten je nach Wahl. Type A ist seit MIDI-Spec-2018 der offizielle Standard, viele Hardware-Synths nutzen aber noch Type B |
| **Logik-Pegel** | 3V3 direkt · 5V via Pull-Up | 3V3 spart Buffer (PD5 direkt → 33R → TRS-Tip), reduziert aber Strom; 5V braucht einen 74AHCT1G125 oder ähnlichen Open-Drain-Buffer |
| **Buchsen-MPN** | PJ-320-Klasse SMT (z. B. C2884109) · TS-2.5mm-SMD · TS-3.5mm-SMD-Switched (für Hot-Detect) | Mechanik / Frontplattenraster — abhängig von der Industrial-Design-Entscheidung über das Frontpanel |
| **Bestückungs-Status** | bestückt · DNP / Future-Expansion | Bei DNP: Pads vorsehen, BOM-Status `DNP`, keine Buchse in CPL |

## Was bewusst NICHT entschieden ist

Erst wenn Topologie + Polarität + Pegel + Buchse + Status alle vier festgelegt sind, sind die exakten Serien-R-Werte ableitbar (MIDI-1.0-3V3: typisch 10 Ω in Reihe nach VCC, 33 Ω vom TX). Vorher Werte raten wäre Pseudo-Engineering.

## Entscheidungs-Prozess

1. Industrial Design + Sound-Design definieren das Use-Case-Szenario (Sync mit anderem Pedal? Externer Sequencer? Modular-Adapter?)
2. Anhand des Szenarios werden die fünf Achsen entschieden
3. Diese ADR wird auf `ACCEPTED` aktualisiert und gibt die finale Schaltung an
4. Generator legt Buchse + R(s) + optional Optokoppler im Audio-Sheet (oder neuem MIDI-Sheet) an

## Status-Auswirkungen

- Manufacturing: **kein Blocker für Schematic-Migration** (Netz existiert, Buchse fehlt). Wäre ein Blocker für „MIDI-bestückte Produktion-Version".
- Phase-5-Profiling: nicht betroffen — USART2-Funktionstest ist mit beliebiger 31250-Baud-Trigger-Leitung möglich
- Phase 6 (Layout): wenn bis Layout-Start keine Entscheidung getroffen ist, MIDI als DNP-Block vorsehen und ADR auf `ACCEPTED (DNP)` setzen
