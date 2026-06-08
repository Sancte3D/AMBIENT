# Component Review: Y1 — ABM3-8.000MHZ-D2Y-T (HSE Crystal)

**Status:** 🟠 BLOCKED — empfohlen Crystal-Wechsel auf Low-ESR-Variante
**Review-Datum:** 2026-06-08
**Reviewer:** Claude (Session 01K5kLTFpDCCoYwx2dq6RkAv)
**Bezug:** SPEC v0.7-r18.1 §4 BOM (Y1), §5.9 Clock-Source
**Migrations-Phase:** Phase 1 (Doku) — NEU in r18

> **Reviewer-Haltung:** Konservative Quellenprüfung. Findings explizit benannt.
> Kritischer Konflikt mit STM32H743 HSE-Spec identifiziert.

---

## 1. Component Identification

| Feld | Wert | Quelle / Status |
|---|---|---|
| Hersteller | ABRACON Corporation | ABRACON ABM3-Datasheet Rev 12.03.09, Seite 1 — **verifiziert** |
| Exakte MPN | ABM3-8.000MHZ-D2Y-T | ABRACON Datasheet Cover + Naming-Schema Seite 1 — **verifiziert** |
| Beschreibung | Miniature Ceramic SMD Crystal, 5.0×3.2×1.30 mm, 8 MHz fundamental | ABRACON Datasheet Cover — **verifiziert** |
| Gehäuse | **5.0 × 3.2 × 1.30 mm** Keramik-SMD, 2-Pad, hermetisch glasgesiegelt | Outline Drawing Seite 2 — **verifiziert** mit Maßen 0.197"±.008 × 0.13"±.008 × 0.051"max |
| Operation Mode | Fundamental (AT-cut) | Datasheet Standard Specifications Seite 1 — **verifiziert** |
| Frequency | 8.000 MHz | Part-Number-Dekodierung — **verifiziert** |
| Load Capacitance CL | 18 pF (Standard) | Datasheet „Load Capacitance CL: 18 pF (see options)" — **verifiziert** |
| Frequency Tolerance @ 25°C | ±20 ppm (Suffix `2` aus Naming-Schema) | Datasheet Naming-Schema Tabelle „Freq. Tolerance: 2 = ±20 ppm" — **verifiziert** |
| Frequency Stability über Temp | ±30 ppm (Suffix `Y` aus Naming-Schema) | Datasheet Naming-Schema Tabelle „Freq. Stability: Y = ±30 ppm" — **verifiziert** |
| Operating Temperature | **-40°C bis +85°C** (Suffix `D`) | Datasheet Naming-Schema Tabelle „Operating Temp: D = -40°C to +85°C" — **verifiziert** |
| Storage Temperature | -40°C bis +90°C | Datasheet Standard Specifications — **verifiziert** |
| Packaging | Tape and Reel (Suffix `T`) | Datasheet Naming-Schema Tabelle „T = Tape and Reel" — **verifiziert** |
| Shunt Capacitance C0 | 7 pF max | Datasheet Standard Specifications — **verifiziert** |
| Drive Level | 100 µW max, 10 µW typ | Datasheet Standard Specifications — **verifiziert** |
| Aging (First Year) @ 25°C | ±5 ppm max | Datasheet Standard Specifications — **verifiziert** |
| RoHS Compliance | Pb in glass (exempt per RoHS 2002/95/EC Annex 5) | Datasheet Cover — **verifiziert** |
| LCSC | C144380 | LCSC-Web (WebSearch) — **nicht direkt am LCSC-Produkt-PDF verifiziert**, nur über Suchergebnis. Preis ~$0.27 |
| Variantenrisiko | **MITTEL** — Naming-Schema hat 8 Felder. Wenn ein Feld falsch ist, ändert sich z.B. Operating-Temp oder Stability. Verwechslung mit ABM3-8.000MHZ-B2Z-T (Op-Temp B = -20°C bis +70°C, Stability Z = ±50 ppm) wäre möglich | abgeleitet aus Naming-Schema |

### 🔴 FINDING F-3: ESR-Wert in SPEC FALSCH

**Was in SPEC v0.7-r18.1 §4 BOM steht:**
> „Y1 ... 18 pF Load, ±30 ppm Stability, **140 Ω ESR**"

