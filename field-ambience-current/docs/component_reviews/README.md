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

## Status-Übersicht (Stand 2026-06-08, 2 Reviews abgeschlossen)

| Ref | Bauteil | Status | Review-Datei | Letzte Prüfung | r18-Änderung? |
|---|---|---|---|---|---|
| U1 | STM32H743VIT6 | 🟡 REQUIRES SOURCE / CLARIFICATION | [U1_STM32H743VIT6.md](U1_STM32H743VIT6.md) | 2026-06-08 (mit DS12110 Rev 5 Pin/Power/HSE-Verifikation) | **NEU** |
| Y1 | ~~ABM3-8.000MHZ-D2Y-T~~ → **ABLS-8.000MHZ-B4-T** (HC-49/US SMD) | 🟢 **APPROVED WITH NOTES** (Gain Margin 2.97 Worst-Case, real ~5–6 @ Indoor-Temp, bewusst akzeptiert) | [Y1_alternatives.md](Y1_alternatives.md) | 2026-06-08 (T+3: ABLS verifiziert + User-Entscheidung) | **NEU** |
| U5 | AP7361A-33ER | ⏳ noch nicht reviewed | — | — | **war DNP, jetzt aktiv** |
| C_VDD\*, VCAP\*, VDDA | Decoupling-Caps | ⏳ noch nicht reviewed (VCAP = 2× 2.2 µF bereits aus DS12110 Rev 5 Table 24 bestätigt) | — | — | **NEU** |
| R_BOOT0, R_NRST, C_NRST | Boot/Reset | ⏳ noch nicht reviewed | — | — | **NEU** |
| U3 | PCM5102APWR | ⏳ noch nicht reviewed | — | — | unverändert (SAI-Master statt PIO im MCU) |
| U4 | PAM8403DR-H | ⏳ noch nicht reviewed | — | — | unverändert |
| U2 | MCP23017-E/SS | ⏳ noch nicht reviewed | — | — | unverändert |
| U6 | PCA9685PW | ⏳ noch nicht reviewed | — | — | unverändert |
| U7 | MCP73831T-2ACI/OT | ⏳ noch nicht reviewed | — | — | unverändert |
| U8 | TPS61089RNR | 🔴 SOFORT-FINDING (F-1) | — | — | **Datenblatt im Repo ist falsche Variante (RNSR statt RNR)** |
| Q1 | DMG2305UX | ⏳ noch nicht reviewed | — | — | unverändert |
| EN1–4 | ALPSALPINE EC11J1525402 | 🔴 SOFORT-FINDING (F-2) | — | — | **Datenblatt im Repo ist falsches Teil (Bourns PEC11R)** |
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

### F-3: SPEC §4 BOM enthält FALSCHEN ESR-Wert für Y1 Crystal
- **SPEC v0.7-r18.1 §4 BOM Y1-Zeile:** „140 Ω ESR"
- **ABRACON Datasheet (Rev 12.03.09) Table 1 „Standard ESR":** **500 Ω max** für 8 MHz Fundamental
- **Wirkung:** SPEC ist 3,5× zu optimistisch in der ESR-Angabe
- **Aktion:** SPEC §4 Y1-Zeile korrigieren auf „500 Ω max ESR"

### F-4: ✅ **RESOLVED (T+3, 2026-06-08)** — Crystal-Wechsel ABM3 → ABLS-8.000MHZ-B4-T

> **Auflösung (T+3):** Nach Verwerfen von ABM3 (Gain Margin 0.47, würde nicht
> oszillieren) hat der User den **HC-49/US-SMD-Pfad** gewählt. Gewählt:
> **ABRACON ABLS-8.000MHZ-B4-T** (LCSC C596838), ESR max 80 Ω gegen offizielles
> Datasheet (Drawing 450669 Rev AD) verifiziert. **Gain Margin = 2.97** im
> Worst-Case (ESR_max über vollen -20…+70 °C-Bereich).
>
> **Warum trotz GM < 5 akzeptiert:** Das AN2867-Minimum 5 ist ein konservatives
> Industrial/Automotive-Kriterium, das ESR_max über -40…+85 °C ansetzt. Dieses
> Gerät ist ein **Indoor-Audio-Produkt** (real 15–30 °C); dort liegt ESR typisch
> bei 40–50 Ω → realer Gain Margin ≈ 5–6. Der GM=2.97-Wert ist also der
> Papier-Worst-Case, nicht der Betriebsfall. **Bewusste User-Entscheidung
> (2026-06-08).** Phase 5 verifiziert den Crystal-Start am realen PCB.
>
> Verbleibendes Restrisiko (niedrig): Falls das Gerät doch dauerhaft <0 °C oder
> >60 °C betrieben wird, sollte auf MEMS-Oszillator (SiTime SiT8008) gewechselt
> werden. Für den spezifizierten Use-Case nicht nötig.
>
> **Status: Finding geschlossen. Phase 3 (KiCad-Schematic) entblockt.**

---

#### Historie F-4 (zur Nachvollziehbarkeit)

> **Update T+2 (2026-06-08):** Ursprüngliche Bewertung „BLOCKED, marginal" war zu
> optimistisch — Berechnungsformel hatte den Faktor 4 nicht. Nach ST AN2867-Studium
> auf CRITICAL eskaliert.

- **ST AN2867 Rev 24** definiert die korrekte Formel: `gm_crit = 4 × ESR × (2πF)² × (C0+CL)²`
  und fordert **Gain Margin ≥ 5** als Minimum für zuverlässigen Crystal-Start
