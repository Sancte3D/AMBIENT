# SOURCING & COMPLIANCE — Field Ambience

Lebendes Dossier für **Beschaffung + EU/DE-Zulassung + Kosten** auf dem Weg
zum Verkauf. Ziel: jede Zeile ist entweder 🟢 (verifiziert & bestellbar),
🟡 (offener Check) oder 🔴 (Blocker). Trust `PROJECT_STATUS.md` für den
Engineering-Stand, dieses Doc für „darf/kann ich es verkaufen".

> **Kein Rechts-/Steuerrat.** Dies ist ein belastbarer Fahrplan des
> Rahmens. Finale EMV braucht ein akkreditiertes Labor; Schwellen/Steuern
> gehören zum Steuerberater. Stand: 2026-07.

---

## 1. Beschaffungs-Status (BOM)

### SMD-Teile — der einfache, grüne Teil
Alle SMD-Parts sind über **LCSC-`C`-Nummern** spezifiziert ⇒ als
**JLC-PCBA-Assembly** in einem Rutsch bestellbar (nicht als LCSC-Einzelkauf).
SMD-BOM ~**$21,30/Board** (@10+). Status **🟢** — das ist der Teil, den man
„easy anfordern" kann.

| Kern-IC | LCSC | ~$/St | Status |
|---|---|---|---|
| STM32H743VIT6 | C… | 8,68 | 🟢 |
| PCA9685 (U6) | C… | 2,93 | 🟢 |
| MCP23017 | C… | 1,62 | 🟢 |
| PCM5102A DAC | C… | 0,99 | 🟢 |
| BQ24074 Power-Path-Charger (r19.18) | C54313 | 2,24 | 🟢 |
| TPS61089 Boost | C… | 0,50 | 🟢 |
| PAM8403 Amp | C… | 0,44 | 🟢 |
| TPS22918 Load-Switch | C131941 | 0,25 | 🟢 |
| USB-C | C165948 | 0,17 | 🟢 |
| JST-PH S2B-PH-SM4-TB | C295747 | 0,10 | 🟢 |
| ~110 R/C-Passives + LEDs + Header | — | ~1,3 | 🟢 |

> ⚠ **Stock-Wachhund:** vor jedem Run LCSC-Bestand der „Extended"-Teile
> prüfen (der Tantal C444831 war zeitweise auf ~121 St.). Bei 0 Stock:
> pin-/footprint-kompatiblen Ersatz suchen und **gegen Datenblatt
> verifizieren** (CLAUDE.md-Regel), nicht blind tauschen.

**Kailh-Choc-Notiz (r19.3):** LCSC C400229 ist oft 0 Stock — egal, die
Zelle ist als **direkt gelöteter THT-Schalter** eh Hand-Montage. Jeder
**Kailh Choc V1 (PG1350-Familie)** passt mechanisch auf den Footprint
`SW_KailhChoc_CPG1350_THT_2P`; für den „Tastatur-Tasten"-Griff eine
**taktile** Variante (Choc Brown) wählen. Feste DE-Quelle:
**keycapsss.com** (1–3 Tage DE-Lieferung), EU-Alternativen 42keebs.eu /
splitkb.com. Für die RoHS-Akte die **Kailh-Herstellererklärung** anfordern
(Hobby-Shops liefern selten ein eigenes Zertifikat).

**Display-Notiz (r19.3):** Feste Quelle **Eckstein Shop (DE), 13,95 €** —
exakt das ST7789V2-170×320-Modul, das der Schaltplan referenziert
(exp-tech / BerryBase / Amazon.de als Alternativen). Die **Modul-Pin-Order
weicht von J3 ab** (J3 = VCC/GND/SCK/MOSI/RES/DC/CS/BLK) → **immer per
8-Draht-Kabel Draht-für-Draht mappen** (Design-Option (b), schon
vorgesehen), nie als Steck-Header. Mechanische Maße = Arons Layout.

### Off-Board / Handteile — hier liegen Kosten UND Doku-Pflicht
Diese Teile kommen NICHT von JLC, werden von Hand montiert, und für den
**Verkauf** brauchen die energie-/akkuführenden die Sicherheitsdoks.