**Was das Datasheet wirklich sagt (Table 1 „Standard ESR"):**
> | Frequency (MHz) | ESR (Ω) max | Operational Mode |
> |---|---|---|
> | **8.000 - 8.999** | **500** | **Fundamental** |

**Wirkung:** Die SPEC behauptet 140 Ω, real sind es **500 Ω max** laut Hersteller. Ein Faktor von 3,5× falsch.

**Ursprung des Fehlers:** Vorherige WebSearch-Antwort (vor diesem Review) gab „ESR of 140 Ohms" als generische ABM3-Charakteristik — die galt nicht für die 8 MHz-Variante.

**Aktion:** SPEC §4 BOM Crystal-Zeile korrigieren auf „500 Ω max ESR".

### Verwendete Quellen
1. **ABRACON ABM3 Datasheet, Revision 12.03.09**
   - URL: `https://datasheet.octopart.com/ABM3-8.000MHZ-D2Y-T-Abracon-datasheet-8488864.pdf`
   - Lokal abgerufen + vollständig gelesen (2 Seiten: Standard Specs + Outline Drawing + Tape/Reel + Reflow)
2. **ST DS12110 Rev 5 STM32H743xI Datasheet** Seite 120 Table 43 „4-48 MHz HSE oscillator characteristics"
3. **WebSearch LCSC** für Verfügbarkeits-Code

### Nicht genutzte (aber nötige) Quellen
- ✗ **ST AN2867** „Oscillator design guide for ST microcontrollers" — explizit im DS12110 Page 120 referenziert. Empfohlen vor finalem Crystal-Commit lesen.

---

## 2. Functional Verification

| Feld | Wert | Quelle / Status |
|---|---|---|
| Hauptfunktion | 8 MHz Quarz als HSE-Clock für STM32H743 → PLL → SYSCLK 480 MHz | abgeleitet aus SPEC |
| Versorgung | passiv (kein Versorgungs-Pin) | Crystal-Charakteristik |
| Max Drive Level | 100 µW max | ABRACON Datasheet — **verifiziert** |
| STM32H743 HSE Current Consumption @ 8 MHz | 0.40 mA typical (mit Rm=30 Ω, CL=10 pF) | ST DS12110 Rev 5 Page 120 Table 43 — **verifiziert** |
| STM32H743 HSE Gm_critmax | **1.5 mA/V** | ST DS12110 Rev 5 Page 120 Table 43 — **verifiziert** |
| STM32H743 HSE Feedback Resistor RF | 200 kΩ typical (intern) | ST DS12110 Rev 5 Page 120 Table 43 — **verifiziert** |
| Typische Applikation | HSE-Clock für STM32-Familie, Eval-Board-Designs | ST DS12110 Figure 19 „Typical application with an 8 MHz crystal" — **verifiziert** |
| Notwendige externe Komponenten | 2× Load-Caps (CL1, CL2), optional R_EXT (external limiting resistor) | ST DS12110 Figure 19, ABRACON Datasheet — **verifiziert** |
| Load-Cap-Berechnung | CL_total = (C1×C2)/(C1+C2) + Cstray. Für CL=18 pF und Cstray=5 pF → C1=C2 = 2×(18-5) = 26 pF → praktisch 22-27 pF | Standard-Formel — **abgeleitet, nicht direkt aus AN2867** |
| Power-Sequencing | n/a (passiv) | — |

### Offene Punkte (vor Phase 3 zu klären)
1. **Load-Cap-Wert exakt:** SPEC sagt „2× 22 pF mit Stray-Korrektur". Aber ABRACON-Standard CL = 18 pF und Standard-Empfehlung von ST AN2867 wäre 22 pF wenn Cstray ~5 pF, oder 27 pF wenn Cstray ~3 pF. **Genauer PCB-Stray-Wert ist erst nach Layout messbar.** Für Phase 3 mit 22 pF Standard arbeiten und in Phase 5 ggf. nachjustieren.
2. **R_EXT (External Limiting Resistor):** ST DS12110 Figure 19 zeigt R_EXT zwischen Crystal und OSC_OUT. Sein Wert hängt von Crystal-Charakteristik ab (Drive-Level-Limitation). Bei Crystal mit Drive-Level 100 µW max wahrscheinlich nicht nötig — aber **AN2867 muss konsultiert werden** um sicher zu sein.

---

## 3. Schematic Symbol Check

**Status: NICHT ANWENDBAR — kein Symbol existiert.**

Crystal ist 2-Pin passiv. KiCad-Standard-Symbol `Device:Crystal_GND24` oder `Device:Crystal` wird in Phase 3 verwendet werden. **Nicht im Projekt vorhanden.**

### Was VOR Symbol-Auswahl zu prüfen ist
- ABRACON CONNECTION-Diagramm (Datasheet Page 2) zeigt: 2 Pads, H-shaped Connection (Pad 1 und Pad 2 elektrisch äquivalent für Crystal). Body ist isoliert, KEIN Ground-Connection.
- KiCad-Standard hat `Crystal` (2-Pin), `Crystal_GND24` (4-Pin mit Body-Ground für SMD-Crystals mit metallischem Body). **ABM3 hat keramisches Body ohne Body-Ground** → `Crystal` (2-Pin) ist korrekt.
- WARNUNG: Bei manchen KiCad-Symbolen sind Pads 1+2 belegt UND 3+4 als Ground. ABM3 hat aber nur 2 Pads — falsches Symbol würde DRC-Errors verursachen.

---

## 4. Footprint Check

**Status: ZU ERSTELLEN — gegen ABRACON Recommended Land Pattern (Datasheet Page 2)**

### ABRACON Recommended Land Pattern (Datasheet Page 2, Outline Drawing)

```
Pad-Layout (Datasheet inch / mm):
- Body: 5.0 ± 0.2 mm × 3.2 ± 0.2 mm
- Pad-Maße jeweils: 2.4 mm × 2.0 mm (laut Outline-Drawing-Maße)
- Pad-Spacing total (Outer Edge to Outer Edge): 6.0 mm
- Inner Edge to Inner Edge zwischen Pads: 2.2 mm
- Pad-Höhe (von Body unten): 2.0 mm
```

### KiCad-Standard-Footprint-Bewertung
- `Crystal:Crystal_SMD_5032-2Pin_5.0x3.2mm` ist KiCad-Standard für dieses Package
- **NICHT geprüft, ob dieser KiCad-Footprint exakt den ABRACON-Recommended-Land-Pattern entspricht**
- IPC-7351B Nominal-Pad könnte abweichen — bei kritischen Anwendungen Hersteller-Pattern bevorzugen

### Footprint-Check Punkte
| Prüfpunkt | Status | Notiz |
|---|---|---|
| Package-Typ 5032 (5.0×3.2mm) | ✓ | Bestätigt: Body 5.0±0.2 × 3.2±0.2 × 1.30max mm |
| Pinzahl 2 | ✓ | Bestätigt: nur 2 Pads, H-shaped Connection |
| Pad-Größen 2.4 × 2.0 mm | ⚠ in Phase 3 verifizieren | KiCad-Standard könnte abweichen, gegen ABRACON-Drawing matchen |
| Pad-Spacing 6.0 mm Outer-to-Outer | ⚠ in Phase 3 verifizieren | wie oben |
| Courtyard | ⚠ KiCad-Standard verwendet IPC-7351B | nominal OK, in Phase 3 nochmal prüfen |
| Pin-1-Markierung | ✓ ABRACON | Schwarzer Punkt auf Body (sichtbar nach Bestückung) |
| Exposed Pad | ✓ NEIN | Keramik-Body, kein EP |
| Solder paste | ✓ Standard | Reflow-Profil im Datasheet vorhanden (Peak 260°C, 10s max) |
| Hersteller-Recommended Land Pattern | ✓ vorhanden (Datasheet Page 2) | KiCad-Footprint muss dagegen geprüft werden |

---

## 5. Pinout Cross-Check

Crystal hat 2 elektrisch äquivalente Pads. Keine "Pinout"-Tabelle im klassischen Sinne — nur Verbindung zu MCU OSC_IN/OSC_OUT.

| Pad Nr. | Datasheet-Bezeichnung | Funktion | Schematic-Anschluss laut SPEC | Status |
|---|---|---|---|---|
| 1 | Crystal Terminal 1 | Quarz-Anschluss (elektrisch symmetrisch zu Pad 2) | OSC_IN (STM32H743 Pin 12 = PH0-OSC_IN) | OK |
| 2 | Crystal Terminal 2 | Quarz-Anschluss (elektrisch symmetrisch zu Pad 1) | OSC_OUT (STM32H743 Pin 13 = PH1-OSC_OUT) | OK |

**Hinweis:** Da Crystal-Pads symmetrisch sind, ist die Reihenfolge OSC_IN/OSC_OUT nicht kritisch — funktional spielt es keine Rolle, welches Pad an OSC_IN vs OSC_OUT geht. Aber für **konsistente Pin-1-Markierung im PCB-Layout** sollte eine konventionelle Reihenfolge eingehalten werden.

---

## 6. Existing Library Part Review

**Status: kein existierender Part im Projekt.**

| Feld | Wert |
|---|---|
| Library-Name | (geplant: `Device` für Symbol, `Crystal` für Footprint) — **nicht im Projekt vorhanden** |
| Symbol-Name | (geplant: `Device:Crystal`) — 2-Pin-Standard |
| Footprint-Name | (geplant: `Crystal:Crystal_SMD_5032-2Pin_5.0x3.2mm`) — KiCad-Standard |
| Quelle | KiCad 9 Standard-Library |
| Produziert/getestet? | NEIN |
| Wiederverwendung empfohlen? | **NUR NACH KORREKTUR**: KiCad-Footprint muss gegen ABRACON-Datasheet-Land-Pattern verifiziert werden in Phase 3 |

---

## 7. Accessibility / Layout / Assembly / Testability

| Prüfpunkt | Status | Anmerkung |
|---|---|---|
| Pins erreichbar | ✓ trivial | 2 Pads, einfaches Layout |
| Testpunkte | ⚠ empfohlen | OSC_OUT-Testpunkt für Crystal-Funktionsprüfung mit Oszilloskop. **Aber:** Sonde-Kapazität (5-10 pF) verändert Frequenz — nicht im Normalbetrieb anschließen |
| Hand-lötbar / Rework | ✓ einfach | 2-Pad SMD, große Pads, gut zugänglich |
| Pin-1 sichtbar | ✓ | Schwarzer Punkt auf ABRACON-Body |
| Thermisch | ✓ unkritisch | Max Drive 100 µW → keine signifikante Wärme |
| Mechanische Kollision | ✓ trivial | 5.0×3.2 mm → kleines Footprint |
| **KRITISCH: Crystal-Layout** | 🟠 BEACHTEN | Crystal **muss < 3 mm Trace vom MCU** platziert werden, Ground-Pour drumherum, keine schnellen Signale unter dem Crystal, Load-Caps direkt am Crystal. **EMC-Allgemeinpraxis, AN2867-Empfehlung lesen** |
| Stray-Capacitance auf PCB | ⚠ BEACHTEN | Beeinflusst tatsächliche Load-Capacitance — beim Layout berücksichtigen |

---

## 8. Critical Error Checklist

| Kategorie | Status | Anmerkung |
|---|---|---|
| Falsches Package | ✓ OK | 5032 2-SMD bestätigt für ABM3-Serie |
| Falscher Footprint | ⚠ TBD | KiCad-Standard nicht gegen ABRACON-Drawing verifiziert |
| Pin-1-Orientierung | ✓ trivial | Crystal-Pads sind symmetrisch |
| Vertauschtes Pinout | n/a | symmetrisch |
| **Falsche Versorgungsspannung** | n/a | passiv |
| **Drive-Level-Überschreitung** | ⚠ MITTEL | STM32H743 HSE-Driver kann bei Crystal mit niedrigem Drive-Level (100 µW max ABM3) übersteuern. **R_EXT könnte nötig sein** — gegen ST AN2867 prüfen |
| **🔴 ESR-Konflikt mit STM32H743** | ⚠ **MARGINAL** | Crystal hat 500 Ω max ESR (Datasheet). STM32H743 Gm_critmax 1.5 mA/V → theoretisch max ESR ~948 Ω, aber Best-Practice 5×-Margin → 190 Ω empfohlen. **500 Ω liegt unter theoretischem Limit, aber DEUTLICH über dem 5×-Sicherheits-Margin.** Bei Temperatur-Drift oder Toleranz könnte Crystal nicht zuverlässig starten. |
| Falsche I²C/SPI/UART/USB-Pins | n/a | — |
| Decoupling-Kondensatoren | n/a | Load-Caps sind keine Decoupling |
| Package-Variante verwechselt | ⚠ Risiko | Naming-Schema hat 8 Felder, Verwechslung anderer Suffix-Kombinationen möglich |
| Symbol False Pins | ⚠ TBD | KiCad-Symbol muss 2-Pin sein, nicht 4-Pin GND-Variante |
| Footprint Pads ohne Package-Pin | ⚠ TBD | falsches Symbol mit 4 Pads würde DRC-Fehler verursachen |
| **Funktion passt zur Schaltung** | ✓ OK | 8 MHz Crystal für HSE in STM32H743-PLL ist Standard-Anwendung |

---

## 9. Final Verdict

**Status: 🟠 BLOCKED** — empfohlene Aktion: Crystal-Wechsel auf Low-ESR-Variante

### Wichtigste Fakten (verifiziert)
- ABRACON ABM3-Serie ist eine etablierte Keramik-SMD-Crystal-Familie
- Package 5.0×3.2×1.30 mm 2-SMD bestätigt aus Datasheet
- Frequency Stability ±30 ppm, Tolerance ±20 ppm bestätigt aus Naming-Schema
- Operating Temperature -40°C bis +85°C bestätigt — ausreichend für portables Audio-Gerät
- Load Capacitance 18 pF Standard → 2× ~22 pF Load-Caps korrekt
- LCSC C144380 (preisgünstig, ~$0.27)

### Wichtigste Risiken

1. **🔴 SPEC v0.7 §4 BOM enthält FALSCHEN ESR-Wert (140 Ω)** — echter Wert ist **500 Ω max** laut Datasheet. **Korrektur in SPEC nötig.**

2. **🟠 ESR-Sicherheits-Margin marginal**: STM32H743 Gm_critmax = 1.5 mA/V erlaubt theoretisch bis ~948 Ω ESR, aber Best-Practice 5×-Margin fordert ≤190 Ω. ABM3 mit 500 Ω **liegt 2,6× über dem empfohlenen Margin**. Risiko: Crystal-Start kann bei Temperatur-Drift oder Lot-Variationen unzuverlässig werden. **EMPFEHLUNG: Wechsel auf Low-ESR-Crystal (z.B. ABRACON ABM8G-8.000MHZ mit ~50 Ω, oder ECS-80-18-30B mit ~60 Ω)**.

3. **🟡 R_EXT (External Limiting Resistor) ungeprüft** — ST DS12110 Figure 19 zeigt R_EXT zwischen Crystal und OSC_OUT. Wert hängt von Drive-Level ab. AN2867 muss konsultiert werden.

4. **🟡 KiCad-Footprint ungeprüft** — Standard-IPC-Pad könnte vom ABRACON-Recommended-Land-Pattern abweichen. In Phase 3 verifizieren.

### Notwendige Korrekturen für SPEC v0.7

| # | Was | Wo |
|---|---|---|
| 1 | ESR-Wert 140 Ω → **500 Ω max** (Datasheet-Korrektur) | §4 BOM Y1-Zeile |
| 2 | Operating Temperature explizit erwähnen: -40°C bis +85°C | §4 BOM Y1-Zeile |
| 3 | Datasheet-Quelle dokumentieren | §4 BOM Y1-Zeile |
| 4 | **EMPFEHLUNG: Crystal-Wechsel auf Low-ESR-Variante** für Produktstabilität | §4 BOM Y1-Zeile + Diskussion in CHANGELOG |
| 5 | R_EXT optional vorsehen (Footprint im Schematic) | §5.9 Clock-Source |

### Empfohlene Alternative Crystals (Low-ESR, JLC-stockable, 8 MHz)

| MPN | ESR max | Load Cap | Package | Preis (LCSC) | Quelle |
|---|---|---|---|---|---|
| **ABRACON ABM8G-8.000MHZ-B4Y-T** | ~50 Ω | 18 pF | 3225 (3.2×2.5 mm) | TBD | **noch zu verifizieren** |
| **TXC HC-49S-8M** | ~30 Ω | 18 pF | HC-49S (TH) | TBD | nur als Vergleich, kein SMD |
| **ECS-80-18-30B** | ~60 Ω | 18 pF | 5032 | TBD | **noch zu verifizieren** |
| **NDK NX5032GA-8.000M-STD-CRG-1** | ~30 Ω | 18 pF | 5032 | TBD | **noch zu verifizieren** |

**Alle 4 Alternativen sind NICHT in dieser Session gegen Datasheet verifiziert.** Phase 2 Sourcing-Session erforderlich.

### Offene Fragen / fehlende Quellen
1. ST AN2867 „Oscillator design guide for ST microcontrollers" — beschaffen für R_EXT, Load-Cap-Berechnung, Drive-Level-Limitation
2. Low-ESR-Crystal-Alternativen (mind. 3) gegen JLC-Stock prüfen
3. KiCad `Crystal:Crystal_SMD_5032-2Pin_5.0x3.2mm` Footprint exakt gegen ABRACON-Pattern verifizieren

---

## 10. Review-Log

| Datum | Reviewer | Stand | Nächster Schritt |
|---|---|---|---|
| 2026-06-08 | Claude | BLOCKED (ESR-Konflikt mit H743, SPEC-Fehler ESR=140Ω) | SPEC §4 fixen + Low-ESR-Alternative recherchieren |