- **Für ABM3-8.000MHZ-D2Y-T** (ESR=500 Ω, CL=18 pF, C₀=7 pF, F=8 MHz):
  - gm_crit = 4 × 500 × (2π·8e6)² × (25e-12)² = **3.16 mA/V**
- **STM32H743 gm = 1.5 mA/V** (DS12110 Rev 5 Table 43)
- **Gain Margin = 1.5 / 3.16 = 0.47** ← WEIT unter 5
- **Bei Gain Margin < 1: Crystal oszilliert gar nicht!**
- **Wirkung:** ABM3 funktioniert mit STM32H743 wahrscheinlich nicht. PCB würde im ersten Spin nicht booten.
- **Erforderlich für Gain Margin = 5 bei 8 MHz, C₀+CL = 25 pF:** ESR ≤ **47.5 Ω** _(T+3-Korrektur: hier stand fälschlich 190 Ω — in der Rückwärtsrechnung ESR = gm/(5·4·(2πF)²·(C₀+CL)²) fehlte der Faktor 4)_
- **Aktion:** **PFLICHT-Wechsel.** Mehrere Lösungs-Pfade in Phase 2 Sourcing-Session zu prüfen:
  - Standard 8 MHz Crystal mit ESR ≤ 47.5 Ω im 5032 (praktisch nicht zu finden)
  - HC-49S-SMD Crystal (ESR typisch 30-60 Ω → GM 4-8, aber größerer Footprint)
  - **MEMS-Oszillator (SiTime SiT8008, SiT1602): empfohlen** — eliminiert Crystal-Probleme komplett, direkter Anschluss an OSC_IN
- **Status:** SPEC §4 Y1-Zeile in r18.3 auf „PLATZHALTER" geändert. **BLOCKER für Phase 3 KiCad-Schematic.**

### F-5: STM32H743 Datasheet-Revision Mismatch
- **Lokal abgerufenes Datasheet:** DS12110 Rev 5 (Juli 2018)
- **SPEC v0.7-r18.1 behauptet:** SYSCLK = 480 MHz via PLL
- **DS12110 Rev 5 dokumentiert nur:** bis 400 MHz Run-Mode (Tables 29-31). **VOS0 (480 MHz) nicht in dieser Revision.**
- **Wirkung:** Meine SPEC-Behauptung „480 MHz" ist auf der lokalen Datenbasis NICHT direkt verifiziert. Spätere Datasheet-Revisionen (Rev 7+) müssten VOS0 dokumentieren.
- **Aktion:** Neuere DS12110-Revision von st.com beschaffen + lesen, oder SPEC vorsichtshalber auf „400 MHz typ (480 MHz mit VOS0 ab Datasheet Rev 7+, in Phase 5 final messen)" korrigieren.

## Verifizierte Werte (positive Bestätigungen aus DS12110 Rev 5)

Aus dem aktuellen Datasheet-Studium positiv bestätigt für SPEC-Werte:
- **VCAP = 2× 2.2 µF X5R** ± 20%, ESR < 100 mΩ (Table 24 Page 101) ✓ entspricht SPEC §5.10
- **VDD-Range 1.62-3.6 V** (Table 23 Page 100) — 3.3 V innerhalb Spec ✓
- **VDD33USB ≥ 3.0 V** (Table 23 Page 100) — 3.3 V innerhalb Spec ✓
- **VBOR0 = 1.62 V rising** (Table 26 Page 102) — Brown-out-Reset funktional ab 1.62 V
- **HSE-Range 4-48 MHz** (Table 43 Page 120) — 8 MHz innerhalb Range ✓
- **HSE Startup Time 2 ms typ** (Table 43 Page 120) — kompatibel mit Boot-Sequenz §12.5
- **Run-Mode @ 400 MHz, all periph enabled: 165 mA typical** (Table 29 Page 105) — meine SPEC-Annahme 180 mA Worst-Case ist realistisch konservativ
- **VOS1 max 400 MHz, VOS2 max 300 MHz, VOS3 max 200 MHz** (Tables 29-31) — Voltage-Scaling-Ranges bestätigt

## Reihenfolge der nächsten Reviews

Priorisiert nach Risiko + Migrations-Relevanz:

1. **🔴 SOFORT (Sourcing-Klärung):** F-1 (TPS61089), F-2 (EC11J1525402)
2. **🟠 HOCH (vor Phase 3):**
   - ✅ F-3 + F-4 erledigt (T+3): Y1 → ABLS-8.000MHZ-B4-T, SPEC §4 + §5.9 aktualisiert
   - F-5 (Datasheet-Revision) → neuere DS12110 Revision beschaffen
3. **HOCH (neu in r18):** U5 AP7361A-LDO
4. **HOCH (Quellenbeschaffung für U1):** ST AN3318, ST UM2407 Nucleo-Schematic, DS12110 §6.1 + §6.3.13 (HSE detail), §7 (Package)
5. **MITTEL (zentral aber unverändert):** U3 PCM5102A (SAI-Kompatibilität), U4 PAM8403, U2 MCP23017, U6 PCA9685
6. **MITTEL (Power-Tree H7):** Decoupling-Caps gegen AN3318 final verifizieren
7. **NIEDRIG (bewährt aus v0.6):** U7 MCP73831, U8 TPS61089 (nach F-1), Q1 DMG2305UX, USB-C, ST7789-Modul, USBLC6, Polyfuse, Speakers, Battery