| Teil | Quelle | ~$/St | Doks fürs Inverkehrbringen | Status |
|---|---|---|---|---|
| 4× ALPS EC11-Encoder | LCSC C202365 | 1,91 | RoHS | 🟢 |
| 5× Kailh Choc V1 (PG1350) | **keycapsss.com (DE)** / 42keebs.eu / splitkb | ~1 | RoHS (Kailh-Herstellererklärung anfordern) | 🟢 s. Notiz |
| 5× HX B3F Modifier | LCSC C36498965 | 0,06 | RoHS | 🟢 |
| 2× Speaker CMS-402811-28SP | DigiKey | 3 | RoHS | 🟢 |
| Display Waveshare 1.9″ ST7789V2 | **Eckstein (DE) 13,95 €** / exp-tech / BerryBase | 14 | RoHS (Waveshare); **immer per 8-Draht-Kabel** an J3 | 🟢 s. Notiz |
| **LiPo 2000 mAh** | **s. §2** | 8 | **UN38.3 + IEC62133 + PCM + RoHS** | 🟡 s. §2 |
| USB-Netzteil (Beipack, optional) | zertifiziert kaufen | — | **muss selbst CE/LVD tragen** | 🟡 fertig-zert. wählen, nie selbst bauen |
| Schrauben/Mesh/Standoffs | Reichelt/Ali | 2 | — | 🟢 |

---

## 2. Akku-Entscheidung (verifiziert)

**Board erwartet:** JST-PH 2.0, **+ auf J9 Pin 1 (`BAT_PLUS`)**, Ladestrom
500 mA (R_PROG 2k). Referenz-Formfaktor 503759 (~59×37×**5** mm).

**Empfehlung: Adafruit 2000 mAh (Produkt 2011)** — Datenblatt-verifiziert:
2000 mAh, 60×36×**7** mm, JST-PH, **integriertes Protection-PCM**, **RoHS-2**,
**UN38.3-Report + MSDS + Datenblatt als PDF**. Bezug DE: Mouser / DigiKey /
BerryBase. Das ist das „grünes-Licht"-Profil: alle Doks liegen bei,
**Polarität linienweit konsistent** → einmal prüfen, ewig nachbestellbar.

**Polarität — Board-Seite verifiziert 🟢** (r19.2, aus dem Schaltplan
getract): J9 `Conn_01x02` Pin 1 = `BAT_PLUS` (+), Pin 2 = `GND`. Beweis:
Symbol-Pin 1 (3.81, +1.27) → nach Y-Spiegelung Schaltplan-Punkt
(53.81, 78.73) → Wire → `BAT_PLUS`-Label (60, 78.73); Pin 2 → (53.81,
81.27) → GND. Damit ist **Footprint-Pad 1 = Plus**. Topologie komplett:
`BAT_PLUS` erreicht (via F2 PTC) BQ24074-BAT und den
BAT_SENSE-Teiler; GND sauber.

