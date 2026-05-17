# Field Ambience PCB — TODO vor PCB-Layout & JLCPCB-Order

Stand nach PR-#1-Reviews durch ChatGPT (zwei Iterationen). Diese Liste
trackt alle offenen Issues bevor PCB-Layout begonnen werden darf.

## Status-Legende

- ✅ DONE — Fix ist auf PR #1 (Branch `claude/read-start-here-YDlCd`)
- 🟡 IN-PROGRESS — Teilweise adressiert, braucht Verifikation
- 🔴 BLOCKER — Muss vor PCB-Layout erledigt werden
- 🟠 IMPORTANT — Vor JLCPCB-Order erledigen, aber kein PCB-Layout-Blocker
- 🟢 NICE-TO-HAVE — Verbessert Qualität, kein harter Blocker

---

## ✅ Behoben (auf PR #1 HEAD)

### v0.6.3-r3 (Commit folgt nach diesem Push)
- **N2 I²S Series-Resistoren** im pi_sheet: R_BCK, R_LRCK, R_DOUT 33Ω 0603 direkt am Pi-Header
- **I1 Power-Budget** realistisch neu kalkuliert in SPEC v0.6.3-r3 (PAM8403 @ 4Ω = 1.4A)
- **I2 USB-C Power-Negotiation** Decision dokumentiert: Variante A (Firmware-Volume-Clamp) für Prototyp
- **I3 Inrush-Strategie** dokumentiert: Low-ESR Polymer-Cap (PCV1A102MCL1GS) + DNP-NTC-Inrush für Production
- **I4 4-Layer-Stack-Up** überdacht: Signal/GND/**+5V**/Signal (statt +3V3) — +5V ist die Hochstrom-Schiene
- **I5 Mechanical Coordinates** Template-Datei `mechanical_coordinates.md` erstellt mit allen X/Y für Layout
- **I6 BOM-Split** in SPEC v0.6.3-r3: Section A (JLC SMT, ~70 SKUs), B (manuell, ~15 Items), C (TBD)
- **N3 ERC-Report-Workflow** in SPEC dokumentiert

### v0.6.3-r2 (Commit eed6d6b)
- **B4** PAM8403 /SHDN /MUTE Hardware Pull-Downs (R_MUTE_PD, R_SHDN_PD 10k)
- **I7** UART rename: PICO_TX_PI_RX, PI_TX_PICO_RX
- **N4** Titleblock-Rev-Sync v0.6.3
- **N5** Stale comments cleanup

### v0.6.3 (Commit 5ca0bcb)
- **C1 CRITICAL USB-C VBUS/GND** Pin-Numbers per USB Type-C Spec Rev 2.1 korrigiert

### v0.6.2 (Commit d30ab77)
- **B0 BLOCKER PAM8403H** Pinout per PDF im Repo verifiziert + C_VREF Bypass-Cap

### v0.6.1 (Commit cc57af5)
- **A0 PCM5102A** Pinout nach TI Datasheet SLAS859C korrigiert
- **BOM** LCSC-Numbers normalisiert (C107671, C17337)

### v0.6 (urspr. PR)
### v0.6 → v0.6.1: PCM5102A-Symbol nach TI-Datasheet korrigiert
- War: LRCK/BCK/DIN auf Pin 1/2/3 (erfunden)
- Jetzt: TI SLAS859C — CPVDD=1, OUTL=6, AVDD=8, BCK=13, DIN=14, LRCK=15, DVDD=20

### v0.6.1 → v0.6.2: PAM8403H-Symbol nach Datasheet-PDF verifiziert
- War: DS31295-Vermutung (OUTL+/− vertauscht, VREF fehlt, SHDN/PVDD/PGND verdreht)
- Jetzt: `PAM8403H.PDF` im Repo verifiziert — alle 16 Pins korrekt, C_VREF Bypass-Cap auf Pin 8

### v0.6.2 → v0.6.3: USB-C-Receptacle-Pinout nach USB Type-C Spec
- War: VBUS auf A1/A4/B1/B4, GND auf A12/A9/B9/B12 — wäre Short beim ersten USB-Stecken
- Jetzt: GND = A1/A12/B1/B12, VBUS = A4/A9/B4/B9 per USB Type-C Spec Rev 2.1 Table 3-1

### LCSC-BOM-Normalisierung
- C9900003814 (nicht existent) → **C107671** PCM5102APWR
- C84368 (nicht existent) → **C17337** PAM8403H

---

## 🔴 BLOCKER — Muss vor PCB-Layout in KiCad GUI erledigt werden

### B1. PAM8403H SOIC-16 Footprint vs. Symbol verifizieren
- KiCad-Footprint `Package_SO:SOIC-16_3.9x9.9mm_P1.27mm` öffnen
- Pad-Nummerierung 1..16 gegen Datasheet-Pinout vergleichen
- Pin-1-Markierung muss zur Pin-1 des Datasheets passen
- Wenn nicht: anderen Footprint wählen oder custom erstellen

### B2. USB-C Footprint vs. Symbol verifizieren
- Standard-Footprint: `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12`
- Pad A1 muss auf GND-Net mappen, Pad A4 auf VBUS, etc.
- KiCad's eingebautes ERC sollte das fangen, aber explizit prüfen

