# ADR-0004: MIDI-Out-Hardware — Design-Entscheidung offen

**Status:** OPEN (Design-Entscheidung, kein Engineering-Blocker)
**Datum:** 2026-06-11 (r18.6)
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
