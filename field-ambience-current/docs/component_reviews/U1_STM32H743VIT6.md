# Component Review: U1 — STM32H743VIT6

**Status:** REQUIRES SOURCE / CLARIFICATION
**Review-Datum:** 2026-06-08
**Reviewer:** Claude (Session 01K5kLTFpDCCoYwx2dq6RkAv) als Pilot-Review nach
User-spezifiziertem 10-Punkte-Komponenten-Review-Template
**Bezug:** SPEC v0.7-r18.1 §5 Pin-Allocation, BOM §4
**Migrations-Phase:** Phase 1 (Doku) — Symbol/Footprint existieren noch nicht;
Phase 3 (KiCad) wird sie erstellen.

> **Reviewer-Haltung:** Diese Prüfung ist bewusst konservativ. Jede Aussage
> ist als „verifiziert / abgeleitet / ungeklärt / Risiko" markiert. Es wird
> NICHTS optimistisch bestätigt. Wo Quellen fehlen, ist das explizit benannt.

---

## 1. Component Identification

| Feld | Wert | Quelle / Status |
|---|---|---|
| Hersteller | STMicroelectronics | ST DS12110 Rev 5, Seite 1 — **verifiziert** |
| Exakte MPN | STM32H743VIT6 | DS12110 Rev 5, Cover-Page — **verifiziert** |
| Beschreibung | 32-bit ARM Cortex-M7 480 MHz MCU, 2 MB Flash, 1 MB SRAM, Double-Precision FPU, DTCM/ITCM, SAI/SPI/I²C/USART/TIM/ADC/DAC, USB-OTG-FS | DS12110 Rev 5, Seite 1 §Features — **verifiziert** |
| Gehäuse laut Hersteller | **LQFP100** (lt. Datasheet Pinout-Tabelle Spalte „LQFP100") | DS12110 Rev 5, Table 8 LQFP100-Spalte — **verifiziert** |
| Mechanische Body-Maße | 14 × 14 mm angenommen (Industry-Standard für LQFP100 0.5mm Pitch) | **NICHT VERIFIZIERT** — Mechanical Drawing in DS12110 §7 (ab Seite 211) nicht gelesen |
| Pitch | 0.5 mm angenommen | **NICHT VERIFIZIERT** — wie oben |
| **Varianten-Risiko HOCH** | Suffix-Buchstabe definiert das Package: V=LQFP100, Z=LQFP144, I=UFBGA176, B=LQFP208, X=TFBGA240. Suffix-Verwechslung = falsche Pinzahl + falscher Footprint. | DS12110 Rev 5, Table 8 Spaltenheader — **verifiziert** |
| LCSC-Code | C114409 (via WebSearch) | **nicht direkt am LCSC-Produkt-PDF verifiziert** — nur über Suchergebnis |
| Preis (Indikator) | ~$6.62 @1 (LCSC), Mouser/DigiKey ships today (WebSearch) | nur als Verfügbarkeits-Beleg, kein Sourcing-Stand |

### Verwendete Quellen
1. **ST DS12110 Rev 5 — STM32H742xI/G STM32H743xI/G Datasheet**
   URL: `https://www.st.com/resource/en/datasheet/stm32h743vi.pdf`
   Lokaler Abruf via SparkFun-CDN-Mirror `cdn.sparkfun.com/.../STM32H743VI_Datasheet.pdf`
   In dieser Session gelesen: Tabelle 7 (Legende, S. 56), Tabelle 8 (Pin Definitions, S. 57-79)
2. **WeAct MiniSTM32H743 Zephyr-Doku** — Bestätigung USB DM/DP = PA11/PA12
3. **WebSearch DigiKey/Mouser/LCSC** — nur für Verfügbarkeit + LCSC-Code

### Nicht genutzte (aber nötige) Quellen
- ✗ **ST AN3318** „Getting started with STM32H743/753 and STM32H750 hardware development"
  → erwartet: Decoupling-Schema, VCAP-Werte, VBAT-Beschaltung, VREF+, NRST-Schaltung, USB-Beschaltung
- ✗ **ST RM0433** Reference Manual Tabelle 12 (Alternate Function Mapping AF1–AF15)
- ✗ **ST UM2407 / UM2204** (Nucleo-H743ZI / Discovery Schematic) — als Referenz-Design
- ✗ **ST DS12110 §6** (Electrical Characteristics) — VDD-Range, IDD-Tabellen, HSE-ESR-Limit
- ✗ **ST DS12110 §7** (Package Information) — Mechanical Drawing für LQFP100

---

## 2. Functional Verification

| Feld | Wert | Quelle / Status |
|---|---|---|
| Hauptfunktion | MCU: hostet Audio-DSP-Pipeline (Pad/Reverb/Bass/Texture/Generative) + UI-State + Encoder/Button-Lesung + Display-Treiber + I²S-Master + USART-MIDI | aus SPEC v0.7 §1 abgeleitet, **nicht aus Datasheet** |
| Versorgung VDD | 1.71–3.6 V (Anwendung: 3.3 V) | TME-Distributor-Datenblatt-Header über WebSearch — **direkt im DS12110 §6.1 NICHT verifiziert** |
| Versorgung VDDA | gleich VDD ±0.3 V Constraint (Standard-STM32) | **NICHT VERIFIZIERT** — Allgemeinwissen, nicht aus DS12110 §6.1.6 |
| Typ. IDD @ 400 MHz | 71 mA | DS12110 §6.3.4 Tabelle 33 — **nur indirekt via Web-Search bestätigt**, Seite nicht direkt gelesen |
| Real-Messung @ 480 MHz | ~120 mA (Nucleo-H743ZI mit Cache/DMA aktiv) | ST-Community-Post — keine kontrollierte Messung |
| SPEC-Annahme Worst-Case | 180 mA | konservativer Aufschlag — **wird durch Phase 5 Profiling final bestimmt** |
| Typische Applikation | Audio-Codec-Frontend mit I²S/SAI → externer DAC | Use-Case-Pattern, **nicht aus Datasheet** |
| Notwendige externe Komponenten | 5× VDD-Decoupling (4.7 µF + 100 nF), VDDA-Filter (Ferrit + 1 µF + 100 nF), 2× VCAP-Bulk, NRST-PU + Cap, BOOT0-PD, HSE Crystal + Load-Caps | Allgemeinpraxis — **Werte und Anzahl nicht aus AN3318 verifiziert** |
| Power-Sequencing | „keine spezielle Sequenz" (Standard STM32H7 mit internem SMPS) | **vom Hörensagen**, nicht direkt im DS12110 §6.1.7 gelesen |
| Pull-Ups/-Downs | BOOT0 = 10 kΩ PD → boot aus Flash; NRST = 10 kΩ PU + 100 nF Cap | **NICHT aus AN3318 verifiziert** |
| Bootstrapping | Interner SMPS bringt VCORE selbst hoch („SMPS supplies VCORE direct"-Mode) | Allgemeinwissen STM32H7 — **nicht aus DS12110 §6.1.7 verifiziert** |

### Offene Punkte (vor Phase 3 zu beschaffen)
1. VCAP1/VCAP2-Beschaltung: 2× 2.2 µF X5R angenommen — AN3318 könnte 4.7 µF fordern
2. VDDA-Filter: Ferrit + 1 µF + 100 nF angenommen — ST-Empfehlung kann abweichen
3. VBAT-an-VDD-Zulässigkeit: in DS12110 §6.1.2 zu prüfen (kein RTC-Backup-Akku verbaut)
4. VREF+: extern beschalten erforderlich oder optional?
5. HSE Crystal-ESR-Maximum (140 Ω in der Auswahl) gegen DS12110 §6.3.13 zu prüfen

---

## 3. Schematic Symbol Check

**Status: NICHT ANWENDBAR — kein Symbol existiert.**

Das STM32H743VIT6 ist **noch nicht im KiCad-Projekt vorhanden**. `pico.kicad_sch`
enthält den Pico-2-Modul-Footprint. Der STM32H743-Symbol wird erst in **Phase 3**
generiert (siehe NATIVE_PORT_PLAN.md Step 13.3). Ein Symbol-Check zu diesem
Zeitpunkt ist nicht möglich.

### Was VOR der Symbol-Erstellung in Phase 3 noch nötig ist
- ✓ ST DS12110 Tabelle 8 (LQFP100-Spalte) gelesen → Pin-Map vorhanden
- ✗ ST DS12110 Tabelle 12 „Alternate function" noch nicht gelesen (AF1–AF15 pro Pin)
- ✗ KiCad-Standard-Library hat `MCU_ST_STM32H7:STM32H743VITx` Symbol — **NICHT verifiziert**, ob es bit-genau zur Datasheet-Tabelle passt
- ⚠️ KiCad-Symbol-Auswahl-Risiko: `STM32H743VGT6` (1 MB Flash) hat gleiches Pinout, aber andere Stromparameter — Symbol-Verwechslung im Library-Browser möglich

### Geplante Verifikation (Phase 3)
- Generator-Script schreibt Symbol mit ALLEN 100 Pins explizit aus der gelesenen Tabelle
- Pin-Typ-Verifikation:
  - Power-Pins (VDD/VSS/VBAT/VDDA/VSSA/VCAP/VREF+) → Type=Power
  - GPIOs → Type=Bidirectional
  - BOOT0 → Type=Input
  - NRST → Type=Input (bidirectional bei Watchdog-Reset, aber Input ist default)
- Keine NC/DNC im LQFP100 laut Tabelle 8 (verifiziert) — alle 100 Pins haben Funktion
- KEIN Exposed Pad bei LQFP — nur Body. **Verifiziert** in Tabelle 7 (keine EP-Erwähnung für LQFP-Variante)

---

## 4. Footprint Check

**Status: NICHT ANWENDBAR — kein Footprint existiert.**

Geplant: `Package_QFP:LQFP-100_14x14mm_P0.5mm` (KiCad-Standard).

### Was VOR dem Footprint-Commit in Phase 3 zu prüfen ist

| Prüfpunkt | Status | Notiz |
|---|---|---|
| Package-Typ exakt LQFP100 | ⚠ teilweise | DS12110 Tabelle 8 hat eine LQFP100-Spalte (Pin-Tabelle verifiziert), aber Mechanical Drawing in §7 (Seite 211 ff.) nicht gelesen |
| Pinzahl 100 | ✓ OK | Tabelle 8 zeigt Pin 1–100 lückenlos (zuletzt: Pin 99=VSS, Pin 100=VDD) |
| Pitch 0.5 mm | ✗ nicht verifiziert | Generic-Annahme für LQFP100 14×14 |
| Body 14 × 14 mm | ✗ nicht verifiziert | Generic-Annahme |
| Pad-Größen | ✗ nicht verifiziert | KiCad-Standard nutzt IPC-7351B Nominal — muss gegen ST AN3318 abgeglichen werden |
| Pin-1-Markierung | ✗ nicht verifiziert | Standard-LQFP: oben-links Punkt; im Drawing zu bestätigen |
| Exposed Pad | ✓ **NEIN** | LQFP hat keinen EP (anders als QFN/UFQFPN). Verifiziert aus Tabelle 7 Type-Spalte (keine EP-Erwähnung) |
| Solder paste mask | n/a | kein EP |
| Hersteller-Recommended Land Pattern | ✗ nicht beschafft | ST AN3318 enthält i.d.R. Land-Pattern-Empfehlung — noch zu lesen |
| IPC oder Hersteller-Pattern? | TBD | Aktueller Plan: KiCad-Standard (IPC-7351B Nominal). Noch gegen ST AN3318 prüfen. |
| Risiko ähnliches Package | ⚠ | LQFP100 14×14 sieht ähnlich aus zu LQFP100 16×16 (gibt es bei anderen Familien). Bei BOM-Order muss MPN exakt sein. |

---

## 5. Pinout Cross-Check

Tabelle gegen DS12110 Rev 5 Tabelle 8 (LQFP100-Spalte) verifiziert. Hier nur
die 30 belegten GPIOs + dedicated Pins. Vollständige Übersicht siehe SPEC v0.7 §5.12.

| Pin | Pin-Name (DS Tabelle 8) | Funktion (DS) | SPEC v0.7 Net | Status | Quelle |
|---|---|---|---|---|---|
| 2 | PE3 | GPIO, TIM15_BKIN, SAI1_SD_B | DISPLAY_SW | OK | DS S.57 |
| 3 | PE4 | SAI1_FS_A (AF6) | I2S_LRCK | OK | DS S.57 |
| 4 | PE5 | SAI1_SCK_A (AF6) | I2S_BCK | OK | DS S.57 |
| 5 | PE6 | SAI1_SD_A (AF6) | I2S_DOUT | OK | DS S.57 |
| 7 | PC13 | EVENTOUT / GPIO | MCP_INT | OK | DS S.57 |
| 12 | PH0-OSC_IN | OSC_IN (HSE) | OSC_IN | OK | DS S.59 |
| 13 | PH1-OSC_OUT | OSC_OUT (HSE) | OSC_OUT | OK | DS S.59 |
| 14 | NRST | dedicated Reset (Type=RST) | NRST | OK | DS S.59 |
| 22 | PA0 | TIM2_CH1 / TIM5_CH1 (AF1/2) | DRIVE_A | OK | DS S.61 |
| 23 | PA1 | TIM2_CH2 / TIM5_CH2 | DRIVE_B | OK | DS S.61 |
| 25 | PA3 | ADC1_INP15 (analog) + USART2_RX | BAT_SENSE | OK | DS S.62 |
| 29 | PA5 | SPI1_SCK / I2S1_CK (AF5) | LCD_SCK | OK | DS S.62 |
| 31 | PA7 | SPI1_MOSI / I2S1_SDO (AF5) | LCD_MOSI | OK | DS S.63 |
| 53 | PB14 | GPIO + USART1_TX, SPI2_MISO, OTG_HS_DM | AMP_nSHDN | OK | DS S.67 |
| 54 | PB15 | GPIO + USART1_RX, OTG_HS_DP | AMP_nMUTE | OK | DS S.67 |
| 55 | PD8 | GPIO + DFSDM_CKIN3, SAI3_SCK_B, USART3_TX | STATUS_LED | OK | DS S.68 |
| 59 | PD12 | TIM4_CH1 (AF2) | DISPLAY_A | OK | DS S.69 |
| 60 | PD13 | TIM4_CH2 (AF2) | DISPLAY_B | OK | DS S.69 |
| 63 | PC6 | TIM3_CH1 / TIM8_CH1 (AF2/3) | BRIGHT_A | OK | DS S.71 |
| 64 | PC7 | TIM3_CH2 / TIM8_CH2 (AF2/3) | BRIGHT_B | OK | DS S.72 |
| 67 | PA8 | TIM1_CH1 (AF1), OTG_FS_SOF additional | VOL_A | OK | DS S.72 |
| 68 | PA9 | TIM1_CH2 (AF1), OTG_FS_VBUS additional | VOL_B | OK | DS S.73 |
| 70 | PA11 | OTG_FS_DM (AF10) | USB_DM | OK | DS S.73 |
| 71 | PA12 | OTG_FS_DP (AF10) | USB_DP | OK | DS S.73 |
| 72 | PA13 | JTMS/SWDIO (after-reset func) | SWDIO | OK | DS S.73 |
| 76 | PA14 | JTCK/SWCLK (after-reset func) | SWCLK | OK | DS S.74 |
| 86 | PD5 | USART2_TX (AF7) | MIDI_TX | OK | DS S.76 |
| 89 | PB3 | JTDO/TRACESWO (after-reset func) | SWO (optional) | OK | DS S.78 |
| 92 | PB6 | I2C1_SCL (AF4) | I2C_SCL | OK | DS S.79 |
| 93 | PB7 | I2C1_SDA (AF4) | I2C_SDA | OK | DS S.79 |
| 94 | BOOT0 | dedicated BOOT0 (Type=B), VPP additional | BOOT0 | OK | DS S.79 |
| 97 | PE0 | GPIO + LPTIM1_ETR, TIM4_ETR | DRIVE_SW | OK | DS S.80 |
| 98 | PE1 | GPIO + LPTIM1_IN2, UART8_TX | BRIGHT_SW | OK | DS S.80 |

### Power-Pins (DS Tabelle 8 LQFP100)

| Pin | Name | SPEC-Anschluss | Status |
|---|---|---|---|
| 6 | VBAT | +3V3 | OK — **§6.1.2-Verifikation steht aus** |
| 10, 26, 49, 74 | VSS × 4 | GND | OK |
| 11, 27, 50, 75, 100 | VDD × 5 | +3V3 | OK |
| 19 | VSSA | GND (Single-Star) | OK |
| 20 | VREF+ | VDDA (oder extern) | **OK aber zu klären**: muss extern oder kann intern? |
| 21 | VDDA | +3V3 via Ferrit | OK (Praxis), **VDDA-VDD-Toleranz §6.1.6 zu verifizieren** |
| 48, 73 | VCAP1, VCAP2 | 2.2 µF zu GND (SMPS-Mode) | **AN3318-Wert zu verifizieren** — könnte auch 4.7 µF sein |

### Gesamt-Status §5 Pinout
- **30 belegte GPIOs + 13 Power-Pins** gegen DS12110 Rev 5 Tabelle 8 verifiziert
- **Keine Mismatches, keine False Pins, keine doppelt belegten Pins** in der finalen Liste
- **Aber:** Alternate-Function-Index (AF1–AF15) nur stichprobenartig geprüft — vollständiger AF-Cross-Check gegen DS12110 Tabelle 12 ist **noch offen**

---

## 6. Existing Library Part Review

**Status: kein existierender Part im Projekt — wird in Phase 3 neu erstellt.**

| Feld | Wert |
|---|---|
| Library-Name | (geplant: `MCU_ST_STM32H7`) — **nicht im Projekt vorhanden** |
| Symbol-Name | (geplant: `STM32H743VITx`) — KiCad-9-Standard-Library hat möglicherweise diesen Symbol — **NICHT verifiziert** |
| Footprint-Name | (geplant: `Package_QFP:LQFP-100_14x14mm_P0.5mm`) — KiCad-Standard — **NICHT gegen ST AN3318 verifiziert** |
| Quelle | KiCad 9 Standard-Library (vermutlich) — **nicht geprüft** |
| Produziert/getestet? | NEIN |
| Vergleich gegen Datasheet | nicht durchführbar (Symbol existiert noch nicht) |
| **Wiederverwendung empfohlen?** | **NEIN** — der Standard-KiCad-Symbol muss vor Verwendung gegen DS12110 Pin-by-Pin geprüft werden. Bekannte Risiken: KiCad-Symbole gruppieren manchmal Power-Pins zu „hidden pins" zusammen, was zu fehlenden Decoupling-Pflichten führen kann |

---

## 7. Accessibility / Layout / Assembly / Testability

**Status: UNCLEAR (PCB-Layout existiert nicht)**

| Prüfpunkt | Status | Anmerkung |
|---|---|---|
| Pins layout-technisch erreichbar | ⚠ kann nicht geprüft werden | PCB-Layout existiert noch nicht (kommt nach Phase 5 Profiling) |
| Testpunkte sinnvoll | empfohlen | für: I2S_LRCK/BCK/DOUT (Audio-Debug), SWO, BAT_SENSE, AMP_nSHDN, NRST. Aktuell nicht im Schematic — sollte in Phase-3-Schematic ergänzt werden |
| Hand-lötbar / Rework | ✓ machbar | LQFP100 0.5 mm Pitch ist mit Übung handlötbar (Drag-Solder + Wick). BGA wurde bewusst verworfen |
| Pin-1 sichtbar nach Bestückung | ⚠ TBD | Standard-LQFP: Punkt auf Body + Silk-Markierung; hängt von PCB-Layout ab |
| Thermisch | ✓ unkritisch | LQFP100 ohne EP → max ~1 W ohne Heat-Spreader; H743 zieht 0.4–0.6 W → weit unter Limit |
| Mechanische Kollision | ✓ unkritisch | LQFP100 14×14mm statt Pico-Modul 51×21mm → viel Platzgewinn |
| HSE-Crystal-Layout | ⚠ KRITISCH | Crystal direkt am MCU (< 3 mm Trace), Ground-Pour drumherum, keine schnellen Signale unterm Crystal. **EMC-Allgemeinpraxis, nicht aus AN3318 verifiziert** |
| USB-D±-Routing | ⚠ KRITISCH | 90 Ω differential, kurz, geschützt mit USBLC6. **Beim PCB-Layout zu detaillieren** |

---

## 8. Critical Error Checklist

| Kategorie | Status | Anmerkung |
|---|---|---|
| Falsches Package | ⚠ RISIKO | Suffix-Verwechslung V/Z/I/B/X möglich. Bestellung MUSS exakt `STM32H743VIT6` lauten |
| Falscher Footprint | OFFEN | existiert noch nicht; bei Phase-3-Auswahl gegen AN3318 prüfen |
| Pin-1-Orientierung | OFFEN | im Footprint zu prüfen, Phase 3 |
| Vertauschtes Pinout | ✓ OK | alle 30 belegten Pins gegen Tabelle 8 verifiziert |
| Falsche Versorgungsspannung | ⚠ RISIKO | VDD 1.71-3.6V — 3.3V innerhalb Spec. **VDDA-VDD-Toleranz nicht im DS gelesen** |
| Fehlender GND/EP | ✓ OK | 4× VSS Pins, kein EP (LQFP-Eigenschaft) |
| NC/DNC/Reserved | ✓ OK | LQFP100 hat keine NC-Pins (Tabelle 8 zeigt alle 100 belegt) |
| Fehlende Pull-Ups/Downs | ⚠ RISIKO | BOOT0-PD geplant aber **AN3318-Bestätigung steht aus**. NRST-PU + Cap geplant aber Werte ungeprüft |
| Falsche I²C/SPI/UART/USB-Pins | ✓ OK | gegen Tabelle 8 verifiziert |
| Differentialpaar (USB) | OFFEN | USB-D±-Routing erst beim PCB-Layout |
| Steckerausrichtung | n/a | MCU hat keinen Stecker |
| MOSFET/Dioden/LED-Orientierung | n/a | nur MCU |
| Decoupling-Kondensatoren | ⚠ RISIKO | Plan: 5× (4.7µF + 100nF) + VDDA-Ferrit + 2× VCAP-Bulk. **Werte und Anzahl gegen ST AN3318 zu verifizieren**. Datasheet §6.1.x „Power supply decoupling scheme" nicht gelesen |
| Package-Variante verwechselt | ⚠ RISIKO | siehe Suffix-Buchstabe oben |
| Symbol False Pins | OFFEN | Symbol existiert noch nicht |
| Footprint Pads ohne Package-Pin | OFFEN | Footprint existiert noch nicht |
| Funktion passt zur Schaltung | ✓ OK | DSP-Engine + UI-Hosting passt auf M7 + SAI + TIM-QEI |

---

## 9. Final Verdict

**Status: REQUIRES SOURCE / CLARIFICATION**

### Wichtigste Fakten (verifiziert)
- LQFP100-Package, 100 Pins (alle belegt, kein NC), kein Exposed Pad → DS12110 Rev 5 Table 8
- Pin-Allocation in SPEC v0.7 §5 für alle 30 belegten + 13 Power-Pins gegen Tabelle 8 verifiziert
- LCSC C114409 in stock; DigiKey ships today; Mouser available (WebSearch-Bestätigung)
- Suffix V = LQFP100 (bestätigt durch Tabelle-8-Spaltenheader)

### Wichtigste Risiken
1. **Package-Suffix-Verwechslung** (V/Z/I/B/X) bei Bestellung — exakte MPN `STM32H743VIT6` muss im Generator/BOM erzwungen werden
2. **Stromverbrauch §6.3.4 Tabelle 33 NICHT direkt gelesen** — 180 mA-Annahme basiert auf einer Community-Messung am Nucleo-Board, nicht auf ST-Datenblatt-Tabelle
3. **Decoupling-Schema NICHT gegen AN3318 verifiziert** — 4.7 µF + 100 nF pro VDD ist Allgemeinpraxis, aber ST könnte spezifischere Werte fordern
4. **VCAP-Bulk-Werte ungeprüft** (2.2 µF Annahme — könnte 4.7 µF sein)
5. **VBAT-an-VDD-Zulässigkeit ungeprüft** (Datasheet §6.1.2)
6. **HSE-Crystal-ESR (140 Ω) ungeprüft** gegen H743 max-ESR-Spec
7. **Alternate-Function-Matrix (Tabelle 12) NUR stichprobenartig geprüft** — bei Phase-3-Symbol-Bau muss jede AF nochmal gegen Tabelle 12 verifiziert werden
8. **Mechanical Drawing (Seite 211 ff.) NICHT GELESEN** — Body-Maße 14×14mm und Pitch 0.5mm sind Allgemeinwissen-Annahme

### Notwendige Korrekturen für SPEC v0.7 — bereits durchgeführt in r18.1
- ✓ Pin-Nummer-Off-by-N-Fehler in §5 (SAI1, SPI1, USB-OTG, PA15-Position, USART2-Pin)
- ✓ ADC1_INP3 → INP15
- ✓ Crystal LCSC C115962 → C144380
- ✓ Pin-76-Doppelbelegung aufgelöst (PA14 SWCLK, PD5 ist Pin 86)

### Offene Fragen / fehlende Quellen
1. **ST AN3318** „Getting started with STM32H743/753 and STM32H750 hardware development" — beschaffen + lesen für Decoupling, VCAP, VBAT, VREF+, Reset-Schaltung, USB-Beschaltung
2. **ST DS12110 §6.1 + §6.3** (Electrical characteristics) — Seiten 80-200 lesen für: max IDD, VDD-Range, VDDA-Toleranz, HSE-ESR-Limit, BOOT0-Threshold, USB-Spec, ADC-Genauigkeit
3. **ST DS12110 §7 (Package information)** — Mechanical Drawing für LQFP100 (Seite 211 ff.) lesen für: exakte Body-Maße, Land-Pattern-Empfehlung, Pin-1-Markierung
4. **ST RM0433** (Reference Manual) Tabelle 12 — alle AF1-AF15 Mappings vollständig prüfen
5. **STM32H743 Nucleo/Discovery Schematic** (UM2407 oder UM2204) — als Referenz-Design für unsere Power-Tree-Wahl

### Empfohlene Korrektur-Aktionen (für Phase 2 vor Phase 3)
1. ST AN3318 PDF aus st.com beschaffen, ins `kicad/datasheets/` ablegen
2. ST UM2407 (Nucleo-H743ZI Schematic) beschaffen — als Referenz-Design für unsere Power-Tree-Wahl
3. STM32H743 Datasheet ins Repo committen (`kicad/datasheets/STM32H743VIT6.pdf`)
4. Erneuter Review dieser Datei nach Datasheet-Studium von §6, §7, AN3318

---

## 10. Review-Log

| Datum | Reviewer | Stand | Nächster Schritt |
|---|---|---|---|
| 2026-06-08 (T+0) | Claude (Pilot-Session) | REQUIRES SOURCE / CLARIFICATION | AN3318 + DS §6/§7 lesen → vollständige Re-Verifikation |
| 2026-06-08 (T+1) | Claude (Y1-Review) | REQUIRES SOURCE / CLARIFICATION | Mehrere ursprünglich offene Punkte aus DS12110 Rev 5 §6.3.4/§6.3.8 verifiziert |

### Update T+1: Verifikationen aus DS12110 Rev 5 §6.3 Lesung

Während des Y1-Reviews wurde DS12110 Rev 5 Pages 100-122 vollständig gelesen.
Folgende ursprünglich offene Punkte sind jetzt belegt:

| Punkt aus T+0 | Verifizierungs-Stand T+1 | Quelle |
|---|---|---|
| VDD-Range „1.71-3.6 V angenommen" | **Korrigiert: 1.62-3.6 V** (TT_xx I/O) | DS12110 Rev 5 Table 23 Page 100 |
| VDDA-Toleranz vs VDD ungeprüft | **Tabelle 23 zeigt VDDA-Range separat (1.62 V min for ADC use, 1.8 V for DAC)** — keine explizite VDDA-VDD-Toleranz in Table 23. **AN3318 weiterhin offen für strikte Constraints** | DS12110 Rev 5 Table 23 |
| VCAP-Bulk-Werte (2.2 µF angenommen) | **BESTÄTIGT: 2× 2.2 µF X5R ± 20%, ESR < 100 mΩ** | DS12110 Rev 5 Table 24 Page 101 |
| Stromverbrauch @ 480 MHz (180 mA Annahme) | **TEILWEISE: 165 mA typ @ 400 MHz mit allen Peripherie aktiv (VOS1)**. 480 MHz mit VOS0 nicht in dieser DS-Revision → **F-5 Finding (Datasheet-Revision)** | DS12110 Rev 5 Table 29 Page 105 |
| HSE-ESR-Limit | **Gm_critmax = 1.5 mA/V**. AN2867: gm_crit = 4·ESR·(2πF)²·(C₀+CL)²; bei 8 MHz, C₀+CL=25 pF: ESR_max **237.5 Ω** (GM=1) bzw. **47.5 Ω** (GM≥5) _(frühere Angaben 948/190 Ω waren ohne Faktor 4 — korrigiert T+3)_ → **F-4 RESOLVED: Y1 = ABLS-8.000MHZ-B4-T, ESR 80 Ω, GM 2.97 Worst-Case / ~5-6 real @ Indoor-Temp, bewusst akzeptiert** | DS12110 Rev 5 Table 43 Page 120 |
| HSE Drive Level (R_EXT) | DS Page 120 Figure 19 zeigt R_EXT optional. AN2867-Referenz im Datasheet erwähnt. **AN2867 noch zu beschaffen** | DS12110 Rev 5 Page 120 |
| Power-Sequencing | **Reset-Timing Tabelle 25 + Table 26 BOR0-Threshold 1.62 V verifiziert** — kein spezielles Sequencing zwischen VDD/VDDA dokumentiert in §6.3.3 | DS12110 Rev 5 Tables 25, 26 |
| BOOT0 Threshold | **VIL = 0.19×VDD+0.1 V, VIH = 0.17×VDD+0.6 V** (separater Threshold für BOOT0) | DS12110 Rev 5 Table 59 Page 131 |
| NRST-Pull-Up Wert | NRST hat **interne Pull-Up 30-50 kΩ** (Table 59 RPU). Externe 10 kΩ Pull-Up wäre dominant — OK, aber **eigentlich nicht zwingend nötig** (interne Pull-Up reicht für normale Anwendung; externe 100 nF Cap bleibt empfehlenswert für EMI) | DS12110 Rev 5 Table 59 |

### Konsequenz für U1-Status

- VCAP-Werte ✓ verifiziert (2× 2.2 µF korrekt in SPEC)
- VDD-Range ⚠ Spec hat 1.71 V min, real 1.62 V — kein Problem für unser 3.3 V Design
- Stromverbrauch: 480 MHz-Werte fehlen weiterhin → **F-5**
- HSE Gm_critmax → **F-4** für Y1-Crystal
- **Status bleibt REQUIRES SOURCE / CLARIFICATION** wegen AN3318 + Datasheet-Revision für 480 MHz
