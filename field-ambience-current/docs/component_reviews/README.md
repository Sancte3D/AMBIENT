# Component Reviews — Field Ambience

Konservative, faktenbasierte Reviews aller PCB-Bauteile gegen offizielle
Herstellerdatenblätter und Package Drawings. **Annahme bis zur Verifikation:
jedes Symbol/Footprint ist potenziell falsch.**

## Review-Regeln (Kurzfassung)

1. Quellen müssen offiziell sein (Hersteller-Datasheet, Package Drawing, AN, Eval-Board-Schematic)
2. Distributor-Daten nur ergänzend, niemals als Pinout-/Footprint-Quelle
3. Jede Aussage trägt eine Quelle (Dokument + Revision + Seite/Tabelle)
4. Strikte Trennung: Fakt aus Quelle / Abgeleitet / Ungeklärt / Risiko
5. „Sollte passen" / „vermutlich" / „wahrscheinlich" sind verboten ohne explizite Unverifiziert-Markierung
6. Bestehende Library-Parts dürfen **nicht** blind wiederverwendet werden
7. Status-Werte: `APPROVED` / `APPROVED WITH NOTES` / `BLOCKED` / `REQUIRES SOURCE/CLARIFICATION` / `FAIL`

Volle Regeln: siehe Session-Log-Reviewer-Template (User-Vorgabe 2026-06-08).

## Status-Übersicht (Stand 2026-06-08)

| Ref | Bauteil | Status | Review-Datei | Letzte Prüfung | r18-Änderung? |
|---|---|---|---|---|---|
| U1 | STM32H743VIT6 | 🟡 REQUIRES SOURCE / CLARIFICATION | [U1_STM32H743VIT6.md](U1_STM32H743VIT6.md) | 2026-06-08 (Pilot) | **NEU** |
| Y1 | ABM3-8.000MHZ-D2Y-T | ⏳ noch nicht reviewed | — | — | **NEU** |
| U5 | AP7361A-33ER | ⏳ noch nicht reviewed | — | — | **war DNP, jetzt aktiv** |
| C_VDD\*, VCAP\*, VDDA | Decoupling-Caps | ⏳ noch nicht reviewed | — | — | **NEU** |
| R_BOOT0, R_NRST, C_NRST | Boot/Reset | ⏳ noch nicht reviewed | — | — | **NEU** |
| U3 | PCM5102APWR | ⏳ noch nicht reviewed | — | — | unverändert (SAI-Master statt PIO im MCU) |
| U4 | PAM8403DR-H | ⏳ noch nicht reviewed | — | — | unverändert |
| U2 | MCP23017-E/SS | ⏳ noch nicht reviewed | — | — | unverändert |
| U6 | PCA9685PW | ⏳ noch nicht reviewed | — | — | unverändert |
| U7 | MCP73831T-2ACI/OT | ⏳ noch nicht reviewed | — | — | unverändert |
| U8 | TPS61089RNR | 🔴 SOFORT-FINDING | — | — | **Datenblatt im Repo ist falsche Variante (RNSR statt RNR)** |
| Q1 | DMG2305UX | ⏳ noch nicht reviewed | — | — | unverändert |
| EN1–4 | ALPSALPINE EC11J1525402 | 🔴 SOFORT-FINDING | — | — | **Datenblatt im Repo ist falsches Teil (Bourns PEC11R)** |
| J1 | USB-C TYPE-C-31-M-12 | ⏳ noch nicht reviewed | — | — | unverändert |
| LCD-Modul | ST7789 1.9" 320×170 | ⏳ noch nicht reviewed | — | — | unverändert (Display selbst), SAI-Anschluss neu |
| USB-ESD | USBLC6-2SC6 | ⏳ noch nicht reviewed | — | — | unverändert |
| F1 | Polyfuse 1812L300 | ⏳ noch nicht reviewed | — | — | unverändert |
| Speakers | PUI AS04008PS 8 Ω | ⏳ noch nicht reviewed | — | — | unverändert |
| Battery | LiPo 5000 mAh | ⏳ noch nicht reviewed | — | — | unverändert |

### Status-Legende
- ✅ **APPROVED** — vollständig verifiziert, keine Findings, Wiederverwendung sicher
- 🟢 **APPROVED WITH NOTES** — verifiziert, kleine Hinweise zu beachten
- 🟡 **REQUIRES SOURCE / CLARIFICATION** — Hauptquellen fehlen, Review unvollständig
- 🟠 **BLOCKED** — Findings müssen vor Wiederverwendung gefixt werden
- 🔴 **FAIL / SOFORT-FINDING** — Datenblatt-Mismatch, falscher Part, kritischer Fehler
- ⏳ noch nicht reviewed

## Sofortige Findings (vor Phase 3 zu klären)

### F-1: TPS61089-Varianten-Mismatch
- **Repo-Datasheet:** `kicad/datasheets/TPS61089RNSR.pdf` (RNSR-Variante)
- **SPEC v0.7 §2.2:** `TPS61089RNR` (RNR-Variante, r12-B11 Wechsel wegen JLC-Stock)
- **Wirkung:** RNR und RNSR sind unterschiedliche Packages (RNR = QFN-11 HotRod, RNSR = ähnlich aber andere Pin-Belegung möglich)
- **Aktion:** RNR-Datasheet beschaffen, RNSR-Datei umbenennen oder löschen, Pin-Belegung gegen aktuellen Schematic prüfen

### F-2: EC11-Encoder-Datasheet falsch
- **Repo-Datasheet:** `kicad/datasheets/PEC11R-4215F-S0024.pdf` (Bourns PEC11R)
- **SPEC v0.7 §4:** `ALPSALPINE EC11J1525402` (ALPS, anderer Hersteller)
- **Wirkung:** PEC11R ≠ EC11J — andere Mechanik (Pitch, Schaftlänge), andere Detents, anderer Push-Switch
- **Aktion:** ALPSALPINE EC11J1525402 Datasheet beschaffen, PEC11R-Datei entfernen oder ins `legacy/`-Archiv

## Reihenfolge der nächsten Reviews

Priorisiert nach Risiko + Migrations-Relevanz:

1. **Sofort (Sourcing-Klärung):** F-1 (TPS61089), F-2 (EC11J1525402)
2. **HOCH (neu in r18):** Y1 ABM3-Crystal, U5 AP7361A-LDO
3. **HOCH (Quellenbeschaffung für U1):** ST AN3318, ST UM2407 Nucleo-Schematic, DS12110 §6 + §7
4. **MITTEL (zentral aber unverändert):** U3 PCM5102A (SAI-Kompatibilität), U4 PAM8403, U2 MCP23017, U6 PCA9685
5. **MITTEL (Power-Tree H7):** Decoupling-Caps gegen AN3318 final verifizieren
6. **NIEDRIG (bewährt aus v0.6):** U7 MCP73831, U8 TPS61089 (nach F-1-Klärung), Q1 DMG2305UX, USB-C, ST7789-Modul, USBLC6, Polyfuse, Speakers, Battery