### B3. Echter KiCad GUI ERC-Lauf + Report ins Repo
- KiCad öffnen → Tools → Electrical Rules Checker → Run
- Report exportieren als `reports/ERC_2026-MM-DD.txt`
- Alle Errors fixen, Warnings dokumentieren oder als acceptable markieren
- ERC-Settings sind in `.kicad_pro` schon streng eingestellt

### B4. PAM8403 /SHDN und /MUTE Hardware-Defaults
- Beide Pins sind ACTIVE LOW (Datasheet bestätigt)
- Während Pico-Reset/Boot floaten GPIOs — Amp könnte un-defined hochfahren
- **Fix:** Pull-Down 10k auf jedem Pin (R_SHDN_PD, R_MUTE_PD) zu GND
  → Default-State = LOW = Amp aus + muted
  → Pico-Firmware zieht aktiv HIGH wenn power-sequencing es zulässt
- BOM-Update: 2× R 10k

---

## 🟠 IMPORTANT — Vor JLCPCB-Order erledigen

### ✅ I1-I6 + N3 — In v0.6.3-r3 als Doc-Updates adressiert (siehe SPEC)

Details siehe Errata-Sektion v0.6.3-r3 in `field_ambience_pcb_SPEC_v0.6.md`.

### Reste/Refinements (low priority, vor PCB-Layout aber doch erledigen):

### I1-orig. Power-Budget realistisch neu rechnen
- PAM8403 @ 2× 4Ω BTL bei 3W out: ~1.4 A nur für Amp (statt 350 mA in v0.6)
- Pi Zero 2 W peak: ~700 mA (CNX-Messung, korrekt in v0.6)
- Pico, OLED, MCP, PCM, Encoder: ~250 mA gesamt
- **Realistic peak: ~2.4 A statt 1.4 A**
- Polyfuse 2A/4A trippt im Worst-Case → höher dimensionieren ODER Volume-Limit firmware

| Last | Idle | Typical Audio | Worst Case (loud 4Ω) |
|---|---|---|---|
| Pi Zero 2 W | 250 mA | 500 mA | 700 mA |
| Pico 2 | 30 mA | 50 mA | 50 mA |
| OLED SSD1322 | 50 mA | 150 mA | 250 mA |
| MCP23017 | 5 mA | 20 mA | 25 mA |
| PCM5102A | 20 mA | 30 mA | 30 mA |
| **PAM8403 @ 4Ω** | 80 mA | 600 mA | **1400 mA** |
| **Total** | **435 mA** | **1350 mA** | **2455 mA** |

### I2. USB-C Power-Negotiation
- 5.1kΩ-CC-Pulldowns = "Sink"-Signal, garantieren keine 3A
- Source kann Default Current (~500 mA), 1.5 A oder 3 A liefern
- Optionen:
  - (A) **Konservativ**: Firmware-Volume-Clamp auf max ~50% → 1.5 A budget ausreichend
  - (B) **Korrekt**: USB-PD-Sink-Controller wie CYPD3177 oder TPS25750 → echte 3A-Negotiation
- Für Prototyp: (A) reicht; für Produkt: (B) empfohlen

### I3. 1000µF Bulk-Cap Inrush-Strategie
- Roher 1000µF Elko hinter USB-C ist hoher Einschaltstrom-Spike
- Polyfuse begrenzt nicht ideal Inrush
- Optionen:
  - NTC-Inrush-Limiter (z.B. CL-130) in Serie mit Bulk
  - Soft-Start Load-Switch (z.B. TPS22810 mit RC-Slew-Control)
  - Bulk-Cap auf 470µF reduzieren + 100Ω Pre-Charge-R + MOSFET-Bypass
- Mindestens: Low-ESR-Spec für Elko (z.B. EEE-FK1A102P)

### I4. 4-Layer Stack-Up überdenken
- Aktuell: Signal / GND / 3V3 / Signal
- Besser für dieses Board: **Signal / GND / +5V / Signal**
  - +5V ist die kritische Hochstrom-Schiene (Pi, Amp, OLED VBAT, USB-In)
  - +3V3 hat nur ~80 mA Last (kann lokale Pours bekommen)
- GND-Plane darf nicht zerschnitten werden unter USB, I²S, Audio

### I5. Mechanische Koordinaten-Tabelle erstellen
Für PCB-Layout zwingend:
- Encoder-Mitten (4×) x/y, Shaft-Höhe
- Cell-Switch-Mitten (5×) x/y, Hot-Swap-Socket-Orientierung
- Modifier-Switch-Mitten (5×) x/y
- OLED-Header J3 x/y + Modul-Active-Area-Position
- Speaker-Mitten J6/J7 x/y, Cutout-Durchmesser
- USB-C Edge-Offset
- Pi Zero 2 W-Position + Keepout (5×30 mm under-PCB)
- Mounting-Holes (4× Standard M3?)
- Component-Height-Zones (was darf wo hoch sein)

