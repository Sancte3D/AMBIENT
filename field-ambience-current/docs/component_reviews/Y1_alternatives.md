# Y1 Crystal-Auswahl: AN2867-Konforme Optionen für 8 MHz mit STM32H743

**Stand:** 2026-06-08
**Bezug:** F-4 Gain-Margin-Fail mit ABM3 (GM=0.47), Y1 BLOCKED für Phase 3
**User-Entscheidung:** HC-49S-SMD-Crystal bevorzugt
**Status:** 🟠 weiter offen — Standard-HC-49S bei 8 MHz erreicht AN2867-Min 5 nicht

## Anforderung (AN2867 Rev 24)

Crystal mit Gain Margin **≥ 5** für STM32H743 mit:
- gm = 1.5 mA/V (DS12110 Rev 5 Table 43, verifiziert)
- F = 8 MHz

**Erforderlich:** ESR_max ≤ 47.5 Ω bei C₀+CL = 25 pF (Standard 18 pF CL + 7 pF C₀)
**Oder:** kleineres C₀+CL erlaubt höheren ESR

## Geprüfte Kandidaten

### 1. ABRACON ABLS-8.000MHZ-B4-T — 🟠 GM = 2.97 (knapp)
- **Verifiziert aus ABRACON Datasheet (Drawing 450669 Rev AD, Sept 2022):**
  - ESR max: 80 Ω (Table 1 für 8.000-8.999 MHz Fund.)
  - C₀ max: 7 pF
  - CL: 18 pF Standard
  - Op-Temp: -20°C ~ +70°C (B-Suffix)
  - Freq Tolerance: ±30 ppm (4-Suffix)
  - Package: HC-49/US SMD, **11.4 × 4.7 × 4.2 mm**
- **LCSC C596838**, ~$0.24
- **Gain Margin (berechnet):** 1.5 / (4 × 80 × 1.579e-6) = **2.97**
- **AN2867-Status:** ❌ unter Minimum 5
- **Real-World:** GM=3 ist Grenzbereich — funktioniert meist, aber Temperatur-Drift + Aging können auf <1 fallen → kein Start
- **Empfehlung:** Nur als Notlösung mit erhöhtem Profiling-Aufwand

### 2. ABRACON ABM3-8.000MHZ-D2Y-T — 🔴 GM = 0.47
- ESR max: 500 Ω (Datasheet Rev 12.03.09 Table 1)
- **Wird mit STM32H743 NICHT oszillieren** (siehe Y1-Review)
- **NICHT verwenden**

### 3. NDK NX5032GA-8MHz — 🔴 GM ≈ 2.2 (laut Web-Recherche, **nicht direkt am Datasheet verifiziert**)
- ESR ~300 Ω, CL ~8 pF, C₀ ~7 pF (laut Web-Search, **Datasheet noch nicht gelesen**)
- LCSC C1986270
- **Vorläufige Berechnung:** mit CL=8, C₀=7: C₀+CL=15 pF, gm_crit = 4×300×2.53e15×2.25e-22 = 0.683 mA/V → GM=2.2
- **Vor Verwendung Datasheet direkt verifizieren**

### 4. MEMS-Oszillator (SiTime SiT8008 / SiT1602) — ✅ EMPFOHLEN
- Aktiver Oszillator, **keine Crystal-Gain-Margin-Probleme**
- 3.3 V Versorgung, direkt an OSC_IN, OSC_OUT bleibt frei
- ±25 ppm typ, deutlich besser als Standard-Crystal
- Sehr klein (2.5×2.0 mm CSP)
- LCSC: SiT8008BCED-13-30E-8.000000 (Beispiel-MPN, **noch zu verifizieren**)
- Preis: ~$1-2 statt $0.20-0.30 für Crystal
- **Eliminiert das gesamte Crystal-Auswahl-Problem**

## Drei mögliche Pfade

### Pfad A: ABLS-8.000MHZ-B4-T akzeptieren (GM=2.97)
- **Pro:** Kein längeres Sourcing, KiCad-Footprint einfach (Standard HC-49S)
- **Contra:** Unter AN2867-Min, Risiko unzuverlässiger Start bei Temperatur-Extremen
- **Mitigation:** Phase 5 Profiling mit Temperatur-Tests (-20°C bis +70°C), R_EXT-Tuning optional
- **PCB-Footprint:** Recommended Land Pattern aus Datasheet Page 3: 5.6 × 2.1 mm Pads, 9.5 mm Spacing

