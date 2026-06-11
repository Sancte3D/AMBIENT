# PCB-Layout-Status — Field Ambience

**Stand: 2026-06-11 (v0.7-r18.5)**

## Kurzfassung

**Es existiert KEIN `.kicad_pcb`. Es können KEINE Gerber, keine BOM/CPL-Exports
und keine Bestellung erzeugt werden.** Der Schaltplan ist mit r18.5 auf die
STM32H743-Architektur migriert (Generator-basiert, strukturell validiert),
aber Layout (= NATIVE_PORT_PLAN Step 13 **Phase 6**) ist bewusst noch nicht
gestartet: Davor steht das **Profiling-Acceptance-Gate** (Phase 5, echte
H743-Hardware, DWT->CYCCNT, < 40 % Block-Zeit Worst-Case).

## Was fertig ist (r18.5)

- 8 Sheets, generiert aus `generate_kicad_project.py`, alle paren-balanced
- STM32H743VIT6-Symbol: 100 Pins, doppelt verifiziert (SPEC §5 ↔ offizielle
  KiCad-Lib, beide aus DS12110 abgeleitet, 52/52 belegte Pins identisch)
- Alle 100 MCU-Pins verdrahtet oder als no_connect markiert (geprüft)
- Hier-Label ↔ Root-Pin-Crossref: alle 7 Child-Sheets PASS
- Support-Schaltung komplett: HSE (Y1 ABLS + 2×22 pF), VCAP 2×2.2 µF,
  5×(4.7 µF+100 nF) VDD, VDDA-Ferrit-Filter, BOOT0-PD, NRST-PU+C, SWD-J4,
  BAT-Teiler, Status-LED, USB-FS an PA11/PA12 (USBLC6 bleibt davor)
- +3V3 kommt jetzt vom U5 AP7361A im power_tree (Pico-SMPS entfällt)
- LCD-Sheet: 8-Pin-ST7789 + Backlight-Pfad (PCA9685-Kanal 12 → 2N7002)

## Harte Blocker VOR Layout-Start (Reihenfolge)

| # | Blocker | Was genau zu tun ist |
|---|---|---|
| B-LDO | 🔴 AP7361A-Pinout unverifiziert | Offizielles AP7361A-Datasheet (diodes.com) beschaffen; SOT-89-5-Zuordnung gegen Symbol prüfen (angenommen: 1=ADJ/NC, 2=OUT, 3=IN, 4=GND/Tab, 5=EN — aus AP7361-DS33626-Lesart). IN/OUT-Tausch = totes Board. Zusätzlich korrekte LCSC-Nr. (SPEC-§4-Eintrag C156144 war ein 910-Ω-Widerstand; Kandidat C150719 = AP7361-33E-13) |
| B-F1 | 🔴 TPS61089-Datasheet falsche Variante | ti.com-Datasheet TPS61089 **RNR** laden, Pin-Belegung gegen `battery.kicad_sch` U8 prüfen; `datasheets/TPS61089RNSR.pdf` ersetzen |
| B-F2 | 🔴 Encoder-Drawing fehlt | ALPSALPINE **EC11J1525402**-Drawing beschaffen; Land-Pattern gegen FP `RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` prüfen (Schaftlänge! Frontplatte!); LCSC-Nr. ermitteln |
| B-SW12 | 🔴 Modifier-Button-FP falsch | SW6-10 (HX 12x12x7.3TPFT-B) sitzen auf ~6-mm-TL3342-Pattern. Eigenen 12×12-SMD-FP nach HX-Datasheet (C36498966) anlegen |
| B-MIDI | 🟠 MIDI-Buchse fehlt | MIDI_TX-Netz (PD5) existiert, aber keine TRS-Type-A-Buchse + Serien-R im Design. SPEC §4 um BOM-Zeile ergänzen (MIDI-1.0-3V3-Spec: 10R an 3V3/Ring, 33R an TX/Tip), audio- oder eigenes Sheet erweitern |
| B-FP | 🟠 Neue Footprint-Namen ungeprüft | Alle Properties `FP_VERIFY`/`FP_MISMATCH` in den Sheets abarbeiten (LQFP-100, HC49-SD-Crystal, SOT-89-5, SOT-23/2N7002, PinHeader_1x08, Choc-Hotswap-Pads = bestehender B0-B2-Blocker) |
| B-TBD | 🟠 BOM-Lücken | Alle `TBD-VERIFY`-LCSC-Felder füllen (2.2 µF VCAP, 4.7 µF VDD, 22 pF C0G, 2N7002, LDO-Caps) — JLC-Stock prüfen |
| B3 | 🟠 GUI-ERC | KiCad-GUI-ERC über das ganze Projekt (headless nicht verfügbar; bekannte Absichts-Warnungen: MIDI_TX/SWO-Single-Label) |
| GATE | 🔴 Phase-5-Profiling | Engine auf echtem STM32H743 (Nucleo/Eval) profilen — erst bei PASS lohnt Layout |

## Layout-Vorbereitungs-Checkliste (wenn Blocker geräumt)

1. `mechanical_coordinates.md` neu festlegen (aktueller Stand ist Pico-Ära:
   Pi-J2/Pico-Positionen obsolet)
2. Stack-Up: 4-Lagen Sig/GND/+5V/Sig (SPEC §9) ins Board-Setup übernehmen
3. Placement-Reihenfolge: U1+Decoupling/VCAP (kürzeste Loops) → Y1 (<10 mm,
   GND-Guard) → USB-Differentialpaar 90 Ω → Audio-Block (PCM→PAM→Speaker,
   Masse-Stern §5.10) → Boost L1-Loop minimal → Encoder/Buttons nach
   Frontplatten-Raster
4. DRC-Profil: JLC 4-Lagen-Standard (Track/Space/Via aus JLC-Capabilities)
5. Erst danach: Gerber + JLC-BOM/CPL-Export

## Verifikations-Log

- 2026-06-11: Struktur-Validierung (Skript): paren-balance OK (alle 10
  Dateien), 100/100 MCU-Pins angebunden, Crossref 7/7 PASS, Root-Label-
  Brücken vollständig paarig. GUI-ERC steht aus (B3).
