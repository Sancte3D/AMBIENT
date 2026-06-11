# PCB-Layout-Status — Field Ambience

**Stand: 2026-06-11 (v0.7-r18.6)**

## Kurzfassung

**Es existiert KEIN `.kicad_pcb`. Keine Gerber, keine BOM/CPL-Exports, keine Bestellung.** Der Schaltplan ist auf STM32H743 migriert (r18.5) und in r18.6 mit allen verfügbaren öffentlichen Datenblatt-Quellen verifiziert. Layout (= NATIVE_PORT_PLAN Step 13 **Phase 6**) ist bewusst noch nicht gestartet: davor steht das **Phase-5-Profiling-Acceptance-Gate** (DWT->CYCCNT < 40 % Block-Zeit auf echter H743-Hardware).

## Was fertig ist

### r18.5 (Schematic-Migration)
- 8 Sheets generiert, paren-balanced, 100/100 STM32-Pins angebunden, Hier↔Root-Crossref 7/7 PASS
- STM32H743VIT6-Symbol gegen offizielle KiCad-Lib + DS12110 verifiziert (52/52 belegte Pins)
- Komplette Support-Schaltung (HSE, VCAP, VDD-Bank, VDDA-Filter, BOOT0/NRST, SWD, BAT-Teiler, Status-LED)
- LCD-Sheet mit ST7789 + Backlight-Pfad (PCA9685 → 2N7002)

### r18.6 (Engineering-Verifikation, dieser Commit)
- 🟢 **B-LDO geschlossen**: Symbol/Verdrahtung/Variante auf **AP7361C-33Y5-13** umgestellt — User-verifiziertes SOT-89-5-Pinout (1=EN, 2=GND, 3=ADJ/NC, 4=IN, 5=OUT) aus Diodes-DS via mouser.de/datasheet/3/175/1/AP7361.pdf. Die r18.5-Annahme war FALSCH (hätte IN/OUT/EN vertauscht). LCSC **C460397**. Falsche Kandidatur C150719 verworfen (= SOT-223, falsches Package). Cin/Cout 4.7 µF X5R 0603 (DS-empfohlen).
- 🟢 **B-F1 geschlossen**: TPS61089RNR-Pinout im Symbol stimmt mit TI Pin-Functions Table 6-1 (Rev C) überein (1=FSW, 2=VCC, 3=FB, 4=COMP, 5=GND, 6=VOUT, 7=EN, 8=ILIM, 9=VIN, 10=BOOT, 11=SW). Footprint-Name auf RNR0011A 2.0×2.5 mm VQFN-HR umgestellt. Falsches RNSR-PDF → `datasheets/legacy/`.
- 🟢 **B-F2 teilweise geschlossen**: Encoder ist EC11J1525402, **LCSC C209762** (JLCPCB-verifiziert), SMD 15/30 mit Push-Switch. Status **NRND** (ALPS markiert „Not Recommended for New Designs") — als `Status`-Property + ADR-Hinweis. Prototype-OK, Serie braucht eigene Entscheidung.
- 🟢 **B-SW12 geschlossen**: Custom-Footprint `field_ambience:SW_HX_12x12x7.3_SMD-4P` (`libraries/field_ambience.pretty/`), basierend auf User-verifizierten LCSC-Daten (C36498966: SMD-4P, 11.8×11.8 mm, 7.3 mm Höhe, SPST, 250 gf, 12 V/50 mA, 100 k Zyklen). 5 Symbole jetzt mit korrektem FP. `fp-lib-table` erweitert.
- 🟢 **B-TBD geschlossen**: 4× TBD-VERIFY-Caps gefüllt — VCAP `C24539`, VDD-Bulk `C45783`, HSE-22 pF C0G `C1804`, 2N7002 `C8545`. Alle mit `VERIFY-STOCK`-Property statt blanker LCSC-Nr.
- 🟢 **MIDI geklärt**: nicht „fehlend" sondern OFFENE DESIGN-ENTSCHEIDUNG (5 Achsen) — ADR-0004. Generator-Text-Note im STM32-Sheet aktualisiert.
- 🟢 **ERC/DRC-Checkliste**: `docs/hardware/ERC_DRC_CHECKLIST.md` mit bewussten Warnungen + Manufacturing-Gates.

## Verbleibende Blocker

| # | Sev | Was | Was genau zu tun |
|---|---|---|---|
| B-FP | 🟠 | Footprint-Land-Pattern-Verifikation | `FP_VERIFY`-Properties in den Sheets abarbeiten (LQFP-100, HC49-SD-Crystal, SOT-89-5, SOT-23/2N7002, EC11J-SMD, PinHeader_1x08, RNR0011A) gegen die jeweiligen Datasheet-Mechanical-Drawings |
| ADR-0004 | 🟠 | MIDI-Design-Entscheidung | docs/decisions/ADR-0004 — 5 Achsen entscheiden, Generator erweitern (oder als DNP committen) |
| B3 | 🟠 | KiCad-GUI-ERC | Einmal in KiCad 9 öffnen, ERC laufen lassen (kicad-cli aktuell nicht in der Umgebung verfügbar). Erwartete Warnungen / verbotene Warnungen siehe ERC_DRC_CHECKLIST.md |
| GATE | 🔴 | Phase-5-Profiling | DSP-Engine auf echter H743 (Nucleo/Eval) profilen — erst bei PASS lohnt Layout |
| GATE | 🔴 | **Phase 6 Layout** | Erst nach Phase 5 starten |

## Verifikations-Log

- 2026-06-11 r18.5: Struktur-Validierung PASS (paren-balance 10/10, 100/100 MCU-Pins, Crossref 7/7)
- 2026-06-11 r18.6: nach LDO-Pinout-Korrektur und Footprint-/MPN-Updates: alle Skript-Checks wiederholt, PASS

## Manufacturing-Readiness

Vorher (r18.5): 2.5/10. Jetzt (r18.6): **4/10** — Schematic ist nicht nur strukturell, sondern jetzt auch elektrisch verifiziert (LDO-Bestücker-Killer ist weg). Der Weg auf 6: B-FP + ERC. Auf 9: Phase 5 + Layout + DRC + JLC-Exports.