Empfohlen: separate `mechanical_coordinates.md` mit Tabelle + DXF
(KiCad kann DXF importieren als Board-Outline + Drill-Marks)

### I6. BOM-Split sauber dokumentieren
Aktuelle Mischung in v0.6 BOM ist nicht "Full PCBA". Tatsächlich:

**JLCPCB SMT-bestückt (ca. 70 Bauteile):**
- Alle Rs, Cs, FB, D1 (USBLC6), D2 (TVS), F1 (Polyfuse), LED
- U2 MCP23017, U3 PCM5102A, U4 PAM8403H
- J1 USB-C, J3 OLED-Header, J4 SWD-Header, J6/J7 Speaker-Headers
- SW11 Reset, SW12 BOOTSEL
- C_BULK 1000µF Elko

**Manuell zu bestücken (du lieferst, mein "DNP" bei JLC):**
- U1 Pico 2 Modul (Pin-Header THT)
- 10× Kailh Choc V2 Hot-Swap Sockets
- 5× Choc V2 Stabilizer
- 2× PUI AS04008PS Speaker
- 4× EC11 Encoder (wenn nicht SMD-Variante)
- OLED ER-OLEDM032-1W Modul
- Pi Zero 2 W Modul + GPIO-Header J2

**TBD (noch zu sourcen):**
- BOOTSEL-Switch-Caps (5)
- Custom MX-Stem-Cell-Caps (Silikon)
- Gehäuse-Schrauben/Schraubdome

### I7. UART Net-Naming disambiguieren
- Aktuell: `UART0_TX` / `UART0_RX` auf beiden Sheets (Pico und Pi)
- Mehrdeutig: TX/RX ist perspektivisch
- Besser:
  - `PICO_TX_PI_RX` (Pico GP0 → Pi GPIO15)
  - `PI_TX_PICO_RX` (Pi GPIO14 → Pico GP1 via R1 1k)
- Trivial-Rename im Generator + Re-Generate

---

## 🟢 NICE-TO-HAVE — Quality-Improvements

### N1. XSMT per Pico-GPIO statt nur Pull-Up
- Aktuell: PCM5102A XSMT ist statisch via R_XSMT pull-up
- Besser: Pico GPIO steuert XSMT (z.B. GP25 oder ein freier ADC-Pin)
- Erlaubt explizite Pop-Suppression beim PCM5102A
- Reserve-Pin in SPEC v0.6 §5: GP26 ist STATUS_LED, GP27/28 sind Amp.
  Kein freier digital-GPIO mehr — nutze MCP23017 reserve-Pin GPA5/GPB5

### N2. I²S Series-Resistoren am Pi
- 22-47Ω series an BCK, LRCK, DOUT direkt am Pi-Header
- Verbessert Signal-Integrity über lange Traces (320mm Board)
- 3× R 33Ω 0603 in Sheet 7

### N3. ERC + DRC Report ins Repo committen
- Nach jedem Major-Fix: `reports/ERC_YYYY-MM-DD.md`
- Format:
  ```
  KiCad Version: X.Y.Z
  Date: YYYY-MM-DD
  Schematic Hash: <git-sha>
  Errors: N
  Warnings: M (akzeptiert: list)
  ```

### N4. Audio-Sheet Titleblock auf v0.6.3 syncen
- Aktuell zeigt audio.kicad_sch noch v0.6.1 im Titleblock
- Generator-Side fix: rev in `body = ...` updaten

### N5. Stale Comments im Generator aufräumen
- Comments in `power_tree_sheet()` und `audio_sheet()` referenzieren teilweise
  noch alte Pin-Numbers oder alte Decoupling-Strategien
- Regelmäßig nach jedem Fix-Commit grep-en + cleanup

---

## Prozess-Regeln für nächste PRs

1. **Niemals nur das `.kicad_sch` patchen** — immer den Generator ändern und re-generieren
2. **Vor jedem Push:** `python3 generate_kicad_project.py` neu laufen lassen
3. **Vor jedem Push:** Analyzer-Status checken (errors/warnings)
4. **Vor jedem Major-Fix:** Datasheet PDF im Repo + Pin-für-Pin gegenprüfen
5. **PR-Description und Commit-Messages müssen die Wahrheit beschreiben** —
   nicht zwischen "geplanter Fix" und "wirklich im File" verwechseln
6. **Bei jedem Pinout-Fix:** Analyzer-`pin_nets`-Dump im Commit-Message zeigen
   als Beweis

---

## Aktueller Analyzer-Status (Commit 5ca0bcb)

```
errors=3 (alle 3 sind VM-001 false-positives für Pico 5V/3V3 domain crossing —
         Heuristik des kicad-happy-Analyzers erkennt nicht, dass Pico GPIOs
         3V3-tolerant sind obwohl VBUS-Pin 5V führt)
warnings=19 (alle non-blocking: DS-002 datasheets dir, NT-001 single-pin nets
            auf intentional NC-pins, RS-001 power-flag-Source-Erkennung)
info=95
total_components=85, total_nets=117
```

KiCad GUI ERC kann zusätzliche Issues finden, die kicad-happy nicht erkennt.
**Daher: B3 ist Pflicht vor PCB-Layout.**
