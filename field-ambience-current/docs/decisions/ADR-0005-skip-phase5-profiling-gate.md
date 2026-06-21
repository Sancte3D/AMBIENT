# ADR-0005: Phase-5-Profiling-Gate übersprungen — direkt zu Layout-Vorbereitung

**Status:** ACCEPTED (User-Entscheidung 2026-06-11)
**Date:** 2026-06-11
**Supersedes:** Phase-5-Acceptance-Gate aus ADR-0002 (das ADR bleibt, das Gate wird entschärft)

## Context

ADR-0002 etablierte die Migration RP2350 → STM32H743 mit einem **Phase-5-Profiling-Acceptance-Gate** als harte Vorbedingung für jede Layout-Arbeit:

> Bevor irgendjemand PCB-Layout investiert, muss bewiesen sein, dass die Audio-Engine auf real STM32H743-Hardware (< 40 % Block-Zeit Worst-Case, DWT→CYCCNT) läuft.

Das Gate war eine **konservative Reaktion** auf die ursprünglich falsche "Pico 2 reicht knapp"-Einschätzung. Es sollte verhindern, dass dieselbe Methodik-Lücke (Op-Counting statt Messung) noch einmal ein totes Board produziert.

In der Zwischenzeit (Stand r18.6) ist klar:

- Die Audio-Engine läuft bereits **bestätigt** auf RP2350 (Pico 2, Steps 1-11 Hörtest, plus Step-12b auf der Bench)
- STM32H743 hat objektiv **3-4× CPU-Headroom** ggü. RP2350 (480 MHz M7 mit DTCM/ITCM + Double-Precision FPU vs. 150 MHz M33 ohne FPU)
- Alle SAI/DMA/TIM-QEI-Peripherie ist **dedizierte Hardware**, nicht Software-Imitation wie auf dem Pico (PIO)
- Bandbreiten-Budget (SAI @ 44.1 kHz/16-bit/Stereo = 1.4 Mbit/s) ist für H743 zwei Größenordnungen unterhalb der Capability

Der **erwartete CPU-Bedarf** auf H743 ist ~10-15 % (Cross-Linear aus Pico-2-Messung skaliert mit Takt + FPU-Boost). Das ist so weit unterhalb des 40 %-Gates, dass die Wahrscheinlichkeit eines Fail-Closes ≈ 0 ist.

**Kosten des Gates:** Nucleo-H743ZI2 (~25 €) + PCM5102A-Breakout (~5 €) + Firmware-HAL-Abstraktion (mehrere Sessions) + Profiling-Setup. **Zeit-Kosten: Wochen.**

## Decision

**Phase-5-Profiling-Gate wird übersprungen.** Layout-Arbeit (Gate 4+) ist nicht mehr von einem real-hardware Profiling-Beweis blockiert.

Konkrete Konsequenzen:

1. **PCB-Layout darf vor Firmware-Port starten** — Gate 4 ist offen, wenn Gate 2 (ERC) und Gate 3 (Footprint-Verifikation) sauber sind
2. **Firmware-Migration läuft parallel**, nicht als Vorbedingung. HAL-Abstraktion + STM32-Build kommt, aber sie ist nicht mehr der kritische Pfad zur ersten Board-Bestellung
3. **9-Gate-Modell (ADR-0003) bleibt formal gleich** — Gate 4 ist "PCB-Layout vorhanden", nicht "Layout-vor-Profiling-vorbereitet"

## Consequences

**Positive:**
- Direkter Pfad zu Layout-Bereitschaft: **ERC → FP_VERIFY → Mechanical-Update → Layout → DRC → JLC**
- Keine Wochen-Wartezeit auf Hardware-Bestellung
- Parallelisierung: Layout-Arbeit + Firmware-Migration laufen unabhängig
- Geringere Up-Front-Hardware-Kosten (~30 €) gespart

**Negative — bewusst akzeptiert:**
- **Falls die Engine doch nicht passt:** First-Spin-Prototyp läuft nicht mit voller Funktionalität. Mitigation: nur **ein** Prototyp-Spin (5 Boards bei JLC ~30 €, nicht 100), Bring-Up zeigt das Problem; Firmware kann angepasst werden (Polyphony reduzieren, Reverb-Tail verkürzen) oder Layout-Spin 2 mit anderem Chip
- **Pin-Allocation-Fehler:** wenn ein in SPEC §5 gewählter Pin im realen Chip eine Konflikt-Funktion hat, merken wir das erst beim Bring-Up. Mitigation: KiCad-GUI-ERC + manuelle DS12110-Cross-Check der wichtigsten Multiplexings (SAI1, SPI1, I2C1, TIMs für QEI, OTG-FS) — das ist Layout-Vorarbeit und keine Hardware
- **HAL-Bugs:** SAI-Init, DMA-Config, TIM-QEI-Setup, USB-FS-Stack — können erst am realen Board verifiziert werden. Mitigation: STM32CubeMX-Referenz-Code für die Init-Sequenz nutzen; STM32-Beispielprojekte sind robust für die Standard-Peripherie

**Worst-Case-Pfad:**
- Board kommt zurück, Engine zu lahm → Reverb runter, Polyphony runter, Generative-Bed-Rate halbiert. **Das Board funktioniert dann immer noch** als reduziertes Produkt.
- Pin-Konflikt entdeckt → einzelner Layout-Spin-2 mit Pin-Reallocation. Kostet 30 € + 2 Wochen.

Erwartungswert dieser Risiken: deutlich < 6 Wochen Wartezeit aufs Profiling-Gate.

## Was an ADR-0002 bleibt

- Migration auf STM32H743 bleibt richtig
- Pin-Allocation aus SPEC §5 bleibt verbindlich
- Native-Port-Plan-Phasen 2 (HAL-Abstraktion) und 4 (STM32-Firmware) bleiben Arbeitspakete, nur nicht mehr als Vorbedingung für Layout

## Was sich ändert

| Vorher | Jetzt |
|---|---|
| Layout-Start nur nach Phase-5-Profiling-Pass | Layout-Start nach ERC-clean + FP_VERIFY-done |
| Firmware-Migration ist kritischer Pfad | Firmware-Migration ist Parallel-Track |
| Nucleo-Bestellung blockiert Fortschritt | Nucleo optional (kann während Layout-Wartezeit getestet werden) |
| Worst-Case: "Board läuft nicht" | Worst-Case: "Board läuft mit reduzierter Funktionalität" |

## Related ADRs

- ADR-0002 — definiert die ursprüngliche Migration mit dem Gate; bleibt gültig minus Gate
- ADR-0003 — 9-Gate-Manufacturing-Modell, formal unverändert
- ADR-0004 — MIDI bleibt offene Design-Entscheidung (kann mit DNP gelöst werden, blockiert nicht)