Rest — nur noch am physischen Teil (kein File-Thema mehr):
1. 🟡 **Bench-Check**: mit Multimeter bestätigen, dass Adafruits Stecker
   beim Einstecken **+ auf Pad 1** liefert (Adafruit warnt: *„other brands
   may have reverse polarity"*). Adafruit-Polung linienweit konsistent →
   einmal messen, dann fix. Erledigt wenn der Akku physisch da ist.
2. **Dicke +2 mm** (7 mm statt 5 mm) → an Arons PCB-/Gehäuse-Layout
   delegiert, kein Blocker für die Schaltung. Falls zu eng: 503759-Zelle
   (5 mm) mit eigenem UN38.3-Report + PCM, Doks+Polung pro Teil verifizieren.

**Kein PCB-/R_PROG-Change nötig** — nur die BOM-Zeile (Link/MPN).

---

## 3. EU/DE-Zulassungs-Checkliste (vor dem VERKAUF)

Für GENAU dieses Gerät (**kein Funk** → RED entfällt; **<50 V** → LVD am
Gerät entfällt). CE = **Selbst-Deklaration, keine Benannte Stelle nötig.**

| Pflicht | Was | Status |
|---|---|---|
| **EMV 2014/30/EU** | Labormessung EN 55032 (Aussendung) + EN 55035 (Störfestigkeit) | 🔴 offen (der eine echte Test) |
| **RoHS 2011/65 + 2015/863** | bleifreie JLC-Bestückung + Bauteil-RoHS-Doks + Erklärung | 🟡 sammeln |
| **CE-Konformitätserklärung** | listet EMV + RoHS; Technische Doku 10 J. aufbewahren | 🔴 nach EMV |
| **GPSR (EU) 2023/988** | Risikoanalyse, Rückverfolgbarkeit, Sicherheitshinweise (DE) | 🔴 offen |
| **WEEE / ElektroG** | stiftung ear-Registrierung → WEEE-Reg.-Nr. DE, Mülltonnen-Symbol | 🔴 vor Verkauf |
| **BattG (Batterie)** | stiftung ear (Batterie-Melderegister) | 🔴 vor Verkauf |
| **VerpackG** | LUCID-Registrierung (ZSVR) + duales System | 🔴 vor Verkauf |
| **Batterie UN38.3 + IEC62133** | vom Zell-Hersteller (Adafruit liefert) | 🟡 mit Akkuwahl |
| **USB-Netzteil** | fertig CE/LVD-zertifiziert kaufen | 🟡 |
| Kennzeichnung | CE + Mülltonne + Li-Symbol + Typenschild (Name/Adresse/Modell/Seriennr.) aufs Gehäuse | 🔴 |

**Nicht anwendbar (dokumentiert, damit klar):** RED (kein Funk), LVD am
Gerät (<50 V), Benannte Stelle (EMV/RoHS = Selbstdeklaration).

---

## 4. Kosten — drei Töpfe

### A) Stückkosten (BOM + Fertigung) — dokumentiert
| Menge | ~$/Gerät |
|---|---|
| 5er (Prototyp) | 89–99 |
| 10er | 72–82 |
| ~100er (Schätzung) | 55–70 |

Kostentreiber sind die **Handteile** (4 Encoder, Display, Speaker, Akku) +
Handmontage — **nicht** die Chips (SMD-BOM nur ~$21/Board). Skaliert normal.

### B) Einmalige Zulassung — der eigentliche „teuer"-Hebel
EMV-Messung **3.000–8.000 €** (+ optional Beratung 1–3k) = **~4–10k € einmalig.**

**Amortisation** (bei ~7.000 € Zulassung):
| Stückzahl | Zulassungsanteil/Gerät |
|---|---|
| 20 | ~350 € 😱 |
| 100 | ~70 € |
| 200 | ~35 € |
| 500 | ~14 € |
| 1000 | ~7 € ✅ |

⇒ **Nicht die BOM macht klein-Serien teuer, sondern die Zulassung pro
Stück.** Lohnt sich ab ~einigen hundert Einheiten.

### C) Laufende Pflichtgebühren (DE)
ear (WEEE+Batterie) + LUCID/duales System ≈ **einige hundert €/Jahr** bei
kleiner Stückzahl. Pflicht, aber überschaubar.

### Kosten-Hebel (billig halten)
1. **Kein Funk** (✅ so lassen) → keine RED-Funkmessung.
2. **Fertig-zertifiziertes USB-Netzteil** → keine LVD-Zulassung.
3. **Akku mit mitgelieferten Doks** (Adafruit) → keine eigenen UN38.3/IEC-Tests.
4. **EMV-Pre-Compliance** vor dem teuren Termin → keine teuren Wiederholungen.
5. **Selbst-Deklaration** (RoHS/EMV) → keine Zertifizierungsstellen-Gebühr.

---

## 5. Go-to-Market — Cashflow-Reihenfolge (0-Kapital-Bootstrap)

Grundregel: **kein Geld ausgeben, bevor Nachfrage bewiesen & Kapital da ist.**
Crowdfunding zahlt Zulassung + Serie — nicht die eigene Tasche.

1. **Jetzt: 5 Prototypen bauen.** Muster, KEIN Inverkehrbringen → CE/WEEE/
   VerpackG greifen noch NICHT. Kein Gewerbe nötig.
2. **Story + Demo.** Firmware/Sound ist da; die Audio-Demos in
   `demos/audio/` + ein Video sind die Kampagnen-Munition.
3. **Crowdfunding** (Startnext für DE, oder Kickstarter): Vorbestellungen =
   Kapital. Hier wird Nachfrage bewiesen, BEVOR die 5k EMV fällig sind.
4. **Erst nach Funding:**
   - Gewerbe anmelden (Kleinunternehmer §19 UStG ist billig; ab
     Funding-Höhe ggf. reguläre USt → **Steuerberater** zur Schwelle).
   - EMV-Messung + CE-Doku **aus dem Funding** bezahlen.
   - ear (WEEE+Batterie) + LUCID registrieren.
5. **Ausliefern.** An Backer liefern = Inverkehrbringen → CE/WEEE/VerpackG
   müssen davor fertig sein (aus dem Funding finanziert).

**Kernpunkt:** Zulassung + Registrierungen kommen **nach** dem Funding und
werden **aus** dem Funding bezahlt. 0 Kapital vorab ist für gecrowdfundete
Hardware der Normalfall — das Risiko ist nur, die 5k auszugeben BEVOR Backer
da sind. Tut man nicht.

---

## 6. Nächste Schritte
- [x] Akku-Polarität Board-Seite verifiziert — **J9 Pin 1 / Pad 1 = Plus** (r19.2)
- [ ] Bench-Check: Adafruit-Steckerpolung mit Multimeter (wenn Akku da ist)
- [ ] Akku-7-mm-Fit → Arons PCB-/Gehäuse-Layout (kein Schaltungs-Blocker)
- [x] Kailh-Choc feste Quelle: **keycapsss.com (DE)**, taktil (r19.3)
- [x] Display feste Quelle: **Eckstein (DE) 13,95 €**, per Kabel an J3 (r19.3)
- [ ] Kailh- + Waveshare-RoHS-Herstellererklärungen in die Akte legen
- [ ] EMV-Pre-Compliance planen (nach 5er-Prototyp)
- [ ] Kampagnen-Video + Demo-Seite