### Pfad B: 8 MHz Crystal mit ESR ≤ 50 Ω suchen
- Weitere Hersteller checken: KDS, NDK NX3225, TXC, Murata
- Custom-ESR-Variante ABLS-R040 (Custom ESR 40 Ω) — möglich aber nicht JLC-Standard-Stock
- **Aufwand:** zusätzliche Sourcing-Session
- **Risiko:** möglicherweise nicht JLC-bestückbar

### Pfad C: MEMS-Oszillator (SiTime)
- **Pro:** Eliminiert alle Crystal-Probleme, deutlich höhere Genauigkeit, kleineres Footprint
- **Contra:** ~5-10× teurer als Crystal, 4-Pin-Symbol/Footprint (anderes als 2-Pin Crystal)
- **PCB-Footprint:** SiTime CSP-4 2.5×2.0 mm — sehr kompakt

## Empfehlung für User-Entscheidung

| | Pfad A (ABLS) | Pfad B (Crystal-Sourcing) | Pfad C (MEMS) |
|---|---|---|---|
| Sicherheit | 🟡 Grenzbereich | 🟢 hoch wenn gefunden | 🟢 sehr hoch |
| Zeitaufwand | 1 Tag (KiCad) | 2-3 Tage (Sourcing+Verify) | 1 Tag (KiCad) |
| BOM-Kosten | $0.24 | $0.30-0.50 | $1-2 |
| Phase-3-Risiko | mittel | klein | klein |
| Phase-5-Risiko (PCB-Test) | hoch (Crystal-Start) | klein | sehr klein |

**Reviewer-Empfehlung:** **Pfad C (MEMS)** — eliminiert das gesamte Risiko-Profil. Für ein Produkt-Design (nicht Hobby-Prototyp) sind die ~$1.50 Mehrkosten ein guter Trade gegen Wochen potentieller Crystal-Start-Probleme.

Aber **User hat HC-49S-SMD gewählt** → Pfad A (ABLS) ist die direkte Umsetzung. **Mit Hinweis auf GM=2.97**.

## ✅ Entscheidung (2026-06-08)

**Gewählt: Pfad A — ABRACON ABLS-8.000MHZ-B4-T (LCSC C596838).**

User-Begründung: das Gerät wird keinen extremen Temperaturen ausgesetzt
(Indoor-Audio, 15–30 °C). Der Worst-Case-GM von 2.97 setzt ESR_max über
-20…+70 °C an; im realen Betrieb liegt ESR bei ~40–50 Ω → realer Gain Margin
≈ 5–6, also AN2867-konform für den tatsächlichen Use-Case. Bewusst akzeptiert.

**Umgesetzt:**
- SPEC §4 BOM Y1-Zeile: PLATZHALTER → ABLS-8.000MHZ-B4-T, C596838
- SPEC §5.9: ESR-Wert auf 80 Ω korrigiert, Gain-Margin-Hinweis + Load-Cap-
  Tuning-Note ergänzt
- README F-4: RESOLVED, Phase 3 entblockt

**Offen für Phase 3 (KiCad):**
- Footprint: HC-49/US-SMD Land-Pattern gegen Datasheet Page 3 verifizieren
  (5.6×2.1 mm Pads, 9.5 mm Spacing). KiCad-Standard `Crystal:Crystal_HC49-U_*`
  prüfen — KiCad hat primär den THT-HC49-Footprint; für die SMD-Variante ggf.
  Land-Pattern aus dem Datasheet selbst zeichnen oder passenden SMD-Footprint
  suchen.
- Symbol: `Device:Crystal` (2-Pin, kein GND-Body).

**Offen für Phase 5 (PCB-Test):**
- Crystal-Start verifizieren (Oszilloskop an OSC_OUT, Sonde-Kapazität beachten).
- Load-Caps gegen gemessene Frequenz justieren (Startwert 22 pF, ggf. 24–27 pF).
- Optional bei Temperatur-Extremen: GM nachmessen, sonst MEMS-Fallback (SiT8008).
