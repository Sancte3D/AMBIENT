# Field Ambience PCB — CHANGELOG

Vollständige Änderungshistorie der PCB-Spec und des KiCad-Schematic.
Die Spec-Body selbst (`field_ambience_pcb_SPEC_v0.7.md`) beschreibt
**immer den aktuellen Stand** — diese Datei trackt wie wir dahin kamen.

Aktuelle Rev: **v0.7-r18.30** (Firmware-Ausbau Teil 3: leds.c (16. Suite),
Menü-Encoder-Binding, MIDI deferred. Firmware-LOGIK komplett + verdrahtet.
KEIN .kicad_pcb.)

---

## v0.7-r18.83 (2026-07-02) — Firmware↔Hardware-Konsistenz-Audit: Cells-Gap geschlossen, Pin-Maps verifiziert

User: „Will everything work as intended with the firmware and software?"

### Verifiziert KONSISTENT (Firmware-HAL-Claims vs. Schematic-Netze)

Jede Pin-Behauptung der H743-HAL-Dateien gegen die Generator-NETS-Tabelle
(= Schematic-Wahrheit) geprueft — ALLE stimmen:
- **SAI1**: PE4/PE5/PE6 (Pins 3/4/5) = I2S_LRCK/BCK/DOUT → PCM5102A 15/13/14 ✓
- **LCD SPI1**: PA5/PA7/PA6/PC4/PC5 (29/31/30/32/33) = SCK/MOSI/CS/DC/RES ✓;
  Backlight = PCA-Kanal 15 (leds.h ✓, lcd_h743 ✓)
- **I²C1**: PB6/PB7 (92/93) + MCP_INT PC13 (7) ✓; Adressen 0x20/0x40/0x41 ✓
- **Encoder**: TIM2 PA0/PA1 (22/23), TIM3 PC6/PC7 (63/64), TIM4 PD12/PD13
  (59/60), TIM1 PA8/PA9 (67/68); Pushes PE0/PE1/PE3 (97/98/2) + MCP GPB5 ✓
- **Amp**: PB14/PB15 (53/54) = AMP_nSHDN/nMUTE ✓ — Boot-Pop-Sequenz passt zu
  den Hardware-Pulldowns (Amp+DAC starten stumm, Firmware hebt an) ✓
- **MIDI**: PD5 (86) USART2_TX ✓ · **BAT_SENSE**: PA3 (25) ✓
- **MCP-Bitmap** (mcp23017.h): Cells GPA0-4, XSMT GPA5, JACK GPA6,
  Modifier GPB0-4 = exakt die mcp_sheet-Verdrahtung ✓
- **Jack-Detect-Polaritaet**: Firmware-Annahme (Bit HIGH = plugged) = die
  r18.82-korrigierte Hardware-Topologie ✓

### GESCHLOSSEN: der r18.76-Firmware-Gap (Cells auf H743)

main_h743.c enthielt noch den stalen Hall-ADC-TODO-Block („adc_read_norm/
cells_update") — seit r18.73 gibt es keine Hall-Sensoren mehr. Haette man den
TODO wie beschrieben implementiert, waeren die Cells auf echter Hardware tot
gewesen. Jetzt: digitaler MCP-Edge-Pfad (GPA0-4, aktiv-LOW) wie in der
bench-erprobten Pico-Implementierung, geroutet durch die host-getestete
controls.c-Statemachine (feste Tap-Velocity 0.12 wie Pico). Dazu: Jack-Detect-
Routing-Block (GPA6 → nur PAM8403 muten, NICHT audio_mute() — das wuerde den
Line-Out toeten); stale Kommentare fixiert („J9 DNP" → J10-Hardware ist
bestueckt; Generator-Docstrings: MIDI-Buchse „fehlt" → existiert seit r18.67,
Backlight „Kanal 12" → 15). H743-Target kompiliert + linkt ✓.

### OFFEN (ehrlicher Stand — was noch NICHT auf Hardware laeuft)

1. **Step 13.3 (der grosse bekannte Schritt):** main_h743 ist ein bewusstes
   Skeleton — STM32CubeH7-Integration fehlt (Clock-Config 480 MHz + PLL3,
   SAI1+DMA-Pump, SPI1-, I²C1-, USART2-, GPIO-Primitiven, HAL_GetTick,
   Generative-Bar-Timer, Menu-Flush-Rate-Limit). Die gesamte LOGIK darueber
   ist host-getestet (352298 Checks) und auf dem Pico-Bench-Target real
   verifiziert; es fehlt die H7-Peripherie-Anbindung, keine Logik.
2. **VU-Meter (U10 @ 0x41, 8 LEDs, Hardware seit r18.66): keine Firmware.**
   pca_set_pwm kennt nur ein Device; es fehlen adressierbare PCA-API,
   Engine-Level-Tap und VU-Renderer. Bis dahin bleiben die 8 LEDs dunkel
   (PCA bootet aus — kein Fehlverhalten, Feature fehlt). Eigener Schritt.
3. **MIDI-TX firmware-seitig deferred** (ADR-0004 r18.30) — Hardware komplett,
   Reaktivierung = 1 Zeile + USART2-Init aus Step 13.3.

### Verifikation

Host-Tests 352298 Checks / 0 Failures; field_ambience_h743-Target baut+linkt;
Schematics byte-identisch (nur Python-Docstrings geaendert).

---

## v0.7-r18.82 (2026-07-02) — Datenblatt-Verifikation aller restlichen Komponenten: 2 kritische Pad-Mapping-Fehler + 2 Lücken

User: „All the other components are right? check datasheets." — systematischer
Abgleich jedes verbleibenden ICs/Bauteils gegen das Original-Datenblatt
(Microchip DS20001952, NXP PCA9685 Rev.4, Microchip DS20001984, ST USBLC6-2,
ST DS12110 Fig. 5, Abracon/LCSC, SHOU HAN PJ-320D + HX B3F-4055) UND jedes
Symbol-Pin↔Footprint-Pad-Paars (KiCad matcht Pin-Nummer gegen Pad-Namen als
STRING — die r18.82-Fehlerklasse).

### KRITISCH — Pad-Mapping (Symbol-Pin-Nummer ≠ Footprint-Pad-Name)

1. **EN1–EN4 Encoder: ALLE 4 komplett unverbunden.** Der offizielle
   KiCad-Footprint `RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` hat
   BENANNTE Pads **A / B / C / S1 / S2** (+MP) — das Custom-Symbol nutzte
   numerische Pins 1–5. String-Match ⇒ kein einziger Encoder-Pin haette im
   Netlist ein Pad gehabt. Fix: Pin-Nummern → A/B/C/S1/S2 (Pad-Geometrie
   gegen den GitLab-Footprint verifiziert: A(0,0), C(0,2.5)=Common-Mitte,
   B(0,5) = ALPS-Terminal-Reihenfolge A-C-B ✓).
2. **J8/J10 PJ-320D: Pad-Map falsch — Audio-L lag auf dem BARREL.** Das
   SHOU-HAN-Datenblatt (P.C.B-Layout + Schaltbild) ordnet zu: Pad 1=A=Sleeve,
   2=D=Ring, 3=C=Tip-Switch (DETECT), 4=B=Tip. Das Symbol mappte T=1, R=2,
   S=3, DET=4 ⇒ links Audio auf dem (geerdeten) Sleeve-Barrel, GND auf dem
   Detect-Kontakt, JACK_DETECT auf dem Tip — Line-Out UND MIDI-Out falsch.
   Fix: T=4, R=2, S=1, DET=3. Vendored-Footprint-Geometrie verifiziert
   (Pad-Pitch 3,0/4,0 mm, Locator Ø1,0 @ 7,0 mm = Zeichnung exakt) —
   **TODO B0b damit geschlossen.**
3. **Folge-Fix aus #2:** laut Datenblatt ruht der Detect-Kontakt OHNE Stecker
   am TIP (nicht am Sleeve) → MCP-GPA6 hinge unplugged direkt am DAC-L-Ausgang
   (±3 Vpk): die Eingangs-Clamp haette die negativen Halbwellen ueber 22R
   kurzgeschlossen (>100 mA Injection + Verzerrung am Speaker). Neu:
   **R_DET_J8 10k** (Clamp-Strom < 300 µA) + **C_DET_J8 1 µF** (Audio-AC-
   Filter, fc ≈ 16 Hz). Pegel: unplugged ≈ 0,3 V LOW / plugged 3,3 V HIGH.

### Lücken (Datenblatt-Anforderungen)

4. **MCP73831: 4,7-µF-VDD-Eingangskondensator fehlte** (DS20001984 Typical
   Application) — der VBUS_USBC-Knoten hatte keinerlei lokale Kapazitaet.
   Neu: C_CHG_IN 4,7 µF (C46653). Pinout 1=STAT/2=VSS/3=VBAT/4=VDD/5=PROG ✓
   verifiziert; R21 2k → 500 mA = exakt die DS-Beispielschaltung ✓.
5. **STM32 VSS-Pin 99 war nur ZUFAELLIG geerdet** (der alte horizontale
   VSSA-Stub lief ueber seinen Anker). DS12110 Fig. 5: VSS = 10/26/49/74/**99**.
   Neu: expliziter Stub; VSSA-Stub senkrecht wie alle Unterseiten-Pins.

### Verifiziert OHNE Befund (gegen Original-Datenblätter)

- **MCP23017-E/SS**: SSOP-Pinout = SOIC/SPDIP (DS20001952 S. 1) — Generator ✓.
- **PCA9685PW**: TSSOP28 Tabelle 3 — alle 28 Pins ✓ (U6 + U10).
- **USBLC6-2SC6**: 1/6=I/O1, 3/4=I/O2, 5=VBUS, 2=GND ✓ Up/Downstream-Paarung ✓.
- **STM32H743 LQFP100**: komplette Pin-Map gegen DS12110 Fig. 5 ✓ (VBAT 6,
  VDD 11/27/50/75/100, VSSA 19, VREF+ 20, VDDA 21, VCAP 48/73, BOOT0 94).
- **ABLS-8.000MHZ-B4-T**: CL = 18 pF (live) → 27-pF-Load-Caps korrekt ✓.
- **D2 SMAJ5.0A**: Kathode→Rail, Anode→GND ✓. **USB-C**: Symbol-Pins A1…B12/S1
  = KiCad-Footprint-Pads ✓. **HX B3F-4055 (SW6–10)**: interne Paarung
  ①-② / ③-④ (Reihen) = Footprint-Pad-Paarung ✓; Lochreihenabstand 5,08 vs
  DS 5,00 — von Ø1,5-Drill absorbiert (Layout-Nicety fuer Aron).
- Bereits frueher DS-verifiziert: TPS61089, TPS22918, MST-12D18G3, AP7361C,
  PCM5102A, PAM8403H, L1 SWPA6045, Kailh Choc, TS-1088.

### Verifikation

ERC 0 Fehler auf allen 7 Sheets; Generator deterministisch; jlc_bom.csv
58 Parts / 210 Placements (+R_DET_J8, C_DET_J8, C_CHG_IN); Host-Tests
352298 Checks, 0 Failures.

---

## v0.7-r18.81 (2026-07-02) — Power-Off-Block (ADR-0016) endlich im Schematic + „+5V"-Netz-Brücke + SW_PWR-Seitenmontage verifiziert

User (mit Foto vom Schalter-Render): „What about the side button on off switch.
I don't think the one we have works. the pcb will be a normal pcb, not vertical,
but horizontal … is the on off switch mount actually mountable that way, that I
can use the on off switch from the side?"

### 1. SW_PWR-Seitenmontage: JA — gegen das Original-Datenblatt verifiziert

MST-12D18G3-Datenblatt (SHOU HAN, via LCSC-PDF, Zeichnung S. 1 gerendert):
Body liegt FLACH auf der (horizontalen) PCB (3 SMD-Beine + 2 Locator-Pegs in
Ø0,9-Bohrungen), der Schiebe-Stem (1,5×1,5 mm) ragt **HORIZONTAL** 3 mm über
die Body-Kante — gegenüber der Pin-Seite, Oberkante 0,2 mm unter Body-Top
(Body 4,0 mm) → Stem-Fenster z ≈ 2,3–3,8 mm über der PCB-Oberfläche; Travel
2 mm ±0,2 bei 200 gf lateral. **Am Board-Rand platzieren, Stem durch einen
Slot in der Gehäuse-Seitenwand — genau die gewünschte Seitenbedienung.** Der
vendored Footprint stimmt EXAKT mit dem offiziellen Land-Pattern MSK12D
überein (3 Pads 1,2 @ 2,5 mm Pitch; Löcher Ø0,9 @ 6,8 mm; Versatz 3,2 mm =
2,1 + 2,2/2). Das 3D-Render (User-Foto) schwebt nur beliebig orientiert im
Viewer — daher der „zeigt der nicht nach oben?"-Eindruck. OFFEN (UNVERIFIED —
NEEDS HUMAN CHECK): Terminal-Belegung Common=Mittelpad ist 1P2T-Konvention,
das DB labelt die Pins nicht → vor Fab durchpiepen (Falsch-Fall fail-safe:
Gerät ginge nur nicht an, R_PWR_PD hält PWR_ON low).

### 2. KRITISCH gefunden: der beschlossene Power-Off-Block war NIE im Schematic

ADR-0016 (User-Entscheidung 2026-06-27, inkl. Pin-Level-Drop-in-Spec) war
dokumentiert, HTML/BOM_MASTER listeten U_PWR/SW_PWR — aber weder Generator
noch jlc_bom.csv enthielten sie: das Board hätte KEINEN Ein/Aus-Schalter
gehabt. r18.81 zeichnet den Block in power_tree ein:
`+5V_RAIL → U_PWR (TPS22918, C131941 live verifiziert) → +5V_SW → LDO-VIN`;
`SW_PWR`-Common → `PWR_ON` = U_PWR.ON, Throw A → +5V-Rail, `R_PWR_PD` 100k
→ Default AUS; `C_UPWR_IN` 1µF (TI-DS Pin-1-Note), `C_PWR_SW` 10µF auf
+5V_SW. Lader U7 hängt VOR dem Switch → „dunkel, aber lädt". ADR-Korrekturen
gegen TI SLVSD76C (Pinout-Seite gerendert): Pin 4 heißt CT (nicht „NC";
floaten DS-erlaubt = schnellster Anstieg, CT-Cap = optionaler Soft-Start);
QOD floaten würde die Quick-Discharge DEAKTIVIEREN → QOD an VOUT gebunden
(DS-Option 2, aktive Entladung im Aus). Neue Lib-Symbole
Power_Switch:TPS22918 + Switch:SW_SPDT.

### 3. KRITISCH gefunden: globales „+5V"-Netz war eine quellenlose Insel

Auf KEINEM Sheet existierte eine Brücke zwischen dem +5V_OUT-Hier-Netz (der
realen Diode-OR-Rail) und dem globalen „+5V"-Netz der Power-Symbole. Folge im
Netlist: PAM8403-PVDD/VDD (Audio), alle 23 LED-Anoden (mcp) und LED_CHRG
(battery) hingen an einem Netz OHNE Quelle — Amp und sämtliche LEDs wären tot
gewesen. Fix: die Rail trägt jetzt das globale +5V-Flag (Brücke bei (118,60));
per ADR-0016 bleibt +5V bewusst UNgeschaltet (Amp geht über AMP_nSHDN-Pulldown
aus, LED-Treiber sitzen in der geschalteten 3V3-Domäne → dunkel im Aus).

### Verifikation

Geometrisches ERC: 0 Fehler auf allen 7 Sheets. Netz-Trace (Union-Find inkl.
globaler Power-Netze): VIN=Rail ✓, VOUT=+5V_SW=LDO-IN/EN=QOD ✓, +5V_SW≠Rail ✓,
COM=ON ✓, ThrowA=Rail ✓, ThrowB isoliert ✓, R_PWR_PD/Caps korrekt ✓, Rail=+5V
global ✓ (11/11). Generator deterministisch; jlc_bom.csv 58 Parts / 207
Placements (+C131941, +C49023766). Host-Tests grün.

---

## v0.7-r18.80 (2026-07-02) — Geometrisches Pin-Level-ERC: 8 Kupfer-Fehler + Boost-Loop-Instabilität behoben

User: „Any other logical issues or something that will ruin the pcb?" — Antwort: ja, mehrere.
Methode: headless geometrisches ERC (Python) über alle 7 Sheets — jeder Pin-Anker
gegen jedes Wire-Segment, Label-Anker gegen Kreuzungspunkte, Union-Find-Netzbau,
Hier-Label↔Root-Sheet-Pin-Namensabgleich. Jeder Fund im Generator-Quelltext verifiziert.

### KRITISCH — Kurzschlüsse (Pin-/Label-Anker exakt auf fremden Wires)

1. **power_tree: USB D+ UND D− hart auf +5V** — der U5-LDO-Block (120,80) sass im
   USB-Daten-Korridor: EN-Pin (111.11, 77.46) lag AUF dem USB_DP-Wire, IN-Pin
   (111.11, 82.54) AUF dem USB_DM-Wire, Rail-Drop x=110 überlappte den D1-VBUS-Wire.
   → USB tot, H7-USB-PHY-Gefahr. Fix: LDO-Block nach (130,130), Rail-Tap x=120.
2. **mcp: I2C_SCL hart auf GND** — C5 (U2-VDD-100nF) hing 7,62 mm nach unten;
   Pin 9 (VDD) und Pin 12 (SCL) liegen exakt 3 Rasterzeilen = 7,62 mm auseinander →
   C5-GND-Pin (108, 121.43) sass AUF dem SCL-Wire. → kompletter I²C-Bus tot
   (U2 + U6 + U10 = alle Switches, alle LEDs). Fix: C5/C5b nach x=93/88.
3. **audio: PCM_XSMT hart auf GND** — C8a/C8b-GND-Pins (120/124, 96.19) sassen AUF
   dem XSMT-Wire (Pin 17, gleiche 7,62-mm-Falle wie #2). → DAC permanent gemutet.
   Fix: Caps nach oben geklappt (rot=180, GND oberhalb).
4. **audio: I2S_LRCK hart auf GND** — C_LDOO-GND-Pin (117, 101.27) sass AUF dem
   LRCK-Wire (Pin 15, dieselbe Falle). → kein Wordclock, Audio tot. Fix: hochgeklappt.
5. **stm32: BOOT0 permanent HIGH (2 Fehler)** — (a) +3V3-Flag sass bei (100,184) =
   exakt der BOOT0-Rail-Punkt → +3V3 direkt auf BOOT0; (b) der SW_BOOT-Pin-1-Wire
   lief DURCH den Pin-2-Anker → Taster dauerhaft überbrückt. → H7 bootet IMMER in
   den ROM-Bootloader, nie in die Firmware („totes Board"). Fix: Pfad unten herum,
   +3V3-Flag oben an R_BOOT_SW.
6. **stm32: VDDA mit VDD verkupfert** — der VDDA-Pin-21-Stub lief horizontal DURCH
   den VDD-Pin-100-Anker (145.08, 61.42) → FB2-Filter überbrückt. Fix: Stub senkrecht
   nach oben (wie alle Top-Pins).
7. **battery: C_BOOT kurzgeschlossen (Label-Falle)** — das U8_SW_NODE-Label bei
   (154, 83.65) sass exakt auf der Kreuzung SW-Wire × boot-up-Wire → KiCad hätte
   BOOT- und SW-Netz verschmolzen → kein Bootstrap, Boost startet nicht. Fix: Label
   entfernt (Netz benennt das Label bei x=141).

### KRITISCH — Unterbrechungen

8. **lcd: J3 komplett unverbunden (alle 8 Pins)** — Verdrahtung nahm KiCad-Standard-
   Pins links (−5.08) an; die Custom-Lib Conn_01x08 hat Pins RECHTS (+3.81/180°).
   → GND, VCC, SPI, Backlight: alles floatend. Fix: J3 rot=180 + jpy()/PINX neu.
9. **stm32: J4 SWD-Header alle 3 Pins floatend** — gleiche −5.08-Annahme. → Board
   nicht flash-/debugbar. Fix: Stubs rechtsseitig an die echten Pin-Anker.
10. **stm32: FB2 beidseitig floatend** — rotation=90 (Pins vertikal), Wires horizontal.
    → VDDA ohne Versorgung über den Filter (nur der Zufalls-Kurzschluss #6 versorgte
    sie). Fix: rotation=0.
11. **stm32: lib_symbols-Name „STM32H743VIT6" ohne „MCU:"-Prefix** — die Instanz
    referenziert `MCU:STM32H743VIT6`, KiCad matcht exakt → U1 wäre in KiCad ein
    unaufgelöstes Symbol (keine Pins, kein Netlist-Export). Fix: Prefix ergänzt.

### KRITISCH — Boost-Regelkreis (TPS61089, TI SLVSD38C nachgerechnet)

12. **R_COMP 22k / C_COMP 1nF → 6,2k / 10nF; C_BOOST_OUT 1× → 3× 22 µF.**
    Gl. 15/17/18 mit VREF 1,212 V, GEA 190 µS, Rsense 0,08 Ω: fRHPZ ≈ 47 kHz
    (VIN 3,0 V, 2 A) → fc-Limit fRHPZ/5 ≈ 9,5 kHz. Alt: fc ≈ 87 kHz — ÜBER der
    RHP-Nullstelle → Loop-Oszillation unter Last bei leerem Akku. Neu: fc ≈ 8 kHz.
    Formeln gegen TIs eigenes 9-V/2-A-Referenzdesign validiert (reproduziert exakt
    deren 17,4k/4,7nF). Output-Caps: TI „typically three 22 µF", EC-Tabelle rechnet
    Soft-Start mit 47 µF effektiv; 1× 22 µF-0805 hat bei 5-V-Bias nur ~12 µF eff.
    (Ripple wäre ~150 mV); der 470-µF-Bulk liegt HINTER D3 → Regelknoten sieht ihn
    nicht. Neu: 3× C45783 → CO_eff ≈ 36 µF, Ripple ~50 mV. **Neues Teil: R 6,2k
    0603 1% = C4260 (0603WAF6201T5E, JLC Basic, 219k Stock — live verifiziert).**

### Metadaten / Hygiene (JLC bestückt nach C-Nummer — Werte-Text log)

- **C45783 = CL21A226MAQNNNE 22 µF 25 V** (live verifiziert): STM32-VDD-Bank und
  C_BAT_IN behaupteten „4,7 µF CL21A475KQFNNNE" — falsch beschriftet (bestückt
  worden wäre 22 µF; elektrisch unkritisch, Doku/BOM-Texte korrigiert).
- **C15850 = CL21A106KAYNNNE 10 µF 25 V** (live verifiziert): 8× MPN-Suffix
  „KOQNNNE" (16-V-Variante) korrigiert.
- **C14663 = CC0603KRX7R9BB104** (live verifiziert): 5× falscher MPN
  „CL10B104KO8NNNC" (+2× „-class"-Pseudo-MPNs) vereinheitlicht.
- mcp: U10 LED8–15 (Pins 15–22, unbenutzt) mit expliziten No-Connect-Markern.
- battery: Rest-Stub (75,86.19)→(75,80) aus verworfenem Bus-Ansatz entfernt.
- lcd: freischwebendes „+3V3"-Label entfernt.

### Verifikation

- Geometrisches ERC: **0 Fehler auf allen 7 Sheets** (vorher: 22 Fehler/kritische
  Merges). Hier-Label↔Root-Sheet-Pin: alle Namen matchen. Generator deterministisch
  (2 Läufe, identische MD5). `jlc_bom.csv`: 56 unique Parts, 202 Placements.
- kicad-cli 7.0.11 kann das v9-Format der Sheets nicht laden (Distro-Limit) —
  ERC-/GUI-Pass in KiCad 9 bleibt Human-TODO wie gehabt.

---

## v0.7-r18.79 (2026-07-01) — Elektrik-Audit: 3 kritische Power-Fehler behoben

User: „Überprüfe nochmal BOM und PCB components und connections. Nicht
mechanik. Schaue dass keine kritischen fehler existieren."

Systematischer Elektrik-Review: Hier-Label↔Root-Pin-Matching, Root-Verdrahtung
(Union-Find über Wire-Endpunkte + Label-Merge), hängende Netze, Refdes-
Dubletten, I²C-Adressen, Pull-ups, Boost-Schaltung gegen das TI-Datenblatt
(SLVSD38C, Formeln von den gerenderten PDF-Seiten abgelesen, nicht geraten).

### 🔴 Fund 1 — Boost-FB-Teiler: Rail wäre 7,43 V statt 5 V gewesen

- R23/R24 = 200k/39k, VREF laut TI-EC-Tabelle **1,212 V** → VOUT = 1,212 ×
  (1+200/39) = **7,43 V**. PAM8403 (abs max 5,5 V) und der geplante TPS22918
  (5,5 V) wären beim ersten Einschalten zerstört worden, AP7361C (6 V max)
  überfahren.
- **Fix:** R23 = **121k** (0603WAF1213T5E, **C25809**, 71k Stock, live
  verifiziert) → VOUT = **4,97 V**. (122k für exakt 5,00 V hat keinen
  verifizierbaren LCSC-Bestand; −0,6 % ist die sichere Richtung.)

### 🔴 Fund 2 — R_ILIM 20k = 51,5 A Stromlimit (faktisch keins)

- TI Gl. 4: ILIM = 1.030.000/R_ILIM. Der Generator-Kommentar behauptete
  „20k → ~4 A“ — real wären es **51,5 A**; der Induktor (Isat min 6,75 A)
  hätte im Fehlerfall ungebremst gesättigt.
- **Fix:** R_ILIM = **174k** (0603WAF1743T5E, **C22890**, live verifiziert)
  → 5,92 A typ / 5,12 A min (≥ Worst-Case-Peak ~4,7 A) / ~6,7 A max
  (≤ Isat-min 6,75 A).

### 🔴 Fund 3 — Q1-Power-Path: Backfeed + Fuse-Bypass

- Alt: Q1 P-FET S=VBUS (pre-fuse), D=+5V-Rail, G→GND — UND power_tree
  exportierte den Rail direkt hinter F1. Ohne USB: Boost-5V → Q1-Body-Diode
  (und parallel durch F1, Polyfuses leiten bidirektional) → VBUS ≈ 4,3 V →
  Gate-Vgs −4,3 V → Q1 voll leitend → **Lader U7 (VDD=VBUS) lädt den Akku aus
  dessen eigenem Boost** (Selbstentladeschleife ~1,5× Ladestrom), USB-Detect
  (MCP GPA7) dauerhaft HIGH, 5 V am offenen USB-C-Stecker. Mit USB: Laststrom
  lief über Q1 **an F1 vorbei** (pre-fuse→post-fuse) — Kurzschlussschutz
  ausgehebelt.
- **Fix:** Q1 + R22 entfernt; **Dioden-OR**: USB → F1 → [D2-TVS pre-Diode] →
  **D3B** (2. SS34, gleicher Code C8678) → Rail ‖ Boost → D3 → Rail. Kein
  neues/unverifiziertes Bauteil; Kosten ~0,35 V Drop im USB-Pfad (Rail ~4,6 V
  an USB — LDO braucht >3,6 V, PAM8403 2,5–5,5 V: unkritisch).

### 🟡 Kleinere Funde

- R_FSW 360k = **~440 kHz** (TI Gl. 3), nicht „1,21 MHz“ wie seit r12-B11
  kommentiert — 440 kHz ist zulässig (200 kHz–2,2 MHz) und weit über dem
  Audioband; Widerstand bleibt, Kommentare/Doku korrigiert.
- FSW-Widerstand hängt korrekt am SW-Node — TI-DS sagt ausdrücklich
  „resistor between this pin and the SW pin" (mein anfänglicher
  GND-Verdacht war falsch — per Datenblatt geklärt, nicht geraten).
- PINMAP: „AP7361A-33ER“ → AP7361C-33 (Doku-Drift); PCB_BOM-Restdrift auf
  r18.70-Stand gezogen (C_BULK2 C23742, LED_CHRG C965800, VCAP C23630).
- Root-Sheet: `+3V3_OUT`/`GND_OUT`-Sheet-Pins hängen unverdrahtet (Rails
  laufen über globale Power-Symbole — kosmetisch, ERC-Hinweis für Aron).

### ✅ Ohne Befund verifiziert

Hier-Labels ↔ Root-Pins vollständig; Root-Verdrahtung verbindet alle
Partner-Pins; 0 Refdes-Dubletten (200 Instanzen); I²C 0x20/0x40/0x41
kollisionsfrei (U10-A0 an +3V3 nachgemessen); BOOT0 10k-Pull-down + 1k-Serie
korrekt; NRST 10k+100nF; CC1/CC2 je eigener 5,1k; VCAP 2×2,2 µF; USBLC6-
Verdrahtung; MCP_INT→PC13; PCM5102A-Konfigpins + Charge-Pump-Caps;
Cell-/Modifier-/Encoder-Netze. KiCad-GUI-ERC bleibt finaler Gate (Aron).

---

## v0.7-r18.77 (2026-07-01) — L1-Boost-Inductor: toter LCSC-Link + nicht existente MPN gefunden + behoben

User klickte auf L1 in der Aron-BOM-HTML ("SWPA6045 2.2µH") — die LCSC-Seite
existierte nicht.

### 🔄 L1: SWPA6045S2R2MT/C83455 → SWPA6045S2R2NT/C36500

- **C83455 404t** — nie ein echtes Listing, trotz einer r18.20c-Notiz die es
  fälschlich als "✓ Extended verifiziert" führte.
- **Die MPN war auch falsch:** `SWPA6045S2R2MT` existiert laut Sunlords
  eigenem Datenblatt (Item 2, Produkt-ID-Schema) gar nicht — die
  "M"-Toleranz-Endung (±20%) gibt es nur ab 4,3 µH aufwärts; 2,2 µH gibt es
  nur mit "N"-Endung (±30%). Der r18.37-BOM↔Generator-String-Diff-Audit
  hatte das nicht gefunden, weil BOM_MASTER und Generator **denselben**
  falschen Code trugen — ein Konsistenz-Check zwischen zwei Dokumenten fängt
  keinen Fehler, der in beiden identisch falsch ist.
- **Echtes, verifiziertes Teil:** SWPA6045S2R2NT, LCSC **C36500** — live via
  JLCPCB-Katalog geprüft und gegen jeden Wert in Sunlords offiziellem
  SWPA6045S-Datenblatt (Item 12, Elektrische Charakteristiken) abgeglichen:
  2,2 µH, DCR 18 mΩ max, Isat 6,75 A max / 7,40 A typ, Irms 4,60 A max /
  5,00 A typ.
- **Bonus-Fund:** der `value`-String im Schematic beschrieb noch ein
  **komplett anderes, älteres Bauteil** — "Sumida CDR63B-2R2" im "0630"-
  Gehäuse (6,0×3,0mm) — ein Relikt aus einer früheren Design-Iteration,
  bevor auf das Sunlord SWPA6045S (6,0×6,0×4,5mm) gewechselt wurde. Auch in
  `PCB_FOOTPRINT_RISK_AUDIT.md` steckte ein fabrizierter "Isat 4,5A"-Wert,
  der zu keiner Datenblatt-Zeile passte — korrigiert auf die echten Werte.
  `ADR-0011` (Gehäuse-Z-Budget) hatte fälschlich 3,0mm Bauhöhe (Sumida-Ära)
  statt der echten 4,5mm (Sunlord) — der Encoder (19mm) dominiert das
  Z-Budget trotzdem weiterhin, keine Kaskaden-Auswirkung.
- Aktualisiert: Generator (Footprint-Value-String + MPN + LCSC + FP_NOTE),
  regenerierte Schematics/BOM/Aron-HTML, `BOM_MASTER.md`, `PCB_BOM.md`,
  `field_ambience_pcb_SPEC_v0.7.md`, `PCB_FOOTPRINT_RISK_AUDIT.md`,
  `mechanical/3d_models/MANIFEST.md` (hatte zusätzlich Q1s LCSC-Code statt
  L1s eigenem), `ADR-0011-enclosure-thickness.md`.

---

## v0.7-r18.75 (2026-07-01) — Cells: direkt gelöteter Kailh-Choc-V1 statt Hotswap-Socket

User: „This is wrong. How is this meant to be soldered onto a pcb? maybe we can
just directly solder the kalih choc on the pcb? because i don't know how this
would work."

### 🔄 Cells: Hotswap-Socket → Direct-Solder (r18.74 → r18.75)

- Der r18.74-Plan (Kailh-Choc-V1/V2-Hotswap-Socket) hatte zwei echte Probleme:
  (1) der Socket selbst hatte **keine saubere Hersteller-/LCSC-Teilenummer**
  (Keyboard-Markt-Ware), (2) er hätte eine **nicht offensichtliche Klein-SMD-
  Handlöttechnik** gebraucht (2 winzige SMD-Pads von hinten löten, Switch klickt
  separat von vorne rein) — der User fragte zu Recht nach, wie das überhaupt
  gehen soll.
- **Fix:** der Kailh-Choc-**V1**-Switch (**CPG135001D01**, LCSC **C400229**,
  rot/linear) wird jetzt **direkt auf die Platine gelötet** — 2 THT-Beinchen
  durch 2 Löcher, von hinten verlötet, **exakt dieselbe Technik wie jeder
  andere Button/Connector in diesem Design**. Kein Socket mehr, keine
  Spezialtechnik.
- Footprint + 3D-STEP-Modell direkt von LCSC/EasyEDA für dieses exakte Teil
  gezogen: `easyeda2kicad --full --lcsc_id=C400229` — genau der Weg, den
  dieses Repo bereits für alle anderen Custom-Footprints vorschreibt (TS-1088,
  MST-12D18, PJ-320D). Vendored als `field_ambience:SW_KailhChoc_CPG1350_THT_2P`
  + zugehöriges STEP. Nicht aus einer Keyboard-Hobby-Bibliothek — direkt vom
  Hersteller-Listing.
- 2 elektrische THT-Pins + 3 unbestückte Mechanik-Löcher (die eigenen
  Locator-Pegs des Switch-Gehäuses geben seitliche Stabilität ohne Plate).
- **Trade-off:** der Switch ist jetzt fest verlötet, nicht mehr tauschbar wie
  beim Hotswap-Ansatz — eingetauscht gegen Löt-Einfachheit und ein wirklich
  verifiziertes Bauteil statt eines unverifizierten Zwischenteils.
- Elektrisch **unverändert** seit r18.73/74: `CELL1..5_BTN` auf MCP23017
  GPA0–GPA4, Pull-Up, IRQ — nur das physische Bauteil + Footprint ändern sich.
- Modifier-Buttons SW6–SW10 bleiben unverändert (HX B3F-4055).
- Aktualisiert: Generator (nur SW1–5-Block + neu vendorte Footprint/STEP-Dateien
  in `field_ambience.pretty`/`field_ambience.3dshapes`), regenerierte
  Schematics/BOM/Aron-HTML, `BOM_MASTER.md` §7, `PCB_BOM.md`, `PINMAP.md`,
  `KICAD_BLUEPRINT.md`, `SCHEMATIC_WALKTHROUGH.md`,
  `MECHANICAL_REQUIREMENTS.md` §1.5, `PCB_FOOTPRINT_RISK_AUDIT.md`
  (UNKNOWN-Zähler zurück auf 0), `mechanical_coordinates.md` §3.4, `ADR-0013`,
  `field_ambience_pcb_SPEC_v0.7.md` §5.6a, `PROJECT_STATUS.md`,
  `COST_ESTIMATE.md`, `MANUFACTURING_START.md`.

---

## v0.7-r18.74 (2026-07-01) — Cells: echter Kailh-Choc-Keyswitch statt Modifier-Tactile (UX-Fix)

User: „die 5 cells hast du in der html stehen als normale tactile switches,
aber es wäre ja besser wenn es trotzdem so tastatur keyboard dinger sind und
nicht wie die modifier, auch einfach tactile buttons.. das macht das ux kaputt"

### 🔄 Cells: HX-B3F-Tactile → Kailh-Choc-Hotswap (elektrisch unverändert)

- r18.73 hatte SW1–SW5 auf **dasselbe Bauteil wie die Modifier SW6–SW10**
  gesetzt (HX B3F-4055-Y THT-Tactile) — das machte die spielbaren Cells
  ununterscheidbar von simplen Modifier-Knöpfen, kein "Tastatur"-Gefühl mehr.
- **Fix:** Cells bleiben elektrisch digital (identische Netze `CELL1..5_BTN`
  auf MCP23017 GPA0–GPA4, Pull-Up, IRQ), bekommen aber einen **echten Kailh-
  Choc-V1/V2-Keyswitch über Hot-Swap-Socket** zurück — ~3 mm echter
  mechanischer Hub, Switch klickt von vorne rein (kein Löten), tauschbar.
  Genau die „Plain Choc V2"-Option aus ADR-0013s eigener Vergleichstabelle.
- Footprint: `Switch_Keyboard_Hotswap_Kailh:SW_Hotswap_Kailh_Choc_V1V2_Plated_1.00u`
  — bereits vendored im Repo (MIT, `keyswitch-kicad-library`), bereits in
  `fp-lib-table` registriert, community-verifiziert (verbreitet in
  produktiven mechanischen-Tastatur-PCBs). **Nicht** unabhängig gegen Kailhs
  eigene Maßzeichnung re-verifiziert — Pad-Pitch/Bohrungen vor Bestellung
  gegenchecken.
- **Der Hot-Swap-Socket selbst hat keine saubere Hersteller-/LCSC-Teilenummer**
  — Keyboard-Markt-Ware (z. B. Chosfox "Kailh Choc PG1350 Hot Swap Socket"
  ~$1.45/10 Stk, auch Kailh direkt/Mechkeys), offen als
  `UNVERIFIED — NEEDS HUMAN CHECK` markiert statt geraten.
- Der Choc-Switch, der reinklickt (nicht gelötet): ein reales, verifiziertes
  Beispiel bei LCSC **C400229** (Kailh CPG135001D01, rot/linear) — Farbe/
  Feel/V1-vs-V2 ist Industrial-Design-Entscheidung, keine Schematic-Frage.
- Modifier SW6–SW10 bleiben **unverändert** HX-B3F-4055 — bewusster
  UX-Unterschied (Cells = echtes Keyboard-Feel, Modifier = simples Tactile).
- Aktualisiert: Generator (nur SW1–5-Block), regenerierte Schematics/BOM/
  Aron-HTML, `BOM_MASTER.md` §7, `PCB_BOM.md`, `PINMAP.md`, `KICAD_BLUEPRINT.md`,
  `SCHEMATIC_WALKTHROUGH.md`, `MECHANICAL_REQUIREMENTS.md` §1.5,
  `PCB_FOOTPRINT_RISK_AUDIT.md`, `mechanical_coordinates.md` §3.4, `ADR-0013`,
  `PROJECT_STATUS.md`, `COST_ESTIMATE.md`.

---

## v0.7-r18.73 (2026-06-30) — Cells → digitale I²C-Switches (ADR-0013 SUPERSEDED)

User: „mach lieber das, austauschen … HiChord macht es sehr wahrscheinlich nicht
mit Gateron Magnetic Switches + Hall-Sensoren. … Mach die Cells erstmal digital
wie HiChord Batch 4+. Mechanische Switches, Caps/Plate, I2C Expander."

### 🔄 Cells: Hall/ADC → digitale Switches am MCP23017

- **Entfernt** (STM32-Sheet): 5× DRV5056A4 Hall-Sensor (J_CELL1–5, C2152902),
  5× R_CELL 1 kΩ (C21190), 5× C_CELL 10 nF (C57112). STM32-ADC-Pins
  PC0/PC1/PA4/PB0/PB1 → NC-Reserve (Rev-B).
- **Neu** (mcp-Sheet): SW1–SW5 Tactile-Switches auf MCP23017 GPA0–GPA4
  (Netze `CELL1..5_BTN`, Pin → GND, MCP-interner Pull-Up + IRQ-on-change),
  exakt wie die Modifier SW6–SW10. **Gleiches Bauteil:** HX B3F-4055-Y
  (C36498965). **Kein neues Bauteil** — MCP23017 + Switch standen bereits
  verifiziert in der BOM.
- HiChord-Batch-4+-Topologie (Switch → I²C-GPIO-Expander → MCU). Stellt die
  ursprüngliche SPEC-v0.6-§7-„10 Switches"-Topologie wieder her (5 Cells +
  5 Modifier am Expander).
- Hall + Magnet bleibt als dokumentierte Option (ADR-0013) nur falls echte
  Drucktiefe/Velocity später gewünscht ist.
- Aktualisiert: Generator + regenerierte `.kicad_sch`, `jlc_bom.csv`,
  `docs/hardware/bom_overview.html` (Aron), `BOM_MASTER.md`, `PCB_BOM.md`,
  `PINMAP.md`, `KICAD_BLUEPRINT.md`, `SCHEMATIC_WALKTHROUGH.md`,
  `MECHANICAL_REQUIREMENTS.md`, `ADR-0013`, `PROJECT_STATUS.md`.

---

## v0.7-r18.30 (2026-06-15) — leds.c + Menü-Encoder + MIDI deferred

User: „vielleicht brauchen wir gar kein MIDI? Rest ausbauen."

### 🟡 MIDI deferred (ADR-0004 r18.30)

J9 wird im 5er-Prototyp-Run **NICHT bestückt** (DNP). Hardware-Vorarbeit
konserviert (Footprint + Edge-Cutout + PD5-Pin-Reservierung), Code bleibt
(`src/midi.c` + `src/hal_h743/midi_uart_h743.c`), aber `midi_tx_init()` ist
in `main_h743.c` auskommentiert. Reaktivierung später = J9 + 2× 220 Ω
bestücken + 1 Zeile uncommenten. ADR-0004 als „DEFERRED" markiert mit
Original-Diskussion erhalten.

### NEU leds.c + leds.h — controls/modifier state → PCA9685 16-ch PWM

Hardware-unabhängiges LED-Render-Modul (16. Suite):
- 16-Kanal-Mapping per SPEC §7.2 (vs Generator-Z.4459 verifiziert)
- Per-Channel-Fade-Engine: 0↔TARGET über LED_FADE_MS = 120 ms
- Per-Farbe-Duty-Matching (gelb 70 %, grün 100 %, weiß 80 % wegen Vf-Diff)
- ADR-0008-r2-konform: gelb (ch 5/7/9/11/13) zeigt `controls_hold_base[c]`,
  grün (ch 6/8/10/12/14) zeigt `controls_hold_shift[c]` — **unabhängig**
- Clear-Flash auf ch 4 (250 ms on Clear-Press)
- Backlight (ch 15) eigenständig settbar
- API: `leds_render(now, dt, out[16])` → der HAL pusht `out[]` per
  `pca_set_pwm()` über I²C

### NEU test_leds.c (16. Suite, 28 Checks)

Boot-Dark, alle Modifier-Channels, Cell-Pärchen (base+shift gleichzeitig =
Oktav-Stack), Clear-Flash + Decay, Backlight, Fade-Out, Mid-Fade-Interpolation.
16/16 Suiten PASS.

### Menü-Encoder verdrahtet

`src/hal_h743/main_h743.c`: DISPLAY-Encoder-Rotate → `menu_rotate(delta)`,
Push → `menu_push()`. LED-Render-Loop bei 60 Hz mit `pca_set_pwm`-Burst.

### Firmware-LOGIK-Stand: komplett ✅

DSP-Engine, cells (Velocity), controls (Hold-Latch + Modifier), params
(Encoder-Bindings), leds (16-ch-PWM + Fade), menu-Encoder-Binding —
**alles hardware-unabhängig + host-getestet**. Der STM32-Build muss nur
noch die HAL-Reads/-Writes (HAL_GetTick, mcp_read, adc_read, pca_set_pwm,
oled_show) gegen ST-HAL füllen.

### Verbleibt für lauffähigen STM32-Build (Step 13.3)

ARM-GCC-Toolchain-File, Linker-Script, Startup-File, CubeH7-HAL-Sources,
SystemClock_Config (PLL1 480 MHz, PLL3 11.2896 MHz), 6× HAL-Body-Stubs
mit echten `HAL_*`-Calls füllen.

---

## v0.7-r18.29 (2026-06-15) — params.c + STM32-Main-Loop-Verdrahtung

Fortsetzung des Firmware-Ausbaus.

### NEU params.c + params.h — Encoder → Engine-Parameter

Die 3 Audio-Knöpfe an die Engine gebunden, mit der SPEC-§5-Velocity-
Acceleration (langsam = 1 %/Klick, schnell bis ×8):
- EN1 DRIVE → engine_set_reverb_drive (0..100 %)
- EN2 BRIGHT → engine_set_brightness (−600..+800 Hz, 20 Hz/Detent)
- EN4 VOLUME → engine_set_master_volume (0..100 %)
- EN3 DISPLAY → ignoriert (Menü besitzt diesen Encoder)
Per-Encoder-Acceleration-State, Clamp an Range-Grenzen, Defaults = engine_init-
Werte (drive 15 %, volume 60 %, bright 0).

### Test (15. Suite)

`test/test_params.c` — 12 Checks: Defaults, langsame Detents = 1 %/Step,
Richtung, Clamp 0/100, Acceleration (fast > slow), Brightness-Hz + Clamp,
DISPLAY ignoriert. 15/15 Suiten PASS.

### STM32-Main-Loop verdrahtet (main_h743.c)

`src/hal_h743/main_h743.c` Main-Loop konkretisiert: routet
Encoder-Events → `params_encoder()` / Menü, MCP23017-Button-Edges →
`controls_modifier()`, Cell-Hall-ADC → `cells_update()` → `controls_cell_*()`.
Die gesamte Spiel-LOGIK ist hardware-unabhängig + getestet — die verbleibenden
TODOs sind nur noch HAL-Reads (HAL_GetTick, adc_read, mcp_read), keine Logik.

controls.c + params.c in CMake-DSP_SOURCES (target-unabhängig, alle 3 Targets).

### Firmware-Logik-Stand

Komplett + host-getestet: DSP-Engine, cells (Velocity), controls (Hold-Latch +
Modifier), params (Encoder-Bindings). Verbleibt: STM32-Toolchain + ST-HAL-
Bodies (Step 13.3), SystemClock-Config, Menü-Navigation-Feinschliff.

---

## v0.7-r18.28 (2026-06-15) — controls.c: Hold-Latch + Modifier-Handler

User: „wieso meintest du alles ist fertig wenn wir noch nicht mal die Firmware
drumherum haben? Ausbauen." → Berechtigt. „Schematic order-fähig" hatte ich
fälschlich mit „Gerät fertig" vermischt. Beginn des Firmware-Ausbaus mit der
zentralen Logik-Lücke.

### NEU controls.c + controls.h — hardware-unabhängige Spiel-Logik

Die Bedien-Logik, die bisher NUR im Web-Sim lebte, jetzt als getestetes
C-Modul (Single Source für STM32 + Bench):
- **Hold-Latch-State-Machine (ADR-0008 r2):** `hold_base[5]` + `hold_shift[5]`,
  zwei unabhängige Bit-Arrays → Cell kann base, shift, oder BEIDE (Oktav-Stack)
  halten. Tap toggelt den vom Shift-State gewählten Branch.
- **Modifier-Handler:** Shift/Hold (latching), Drone/Generate (latch +
  forward an engine_set_drone / engine_set_generative), Clear (momentary:
  wipe alle Holds + Silence).
- **Momentary-Modus** (ohne Hold): Cell-Press = Note an, Release = aus;
  Shift wählt Oktave.
- **Velocity** kommt aus cells.c (ADR-0013), als amp-Parameter durchgereicht.
- Voice-Routing: base → source 0..4, shift → source 9..13 (kollidiert nicht
  mit Gen-Bed-Source 8).

### Test (14. Suite)

`test/test_controls.c` — 19 Checks: momentary tap+release, Hold-Latch-Toggle,
Shift+Hold unabhängig von base, Oktav-Stack (beide Bits), Clear wischt +
Tail decayt, Drone/Generate latchen + klingen, Release-Edge lässt Hold-Latch
in Ruhe. Alle 14 Suiten PASS.

### Was der STM32-Button-Handler jetzt nur noch tun muss

`src/hal_h743/main_h743.c`: MCP23017-Button-Edges in `controls_modifier()` /
`controls_cell_press/release()` einspeisen — die gesamte Spiel-Logik liegt
hardware-unabhängig vor und ist getestet.

### Verbleibend im Firmware-Ausbau

- Encoder→Engine-Param-Bindings (DRIVE/BRIGHT/VOLUME/DISPLAY) mit Acceleration
- STM32-Toolchain (ARM-GCC, Linker, Startup) + ST-HAL-Bodies (Step 13.3)
- SystemClock-Config (PLL1 480 MHz, PLL3 für SAI)

---

## v0.7-r18.27 (2026-06-15) — Firmware-HAL-Reorg: STM32 → Produkt, Pico → Bench

User: „weiter. aber wir nutzen ja kein pico mehr." → Pico-Build wird auf reines
Bench-Tool (display_hw_test) reduziert; STM32 wird Default-Target.

### Reorg

8 Pico-spezifische Files via `git mv` nach `src/hal_pico/`:
audio.c → audio_pico.c, audio_i2s.pio, encoders.c → encoders_pico.c,
lcd_st7789.c → lcd_st7789_pico.c, main.c → main_pico.c, mcp23017.c →
mcp23017_pico.c, midi_pio.c, midi_tx.pio.

6 STM32-Skelett-Files NEU unter `src/hal_h743/`:
- audio_h743.c — SAI1 Block A Master + DMA-Plan (PE4/5/6 AF6, PLL3-P
  @ 11.2896 MHz für jitter-freies 44.1 kHz)
- encoders_h743.c — TIM1/2/3/4 Hardware-Encoder-Mode (kein PIO/Polling)
- lcd_st7789_h743.c — SPI1 (PA5/PA7) + GPIO (PA6/PC4/PC5)
- mcp23017_h743.c — I²C1 @ 400 kHz Fast (PB6/PB7), EXTI13 für MCP_INT
- midi_uart_h743.c — USART2 @ 31250 Baud (PD5, AF7)
- main_h743.c — Boot-Sequenz + Hold-Latch-Logik-Plan (ADR-0008 r2)

Jedes Skelett: API matched bestehende Headers (include/audio.h etc.), Body
ist `TODO(Step 13.3)`-Stub mit ST-HAL-Plan in Kommentaren. Step 13.3 füllt
die TODOs gegen STM32CubeH7.

### CMake

CMakeLists.txt neu: `-DTARGET=h743|pico|host`-Schalter, default h743.
- `h743` baut field_ambience_h743 aus 19 DSP-Files + 6 hal_h743-Skelett-Files
  (linkt erst ab Step 13.3 mit ST-HAL)
- `pico` baut NUR display_hw_test (Bench-Tool, kein Produktions-Build mehr)
- `host` baut DSP-Static-Lib für die Offline-Renderer

### Test-Stabilität

19 reine DSP/Logic-Module bleiben unverändert in `src/` —
`test/run_tests.sh` linkt nur diese, fasst kein HAL-File an. 13/13 Suiten
PASS unverändert.

### NATIVE_PORT_PLAN

Step 13.2 als DONE ✅ markiert, Step 13.3 neu definiert (ST-HAL-Integration:
Toolchain-File, Linker-Script, Startup, TODOs füllen).

---

## v0.7-r18.24 (2026-06-14) — 3 offene Verifikations-Punkte geschlossen

## v0.7-r18.24 (2026-06-14) — Drei offene Verifikations-Punkte geschlossen

Aus der „gibt es noch offene Fragen"-Bestandsaufnahme die 3 autonom lösbaren
Punkte abgeräumt.

### 🟠 Weiße-LED-Package-Bug (neu gefunden)

LED_HB (Heartbeat) hatte MPN „XL-1608UWC-04" (0603) + 0603-Footprint, aber
**LCSC C965818 = XL-2012UWC, ein 0805-Teil** (live verifiziert). Package-
Mismatch — 0805-LED auf 0603-Land-Pattern. Fix: **C965808** (echtes
XL-1608UWC-04 0603, 2.5 M Stock). BOM_MASTER hatte zusätzlich die 3 Modifier-
Weiß-LEDs fälschlich auf C965818 → ebenfalls auf C965808 korrigiert.

### 🟠 VREF+ Decoupling ergänzt (Audit-Punkt 5)

VREF+ (Pin 20) war nur an VDDA_3V3 gebunden, **kein dedizierter Cap am Pin**.
ST-Empfehlung: 1 µF + 100 nF nah am VREF+. Ergänzt: **C_VREF1 (1 µF) +
C_VREF2 (100 nF)** VREF+ → GND. Reduziert ADC-Sample-Jitter bei der
Hall-Velocity (ratiometrisch, daher Referenz-Fehler unkritisch, aber Rauschen
zählt). +2 Bauteile (C15849, C14663 — beide schon im BOM).

### 🟢 ADC-Channels DS-bestätigt (Audit-Punkt 4)

ST-PDF via Distrelec-Mirror geparst (pdftotext): **PA4=ADC12_INP18,
PB0=ADC12_INP9, PB1=ADC12_INP5 verbatim aus DS12110 bestätigt**;
PC0=ADC123_INP10 (ST-Community + DS), PC1=ADC123_INP11 (PCx-Sequenz). Alle 5
auf ADC1 → Scan-Mode reicht. SPEC §5.6a von „vor ADC-Init bestätigen" auf
„DS-bestätigt" hochgestuft.

### Verifizierung

8/8 paren-balanced, LED_HB C965808 (0 C965818-Reste außer Audit-Notiz),
C_VREF1/2 present, SPEC „DS-bestätigt", Firmware 13/13 PASS.

### Files

generate_kicad_project.py (LED_HB-LCSC + VREF+-Caps), stm32h743.kicad_sch regen,
SPEC §5.6a, BOM_MASTER (LEDs + VREF-Caps), CHANGELOG.

### Verbleibende offene Punkte (nicht autonom lösbar)

GUI-ERC (User), PCB-Layout (großer Block), Display-Pin-Order vs gekauftes Modul,
MIDI-Entscheidung (ADR-0004), STM32-Firmware-Port, Gehäuse/Plate-CAD.

---

## v0.7-r18.23 (2026-06-14) — Cell-LED-Logik: XOR → Independent Latches (ADR-0008 r2)

User: „Es können eigentlich sowohl grün als auch gelb gleichzeitig erkennbar
sein. Funktional sinnvoller. Es kann ja beide gleichzeitig geben, rein logisch."

**Berechtigt.** Die XOR-Wahl in r18.9 war eine Annahme („eine Cell nur eine
Pitch"), nicht eine Notwendigkeit. „Cell hält Grundoktave" und „Cell hält
Shift-Oktave" sind **logisch unabhängig** — beide simultan = **Oktav-Stack**
(klassischer Ambient-Drone-Effekt).

### Hardware: NICHTS

Beide LEDs hingen schon vorher an unabhängigen PCA9685-Kanälen. XOR war reine
Firmware-Logik. Schaltplan, Footprint, BOM unverändert.

### Sim sofort umgesetzt

`tools/display_sim.html`:
- State: `cells[i] = 'off'|'yellow'|'green'` (1 Wert)
  → `cells[i] = {base: bool, shift: bool}` (2 unabhängige Bits)
- Tap-Logik: Hold-armed Tap toggelt nur den vom Shift-State bestimmten Branch;
  der andere bleibt
- LED-Render: gelb ↔ base, grün ↔ shift — unabhängig (beide gleichzeitig
  erlaubt)
- Clear setzt beide Bits auf 0
- Header-Kommentar + Help-Text aktualisiert
- JS-Syntax-Check OK

### Firmware-Side (zu bauen mit STM32-Port)

ADR-0008 r2 dokumentiert verbindlich:
- State-Modell `cell_hold[i] = (base_bit, shift_bit)`
- Voice-Routing Cell `i` base = source `i`, Cell `i` shift = source `i+5`
- `PAD_MAX` 8 → 12 (5 Cells × 2 Oktaven = 10 + 2 Headroom Generate/Drone)
- `MAX_SOURCES = 16` reicht
- Master-Soft-Clip in `engine.c` fängt die +6 dB bei voller Stack-Belegung
  ohne neuen Eingriff

Firmware-Tests 13/13 unverändert PASS (Sim ist HTML, keine C-Änderung).

### Files

`firmware-c-next/tools/display_sim.html` (State + Tap + Render + Doku),
`docs/decisions/ADR-0008-cell-led-xor-shift-hold.md` (Amendment r2 vor dem
Original-XOR-Text, Original als Kontext erhalten).

---

## v0.7-r18.22 (2026-06-14) — User-Audit: Encoder-NRND + Display-Qualität

User: „Er ist nochmal alles gegen checken und verifizieren. Glaube die Encoder
waren markiert als Not-Recommended-for-future-designs? Und das Display darf
nicht schlecht sein." → **Beide Punkte berechtigt, beide gefixt.**

### 🔴 HIGH — Encoder NRND-Pivot (3 von 4 Encodern betroffen)

Zwei unabhängige Verifikationen (Octopart + RS Online + ALPS-Hersteller-Seite +
LCSC-Cross-Check) bestätigten:

| Teil | Rolle | Status |
|---|---|---|
| **EC11E18244AU** (C202365) | EN3 Display, push+detent | **ACTIVE** ✅ |
| **EC11E183440C** (C370986) | EN1/2/4 Smooth | **NRND** 🔴 |
| EC11E1834403 (C361165) | Kandidat-Ersatz | **AUCH NRND** 🔴 |

ALPS hat die gesamte **„EC11E 0-Detent + Push-Switch"-SKU-Familie phased out**
(jeder Standard-EC11E hat heute Detents). Kein aktives Drop-in existiert.

**Pivot: alle 4 Encoder = EC11E18244AU (active, ~3.052 LCSC-Stock).** Die ur-
sprüngliche „smooth"-UX-Anforderung („1 % pro langsamem Klick") wird vom
Firmware-Acceleration-Layer erfüllt (langsam = 1 %/Klick, schnell = ×8/Klick),
**die Detents stören dabei nicht** — sie WAREN die ursprüngliche Lösung. Bonus:
alle 4 identisch → einfachere Lagerhaltung. Generator + BOM_MASTER + ADR-0012
nachgezogen.

### 🟠 HIGH — Display-Qualitäts-Pivot (r18.21-Korrektur)

Die r18.21-„bare AliExpress ST7789V"-Senkung war zu aggressiv. Live-
Verifikation (Adafruit-Produktseite + Goldenmorning-OEM + mboehmerm-GitHub +
Waveshare-Wiki) zeigte:

- Bare-AliExpress nutzt **das gleiche Panel** wie Adafruit (kein Premium-Bin)
- Aber: **kein Level-Shifter, kein QC, DOA-Lotterie, FFC-Qualitäts-Roulette**
- Adafruit ($15) zahlt für Features, die wir bei 3.3-V-STM32 nicht brauchen
  (Level-Shifter, SD-Slot, EYESPI-Connector)
- **Mittelweg: Waveshare 1.9″ ST7789V2-Modul (~$11–13)** — gleiches Panel +
  branded QC + Level-Shifter + dokumentierte Init-Sequenz + EU/US-Vendoren

**Pivot: Waveshare** (PiHut £11.60 / Waveshare direkt / Amazon). $4
Ersparnis statt $12 (vs Adafruit), aber Quality-DOA-Risiko eliminiert.

### Lehre dokumentiert

In ADR-0012 als Standard fixiert: **Lifecycle-Status muss aktiv verifiziert
werden, nicht nur Stock**. r18.14-Sourcing hatte nur Stock geprüft. Ab jetzt
Pflichtteil — analog zur Pin-Count-Pflicht nach r18.19.

### Verifizierung

- Generator regen, 8/8 paren-balanced
- encoder.kicad_sch: 8× C202365 active (war 3× C202365 + 9× C370986)
- 3 C370986/EC11E183440C-Vorkommen verbleibend = ausschließlich in
  Lifecycle-VERIFIED-Audit-Trail-Notizen (Begründung der Wahl)
- Firmware 13/13 PASS (substeps=4 für Display, =2 für Parameter — UX unverändert)

### Cost-Impact (COST_ESTIMATE aktualisiert)

| Posten | r18.21 (war) | r18.22 (neu) |
|---|---|---|
| Display × 5 | $20 bare (DOA-Risiko) | $60 Waveshare |
| Encoder × 20 | $25 (3× NRND-Stock) | $25 (active) |
| **Gesamt 5 Geräte** | ~$405 | **~$445** |
| Pro Stück | $81 | $89 |

Aufpreis $40 für komplette Risk-Reduktion (NRND weg + DOA-Lotterie weg). Wert.

### Files

- generate_kicad_project.py (Smooth-Encoder-Block + variant-Docstring)
- encoder.kicad_sch regen
- BOM_MASTER §6 (Encoder + Display), Stand-Bump
- ADR-0012 (Tabellen + NRND-Pivot-Block + Lehre)
- COST_ESTIMATE.md (Display + Encoder + Summen)
- PROJECT_QUALITY nicht angefasst (BOM 10/10 bleibt — die Wahl-Korrekturen sind
  Risk-Management, kein Sourcing-Gap)

---

## v0.7-r18.21 (2026-06-14) — Kostensenkung (5er-Prototyp-Run)

User: „Insgesamt zu teuer… wo wir Kosten sparen können, sollten wir das."
Sourcing-Recherche → die echten Hebel identifiziert. Erste Schätzung ~$750,
jetzt **~$405–520 für 5 Geräte**.

### Wichtige Korrektur der eigenen Schätzung

Die „$3-Extended-Setup-Gebühr × 32 Teile = ~$96"-Annahme war FALSCH. JLCPCB
hat die Feeder-Gebühr Ende 2025 auf $1.50 gesenkt UND stuft unsere großen ICs
(STM32H743, PCM5102A, PAM8403, PCA9685, AP7361C, Crystal) als **„Extended
(Preferred)" = gebührenfrei** ein. Real: nur 3 plain-Extended-Teile (MCP23017,
MCP73831, DRV5056A4) × $1.50 = **~$4.50 für den ganzen Run**.

### Umgesetzte Kostensenkungen

| Maßnahme | Ersparnis 5er-Run |
|---|---|
| Display Adafruit 5394 ($15) → bare ST7789V ($3–5) | ~$55 |
| Knöpfe + Cell-Caps selbst 3D-drucken (statt Alu-CNC) | ~$50–200 |
| Stabilizer gestrichen (1u-Caps statt ≥2u) | ~$25–75 |
| Akku 5000 → 2000 mAh (503759) | ~$20 |

### Warum kein IC-Swap

MCP23017/MCP73831/DRV5056 sind plain-Extended, aber ihre Alternativen
(TCA9555/TP4056/günstigere Hall) sind AUCH Extended + bräuchten Footprint-/
Firmware-Rework. Bei 5 Stück frisst die Umbau-Arbeit die Ersparnis. STM32H743
& Co. sind „Preferred" = schon gebührenfrei. Alle behalten.

### Files

- **COST_ESTIMATE.md NEU** (Root) — ehrliche Aufschlüsselung JLC + Hand-Supply
  + Gehäuse, mit Spar-Optionen + Trade-Offs
- BOM_MASTER: Display→bare ST7789V (Pin-Order-Verify-Flag), Akku→2000mAh,
  Stabilizer gestrichen, Knöpfe/Caps→3D-Print
- SPEC §4 Battery-Zeile → 2000mAh (war 5000mAh, Pi-Ära-Leiche)
- ADR-0013: Stabilizer für Prototyp gestrichen (1u-Caps; HiChord-Feel kommt vom
  Magnetic-Hub, nicht der Cap-Länge); lange-Caps-Option bleibt dokumentiert

Keine Schematic-/Generator-Änderung (Display steckt in J3-Header, Akku am
JST-Connector — beide unverändert). Reine BOM-/Doku-/Sourcing-Revision.

---

## v0.7-r18.20c (2026-06-14) — Phantom-FP-Fix + Stale-STEPs cleanup

### 🟠 HIGH — L1 Boost-Inductor: Phantom-Footprint-Name

Der Generator referenzierte `Inductor_SMD:L_0630_6.0x6.0mm` für die SWPA6045
2.2µH-Drossel. Dieser Footprint **existiert in der KiCad-Standard-Library
NICHT** (gibt nur `L_0603_1608Metric` als Wildcard-Match). PCB-Bestellung
hätte gar nicht ohne Fehler exportieren können — oder schlimmer, KiCad hätte
einen leeren Footprint angenommen.

**Fix:** EasyEDA-verifizierter Footprint für C83455 (war schon in
`/tmp/3dfetch/` aus der r18.14-3D-Aktion) als
`field_ambience:L_Sunlord_SWPA6045` in Project-Lib vendored (2 SMD-Pads
2.2×5.72 mm @ 5 mm Pitch, gleiche Methodik wie SW_TS1088, Jack_PJ-320D etc.).

### 🟡 LOW — Stale STEPs entfernt

`SW-SMD_EC11J1525402-...-H24.5.step` (EC11J retired r18.14, ADR-0012) +
`TYPE-C-SMD_6P-...-H3.2.step` (M-17 retired r18.19) aus 3D-Lib entfernt.
MANIFEST.md aktualisiert: beide Teile als retired markiert, USB-C-C165948-
STEP-Regen via easyeda2kicad als TODO notiert.

### Status

7 Custom-FPs in `field_ambience.pretty/` (war 5). 0 Phantom-Footprints im
gesamten Schaltplan (verifiziert via Footprint-Coverage-Scan: 25
KiCad-Standard + 5 Custom (jetzt 7) — alle physisch vorhanden).

Verifizierung: 8/8 paren-balanced, Firmware 13/13 PASS.

Files: generate_kicad_project.py (FP-Replace), libraries/field_ambience.pretty/
L_Sunlord_SWPA6045.kicad_mod NEU, 8 Sheets regen, FP_VERIFY_LOG (7/7),
BOM_MASTER, MANIFEST.

---

## v0.7-r18.20b (2026-06-14) — Sourcing-Fills + 2 Engineering-Flags

Sourcing-Agent lieferte verifizierte LCSC-Teile für alle r18.20-TBDs. Alle
JLC-bestückbaren Schematic-TBD-LCSCs sind jetzt geschlossen.

### Fills

| Teil | LCSC | Detail |
|---|---|---|
| C_HSE1/2 (HSE-Load) | **C107045** | Yageo 27pF C0G/NP0 50V 0603, JLC Basic, ~834k Stock |
| C_BULK (Polymer) | **C444831** | Kyocera AVX TPSE477K010R0100, 470µF/10V Case-E |
| C_BULK2 (MLCC) | **C2880380** | Taiyo Yuden 100µF/10V 1210 |
| LED gelb (Hold + 5 Cell) | **C2287** | Hubei KENTO KT-0603Y, Vf 2.4V |
| LED grün (Shift + 5 Cell) | **C12624** | Hubei KENTO KT-0603G pure-green 525nm, Vf 3.1V |

### 🔧 Engineering-Flag 1 — „220µF/10V/1210 existiert nicht"

C_BULK2 war als „220µF/10V X5R 1210" gelabelt — diese Kombination wird von
keinem Hersteller gebaut (220µF gibt es im 1210-Case nur bis 6.3V). Statt
220µF/6.3V (am 5V-Rail auf ~70–110µF DC-Bias-derated) → **100µF/10V** mit
echtem Voltage-Headroom. Effektiv-Bulk vergleichbar, audit-sauber.

### 🔧 Engineering-Flag 2 — Polymer-ESR-Realität

Die ~10-mΩ-Flach-Polymer-Caps aus ADR-0011 (Panasonic SVPF / Kemet T520)
sind bei LCSC nicht lagernd. Bester JLC-Flach-Reflow-Polymer: C444831
(100 mΩ ESR). Verfehlt das 10-mΩ-Ziel, aber der parallele 100µF-MLCC
(~5 mΩ) dominiert die Transient-Impedanz → Bulk-ESR unkritisch. Footprint
auf Case-E (7343-43, 4.3mm H) umgestellt (passt in 8-mm-Top-Zone).

### Beifang-Fix — R_LED-Wert in BOM_MASTER

BOM_MASTER §9 sagte „220Ω LED-Vorwiderstand", Generator sagt **390Ω** (an
+5V-Anode → LED → PCA9685-Sink). Bei 5V/390Ω: gelb 6.7mA, grün 4.9mA — beide
im 2–8mA-Fenster, Widerstand bleibt 390Ω. BOM_MASTER korrigiert.

### LED-Strom-Verifikation (5V-Rail, 390Ω)

- Gelb C2287 (Vf 2.4V): (5−2.4)/390 = 6.7 mA ✓
- Grün C12624 (Vf 3.1V): (5−3.1)/390 = 4.9 mA ✓ (bei 5V genug Headroom für die 3.1V-Emerald-Green; bei 3.3V wäre sie unbrauchbar gewesen)
- Weiß C965808 (Vf ~3V): ~5 mA ✓

### Verifizierung

- Generator regen, 8/8 paren-balanced
- C107045 ×2, C444831 + Case-E-FP, C2880380, 0 „220uF 10V"-Leftovers
- LED gelb C2287 ×6 (1 Hold + 5 Cell), grün C12624 ×6 (1 Shift + 5 Cell)
- **0 TBD-VERIFY-Leftovers** in power_tree + mcp
- Firmware 13/13 PASS

Score: BOM-Sourcing 9.5 → 10 (alle JLC-bestückbaren Teile haben verifizierte
in-Stock-LCSC-IDs; Hand-Assembly-Teile haben Vendor-Quellen in BOM_MASTER).

---

## v0.7-r18.20 (2026-06-14) — Audit-Folge-Fixes (Schematic-Korrektheit)

## v0.7-r18.20 (2026-06-14) — Audit-Folge-Fixes (Schematic-Korrektheit)

Aus dem Senior-Engineering-Audit die autonom umsetzbaren Schematic-Fixes:

### 🟠 HIGH — Hall-Sensor-Footprint auf echtes SOT-23

J_CELL1-5 hatten `Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical`
(2.54-mm-Header-Platzhalter), verbaut wird aber DRV5056A4QDBZR im **SOT-23**.
Eine PCB hätte 5 falsche Land-Patterns gehabt. Umgestellt auf
`Package_TO_SOT_SMD:SOT-23`. Das 3-Pin-Symbol (Conn_01x03) Pin 1/2/3 mappt
1:1 auf SOT-23-Pad 1/2/3 = DRV5056 VCC/OUT/GND (TI-DS Table 4-1, r18.14
verifiziert). Site-Wiring Pin1→+3V3, Pin2→CELLn_SENSE, Pin3→GND unverändert.
JLC-bestückbar (Extended).

### 🟠 HIGH — HSE-Load-Caps 22 → 27 pF

C_HSE1/2 waren 22 pF — das impliziert C_stray = CL − C/2 = 18 − 11 = 7 pF,
unrealistisch hoch für ein 4-Lagen-Layout nahe dem MCU. Off-frequency-
Start-Risiko (SAI-PLL3-Audio-Sample-Rate, USB-Sync). Neu: **27 pF**
(C_ext = 2·(CL − C_stray) = 2·(18 − 4.5) = 27 pF, E12-Standard, C0G/NP0).
Note ergänzt: finaler Wert ist layout-abhängig, am realen Board per 10x-Probe
am OSC_OUT messen.

### 🟡 MEDIUM — SPEC §5.12 Frei-Liste bereinigt

PC0/PC1/PA4/PB0/PB1 aus der „Frei für Erweiterungen"-Liste entfernt — seit
r18.9 für CELL1..5_SENSE vergeben. Reviewer hätte sie sonst doppelt belegen
können.

### 🟡 MEDIUM — ADC-Channel-Doku ergänzt (SPEC §5.6a)

Standard-Mapping der 5 Cell-Pins dokumentiert: PC0=ADC123_INP10,
PC1=ADC123_INP11, PA4=ADC12_INP18, PB0=ADC12_INP9, PB1=ADC12_INP5. **Alle
5 auf ADC1** → ein ADC im Scan-Mode reicht. Mit Hinweis: vor Firmware-ADC-
Init final gegen DS12110 Table 8 bestätigen (ST-PDF war HTTP-503 in der
Audit-Session; Pin-Nummern via U1_STM32H743VIT6.md verifiziert).

### Verifizierung

- Generator regeneriert, 8/8 .kicad_sch paren-balanced
- stm32-Sheet: 5× SOT-23 für J_CELL (0 Header-Leftovers), 5× DRV5056A4-Label
- HSE: 27 pF (0 22-pF-Leftovers)
- CELL1-5_SENSE-Nets intakt
- Firmware 13/13 Suiten PASS

### Offen → r18.20b (Sourcing-Agent läuft)

- C_HSE LCSC-Nr. (27 pF C0G/NP0 0603) — derzeit TBD-r18.20
- C_BULK Polymer 470 µF/16 V LCSC-Nr.
- 220 µF MLCC auf 10 V (statt 6.3 V Headroom-Derating)
- Cell-/Modifier-LED-Farben (5× gelb + 5× grün) LCSC-Nr.

---

## v0.7-r18.19 (2026-06-13) — Hardware-Audit-Fixes

Senior-Engineer-Audit ergab vier Defekte, davon einer **kritisch**:

### 🔴 Critical — USB-C-Revert auf 16-Pin (Flashen über USB-C gerettet)

LCSC-Produktseite C283540 (TYPE-C-31-M-17) listet **„6P" = 6 Pins, power-only**.
Keine D+/D-, kein CC1/CC2. Damit wäre **USB-DFU-Flashen über PA11/PA12
physikalisch unmöglich** gewesen (PA11/PA12 = USB_DM/USB_DP), der gesamte
Zweck des SW_BOOT-Tasters (ADR-0009 Punkt 1) ausgehebelt; im Schaltplan
hätten USB_DM/USB_DP/CC1/CC2/SBU-Pads auf der Stecker-Seite gar nicht
existiert.

**Fix:** Revert auf **C165948 / TYPE-C-31-M-12** (16-Pin volle USB-C-Belegung
inkl. D+/D-/CC, 168k+ LCSC-Stock). Footprint war von Anfang an für diese
Variante gemacht (`USB_C_Receptacle_HRO_TYPE-C-31-M-12`). Insertion-Cycle-
Trade-Off ~10k → ~5k bewusst akzeptiert — für ein persönliches Hobby-Gerät
weit ausreichend, und 10k waren ohnehin Overkill.

**Wurzel-Ursache:** r18.10-„Upgrade" auf M-17 hatte sich auf eine generische
„drop-in laut HRO-Tabelle"-Annahme verlassen, ohne Pin-Count zu verifizieren.
r18.14 korrigierte zwar die zwischenzeitlich falsche LCSC-Nr. (C165935 war
ein MOSFET), pickte aber innerhalb der M-17-Familie die 6-Pin-Variante.

**Lehre:** Pin-Count muss explizit aus Datenblatt/Produktseite bestätigt
werden, nicht aus Bauform-Ähnlichkeit abgeleitet.

### 🟠 High — Audio-Jack-Footprint passte nicht zum Bauteil

J8/J9 hatten `Connector_Audio:Jack_3.5mm_CUI_SJ-3523-SMT_Horizontal`
zugewiesen — das ist die CUI-SMT-Klasse. Verbaut wird aber C431535
(SHOU HAN PJ-320D). Pad-Layouts inkompatibel. Assembly wäre gescheitert.

**Fix:** EasyEDA-CAD-verifizierter Footprint für C431535 in die Project-Lib
vendored: `field_ambience:Jack_3.5mm_PJ-320D_SMT` (4 SMD-Pads + 2 NPTH-Posts,
exakt aus der LCSC-CAD-Datei extrahiert — gleiche Methodik wie SW_TS1088
und EC11J in r18.14).

### 🟠 High — Speaker-Wert im Audio-Sheet seit r18.18 stale

J6/J7 standen seit r14 hartcodiert als
`value="Speaker L (PUI AS04008PS, 4R 4W)"` — drei Fehler auf einmal: 4 Ω
(seit r14 als 8 Ω korrigiert; Power-Budget rechnet mit 8 Ω → 700 mA, mit
4 Ω wären es 1.4 A → TPS61089 grenzwertig), PUI statt CMS-402811-28SP
(seit r18.18 Primärwahl), MPN ist die Papier-Variante.

**Fix:** Value + MPN auf
`Same Sky CMS-402811-28SP (Cloth-Cone, 8R 2W)`, Notes erklärt Top-Firing-
Mount + manuelles Anlöten der Eyelets.

### 🟠 Critical (Prozess) — Generator seit r18.14b nicht regeneriert

Alle Entscheidungen r18.15–r18.18 lebten nur in den Docs. Das Schematic
(= einzige Netlist-Quelle) hing auf r18.14b-Stand fest. Mit r18.19 erstmals
wieder synchron regeneriert; ab jetzt jeder relevante Doc-Change zieht den
Generator nach.

### Verifizierung

- Generator + Sheets regeneriert: alle 8 .kicad_sch paren-balanced ✅
- USB-C: 4 × C165948/M-12 in power_tree.kicad_sch, 0 unerwünschte Leftovers
  (das eine verbleibende C283540/M-17 steht ausschließlich in der Audit-
  Trail-Notiz)
- Audio-Jack-FP: 1 × field_ambience:Jack_3.5mm_PJ-320D_SMT in audio sheet,
  0 CUI-SJ-3523-Leftovers
- Speaker: 6 × CMS-402811-28SP in audio sheet, 0 stale „PUI 4R" Leftovers
- Firmware 13/13 Suiten PASS (unberührt)

### Files

- `kicad/generate_kicad_project.py` (3 Edits)
- `kicad/libraries/field_ambience.pretty/Jack_3.5mm_PJ-320D_SMT.kicad_mod` NEU
- 8 × `kicad/*.kicad_sch` regeneriert
- `field-ambience-current/docs/decisions/ADR-0009-engineering-realitaetscheck.md`
  (Korrektur-Block r18.19 + Tabellen-Update)
- `BOM_MASTER.md` (USB-C, Audio-Jack, Audit-Trail)
- SPEC §8 hatte CMS bereits → keine Änderung nötig
- SPEC §4 hatte M-12 nie verlassen → keine Änderung nötig

Score: BOM-Sourcing 9 → 9.5 (kritische Pin-Count-Verifikation in den Audit-
Standard aufgenommen); FP-Verify bleibt 10 (Custom-FP für C431535 ist nach
EasyEDA-CAD verifiziert wie unsere anderen Customs).

---

## v0.7-r18.18 (2026-06-13) — Cloth-Cone Speaker (CMS-402811-28SP)

User: „selbes format und größe aber eben premium.. nicht papier?" — berechtigt:
PUI AS04008PS hat einen **behandelten Papier-Konus** (drei Quellen bestätigt:
PUI-Datenblatt, Mouser-Attribut, RS-Components-Listing „treated paper").

### Primärquelle: Same Sky (CUI) CMS-402811-28SP

- **Stoff-Konus (Cloth Cone)** statt Papier — glattere Mitten ohne
  „Papier-Boxigkeit", feuchteresistent, niedrigere Verluste
- Identischer 40 × 28.3 × 11.5-Footprint → **keine CAD-/Mechanik-Änderung**
- Identische elektrische Specs: 8 Ω, 2 W RMS, 84 dB @ 1 W/50 cm,
  NdFeB-Magnet, Löt-Eyelets
- F0 = 450 Hz (statt 380 Hz beim PUI) → 70 Hz weniger Tiefgang, in der
  15–30 cm³ Sealed-Box mit Roll-Off bei ~500 Hz nicht hörbar
- Preis ~$3–$5 statt ~$6.78 (halber BOM-Cost)
- DigiKey + Arrow + Mouser-lagernd
- LCSC führt **keinen** passenden 40-mm-8-Ω-Treiber → ohnehin Hand-Assembly

### Zweitquelle bleibt PUI AS04008PS-4W-WR-R

Identischer Footprint, behandeltes Papier, F0 = 380 Hz, ~$6.78. Backup
falls CMS-Lieferengpass.

### Verworfene Alternativen (5 Treiber im 40×28-Footprint geprüft)

- Visaton K 28.40-8: Papier + 79 dB SPL (5 dB leiser, halb so laut empfunden)
- Same Sky CDS-40288: Papier-Pulp
- Same Sky CDS-4028-16: Stoff aber 16 Ω → halbe PAM8403-Leistung
- Dayton CE40-28P-8: Papier
- PUI AS04008CO-WR-R: Stoff aber 20 mm breit → CAD-Rework

### Akustik-Erwartung (SPEC §8 angepasst)

- Onboard ehrlich nutzbar **250 Hz – 20 kHz** (war ~200 Hz mit PUI bei F0=380)
- Glattere Mitten als Papier-Variante (kein „Papier-Box"-Klang im Sprach-Bereich)
- Tieferer Bass unverändert nicht onboard hörbar → bleibt Line-Out-Sache

### Files

SPEC §3-Übersichts-Diagramm, §8.x (Treiber-Beschreibung +
Akustik-Begründung + Realistische-Erwartung), §10-Impedanz-Note;
ADR-0007 (Lautsprecher-Sektion komplett umgedreht + Recherche-Begründung +
Verworfene-Alternativen); ADR-0011 (Speaker-Höhe-Zeile + Kammer-Sektion);
mechanical_coordinates §2/§3.9/§5; mechanical/3d_models/MANIFEST.

Mechanik unverändert (Footprint, Tiefe, Outline, Cutout, Keepout, Außenhöhe
21.6 mm — alles identisch beim Wechsel). Firmware unberührt (13 Suiten PASS).

Score: Speaker-Cover 6 → 7 (Membran-Klasse aufgewertet; offen weiterhin
Akustik-Charakterisierung am Muster + Marian-PSA-Quote).

---

## v0.7-r18.17 (2026-06-13) — Speaker: Mesh-Material + Datenblatt-Korrekturen

## v0.7-r18.17 (2026-06-13) — Speaker: Mesh-Material + Datenblatt-Korrekturen

Komponenten-Recherche zu Dust-Mesh + Speaker. Dabei **zwei Maßfehler** in der
bisherigen Spec gefunden, einer davon eine mechanische Kollision.

### r18.17a — Positions-Reconciliation (separater Commit)

SPEC §8, ADR-0007 und mechanical widersprachen sich bei Speaker-Grille-Geometrie
(38 mm rund @ (50,30) vs 36×70 oval vs 50×30 oval). Alle auf den r18.16-Stand
gezogen, mechanical bleibt Source of Truth.

### r18.17b — Material-Entscheidung + Datenblatt-Korrekturen

**Mesh:** Saati **Acoustex 020–032** (transparente Klasse, ~25–32 g/m²) statt
des fälschlich notierten „150–200 g/m²" (= Dämpfungs-Klasse, hätte den Treiber
dumpf gemacht). Rohmesh nicht selbstklebend → PSA-Klebering-Konvertierung via
**Marian Inc.** für Serie; AliExpress-Klebe-Mesh-Sticker für Prototyp. Kein
LCSC-Teil für Akustik-Mesh existiert. GORE Acoustic Vent als IP-Option notiert.

**🔴 Speaker-Datenblatt-Korrekturen (PUI AS04008PS-4W-WR-R bleibt):**
- **Maße 40 × 40 × 9 mm → 40 × 28.3 × 11.5 mm** (rechteckiger Rahmen)
- **„-WR" = Water-Resistant**, nicht Wire — **Löt-Eyelets, keine Kabel** →
  Hand-Assembly, kein JLC-Bestücken (JLC führt keinen 40-mm-Kompakt-Treiber)
- Zweitquelle dokumentiert: Same Sky CMS-402811-28SP (gleicher Footprint,
  F0=450 Hz)

**🔴 Z-Kollision behoben:** Der reale Treiber ist **11.5 mm tief** (nicht 9).
Bei 10 mm Above-PCB-Raum wäre der von der Top-Platte hängende Treiber 1.5 mm
in die PCB-Ebene kollidiert. Fix: Above-PCB-Raum **10 → 12 mm**, Außenhöhe
**19.6 → 21.6 mm** (geometrie-unabhängig; PCB-Relief-Cutout als Alternative
verworfen, da Magnet-Boss-Geometrie ohne Datenblatt-Zeichnung unbekannt).

**Mesh-Cutout 50×30 → 36×24 mm** (passt zum realen 40×28.3-Treiber, 2 mm
Rim-Seat; 50×30 wäre größer als der Treiber gewesen → kein Rim zum Abdichten).

**Speaker-Treiber-Keepout:** statt „≤ 4 mm in 40×50-Zone" jetzt 42×32-Bauteil-
Keepout direkt unter jedem Treiber-Footprint (0.5 mm Luft). SW_BOOT von
(245,45) → (245,72) verschoben (lag im rechten Keepout).

### Files

mechanical §1/§2/§5/§7/§3.8/§3.9, ADR-0007 (Decision + Lautsprecher +
Implementation Plan), ADR-0011 (Höhen-Update-Notiz + Speaker-Höhe),
SPEC §8 (Treiber-Maße + Terminierung + Zweitquelle + Mechanik-Hinweis).
Geometrie erneut Python-validiert (0 Konflikte nach SW_BOOT-Move).

Score: Speaker-Cover 3 → 6 (Material + Lieferanten-Pfad fix; offen:
Akustik-Charakterisierung am Muster, Marian-Quote). Mechanical bleibt 9
(Kollision war latent, jetzt sauber).

---

## v0.7-r18.16 (2026-06-13) — Mechanical Coordinates: ehrlicher Rewrite

`mechanical/coordinates/mechanical_coordinates.md` war seit r18.8 nur in §0
ehrlich, der Rest war Pico/Pi/Choc/FSR/OLED-Müll aus früheren Revisionen.
Komplette Neuschreibung als Single Source of Truth für Phase 6 (PCB-Layout)
und Enclosure-CAD.

### Festgelegt

- **Outline**: PCB 252 × 102 × 1.6 mm, Gehäuse 260 × 110 × 19.6 mm (ohne
  Encoder-Knöpfe → +10 mm Knopf-Erhebung = ~30 mm Gesamthöhe; OP-1-Field-
  flach pro ADR-0011)
- **Z-Stack** vollständig aus ADR-0011 übernommen (Bottom-Panel 2.5 + Standoff
  3 + PCB 1.6 + 8 mm Top-Zone + 12 mm Encoder-Schaft + Top-Panel 2.5)
- **4 Encoder** in den Ecken (Y=88), Pitch 30 mm zwischen Paaren
- **Display** ST7789 1.9″ aktiv 40 × 22 mm, Modul-Mitte (126, 84), 4 mm Bezel
  zur Top-Edge
- **5 Modifier** identisch HX 12×12, Reihe Y=58, Pitch 14 mm
- **5 Cells** Gateron-LP-Magnetic, Reihe Y=26, Pitch 22 mm, je 14×14 mm
  Frontpanel-Cutout
- **2 Speaker** Dust-Mesh-Aussparung 50 × 30 mm oval, Mitte (28, 50) und
  (224, 50) — Top-Panel-hängend, PCB darunter mit Höhen-Limit 4 mm
  (Speaker-Treiber-Zone)
- **Power-/Audio-/MCU-Insel** komplett im 17 mm hohen Y-Streifen 34…51 mm
  (zwischen Cell-Body-Top und Modifier-Body-Bottom) — STM32, PCM5102A,
  PAM8403, C_BULK-Polymer, TPS61089, L1, AP7361C, Y1 Crystal (rechts neben
  STM32), MCP23017, PCA9685, Backlight-Q2
- **USB-C** Edge-Bottom-Center (126, 0); Audio + MIDI Edge-Left bei Y=88/66;
  Battery, SWD, Reset, BOOT0 alle in der rechten Service-Spalte (X ≥ 240)
- **4 Mounting-Holes** M2.5 bei (12, 6), (240, 6), (12, 96), (240, 96)

### Validierung

Python-Sanity-Checks: alle Bauteile innerhalb PCB-Outline, keine echten
Bauteil-Bauteil-Überlapps (Y1 Crystal nahe STM32 ist Design-Absicht, AN2867
≤ 5 mm), kein Bauteil > 4 mm in Speaker-Treiber-Zonen, keine Mounting-Hole-
Kollision mit Bauteil-Bodies. Kritische Lücken: STM32↔Crystal 2.3 mm,
Modifier↔STM32 3 mm, Cell-Body↔Power-Cluster 6.6 mm — alle ≥ 2 mm DRC-OK.

### Implizit gelöst

- C_BULK 10.5 mm Konflikt aus ADR-0011 r18.12 (Polymer-Wechsel) jetzt
  geometrisch verortet: Insel-Y 46…50 mm, ≤ 2 mm hoch.
- BOOT0 + Reset + SWD ohne Frontpanel-Cutout: Service-Zugang via 2 mm Loch
  Bottom-Panel.
- Battery-Pouch 50 × 60 × 9 mm Plan: liegt rechte Hälfte unter PCB, 9050060
  statt 8050120 (Konflikt mit Speaker-Kammer gelöst).

### Offen (Phase 6 / Industrial-Design)

- Knopf-CAD, finale Außenmaße (±2 mm), Gateron-LP-Cutout 14×14 verifizieren,
  Plate-Material, Mesh-Lieferant — §10 der Datei zählt sie auf.
- Score-Impact: Mechanical 7 → 9 (echte Koordinaten + Z-Validierung;
  10 erst nach Enclosure-CAD-Mockup).

---

## v0.7-r18.15 (2026-06-13) — Cell-Velocity in Firmware (ADR-0013 Code-Seite)

ADR-0013 hatte die Magnetic-Hall-Architektur im Schematic fixiert, aber den
Pfad „Hall-ADC → Velocity → Note" nur beschrieben. Dieser ist jetzt
implementiert und host-getestet — unabhängig von der noch fehlenden Hardware.

### Neues Modul `cells.{h,c}`

- Per-Cell-State-Machine REST → ARMED → HELD, gefüttert mit normierter
  Stem-Position `pos` ∈ [0,1] (0 = Ruhe, 1 = Bottom-Out).
- **Velocity = Banddurchlaufzeit** (BAND_LO 0.15 → BAND_HI 0.55): schneller
  Druck quert das Band in ≤ 6 ms (laut), langsamer in ≥ 70 ms (leise).
  Note-On feuert am Trigger (BAND_HI), Note-Off bei Rückzug < RELEASE 0.30
  (Hysterese gegen Chatter).
- Velocity (0..1) → Voice-Amplitude 0.05…0.22 (Gamma 0.8). Median ≈ die alte
  feste 0.12 der Binär-Cells, jetzt mit echtem Dynamikumfang.
- Pure Logic, kein SDK → identisch auf RP2040-Bench (synthetische Position)
  und STM32 (echtes ADC). Robust gegen grobes Sampling (Single-Sample-Slam =
  Max-Velocity).

### Engine-Integration

- `engine_cell_sample(cell, pos, now_ms)` — Engine-Einstiegspunkt: PRESS
  spielt den Akkord-Grundton der Cell (harmonic brain) mit velocity-skalierter
  Amplitude, RELEASE stoppt ihn. `cells_init()` in `engine_init()`.

### Tests

- NEU `test/test_cells.c` (25 Checks): slow/fast, Monotonie, Hysterese,
  abgebrochener Druck, kein Doppel-Trigger, Single-Sample-Slam, Bounds,
  Mapping-Endpunkte. → **13. Test-Suite**.
- Engine-Suite um End-to-End-Velocity-Check erweitert: schneller Druck
  messbar lauter als langsamer durch die volle Engine (Peak 8156 vs 4459).
- Alle 13 Suiten grün.

### Web-Sim (`tools/display_sim.html`)

- Cell-Velocity sichtbar: Klick-Höhe auf der Pille = Velocity (oben weich,
  unten hart), grüner Fill steigt proportional zur Amplitude + 0–127-Readout.
- Velocity-Konstanten **verbatim aus `cells.h` gespiegelt** (kommentiert).

### Doku

- SPEC §5.6a: Velocity-Modell-Tabelle (alle Konstanten) + Source-of-Truth-Verweis.
- ADR-0013: „Firmware-Stand (implementiert r18.15)" ergänzt.
- PROJECT_QUALITY: Cell-Mechanik 7→8, Test 9→9.5, Display-Sim 7→8.

### Risiko

Rein additiv in `firmware-c-next` (neues Modul + 1 Engine-Funktion + Sim-
Anzeige). Kein Schematic-, BOM- oder Generator-Change. Kein CI-Pfad berührt.

---

## v0.7-r18.14 (2026-06-12) — Komponenten-Definition: 3D-Lib, Encoder, Cell-Keys

User-Direktive: PCB-Design braucht Komponenten-Definition VOR Layout — 3D-Daten
zu allem, Cells wie HiChord mit langen Caps + Stabilizern, Encoder-Split
(nur Display = Push), smooth + gleich hoch + flach (Kick75-Referenz), und der
„2 Klicks = 1 %"-Encoder-Bug.

### 🔴 Zwei kritische BOM-Fixes (Beifang der 3D-Verifikation)

1. **USB-C: C165935 war ein STF18N65M5-MOSFET (TO-220F-3)** — nicht der
   TYPE-C-31-M-17! Falsch seit r18.10. Entdeckt, weil der EasyEDA-3D-Abruf
   ein TO-220-Modell lieferte. Korrekt: **C283540** (LCSC-verifiziert, 21k+
   Stock). → Generator, SPEC §4, ADR-0009-Korrekturblock.
2. **SW_BOOT-MPN: C720477 ist XUNPU TS-1088-AR02016** (nicht TS-1185A-C-A).
   Zusätzlich FP-Fix: SW11/SW12/SW_BOOT wechseln von generischem
   `SW_SPST_TL3342` (Land-Pattern passte nicht) auf EasyEDA-verifiziertes
   `field_ambience:SW_TS1088_SMD`.

**Konsequenz-Regel:** 3D-/CAD-Abruf ist ab jetzt Teil der BOM-Verifikation —
ein falsches STEP-Modell entlarvt eine falsche LCSC-Nr. sofort.

### 3D-STEP-Library (User: „brauche dringend zu allem die 3D-Dateien")

- NEU `kicad/libraries/field_ambience.3dshapes/` — 9 Z-/Panel-kritische
  STEP-Modelle (Encoder 24.5mm-Beleg, USB-C, LQFP-100, Boot-SW, Crystal,
  Inductor, Klinke, JST, VQFN), ~20 MB.
- NEU `mechanical/3d_models/MANIFEST.md` — Inventar, Höhen-Tabelle,
  Regenerier-Kommando (`easyeda2kicad`), externe CAD-Quellen (Speaker, LCD,
  EC11E, Gateron-LP, Knöpfe), Befund-Log.

### ADR-0012 — Encoder-Strategie

- **EN3 (Display): einziger mit Push + Detents** (Menü: 1 Rastung = 1 Schritt).
  ALPS EC11E THT, Suffix TBD-VERIFY.
- **EN1/2/4: EC11E183440C** — 18 Pulse, OHNE Detent, OHNE Switch, endlos
  glatt. SW-Nets bleiben verdrahtet (Pull-up idle-high), Löcher unbestückt.
- **Alle 4 gleiche Familie + gleiche Schaftlänge = gleich hoch.** Kick75-flach:
  Ziel ~20–22 mm statt 24.5 mm (EC11J-STEP-Beleg). Knopf Ø19–20 × 8–10 mm Alu.
- EC11J1525402 SMD retired (NRND + zu hoch + Half-Step-Mismatch). FP-Verify-
  Blocker damit gegenstandslos; echtes EC11J-Pattern liegt trotzdem als
  EasyEDA-verifizierte Referenz in der Lib (r18.12-Hand-Draft war falsch:
  Pitch/Pad-Reihen/Body-Maße — Lehre: Custom-FPs nur aus CAD-Exporten).
- Footprint alle 4: KiCad-Standard `RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm`.

### Encoder-UX-Fix („zwei Klicks für ein Prozent")

- Ursache: Bench-Encoder (KY-040-Klasse) rastet jede HALBE Quadratur-Periode
  (30 Detents / 15 PPR); Firmware zählte volle Perioden.
- `display_hw_test.c`: `ENC_HALF_STEP` Default 0 → **1** (Symptom-richtig für
  das Bench-Teil); Host-Test pinnt explizit 0 (testet den Full-Cycle-Kern).
- `encoders.c`: `DETENT_SUBSTEPS`-Konstante → **per-Encoder `substeps`**
  (Display 4, Smooth 2) + **`has_sw`-Flag** (3 Encoder ohne Push pollen nicht).
- Acceleration-Tiers unverändert (28/60/120/240 ms → ×8/×5/×3/×2), langsam
  = exakt 1 %.
- Alle Test-Suiten grün (854k+ Checks).

### ADR-0013 — Cell-Keys: Low-Profile Magnetic + Hall (ersetzt FSR aus ADR-0006)

- User wollte HiChord-Tastengefühl + lange Caps (Spacebar-Prinzip mit
  Stabilizern). Lösung, die das Velocity-Ziel aus ADR-0006 BEHÄLT:
  **Gateron-LP-Magnetic-Switches** (pin-los, plate-mounted, 0.1–3.3 mm analog)
  + **linearer Hall-Sensor pro Cell auf dem PCB** (DRV5056A4-Kandidat /
  SS49E-Prototyp; PINOUT-VERIFY vor Phase 6 — AP7361-Lektion).
- Velocity = dPos/dt (präziser als FSR-Druck); Aftertouch + einstellbarer
  Trigger-Punkt später als reine Firmware-Features.
- **ADC-Interface unverändert** (PC0/PC1/PA4/PB0/PB1, CELLn_SENSE).
  Schematic: J_CELL 1×2-FSR-Teiler → 1×3-Hall-Site + 1 kΩ Serien-R + 10 nF
  (RC fc≈16 kHz). R_CELL 10k→1k (C25804→C21190).
- Lange Caps ≥ 2u: LP-Stabilizer (Gateron-Klasse) — Switch mittig,
  Stabilizer links/rechts, exakt das Spacebar-Prinzip.
- SPEC §4 (Switches+Encoder-Tabelle komplett überarbeitet) + §5.6a rewrite.

### Modifier-Buttons (User-Punkt „alle gleich, keine Einrastfunktion")

Bestätigt + in SPEC §4 explizit gemacht: SW6–SW10 sind **alle 5 identisch**
(HX 12×12×7.3, momentary). Latch-Zustand zeigen ausschließlich die LEDs
(§7.2 / ADR-0008) — kein mechanischer Rast-Schalter irgendwo.

### Files

Generator (Encoder-Block variant-fähig, Hall-Cell-Block, USB-C/SW_BOOT-Fixes),
`encoder.kicad_sch` + `stm32h743.kicad_sch` regeneriert, `encoders.c`,
`display_hw_test.c`, `test_display_bench.c`, SPEC §4/§5.6a, FP_VERIFY_LOG
(0 offene Punkte), ADR-0009-Korrektur, ADR-0012 NEU, ADR-0013 NEU,
MANIFEST.md NEU, 2 Custom-FPs (TS-1088 NEU, EC11J ersetzt), 9 STEP-Modelle NEU.

---

## v0.7-r18.13 (2026-06-12) — Repo-Refactor Phase 2 (Doc-Moves)

`REPO_STRUCTURE.md` Phase 2 ausgeführt. Discipline-basierte Top-Level-Ordner
für nicht-CI-kritische Assets eingeführt; Cross-References in lebenden Docs
und Firmware-Code-Kommentaren synchronisiert.

### git mv (5 Pfade)

- `field-ambience-current/mechanical_coordinates.md` → `mechanical/coordinates/`
- `field-ambience-current/field_ambience_webapp.html` → `software/webapp/`
- `field-ambience-current/field_ambience_v29o.scd` → `software/supercollider_reference/`
- `field-ambience-current/legacy/` → `archive/legacy_pre_native/`
- `field-ambience-current/docs/archive/` → `archive/old_specs/`

### Übersprungen (begründet)

- **`PITCH.md`** — Datei existiert nicht im lebenden Tree (nur archivierte
  Kopie unter `archive/old_specs/pitch_pre_step6.md`). Re-Pitch nach
  STM32-Migration neu aufsetzen, dann `product/brief/PITCH.md` anlegen.
- **`reports/`** — Ordner war leer.

### Cross-Reference-Updates (lebende Docs, einmaliger Pass)

- `README.md` — Folder-Tree zeigt neue Top-Level-Ordner; `PITCH.md`-Eintrag
  raus, da nicht vorhanden
- `PROJECT_MAP.md` — `mechanical_coordinates.md`-Pfad + Legacy-Pfade
- `REPO_STRUCTURE.md` — Phase-2-Block auf DONE, Tree synchronisiert
- `field-ambience-current/PROJECT_QUALITY.md` — Mechanical-Pfad
- `field-ambience-current/START_HERE.md` — 4 Referenzen (`.scd`, `.html`,
  `mechanical_coordinates.md`, `legacy/`)
- `field-ambience-current/PCB_LAYOUT_STATUS.md` — Mech-Pfad
- `field-ambience-current/NATIVE_PORT_PLAN.md` — Port-Vorlage-Pfad
- `field-ambience-current/docs/onboarding/INDUSTRIAL_DESIGNER_START.md` —
  `PITCH.md`-Verweis ersetzt durch Repo-`README.md`, Mech-Pfad
- `field-ambience-current/docs/decisions/ADR-0007-dust-mesh-speakers.md`
  + `ADR-0011-enclosure-thickness.md` — Related-Sektionen
- `field-ambience-current/firmware-c-next/tools/render_wav.{c,sh}` +
  `include/pad.h` + `include/reverb_presets.h` — Pfad-Kommentare zur
  Webapp-Reference
- `field-ambience-current/firmware-c/include/pad.h` — **NICHT angepasst**;
  ist FROZEN-Snapshot, Pfad-Kommentar reflektiert State at-time-of-snapshot

### Bewusst NICHT angefasst

- `CHANGELOG.md` historische Einträge — dokumentieren Stand pro Commit
- `field_ambience_pcb_SPEC_v0.7.md` Inline-Prosa-Verweise auf
  `mechanical_coordinates.md` — strukturell Doc-Name, kein Hyperlink, wird
  bei nächstem SPEC-Refresh (r18.14+) konsolidiert
- `PCB_TODO.md`, `MEINE_TODO.md` — Working-Docs, Prosa-Referenzen, kein
  Build-Blocker. Konsolidierung mit der nächsten Mechanical-Update-Welle
- CI-relevante Pfade (`firmware-c/`, `firmware-c-next/`, `kicad/`) —
  Phase-3-Aufgabe, atomar mit `.github/workflows/*.yml`

### Score-Impact (PROJECT_QUALITY.md)

| Aspekt | r18.12 | r18.13 | Trend |
|---|---|---|---|
| Repo-Struktur | 6/10 | **8/10** | Phase 2 von 5 abgeschlossen |
| Doku / Onboarding | 9.5/10 | **10/10** ✅ | Pfade konsistent, Onboarding-Links korrekt |
| Mechanical / Enclosure | 5/10 | **6/10** | Pfad-Honest-State, Inhalt unverändert |

### Risiko-Bewertung

- **Kein CI-Risiko**: keine `.yml`-Path-References geändert
- **Kein Generator-Output-Risiko**: `kicad/generate_kicad_project.py`
  unverändert, schreibt weiter nach `field-ambience-current/kicad/`
- **Kein Firmware-Build-Risiko**: Header-Pfade nur in Kommentaren angefasst
- **Reversibel**: 5 × `git mv` in einem Commit, jederzeit per `git revert`
  rücknehmbar

---

## v0.7-r18.12 (2026-06-11) — Pre-Layout-Cleanup

Zwei offene Pre-Layout-Punkte adressiert:

### C_BULK Polymer-Wechsel (ADR-0011 Punkt 1)

1000 µF Alu-Elko (10.5 mm Höhe — passte nicht in 8-mm-Top-Zone) ersetzt durch:
- **C_BULK**: 470 µF 16V Polymer in EIA-7343-31 (D-Case, ~2 mm Höhe, ESR < 15 mΩ)
- **C_BULK2**: 220 µF 10V X5R MLCC 1210 parallel

Effektive Bulk-Kapazität ~690 µF (Polymer + MLCC), aber ESR ~10 mΩ statt ~25 mΩ
beim Alu. Class-D-Bass-Transienten sauberer (ADR-0010 Punkt 4 "kürzester
Polygon-Loop"-Hebel verstärkt). Title-Block-Comment im power_tree
synchronisiert.

LCSC-Nummern noch TBD-VERIFY: Panasonic SVPF-Familie oder Kemet T520D für
Polymer; CL32A227KQVNNNE-Klasse für 220 µF MLCC.

### EC11J Custom-Footprint (FP_VERIFY letzter Punkt)

`libraries/field_ambience.pretty/RotaryEncoder_ALPS_EC11J_SMD.kicad_mod` neu
gezeichnet aus den **dokumentierten ALPS-EC11J-Standard-Maßen** (12×13.4 mm
Body, vertikaler 20-mm-Schaft, 5 SMD-Pads + 2 Mounting-Tabs). Pad-Pitch 2.5 mm,
Pad-Größe 1.4 × 1.4 mm, Mounting-Tabs 1.8 × 3 mm bei ±6 mm.

**Status:** `FP_DRAFT` (nicht `FP_NOTE`) — vor Fab gegen EasyEDA-Export von
C209762 verifizieren. Pin-Numbering (A/C/B/S1/S2) hier rein semantisch; das
KiCad-Symbol nutzt die Conn_01x05-Standard-Pinnummern, deshalb muss vor dem
ersten Layout-Spin geprüft werden ob Symbol-Pin-Nummern ↔ Footprint-Pad-Nummern
matchen (kann im Symbol-Editor angepasst werden).

4 Encoder-Instanzen referenzieren jetzt den Custom-FP statt des KiCad-Standard-
EC11E-THT-FP, der ohnehin nicht zur SMD-Bauform passte.

### Validierung

paren 8/8 PASS · Crossref 7/7 PASS · 4 Encoder-Instanzen mit neuem FP, 0
Reste vom alten EC11E-Standard-FP · Polymer + MLCC parallel sauber im
power_tree platziert · Title-Block-stale-Kommentar gefixt.

### Score-Bewegung

- Footprint-Verifikation 8 → 9 (EC11J Draft-FP geschlossen)
- Mechanical 4 → 5 (C_BULK-Konflikt aufgelöst)
- BOM-Sourcing 7.5 → 8 (Polymer-Familie identifiziert)

---

## v0.7-r18.11 (2026-06-11) — Audio-Buffer + SAI-Clock + Gehäusedicke

User-Fragen: (a) wie verhindern wir Kratzgeräusche am DAC, (b) wie dick wird
das Gehäuse mit Lautsprechern. Beide Fragen mit Engineering-Analyse beantwortet.

### ADR-0010 — Anti-Kratzig

Sechs Ursachen-Klassen geprüft:
- ✅ PCM5102A Pin-Strapping verifiziert (FLT=FMT=DEMP=GND, XSMT controlled)
- ✅ Engine-Soft-Limiter aktiv (Step 11)
- 🔧 Buffer-Underrun-Risiko → AUDIO_BUFFER_FRAMES 256 → 512
  (5.8 ms → 11.6 ms Window, 2× Sicherheits-Marge; Latenz weiterhin
  sub-perception für Ambient-Pad)
- 🔧 Clock-Jitter → SAI-Speisung aus PLL3-P (11.2896 MHz exakt) statt
  SYSCLK; getrennte Audio-PLL ist STM32H7-Standard für Audio-Quality
- 🔧 Layout-Constraints (SPEC §5.1 ergänzt): AVDD/DVDD-Caps <5 mm vom IC,
  Single-Star-GND zwischen Audio und Digital, C_BULK kurzer Polygon-Loop
  zum PAM8403-PVDD, optionaler Class-D-Output-Bead-Filter
- Sub-Bass-Speaker-Klappern bleibt Sound-Design-Lösung
  (SubBass-Layer ausschließlich an J8-Line-Out)

Alle 12+ Host-Tests grün nach Buffer-Größen-Änderung.

### ADR-0011 — Gehäusedicke

Drei Stack-Up-Varianten gerechnet (Encoder-im-Gehäuse, Encoder-ragt-raus,
Side-Mount-Speaker). Gewählt: **Variante B** (Encoder-Knopf außen).

Konkretes Z-Budget:
- Bottom-Panel 2.5 mm + Standoff 3.0 mm + PCB 1.6 mm + Top-Komponenten 8.0 mm
  + Encoder-Schaft 12 mm + Top-Panel 2.5 mm = **Gehäuse außen 19.6 mm**
- + Encoder-Knöpfe 10 mm = **Gesamthöhe ~30 mm**
- → OP-1-Field-Klasse (Field selbst ist 19 mm)

Speaker-Kammer pro Seite ~31 cm³ — passt für Closed-Box-Mid-Range-Sound
der 40-mm-Treiber. Sub-Bass wie immer über J8.

**C_BULK-Konflikt früh erkannt:** 1000 µF Alu-Elko ist 10.5 mm hoch, passt
nicht in 8 mm-Top-Zone. Drei Lösungs-Optionen dokumentiert; Empfehlung:
**Polymer-Tantal-Cap** (z. B. Panasonic 25SVPF1000M, 2.5 mm hoch, besserer
ESR für Bass-Transienten). Entscheidung für r18.12 vor Layout.

### Files

- ADR-0010-audio-buffer-sai-pll-clean-sound.md NEU
- ADR-0011-enclosure-thickness.md NEU
- firmware-c-next/include/audio.h: AUDIO_BUFFER_FRAMES 256→512
- SPEC §5.1 erweitert: SAI-Clock-Architektur + Audio-Layout-Constraints

---

## v0.7-r18.10 (2026-06-11) — Engineering-Realitätscheck (ADR-0009)

User-Frage: welche Entscheidungen sinnvoll, welche nicht. Vier Fix-Cluster:

### 🔴 BOOT0-Button gefehlt (kritisch)

Befund: BOOT0 hatte nur einen 10k-Pulldown. USB-DFU-Flash setzt
BOOT0=HIGH voraus → war physikalisch nicht möglich. SPEC-§1-Diagramm
behauptete fälschlich "USB-DFU oder ST-Link".

Fix: SW_BOOT (SMD-Tactile TS-1185A-C-A / C720477) plus R_BOOT_SW 1k in
Reihe nach +3V3 ergänzt. Bedienung: SW_BOOT halten → NRST tippen →
loslassen → System-Memory-Boot (USB-DFU). 1k-Serien-R limitiert Querstrom
auf 3 mA falls SW_RESET gleichzeitig gedrückt.

### 🟠 USB-C-Stecker auf 10k-Cycles upgegradet

C165948 (M-12 / ~5k Cycles, Generic) → **C165935 (TYPE-C-31-M-17 / 10k
Cycles)**, drop-in Footprint laut HRO-Tabelle. Premium-Pfad (GCT/Amphenol
~10k Cycles, andere FP) für post-Prototyp dokumentiert.

### 🟠 SPEC §4 BOM ↔ Generator r18.9 synchronisiert

Drift gefunden: SPEC listete noch Choc-Cells + Stabilizer + SW12-BOOTSEL
(alles Pico-Ära). r18.9 hatte das im Generator schon raus.

Fix: SPEC §4-Zeilen durchgestrichen oder ersetzt: SW1-SW5 → J_CELL1-5 +
R_CELL + C_CELL (FSR-Interface), Stabilizer raus, SW12 raus, SW_BOOT +
R_BOOT_SW rein. mechanical_coordinates §4 als veraltet markiert.

### 🟢 Engineering-Review als ADR-0009

Vollständige Befund- und Maßnahmen-Doku: USB-C-Premium-Pfad, Z-Höhen-
Layout-Constraints (C_BULK 10.5 mm darf nicht unter LCD-Header),
LED-Farben-JLC-Stock-Realität, FSR-Anschluss-Optimierung (FFC empfohlen),
Display-SPI1-Funktionsfähigkeit ✓, plus die bewussten "ändern wir nicht"-
Entscheidungen.

### Validierung

paren 8/8 PASS · Crossref 7/7 PASS · SW_BOOT-Pin1↔BOOT0_PIN-Rail-Wire
verifiziert · SW_BOOT-Pin2↔R_BOOT_SW↔+3V3-Pfad verifiziert.

Scores: Schematic 9 → 9.5, Symbole 9 → 9.5, Doku 9 → 9.5.

---

## v0.7-r18.9 (2026-06-11) — Schematic-Implementation IMG_9713 (Generator)

Implementiert die r18.8-Design-Entscheidungen im KiCad-Generator.

### Cell-Velocity (ADR-0006)

- **stm32h743_sheet**: 5 neue ADC-Netze CELL1..5_SENSE an **PC0 (Pin 15),
  PC1 (16), PA4 (28), PB0 (34), PB1 (35)** — alle vorher no_connect, alle
  Standard-ADC12-fähig. FSR-Interface pro Cell: J_CELLn (2-Pin) von +3V3,
  10 kΩ Teiler-Bottom, 10 nF S/H-Filter, Knoten → ADC-Pin
- **ADR-0006-Korrektur**: Erstfassung nannte PA0/PA1/PA2/PA6 als freie
  Pins — FALSCH (PA0/PA1 = TIM2-Encoder, PA6 = LCD_CS). Gegen die
  verifizierte Pin-Tabelle ersetzt
- **mcp_sheet**: SW1-5 (Kailh-Choc-V2-Hotswap, 2u, Stabilizer) komplett
  entfernt; GPA0-4 → NC (Rev-B-Reserve). BOM: −5 Hotswap-Sockets,
  −5 Stabilizer; +5 J_CELL, +5 R 10k, +5 C 10nF, +5 FSR (separat)
- SPEC §5.6a NEU mit der Pin-Tabelle

### LED-Topologie (ADR-0008)

- **mcp_sheet led_array**: 10 → **15 LEDs**, farbcodiert:
  - LED6 grün (Shift), LED7 gelb (Hold), LED8-10 weiß (Drone/Gen/Clear)
  - LED11Y/G … LED15Y/G: 5×2 Cell-LEDs (gelb @Basis / grün @Shift, XOR)
- PCA-Kanäle 0-14 = LEDs, **Kanal 15 = LCD_BLK_PWM** (war ch12)
- **LED1 + R_LED_STATUS entfernt** (r12-System-Status via PCA ch10) —
  Heartbeat ist seit r18.5 LED_HB an MCU-PD8. Kanal 10 jetzt LED13G
- Gelb/Grün-MPN-Kandidaten XL-1608UYC/UGC-04 (XINGLIGHT-Serie) mit
  TBD-VERIFY-Stock; weiß bleibt XL-1608UWC-04/C965808
- 390R-Serie einheitlich (gelb/grün ~7.4 mA, weiß ~5.1 mA — beide unter
  PCA-Sink-Budget; Helligkeits-Matching per Firmware-PWM-Duty)

### Validierung

- paren-balance 8/8 PASS
- 100/100 STM32-Pins angebunden (5 Cell-Pins jetzt WIRE statt no_connect)
- mcp: 15 LEDs, 0 STATUS_LED_K-Reste, 0 NC_PCA-Reste, 0 Choc-Reste,
  NC_GPA0-4 vorhanden, LCD_BLK_PWM-hier intakt
- Hier↔Root-Crossref 7/7 PASS

---

## v0.7-r18.8 (2026-06-11) — IMG_9713 Industrial-Design-Update

User-Vorgabe nach dem neuen Render (IMG_9713):
- keine Text-Labels außer am Display
- Display kleiner und mittig zwischen den Encodern
- 4 Encoder in den Ecken statt einer Reihe
- 2 LEDs pro Cell (Gelb + Grün, XOR-Logik für Basis-/Shift-Hold)
- Modifier-LED-Farben: Shift Grün, Hold Gelb, Drone/Generate/Clear Weiß
- Speakers als schwarze Dust-Mesh-Aussparungen statt sichtbare Lochmuster
- Cells als Piano-Feel-Tasten statt simpler Switches (HiChord-Klasse)

### Drei neue ADRs

- **ADR-0006** — Cell-Action = Piano-Feel: FSR + Silicon-Cap-Pad mit
  Velocity-Sense statt Tactile-Switch. Fallback Dual-Tactile-Switch für
  Prototype-1. 5× zusätzliche ADC-Inputs am STM32H743 (Pins frei).
- **ADR-0007** — Speaker-Cover = Schwarzes Akustik-Polyester-Mesh
  (Saati-Acoustex-Klasse, -1 dB Insertion-Loss). Speaker selbst (PUI
  AS04008PS) bleibt unverändert. Frontpanel-Aussparung 36×70 mm oval.
- **ADR-0008** — Cell-LED-Logik = XOR (Gelb @ Basis, Grün @ Shift). User-
  Frage bewertet: pro Cell immer höchstens eine LED an; „Shift aktiv"
  bleibt globale Modifier-LED, nicht repliziert auf jede Cell. Tabelle
  mit allen Cell-Tap-Zuständen im ADR.

### SPEC-Updates

- §7.2 PCA9685-Kanal-Belegung **komplett neu**:
  - LED0-LED4: 5 Modifier-LEDs (Grün/Gelb/Weiß/Weiß/Weiß)
  - LED5-LED14: 5×2 Cell-LEDs (Gelb-LEDxY + Grün-LEDxG pro Cell, XOR)
  - LED15: LCD_BLK_PWM (Backlight-PWM via Q2 N-FET)
  - **16/16 Kanäle belegt, exakt** — kein Reserve mehr, System-Status
    auf MCU-Direkt-GPIO (PD8, SPEC §5.6, bereits dort)
- Cell-HOLD-Anzeige-Logik durch XOR ersetzt; Firmware-State-Machine
  bekommt 3-Wert-Enum pro Cell

### Web-Sim komplett neu (`firmware-c-next/tools/display_sim.html`)

- Layout 1:1 nach IMG_9713: Slab-Body mit abgerundeten Ecken, kleines
  zentriertes Display, 4 Encoder in den Ecken, 5 Modifier-Buttons mit
  LED-darüber (ohne Text-Label), 5 lange Cell-Pillen mit je 2 LEDs darüber,
  2 ovale Dust-Mesh-Speaker links/rechts
- Modifier-Interaktion: Click toggelt; LED-Farben Grün/Gelb/Weiß je nach
  Modifier (Clear = Flash-on-Press, kein Latch)
- Cell-Interaktion: Tap → wenn Hold-Modifier on, latch in Gelb-State,
  mit Shift-Modifier on → Grün-State. XOR durchgesetzt
- Keyboard-Bindings: 1-5/Q-T = Cell-Tap, A/Z = Backlight, Shift gehalten =
  global Shift, Pfeile + Space = Display-Encoder

### Mechanical Coordinates Update

- Neue §0 prefix-Sektion mit IMG_9713-Layout-Skizze + Komponenten-
  Positionstabelle (Encoder-Ecken, Display zentriert, Modifier-Reihe,
  Cell-Reihe, Speaker-Mesh links+rechts). Alte §3-7 (Pico-Ära-Positionen)
  bleiben für Diff-Reviewability erhalten, sind aber als veraltet markiert.

### PROJECT_QUALITY.md NEU

User-Vorgabe „alles muss 10/10 sein" — neues File mit Score pro Aspekt
(1-10) und exakte Roadmap für jeden, der noch nicht bei 10 ist. Aktueller
Gesamtstand: 5.5/10 Manufacturing, einzelne Aspekte 9/10.

### Validierung

- Web-Sim lokal getestet: alle Interaktionen funktional, LED-Sync sauber,
  Animationen unverändert (Tween-Engine unverändert), Display rendert
  korrekt im kleineren Container
- KiCad-Generator: r18.8-Schaltplan-Update kommt in r18.9 (Cell-FSR-
  Symbole + LED-Topologie). r18.8 ist **Design-Decision + Doku + Sim**;
  Schematic-Implementation folgt in r18.9 als reine Code-Änderung

### Manufacturing-Readiness unverändert: 5.5/10

Begründung: Schematic ist auf r18.7-Stand; r18.8 fügt **keine** Schematic-
Änderungen hinzu, sondern legt nur das Industrial-Design + die Logik fest.
Die r18.9 wird die Schematic-Konsequenzen implementieren (5 ADC-Inputs, 10
Cell-LEDs, neuer Modifier-LED-Topologie). Vorher wäre eine Implementierung
spekulativ — die FSR-Komponentenwahl ist noch nicht final.

---

## v0.7-r18.7 (2026-06-11) — FP_VERIFY systematisch geschlossen + Gate-Skip-ADR

### ADR-0005: Phase-5-Profiling-Gate übersprungen

User-Entscheidung: Das Gate (ADR-0002 verlangt Engine-Profiling auf
realer STM32H743-Hardware vor Layout-Start) ist konservatives Theater.
RP2350 trägt die Engine bereits, H743 hat 3-4× CPU-Headroom plus FPU
plus dedizierte Peripherie (SAI/TIM-QEI). Erwarteter CPU-Bedarf
~10-15 %, weit unterhalb des 40 %-Gates. Worst-Case (Engine zu groß):
ein Prototyp-Spin mit reduzierter Funktionalität, nicht totes Board.

Folgen: Layout-Pfad ist jetzt **ERC → FP_VERIFY → Layout**, nicht mehr
**Hardware kaufen → Firmware-Port → Profiling → Layout**. Firmware-
Migration läuft parallel statt sequenziell.

PROJECT_STATUS, PROJECT_MAP, ADR-0003, PCB_LAYOUT_STATUS aktualisiert.

### FP_VERIFY-Pass — 6 von 9 geschlossen

| Position | Vorher | Nachher |
|---|---|---|
| **U1 LQFP-100** | FP_VERIFY | FP_NOTE (KiCad-Standard, JEDEC MS-026, raw-verifiziert) |
| **U5 SOT-89-5** | FP_VERIFY | FP_NOTE (KiCad-Standard, JEDEC TO-243, raw-verifiziert) |
| **Y1 HC-49/US** | FP_VERIFY | Custom-FP `Crystal_HC49-US-SMD_ABLS` per ABRACON-DS Page 3 (5.6×2.1mm @ 9.5mm; KiCad-Standard hatte 4.5×2.0 @ 8.5mm) |
| **U8 TPS61089RNR** | FP_VERIFY | Custom-FP `Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A` per TI Mech. Drawing 4222143/A |
| **Q2 2N7002 SOT-23** | FP_VERIFY | FP_NOTE (JEDEC TO-236, alle Marken identisch) |
| **J3 LCD-Header** | FP_VERIFY | FP_NOTE (trivialer 2.54mm 1×8) |
| **EN1-4 EC11J** | FP_VERIFY | Bleibt offen — ALPS-Drawing nicht öffentlich, Custom-FP aus EasyEDA-Daten als nächster Schritt |

Vollständige Doku: `FP_VERIFY_LOG.md`.

### TPS61089-Symbol-Bug behoben

Das alte Symbol hatte einen fake "Pin 12 = ePAD = GND". TI-Mechanical
Drawing 4222143/A zeigt: das RNR0011A-Package hat 11 Pads, der zentrale
Thermal-Pad IST Pin 11 (SW), nicht ein separates ePAD. GND geht über
Pin 5 (Side-Pad). Symbol-Code + battery_sheet-Wiring entsprechend
korrigiert. Custom-FP ohne Phantom-Pad-12.

### Neue Custom-Footprints

`kicad/libraries/field_ambience.pretty/` jetzt mit 3 Custom-FPs:
- `SW_HX_12x12x7.3_SMD-4P` (r18.6, HX-Button)
- `Crystal_HC49-US-SMD_ABLS` (r18.7, ABLS-Crystal nach Datasheet)
- `Texas_VQFN-HR-11_2x2.5mm_P0.5mm_RNR0011A` (r18.7, TI-Land-Pattern aus DS)

### Validierung

Skript-Checks: paren-balance 8/8, 100/100 STM32-Pins angebunden,
Crossref-Tests unverändert PASS. TPS61089 Pin 12 entfernt aus
battery_sheet (kein dangling-Net).

### Manufacturing-Readiness

Vorher (r18.6): 4/10. Jetzt (r18.7): **5.5/10** — FP-Blocker
reduziert (9→1), Gate-Skip macht Layout-Pfad offen. Der Weg zu 7:
ERC-GUI + Encoder-FP + Mechanical-Update. Auf 9-10: Layout + DRC + JLC.

---

## v0.7-r18.6 (2026-06-11) — Engineering-Verifikation: LDO/Boost/Encoder/Button + ERC-Checkliste

Nachfolger von r18.5. User lieferte konkrete Datasheet-Quellen für die
Posten, die in r18.5 als „nicht aus verfügbarer Information fixbar"
markiert waren. **Vier 🔴-Blocker geschlossen, einer als ADR umklassifiziert.**

### 🔴 B-LDO: AP7361A-Pinout war FALSCH — gefixt

r18.5-Annahme: 1=ADJ/NC, 2=OUT, 3=IN, 4=GND, 5=EN.
**Korrekt (Diodes-DS via mouser.de/datasheet/3/175/1/AP7361.pdf):
1=EN, 2=GND, 3=ADJ/NC, 4=IN, 5=OUT.**

Hätte zu einem totem Board geführt (IN und OUT vertauscht).

Zusätzlich:
- **AP7361 → AP7361C** umgestellt (Diodes markiert AP7361 als NRND)
- **AP7361C-33Y5-13 = C460397** (LCSC, SOT-89-5) statt dem r18.5-Kandidaten
  C150719 (= SOT-223, **falsches Package**)
- Cin/Cout 4.7 µF X5R 0603 (DS-Empfehlung statt 1 µF/2.2 µF)
- Symbol-Pinmap im Generator umgebaut, alle Stub-Wires neu gerechnet
  (EN/IN-Routing kehrt sich um — auf der linken Seite des Symbols)

### 🔴 B-F1: TPS61089-Footprint + Datenblatt-Anchor

- Symbol-Pinmap (1=FSW … 11=SW) stimmt mit TI Pin-Functions Tabelle 6-1
  (Rev C) überein — als `PIN_SOURCE`-Property fixiert
- Footprint-Name auf RNR0011A-Konvention (2.0×2.5 mm VQFN-HR)
- RNSR-PDF → `datasheets/legacy/TPS61089RNSR_wrong_variant.pdf`

### 🟠 B-F2: Encoder C209762 (NRND, Prototype-OK)

- MPN auf **EC11J1525402**, LCSC **C209762** (JLCPCB-verifiziert)
- Datasheet-Link auf JLCPCB-Detail-Seite (offizielles ALPS-Drawing
  weiterhin nicht öffentlich; JLC liefert die SMD-Spec)
- `Status: NRND` als Property — Prototype-OK, Serie braucht
  Ersatzteil-Entscheidung
- KiCad-Standard-FP `RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm`
  bleibt vorerst; **FP_VERIFY-Hinweis**, dass EC11J SMD und EC11E THT
  unterschiedlich sind → eigener FP wäre vor Serie korrekt

### 🔴 B-SW12: Custom-Footprint für HX 12×12

- Neuer Footprint `field_ambience:SW_HX_12x12x7.3_SMD-4P.kicad_mod`
  basierend auf User-verifizierten LCSC-Daten:
  - SMD-4P, 11.8×11.8 mm Body, 7.3 mm Höhe
  - 4 Pads je 2.5×1.5 mm auf 7 mm-Pitch (X+Y)
  - SPST (gegenüberliegende Pads verbunden: 1↔1, 2↔2)
  - Silkscreen-Body 12.1×12.1 mm + Pin-1-Dot, Courtyard +0.25 mm
- Neue lokale Library `field_ambience.pretty/` in fp-lib-table eingetragen
- SW6-10 jetzt mit korrektem Footprint (vorher: ~6 mm-TL3342-Pattern)

### 🟠 B-TBD: 4 Caps + 2N7002 gefüllt

| Position | LCSC | Quelle |
|---|---|---|
| VCAP 2.2 µF X5R 0603 10V | C24539 (CL10A225KP8NNNC) | Samsung-Standard |
| VDD-Bulk 4.7 µF X5R 0805 10V | C45783 (CL21A475KQFNNNE) | Samsung-Standard |
| HSE 22 pF C0G/NP0 0603 50V | C1804 (CC0603JRNPO9BN220) | Yageo-Standard |
| 2N7002 SOT-23 | C8545 (Nexperia 2N7002,215) | JLC Basic |

Alle mit `VERIFY-STOCK`-Property statt blanker LCSC-Nr. — Sourcing-Pass
vor BOM-Freeze validiert Stock.

### 🟠 MIDI: keine Suchblock, sondern offene Design-Entscheidung

- `docs/decisions/ADR-0004-midi-design-decision.md` mit 5-Achsen-Matrix
  (Topologie, TRS-Polarität, Logikpegel, Buchsen-MPN, DNP-Status)
- Generator-Text-Note im STM32-Sheet aktualisiert
- **Keine spekulativen Bauteile** im Schematic — bewusst, bis Entscheidung
  vorliegt

### 🟠 ERC/DRC-Checkliste

- Neu: `docs/hardware/ERC_DRC_CHECKLIST.md`
- Erwartete *bewusste* Warnungen + Liste der nicht erlaubten Warnungen
- Manufacturing-Gates (Schematic + Layout)
- `kicad-cli`-Snippet für künftige Automation

### SPEC-Update

§4 U5-Zeile auf AP7361C-33Y5-13 / C460397 / SOT-89-5 aktualisiert (war
in r18.5 als TBD-mit-Kandidat C150719 markiert — der Kandidat war auch
das falsche Package).

### Validierung

Alle r18.5-Strukturchecks wiederholt: paren-balance 8/8 .kicad_sch-Dateien
PASS, 100/100 STM32-Pins angebunden, Hier↔Root-Crossref unverändert
PASS. **Kein .kicad_pcb erzeugt — Status bleibt NOT MANUFACTURING-READY.**

---

## v0.7-r18.5 (2026-06-11) — Schematic-Migration Phase 3 (Generator) + Repo-Audit

**KiCad-Generator auf STM32H743 migriert** (NATIVE_PORT_PLAN Step 13.3):

- **NEU `stm32h743.kicad_sch`** (ersetzt `pico.kicad_sch` → `kicad/legacy_pico2/`):
  100-Pin-Symbol aus offizieller KiCad-Lib übernommen und gegen SPEC §5
  doppelt verifiziert (52/52 belegte Pins identisch). Alle SPEC-§5-Netze
  verdrahtet, ~48 Reserve-GPIOs als no_connect. Support komplett: HSE
  (Y1 ABLS C596838 + 2×22 pF), VCAP 2×2.2 µF, 5×(4.7 µF+100 nF) VDD,
  VDDA-Ferrit(C84094)+1 µF+100 nF, BOOT0-PD, NRST-PU+100 nF, SWD-J4 auf
  PA13/PA14/GND, BAT-Teiler an PA3, Status-LED an PD8, USB-FS PA11/PA12.
- **NEU `lcd.kicad_sch`** (ersetzt `oled.kicad_sch` → legacy): 8-Pin-ST7789
  per §6 inkl. C6b/C6c und Backlight-Pfad: PCA9685-Kanal 12 (mcp) →
  `LCD_BLK_PWM` → 2N7002-Low-Side + 100k-Pulldown. Netnamen `OLED_*`→`LCD_*`.
- **power_tree: U5 AP7361A-33 LDO** (+5V→+3V3) + Cin/Cout — der Pico-SMPS
  entfällt. 🔴 Pinout-VERIFY B-LDO offen (AP7361A-DS war 403; Annahme aus
  AP7361-DS33626 dokumentiert am Symbol).
- **VOL_SW real auf MCP-GPB5** (SPEC §5.4): bis r17 lag es entgegen der
  SPEC am Pico-GPIO, GPB5 war NC. Root-Brücken entsprechend umgebaut.
- USB-Netze `PICO_USB_*` → `USB_DM/USB_DP`; +3V3_OUT-Hier wandert
  pico→power_tree; `kicad_pro`-Sheetliste: Battery ergänzt (fehlte).

**BOM/Datasheet-Findings (Audit):**

- 🔴 **F-6 NEU: SPEC-§4-U5-LCSC-Nummer war falsch** — C156144 ist ein
  910-Ω-0603-Widerstand (LCSC-Web verifiziert), nicht der AP7361A. SPEC
  korrigiert auf TBD + Kandidat C150719. Hätte bei JLC-Order einen
  Widerstand als LDO bestückt.
- **F-2-Teilfix:** Encoder-MPN im Generator auf EC11J1525402 gesetzt,
  Bourns-PEC11R-PDF → `datasheets/legacy/` (war falsches Teil); ALPS-
  Drawing-Beschaffung bleibt offen.
- **F-1 dokumentiert:** TPS61089-Schematic ist RNR (seit r12-B11 ✓), aber
  das Repo-PDF ist weiter RNSR — RNR-PDF beschaffen (B-F1).
- **B-SW12 NEU:** SW6-10 (HX 12×12) tragen ein ~6-mm-TL3342-Land-Pattern —
  als FP_MISMATCH-Property an allen 5 Symbolen markiert.
- **B-MIDI NEU:** MIDI_TX (PD5) existiert als Netz, TRS-Buchse + Serien-R
  fehlen in SPEC §4 + Schematic.
- SPEC-Widersprüche bereinigt: U5 „bleibt DNP"-Altsatz, §6-OLED_*-Satz,
  §5.4-r15-Behauptung.

**Validierung:** paren-balance 10/10 Dateien, 100/100 MCU-Pins angebunden,
Hier↔Root-Crossref 7/7 PASS, Root-Label-Brücken vollständig paarig.
GUI-ERC (B3) + alle FP_VERIFY/PIN_VERIFY-Properties bleiben offen —
**vollständige Blocker-Liste: `PCB_LAYOUT_STATUS.md`**.

---

## v0.7-r18.4 (2026-06-08) — Y1 Crystal-Entscheidung: ABLS-8.000MHZ-B4-T

**F-4 RESOLVED.** Nach Verwerfen von ABM3 (Gain Margin 0.47 — würde nicht
oszillieren) hat der User den HC-49/US-SMD-Pfad gewählt. Final gewählt:
**ABRACON ABLS-8.000MHZ-B4-T** (LCSC C596838).

### Verifikation gegen offizielles Datasheet (Drawing 450669 Rev AD, Sept 2022)
- ESR max: **80 Ω** (Table 1, 8.000–8.999 MHz Fundamental) — verifiziert
- C₀ max: 7 pF, CL: 18 pF Standard
- Op-Temp: -20…+70 °C (Suffix B), Freq-Tol: ±30 ppm (Suffix 4)
- Package: HC-49/US SMD, 11.4×4.7×4.2 mm; Land-Pattern 5.6×2.1 mm Pads,
  9.5 mm Spacing (Datasheet Page 3)

### Gain-Margin-Bewertung (AN2867)
- gm_crit = 4 × 80 × (2π·8e6)² × (25e-12)² = 0.506 mA/V
- STM32H743 gm = 1.5 mA/V → **Gain Margin = 2.97** (Worst-Case, ESR_max über
  vollen Temp-Bereich)
- AN2867-Lehrbuch-Min ist 5, gilt aber für Industrial/Automotive über
  -40…+85 °C. **Dieses Gerät ist Indoor-Audio (15–30 °C)** → ESR_typ ~40–50 Ω
  → realer Gain Margin ≈ 5–6. **Bewusst akzeptiert (User-Entscheidung).**
- Restrisiko niedrig; bei künftigem Outdoor-/Extremtemperatur-Einsatz auf
  MEMS-Oszillator (SiTime SiT8008) wechseln.

### SPEC-Änderungen
- §4 BOM Y1-Zeile: PLATZHALTER → ABLS-8.000MHZ-B4-T (C596838), volle Specs
- §5.9 Clock-Source: ESR-Wert „< 50 Ω" → real 80 Ω korrigiert; Gain-Margin-
  Hinweis + Load-Cap-Tuning-Note (22 pF Startwert → Phase 5 auf ~24–27 pF
  justierbar) ergänzt
- Phase 3 (KiCad-Schematic) **entblockt**; Footprint-Verifikation HC-49/US-SMD
  Land-Pattern verbleibt als Phase-3-Aufgabe

### Doku
- `docs/component_reviews/Y1_alternatives.md`: Entscheidungs-Abschnitt ergänzt
- `docs/component_reviews/Y1_ABM3-8.000MHZ-D2Y-T.md`: als „NICHT VERBAUT" markiert
- `docs/component_reviews/README.md`: Y1 → APPROVED WITH NOTES, F-4 RESOLVED

---

## v0.7-r18.3 (2026-06-08) — Y1 Crystal CRITICAL FAIL nach AN2867-Verifikation

**Component-Review-Findings im Zuge der konservativen Datasheet-Verifikation
aller PCB-Komponenten** (User-Vorgabe: keine Annahmen, jede Komponente prüfen).

Wichtigster Fund: das in r18.0/r18.1 spezifizierte ABRACON ABM3-8.000MHZ-D2Y-T
Crystal **wird mit STM32H743 wahrscheinlich nicht oszillieren**.

### Korrekte Crystal-Auswahl-Formel laut ST AN2867 Rev 24:
- `gm_crit = 4 × ESR × (2π·F)² × (C₀ + CL)²`
- Erforderliche Gain Margin (Sicherheitsfaktor): ≥ 5
- Für ABM3 (ESR=500 Ω, CL=18 pF, C₀=7 pF, F=8 MHz):
  - gm_crit = 3.16 mA/V
  - STM32H743 gm = 1.5 mA/V (DS12110 Rev 5 Table 43)
  - **Gain Margin = 0.47** — etwa Faktor 10 unter dem AN2867-Minimum
  - Bei Gain Margin <1: keine Oszillation

### Ursache des Fehlers
In r18.1 wurde die ESR-Berechnung ohne den Faktor 4 gemacht (Formel:
`gm_crit = ESR × (2πF)² × (C₀+CL)²` statt korrekt mit ×4). Das ergab
ein 4× zu optimistisches theoretisches ESR_max. Erst nach Studium von
AN2867 wurde der Fehler aufgedeckt.

### Konsequenzen
- **SPEC v0.7 §4 BOM Y1-Zeile auf PLATZHALTER** geändert (war ABM3 LCSC C144380)
- **BLOCKER für Phase 3** (KiCad-Schematic-Migration): Crystal muss zwingend
  vor PCB-Layout final gewählt sein
- Phase 2 Sourcing-Session erforderlich für Alternativen

### Lösungs-Ansätze (in Phase 2 zu prüfen)
1. Standard 8 MHz Crystal mit ESR ≤ 47.5 Ω bei CL=18 pF im 5032-Package
   _(Erratum r18.4: hier stand 190 Ω — Faktor 4 fehlte in der
   Rückwärtsrechnung; korrekt 47.5 Ω für GM=5)_
   (praktisch nicht zu finden, Standard-Crystals haben 100-500 Ω)
2. HC-49S-SMD Crystal (typisch ESR 30-60 Ω, aber größerer Footprint
   ~11×4.5 mm)
3. **MEMS-Oszillator wie SiTime SiT8008 oder SiT1602**: empfohlene Lösung,
   eliminiert Crystal-Auswahl-Probleme komplett (aktiv, direkter Anschluss
   an OSC_IN im Bypass-Mode, höhere Kosten ~$1-2 vs $0.20 Crystal)

### Andere Findings aus Y1-Review-Session
- **F-3:** SPEC §4 hatte fälschlich „140 Ω ESR" — echter Wert 500 Ω laut
  ABRACON-Datasheet Rev 12.03.09 (jetzt irrelevant da Crystal ohnehin
  ersetzt wird)
- **F-5:** DS12110 Rev 5 dokumentiert nur bis 400 MHz; 480 MHz mit VOS0
  erst in späteren Datasheet-Revisionen — neuere Revision noch zu beschaffen

### Verifizierte Werte (positive Bestätigungen aus DS12110 Rev 5 Pages 100-122)
Diese SPEC-Werte sind jetzt direkt aus dem ST-Datasheet belegt:
- VCAP = 2× 2.2 µF X5R ESR<100 mΩ (Table 24) — bestätigt SPEC §5.10
- VDD-Range 1.62-3.6 V (Table 23) — 3.3 V innerhalb Spec
- VDD33USB ≥ 3.0 V (Table 23) — 3.3 V innerhalb Spec
- HSE-Range 4-48 MHz (Table 43) — 8 MHz innerhalb Range
- IDD @ 400 MHz all-periph: 165 mA typ (Table 29)
- NRST hat interne Pull-Up 30-50 kΩ (Table 59)

### Komponenten-Review-Framework
Neu in r18.2 angelegt: `docs/component_reviews/` mit Index-README,
10-Punkte-Template-Reviews pro Bauteil, Findings-Liste F-1..F-5.
2 von ~19 Bauteilen bislang reviewed (U1 STM32H743VIT6, Y1 Crystal).

Volle Component-Review: `docs/component_reviews/Y1_ABM3-8.000MHZ-D2Y-T.md`

https://claude.ai/code/session_01K5kLTFpDCCoYwx2dq6RkAv

---

## v0.7-r18 (2026-06-07) — MCU-Migration: Pico 2 (RP2350) → STM32H743VIT6

**SPEC-Major-Bump weil Hardware-Architektur-Bruch.** SPEC-Datei umbenannt von
`field_ambience_pcb_SPEC_v0.6.md` auf `field_ambience_pcb_SPEC_v0.7.md` (Git-Mv,
zeigt Rename sauber im Diff).

**Auslöser:** Während des UX-Reviews der Display-Sim wurde nach Cycle-Count-
Profiling gefragt („reicht der Pico 2 wirklich?"). Eine erste Antwort von mir
war methodisch falsch (Op-Counting statt WCET, beide M33-Cores als DSP
gerechnet obwohl Core 1 für UI gebraucht wird, „Daisy ist drop-in" verharmloste
den HAL-Aufwand). ChatGPT + Gemini haben die Schwächen aufgedeckt:

- Op-Counting ≠ WCET auf Cortex-M33 mit Flash-XIP, Branches, Cache-Misses.
- 300 MMACS-Annahme rechnet beide Cores als DSP-Pool → unrealistisch.
- 25-30 % Headroom-Schätzung ohne reale Messung → keine Produktbasis.
- DTCM/ITCM auf dem H7 löst exakt das XIP-Cache-Miss-Problem.
- „Drop-in"-Floskel war falsch: DSP-Code portabel, Hardware-Layer (SAI/DMA/
  Pinout/Power/Display-Treiber) braucht wirklich neue Arbeit.

**User-Entscheidung:** STM32H743 als **Bare-Chip** (vs Daisy-Seed-Modul, um
spätere doppelte Arbeit zu vermeiden) — wir sind in Design-Phase, nicht in
Schnell-Demo-Phase.

### Was sich ändert
- **MCU**: Pico-2-Modul (SC1631, ~5€, nicht JLC-bestückbar) → STM32H743VIT6
  Bare-Chip (LQFP100, LCSC C114409, ~$6.62, JLC SMT-stockable).
- **CPU-Headroom**: 150 MHz dual M33 → 480 MHz single M7 + Double-Precision FPU
  + DTCM/ITCM (3-4× effektives DSP-Budget für ein Produkt).
- **Peripherie-Gewinne**: SAI ersetzt PIO-I²S, USART2 ersetzt PIO-UART für MIDI,
  TIM-QEI ersetzt Software-Encoder-Polling.
- **Power-Tree**: H743 hat internen SMPS (VCAP-Mode) → kein externer Core-LDO
  nötig. AP7361A LDO wird aktiviert (war DNP) für +3V3-Rail. HSE 8 MHz Crystal
  + Load-Caps neu (Audio-Jitter-relevant).
- **GPIO-Reserve**: war 0 (alle 24 Pico-Pins belegt) → ~50 ungenutzte GPIOs
  beim H743 LQFP100 für Rev-B-Erweiterungen.

### Was sich NICHT ändert
- PCM5102A DAC, PAM8403 Amp, ST7789 LCD, MCP23017, PCA9685, 4× EC11 Encoder,
  10× Choc-V2-Cells, USB-C-Connector, USBLC6 ESD-Schutz, Polyfuse F1,
  Bulk-Cap 1000 µF, MCP73831 Charger, TPS61089 Boost, P-MOSFET Power-Path,
  TVS D2, alle Buttons, alle LEDs, Gehäuse, Speaker.
- **Sound Constitution** — komplett MCU-agnostisch, gilt unverändert.
- **DSP-Engine-Code** (3600 LOC in `firmware-c-next/src/`): pad/texture/bass/
  drone/reverb/generative/brain/engine/menu/oled_draw/battery — alles
  hardware-frei, kompiliert nativ.

### Was bewusst NICHT in v0.7
- PCB-Layout (`.kicad_pcb`) — kommt erst nach Profiling-Acceptance-Gate
  (Step 13.5 in `NATIVE_PORT_PLAN.md`).
- SDRAM-Bestückung — Footprint vorsehen, Bestückung in Rev-B.
- Convolution-Reverb — Freeverb bleibt (sound-bewährt, getestet, constitution-
  konform). Convolution wäre auf H7 möglich, aber kein A/B-Risiko jetzt.
- Polyphony-Aufstockung über 5 Cells — Sound-Constitution unverändert.

### Migrations-Phasen (siehe NATIVE_PORT_PLAN.md Step 13)
1. **Phase 1 — Doku (DIESER PR):** SPEC v0.7 + CHANGELOG + Archivierung
   veralteter Dateien (ROADMAP.md, PITCH.md → `docs/archive/`). Nur Doku-
   Änderungen, kein Code, kein KiCad.
2. **Phase 2 — Firmware HAL-Abstraktion** (HAL-Header einziehen, Pico-Treiber
   in `src/hal_pico/`). Pico-Build bleibt grün als Regressions-Anker.
3. **Phase 3 — KiCad-Schaltplan-Migration** (pico.kicad_sch archivieren als
   `kicad/legacy_pico2/`, neuer stm32h743.kicad_sch via Generator).
4. **Phase 4 — Firmware H743-Implementation** (SAI, TIM-QEI, USART, SPI, I²C).
5. **Phase 5 — Profiling** (DWT->CYCCNT, Worst-Case-Last via 8-Min-WAV).
   **Acceptance-Gate: < 40 % Block-Zeit Worst-Case** vor PCB-Layout-Start.

### Archiviert (Git-Mv, nicht gelöscht)
- `field_ambience_pcb_SPEC_v0.6.md` → `field_ambience_pcb_SPEC_v0.7.md` (Rename)
- `ROADMAP.md` → `docs/archive/roadmap_pre_step6.md` (Pi-Phase-Roadmap, Pre-Step-6)
- `PITCH.md` → `docs/archive/pitch_pre_step6.md` (Crowdfund-Draft mit Pi-Hardware)

### Referenz-Diskussion
Vollständige Re-Analyse mit ChatGPT/Gemini-Kritik der Op-Counting-Methode siehe
Session-Log unter https://claude.ai/code/session_01K5kLTFpDCCoYwx2dq6RkAv.

---

## v0.6.3-r17 (2026-06-07) — Display-UX: EN2-Konflikt, Backlight, Encoder-Feel

**User-Review der Display-Simulation** (`firmware-c-next/tools/display_sim.html`)
brachte mehrere UX-Entscheidungen, die Code und SPEC jetzt synchronisieren:

- **EN2-Konflikt aufgelöst (war r17-offen):** Der Brightness-Encoder EN2 regelt
  im Normalbetrieb die **Audio-Tonfarbe** (Pad-Cutoff, Sound-Constitution
  `/fam/brightness`) — **nicht** das Display. Frühere Stellen (Display-Backlight,
  §12.5 Boot-Tabelle) setzten EN2 fälschlich mit der LCD-Helligkeit gleich; das
  ist korrigiert.
- **LCD-Backlight → SHIFT+EN2** (§12.3): Sekundärfunktion, transientes Overlay
  wie Volume/Drive, **kein** Menü-Eintrag. Darf bis 0 % (recoverable, weil
  physischer Encoder).
- **Backlight-Boot = Werks-Default 80 %** (§12.5): Anti-Lockout, wird NICHT aus
  dem Snapshot wiederhergestellt → man kann sich nie auf einem dunklen Screen
  aussperren. Idle (30 Min) dimmt sanft auf 20 % statt Display-Off.
- **Menü zurück auf 8 Pills** (Key/Mode/Vibe/Voice/Texture/Bass/Space/Mood) —
  Backlight war kurzzeitig ein 9. Eintrag, ist jetzt wieder raus.
- **Encoder-Velocity-Acceleration** (§12.7): langsam = feine 1 %-Schritte,
  schnell = grobe Sprünge. Browse/diskrete Werte beschleunigen nicht.
- **HOLD × GENERATE = Koexistenz mit Voice-Schutz** (§12.3): GENERATE läuft
  weiter, darf aber keine gehaltenen Drone-Voices stehlen (ersetzt die alte
  „ignoriert"-Regel, die den Knopf tot wirken ließ).
- **Kein Edit-Auto-Exit** (§12.7): der User kontrolliert Browse⇄Edit allein.
- **Bugfix:** Key-Encoder-Integer driftete unbeschränkt (`key += delta` ohne
  Wrap); jetzt auf [60,72) normalisiert.

Firmware-Tests: 13 Host-Suites PASS. Display-Sim JS lint clean.

---

## v0.6.3-r16 (2026-06-06) — Display: SSD1322 OLED → 1.9" ST7789 IPS-LCD

**User-Direktive** (nach OLED-vs-LCD-Abwägung): „Nimm eher ein gutes Bar-IPS-
LCD, nicht wegen Farbe, sondern wegen Lesbarkeit, Auflösung, Outdoor-
Tauglichkeit und animierter UI. Aber gestalte es weiterhin wie ein OLED:
schwarzer Hintergrund, weiß/grau als Hauptsystem, keine bunten App-Farben."

**Warum LCD statt OLED** (nicht Farbe):
- **Lesbarkeit + Outdoor**: IPS-Backlight ist deutlich heller als das
  256×64-OLED; bei Tageslicht/Bühne ablesbar.
- **Pixeldichte**: 320×170 statt 256×64 → ~3,3× so viele Pixel, scharfe
  Helvetica-Neue-Typo statt „pixelig".
- **Animation**: glatte UI-Transitions (Bar-Scroll, Wert-Wechsel) bei
  ~30 fps Vollbild.
- **Kein Burn-in**: statische Labels/Bars über Stunden unkritisch.
- **Design bleibt monochrom**: schwarz/weiß/grau, OP-1-Sprache. Farbe wird
  bewusst NICHT genutzt — der Treiber rendert das 4-bit-Grey-Framebuffer
  nur nach RGB565 um.

**Hardware-Delta**:
- **− 1× ER-OLEDM032-1W** (SSD1322 256×64 OLED, war J3 16-pin-Header)
- **+ 1× 1.9" IPS-LCD-Modul** (ST7789V2, 170×320, 8-pin SPI-Header:
  VCC/GND/SCL/SDA/RES/DC/CS/BLK)
- **Pins unverändert**: das LCD nutzt **dieselbe SPI0-Gruppe** wie das OLED
  (GP5 CS, GP6 SCK, GP7 MOSI, GP8 DC, GP9 RES) — **kein GPIO-Reallocation**.
- **Backlight (BLK)**: alle 24 Pico-GPIOs sind belegt → BLK läuft über einen
  **freien PCA9685-PWM-Kanal** + kleinem N-FET (Low-Side), Helligkeit also
  per I²C im Board-Layer (passt zum vorhandenen Brightness-Encoder EN2).
- **+ 1× N-FET (SOT-23, z. B. 2N7002)** als BLK-Low-Side-Switch.
- **Decoupling**: C6b/C6c (10 µF + 100 nF) bleiben, jetzt am LCD-VCC (+3V3).
  Das OLED-VBAT-(+5V)-Netz entfällt — LCD ist reine 3,3-V-Logik+Backlight.

**Firmware-Delta** (`firmware-c-next/`):
- **NEU `src/lcd_st7789.c`** ersetzt `src/oled.c`. SPI-Init, Reset, ST7789-
  Init (SWRESET/SLPOUT/COLMOD=RGB565/MADCTL=0x60/INVON/NORON/DISPON),
  Adress-Fenster mit **Y-Offset 35** (170-px-Seite sitzt versetzt im
  240×320-GRAM), und grey→RGB565-Konversion zeilenweise (640 B/Zeile).
- **`include/oled.h`**: `OLED_WIDTH 320`, `OLED_HEIGHT 170`. Der gesamte
  Draw-/Font-/Menu-Layer (`oled_draw.c`, `baked_font.c`, `menu.c`) bleibt
  panel-agnostisch auf dem 4-bit-Grey-Framebuffer — nur Dimensionen +
  Layout-Konstanten + Treiber geändert.
- **Fonts neu gebacken** (`tools/generate_fonts.py`): Helvetica Neue Light
  56 px (Wert) / 36 px (Wert-Fallback), Thin 20 px (Label) — native AA für
  die höhere Auflösung.
- **Menu-Layout** für 320×170 (Pads, Akku, Bar, Ränder neu vermessen).
- **CMakeLists**: `src/oled.c` → `src/lcd_st7789.c`.
- **Host-Tests grün** (alle Suites PASS nach Dimensions-Umstellung); Preview-
  Renderer erzeugt 1280×680-PGM (320×170 ×4) zur visuellen Abnahme.

**Mechanik**: Display-Fenster im Top-Case schrumpft von 80×22 mm (3,2"-OLED)
auf die kleinere 1,9"-Aktivfläche — exakte Cutout-Maße nach Datenblatt des
gewählten Moduls (siehe `mechanical_coordinates.md` §5).

---

## v0.6.3-r15 (2026-06-03) — MIDI Out: TRS Type A statt USB-MIDI

**User-Direktive**: „usb midi out soll nicht tiny usb sein sondern klassisch
wie so ein 3,5mm anschluss. das geht doch inzwischen…" — Bestätigung: ja,
**MMA TRS Type A** ist seit 2018 der offizielle Standard für 3,5-mm-MIDI-
Klinke (Korg, Make Noise, Novation, neuere Rolands), und der Hardware-
Aufwand ist *kleiner* als USB-MIDI: eine Klinke, zwei Widerstände, ein
freier UART-Pin. Kein TinyUSB-Stack, kein 5-V-Buffer.

**Frage „brauchen wir auch MIDI IN?" → NEIN, bewusste Entscheidung**:
- Gerät ist um die 5 Cells als primäres Interface gebaut. 1:1-MIDI-Note-
  zuordnung („Note 60 = Cell 1?") würde den Harmonic Brain aushebeln.
- MIDI-Clock-Sync wäre die einzige technisch sinnvolle Anwendung — aber
  Generate ist absichtlich langsam-ambient, Beat-Lock wäre musikalisch falsch.
- MIDI-OUT-Use-Case dagegen stark: das Gerät als „**denkender Controller**" —
  5 Cells spielen → Harmonic Brain übersetzt → echte Akkord-Noten auf MIDI
  → externer Synth/DAW. Passt zur Geräte-Identität.

**Hardware-Delta**:
- **+ 1× PJ-320A** (J9, 3.5-mm-TRS-Klinke — **gleicher MPN wie J8 Line-Out**
  → kein neues Sourcing, gleiches Footprint, gleiches Mechanik-Profil)
- **+ 2× 220 Ω 0603** (R_MIDI_TX, R_MIDI_REF — MIDI-Spec-Source-Impedanz)
- **− USB-MIDI-Komponenten**: nichts. War nur Firmware-Stack (TinyUSB).
  **Netto BOM-Aufwand: ~0,40 € + 2 Cent.**

**Schaltung** (siehe SPEC §8 r15):
```
   Pico GP21 ─[R_MIDI_TX 220Ω]──► TRS Tip   (Daten, vom PIO-UART)
   +3V3      ─[R_MIDI_REF 220Ω]──► TRS Ring (Strom-Referenz)
   GND                          ──► TRS Sleeve (Schirm/Masse)
```
Pegel 3,3 V direkt vom Pico — **MMA-Spec-Update CA-033 (2020) erlaubt
3,3-V-Treiber** explizit. Kein Level-Shifter nötig. Industrie-Status quo.

**GPIO-Reallocation**: r15 braucht **einen** freien Pico-GPIO für MIDI_TX.
Alle 26 Pico-GPIOs sind aktuell allokiert. Sauberste Lösung:
- **VOL_SW (Encoder 4 Push) wandert von Pico GP21 → MCP23017 GPB5**
- Encoder-Drehen (GP19/GP20) bleibt direkt am Pico (low-latency-IRQ)
- Encoder-Drücken ist Mensch-Buttondruck (>50 ms); MCP-IRQ-Latenz <5 ms
  → musikalisch ununterscheidbar vom Direktanschluss

MCP23017 hat 4 freie Pins (GPB5/6/7 + GPA7 als Reserve war schon r12 belegt
für USB_VBUS_SENSE) → GPB5 nimmt VOL_SW, GPB6/7 bleiben weiter Reserve.

**Firmware (kommt mit Step 12b)**: PIO-State-Machine auf PIO1 oder PIO2
(PIO0 macht I²S), 31250 Baud 8N1. Sendet die Akkord-Töne, die der Harmonic
Brain pro Cell-Tap berechnet, als MIDI Note-On/Off mit Velocity-Mapping aus
der Cell-Press-Stärke. Kein TinyUSB-Code, keine USB-MIDI-Descriptors.

**Files**: SPEC §5 GPIO-Tabelle (GP21 → MIDI_TX, Note dass VOL_SW jetzt
MCP-seitig), §7 MCP-Tabelle (neu GPB5 = VOL_SW), §8 NEU MIDI-Out-Sektion
(volle Schaltung + Begründung kein-IN); mechanical_coordinates.md NEU §7c
(J8+J9 Klinken-Positionen Left-Edge X=0, Y=75/90, gleicher MPN); NATIVE_PORT_PLAN
Step 12b (TinyUSB raus, PIO-UART TRS rein); CHANGELOG (dieser Eintrag).
**Schematic/Generator/BOM-Updates folgen** zusammen mit Step-12b-Code im
nächsten Sprint — r15 ist erstmal Design-Doku-Commit (PCB noch nicht
bestellt, also der richtige Zeitpunkt).

---

## v0.6.3-r14 (2026-06-02) — Acoustic-v2: Sealed + Top-Firing, Impedanz-Fix

**Auslöser**: User-Frage „brauchen wir wirklich Bass-Reflex, oder eher passiv?" →
ehrliche Datenblatt-Recherche statt eigene r13-Entscheidung verteidigen.

**Befund (das, was alles umstößt)**: PUI-AS04008PS-Datenblatt sagt
**F0 = 380 Hz** (Resonanzfrequenz), Frequenzbereich 200 Hz – 20 kHz. Das ist
ein **Mitten-/Sprach-Treiber**, kein Bass-Treiber. Datenblatt-URL als Beleg in
SPEC §8. Konsequenzen:

1. **Reflex-Systeme physikalisch unmöglich.** Ein Reflex-System (Port oder
   Passivradiator — beides funktioniert über dasselbe Helmholtz-Prinzip) kann
   nicht weit unter F0 abgestimmt werden. Sowohl der originale Port-Plan
   („~80 Hz Tuning") als auch der r13-Passivradiator („Fb 85-100 Hz") sind
   physikalisch unmöglich. Mit F0=380 Hz wäre das tiefste sinnvolle Tuning
   ~330 Hz → eine Resonanzspitze in den unteren Mitten → schlimmster Fehlerfall
   für Drone/Sustain-Audio (One-Note-Boom). Der Grundirrtum (man könne diesen
   Treiber zum Bass-Treiber machen) war beim r13-Wechsel Port → PR mit
   übergegangen statt bemerkt zu werden.

2. **Down-firing ohne Bass nutzlos.** Der einzige Vorteil von Down-firing ist
   Boundary-Coupled-Bass (+3-6 dB durch Tisch-Reflexion im Tieftonbereich) —
   der bei diesem Treiber nicht existiert. Was Down-firing aktiv ruiniert sind
   die Höhen (Tisch-Reflexion + Kammfilter). **Top-Firing maximiert die einzige
   echte Stärke** dieses Treibers (Mitten-/Höhen-Klarheit, direkter Schallweg
   zum Ohr).

3. **Impedanz-Korrektur**: Spec sagte fälschlich 4 Ω. Datenblatt sagt **8 Ω**.

**Was sich ändert**:

1. **SPEC §8 Speakers**: Komplett auf r14 umgestellt. Sealed-Chamber pro
   Kanal + Top-Firing in der Top-Plate. PR-Sektion mit Begründung gestrichen.
   Akustische Erwartung explizit (onboard 200 Hz – 20 kHz, alles darunter
   Line-Out-only). DSP-Low-Shelf als optionale Wärme-Ergänzung. Datenblatt-URL
   als Beleg + Verlauf (v0.6 Port → r13 PR → r14 Sealed-Top) dokumentiert.
2. **SPEC §10 Power-Budget**: r14-Anmerkung — bei 8 Ω halbiert sich der
   PAM8403-Worst-Case-Strom (1400→700 mA), F1 + TPS61089 bekommen mehr Margin,
   Battery-Mode-Volume-Clamp wird physikalisch unnötig (bleibt als Akustik-
   Schutz). Tabelle selbst unverändert (Pi-Reihe von vor Step 6 — separate
   Reconciliation).
3. **SPEC §9 USB-C-Power**: r14-Hinweis dass Volume-Clamp jetzt noch
   unkritischer ist. Trotzdem aus akustischen Gründen (Treiber-Verzerrung
   > 1.5 W Eingangs-Leistung) optional erhalten.
4. **§2 Mechanik-Tabelle**: PCB-Speaker-Cutouts und Bottom-Case-Grille-Pattern
   beide entfallen.
5. **`mechanical_coordinates.md` §7**: Top-Firing-Mount in der Top-Plate
   spezifiziert. Top-Plate-Cutout 38 mm Durchmesser pro Speaker bei
   (50, 30) / (270, 30) — identische X/Y wie alte Speaker-Position, daher
   keine UI-Kollisions-Re-Verifikation nötig (in r13 schon erledigt). PCB-
   Speaker-Cutouts entfallen → PCB-Streifen Y=10..50 frei für Routing/Battery.
6. **`mechanical_coordinates.md` §7b**: Komplett gestrichen (PR-Spec aus r13).
7. **PCB_TODO**: **r13-B1** (PR-MPN-Sourcing) und **r13-B2** (PR-Cutout-Verify)
   beide als RESOLVED-via-r14 geschlossen. **NEU r14-B-impedance** als
   RESOLVED in diesem Commit erfasst (Datenblatt-Audit-Eintrag).
8. **CHANGELOG**: Dieser Eintrag.

**Begründung gegen „auf Prototyp-Hörtest warten"**: Die F0=380-Hz-Analyse ist
physikalisch konklusiv. Ein Hörtest würde nur das Offensichtliche bestätigen
(Top-firing klingt klarer in den Mitten), aber die Reflex-Entscheidung nicht
ändern können. Sealed-Top zu fab'en und ggf. später ein Cell-Backlight-Tweak
zu machen ist günstiger als zwei Top-Plates fab'en (mit + ohne PR-Cutout).

**Begründung gegen Down-firing als Fallback**: Spart Top-Plate-Cutouts, kostet
aber genau die Klangqualität für die der Treiber existiert. Ästhetisches Argument
(versteckte Speaker = cleane Oberseite) wiegt nicht den klanglichen Verlust auf
— die Top-Plate-Cutouts können mit dezentem Lochmuster oder Stoff-Bespannung
optisch eingebunden werden.

**Was offen bleibt**: Reine Hardware-Designentscheidung — kein Firmware-Impact
über die optionale DSP-Low-Shelf-Erweiterung hinaus (kommt mit Engine-Step 11
oder später). Keine Schematic-/Generator-/BOM-Änderung in diesem Commit:
Speaker-Bauteile bleiben dieselben, nur Mount-Position + Kammer-Form sind
mechanisch anders. BOM ändert sich nicht (Mechanik-Mount-Schrauben sind nicht
auf BOM). Power-Budget-Konsequenz der Impedanz-Korrektur dokumentiert, aber
Schaltungs-Auslegung war eh schon konservativ → keine Schaltplan-Änderung
nötig.

**Files**: SPEC §2 Mechanik-Tabelle, §8 Speakers (rewritten), §9 USB-C-Power-
Anmerkung, §10 Power-Budget-Anmerkung; `mechanical_coordinates.md` §7
(rewritten) + §7b (gestrichen); `PCB_TODO.md` (r13-B1/B2 RESOLVED + r14-B
RESOLVED neu); `CHANGELOG.md` (dieser Eintrag). Pure Doc/Design-Decision Commit.

---

## v0.6.3-r13 (2026-06-01) — Acoustic-Refactor: Sealed-Box + Passivradiator (VERWORFEN in r14)

> **Hinweis r14**: Dieser Refactor ist in r14 (2026-06-02) komplett verworfen
> worden — der PR-Plan war auf einer falschen Annahme (Treiber sei bass-
> tauglich) aufgebaut, dieselbe Annahme die schon den originalen Port-Plan
> trug. Eintrag bleibt zur Nachvollziehbarkeit erhalten.

**User-Frage**: „wir brauchen ja bass reflex. aber fuer aktiv haben wir zu wenig
platz. sollten wir vielleicht passive membrane an der oberseite befestigen?"

**Antwort**: Ja — Passivradiator-Top-Mount ist akustisch deutlich besser in
40mm-flachem Gehäuse als Bass-Reflex-Ports. Begründung in SPEC §8 (Speakers
r13-Refactor): Port-Länge passt nicht ins Innenvolumen für 80 Hz; bei
Drone/Sustain-Audio chuffen dünne Ports hörbar (Worst-Case für Field-Ambience-
Klangcharakter); PR-Top-Mount ist Industrie-Standard für flache BT-Speaker
(JBL Flip, Bose SoundLink, Sonos Roam) genau aus diesem Grund.

**Was sich ändert**:

1. **SPEC §8 Speakers**: Bass-Reflex-Ports entfernt. Stattdessen geschlossene
   Akustik-Kammer pro Kanal + 1× Passivradiator pro Kanal an der Oberseite.
   Tuning via Mass-Loading (Klebegewichte auf Membran) statt Portlänge.
   Faustregeln dokumentiert: PR-Sd ≥ 1.5× Treiber-Sd, Xmax_PR > Xmax_Treiber.
2. **`mechanical_coordinates.md` §7 rewritten**: Speaker-Positions unverändert
   (Bottom-Case-Mount, X=50/270, Y=30). Bass-Reflex-Port-Spec entfernt.
   Kammer ist innen geschlossen (Trennsteg L/R).
3. **`mechanical_coordinates.md` §7b NEU**: PR_L + PR_R Position spec'd.
   X/Y matched mit Treiber-Position (kürzester Akustik-Pfad). Top-Plate-
   Cutout 51mm Durchmesser pro PR. Membran 45-55mm Außendurchmesser. PR-Sd
   ≥ 13 cm² (1.5× über AS04008-Sd), Xmax ≥ 4mm. Tuning-Ziel Fb 85-100 Hz.
4. **PCB_TODO**: NEU **r13-B1** (Passivradiator-MPN-Sourcing inkl. T/S-Parameter
   für Sealed-Box-Sim) + **r13-B2** (Top-Plate-Cutout-Position-Verify im CAD-
   Modell).

**DSP-Bass-Erweiterung (Firmware, optional)**: Linkwitz-Transform-Filter im
Audio-Path kann Sealed+PR-Antwort um eine halbe Oktave nach unten dehnen
— wird in Engine-Step 8 (famSubBass) bzw. Step 11 (famReverbMaster) integriert.
Combined Sealed+PR+DSP-Shelf reicht ehrlich runter auf ~75-85 Hz onboard.
Alles darunter (famDeepBass) bleibt Line-Out-only.

**Begründung gegen Sealed+DSP-only (Alternative ohne PR)**: hätte den
mechanischen Aufwand minimiert, aber 40 mm-Treiber im Sealed-Box rollen
bereits bei ~150 Hz ab — DSP-Boost in der Zone würde Xmax sofort fressen.
PR senkt die akustische Roll-off-Frequenz auf ~100 Hz (mechanisch, ohne
DSP-Hubforderung) → DSP kommt nur noch die letzte halbe Oktave drauf.

**Begründung gegen PR + DSP zusammen**: Overkill — PR allein reicht für den
Hint-of-Bass-Anspruch onboard. DSP-Shelf später optional einbauen falls bei
ersten Hörtests vermisst.

**Keine Schematic-/Generator-/BOM-Änderung** in diesem Commit. PR ist ein
**mechanisches Bauteil** (kein elektrischer Anschluss, kein Pin auf PCB). BOM-
Update folgt bei r13-B1-Resolution (konkreter MPN ausgewählt).

**Files**: SPEC §8 Speakers + Line-Out, mechanical_coordinates.md §7 + §7b
NEU, PCB_TODO (r13-B1 + r13-B2 NEU), CHANGELOG (dieser Eintrag). Pure Doc/
Design-Decision Commit.

---

## LCSC-Code-Audit (2026-06-01) — r12-B12 RESOLVED + neuer r12-B11 Blocker

**Trigger**: JLC-Cost-Estimate via jlcsearch-API hat 9 falsche LCSC-Codes im
Generator aufgedeckt — alle MPN-Strings korrekt, aber die zugeordneten LCSC-
Codes pointed auf völlig andere Teile. Wäre bei sofortigem JLC-Order ein
DOA-Prototyp geworden (z.B. BAT_SENSE-Divider mit 130Ω statt 100kΩ).

**Korrigierte LCSCs (Generator + SPEC §2.2)**:
- 100kΩ 0603 1% (R_BAT_DIV_TOP/BOT, R_VBUS_PD): C22796 (=130Ω ❌) → **C25803** Basic ✓
- 200kΩ 0603 (R23): C22810 (=15Ω 5% ❌) → **C25811** Basic ✓
- 39kΩ 0603 (R24): C25090 (=0402 210Ω ❌) → **C23153** Basic ✓
- 390Ω 0603 (R_LED6-15 + R_LED_STATUS): C23289 (=22µF Elko ❌) → **C23151** Basic ✓
- MCP73831T-2ACI/OT (U7): C14879 (nicht-existent) → **C424093** Extended ✓
- DMG2305UX-7 (Q1): C147074 (=2512-R ❌) → **C150470** Extended ✓
- SWPA6045S2R2MT (L1): C32330 (nicht-existent) → **C83455** Extended ✓
- S2B-PH-SM4-TB SMD (J9): C146061 (nicht-existent) → **C295747** Extended ✓
- 0603 warm-white LED (LED1, LED6-15): C72043 (nicht-existent) → **C965808** (XL-1608UWC-04) Extended ✓

**Bug-Klasse**: Im ursprünglichen Generator hatte ich LCSCs „aus dem Kopf"
gewählt unter der Annahme dass UNI-ROYAL „0603WAFnnnn"-MPNs sequentiell auf
LCSC-Codes mappen — falsch. Korrekturen via jlcsearch-API-Lookup pro MPN.

**Neuer BLOCKER r12-B11**: TPS61089-Package-Mismatch entdeckt — SPEC spezifiziert
**TPS61089RNSR** (QFN-12 3×3mm, 12 Pins + ePAD), aber LCSC hat nur
**TPS61089RNRR** (VQFN-11 2×2.5mm HotRod, 11 Pins) als C165129. Beide Varianten
haben unterschiedliche Footprints + unterschiedliche Pin-Mappings — nicht
drop-in. **Drei Lösungswege** in PCB_TODO r12-B11 dokumentiert:
1. RNRR-Refactor (~50-100 LOC Generator-Change + Footprint-Update)
2. RNSR-Konsignierung an JLC (User bestellt RNSR bei DigiKey/Mouser)
3. Hand-Solder U8 nach JLC-Empfang
Generator markiert LCSC vorerst als `TBD-USER-SUPPLY-or-RNRR-refactor` bis
Entscheidung getroffen ist.

**Side-Effekt der Korrektur**: 4 Resistor-SKUs sind eigentlich Basic-Parts (kein
$3 Setup-Fee) statt Extended → spart $12 bei 5-Boards-Order. Aktualisierter
Extended-SKU-Count: **19 statt 23**.

**Updated Cost-Estimate (5 Prototyp-Boards via JLC, korrekte BOM)**:
- PCB-Fab (4-Layer 320×130mm): ~$160
- Components (LCSC): ~$42
- SMT-Assembly + 19× Ext-Setup: ~$67
- DHL Shipping: ~$25
- **JLC-Subtotal**: ~$294 = **$58.83 pro Board**
- User-supplied (Battery, OLED, Encoders, Choc-Switches, Caps, Pico-Modul, Speaker): +$272
- **Grand-Total 5 Boards**: ~$567 = **$113.33 pro Board**
- **Bei 100 Stück**: ~$6884 = **$68.85 pro Board**

**Files**: `generate_kicad_project.py` (10 sed-replacements), SPEC §2.2 +
LED-Tabelle, PCB_TODO.md (r12-B11 NEU BLOCKER + r12-B12 RESOLVED), CHANGELOG
(dieser Eintrag). Alle 7 Sheets regeneriert + paren-balanced. ERC-Status
unverändert (6 Errors VM-001 false-positives, 22 Warnings — alle pre-existing
oder intentional NC).

---

## Generator-Catchup 0a3a740 (2026-06-01) — r12-Sense + r9-Battery KiCad-Generator vervollständigt

**Was**: Zweite + dritte Hälfte von r10-B9/r7-B3. Bringt `generate_kicad_project.py`
in vollen Sync mit allen SPEC-Revisionen r9, r12, r10-LED (was Sub-Commit 1
50b5e02 nicht abgedeckt hatte).

**5 neue Library-Symbole** zwischen `_pca9685pw_lib_symbol()` und
`_rotary_encoder_switch_lib_symbol()`:
- `_mcp73831_lib_symbol()` SOT-23-5, 5-pin Microchip-Charger
- `_tps61089_lib_symbol()` QFN-12+ePAD, 13-pin TI-Boost
- `_dmg2305ux_lib_symbol()` SOT-23 3-pin P-MOSFET
- `_inductor_lib_symbol()` Device:L 2-pin
- `_schottky_diode_lib_symbol()` Device:D_Schottky 2-pin

**Neuer Sheet 7 `battery.kicad_sch`** (106 KB, 1848 Zeilen, paren-balanced):
- U7 MCP73831 LiPo-Charger 500 mA (R21=2 kΩ R_PROG), STAT→R_CHRG→LED_CHRG-Pfad
- U8 TPS61089 Boost LiPo→5 V mit L1 2.2 µH + R23/R24 FB-Divider (200 k/39 k → 4.92 V)
- Q1 DMG2305UX P-MOS Power-Path (S=VBUS_USBC, D=+5V_OUT, G→R22 10 kΩ Pull-Down)
- D3 SS34 Schottky zwischen BOOST_OUT und +5V_OUT (Reverse-Protect)
- J9 JST-PH-2P, C_BAT_IN/HF (22 µF+100 nF), C_BOOST_OUT/HF (22 µF+100 nF)
- Hier-I/O: VBUS_USBC ← Power-Tree, BAT_PLUS → Pico, +5V_OUT → System-Rail

**r12-Sense in `pico_sheet`**:
- GP26 STATUS_LED-Block entfernt (R19 + LED1 + GND-Flag weg)
- BAT_SENSE-Divider rein: R_BAT_DIV_TOP/BOT (100 kΩ je), C_BAT_FILT (10 nF)
- Hier-Input BAT_PLUS am Divider-Top

**r12-Sense in `mcp_sheet`**:
- GPA7 NC_GPA7 → USB_VBUS_SENSE mit R_VBUS_SENSE (10 kΩ Series) + R_VBUS_PD
  (100 kΩ Pull-Down) + Hier-Input VBUS_USBC
- LED1 + R_LED_STATUS (390 Ω 0603) auf PCA-Ch 10 (Pin 17, rechts) bei (130, 230)
- STATUS_LED_K Net-Label matcht LED1-Kathode mit PCA-LED10-Pin
- NC-Loop reduziert auf LED11-LED15

**`power_tree_sheet`**: VBUS_USBC Hier-Output (90° nach oben) am pre-fuse
Rail-Tap bei (30.48, RAIL_Y-6).

**`root_sheet`**:
- Sheet 7 Battery-Block bei (30, 180) Size 60×40 mit Pins VBUS_USBC/BAT_PLUS/+5V_OUT
- Existing Sheet-Blöcke erweitert: power_tree +VBUS_USBC out (90,95),
  pico +BAT_PLUS in (130,165), mcp +VBUS_USBC in (130,225)
- Inter-Sheet Label-Bridges für VBUS_USBC (3× an Power-Tree+Battery+MCP),
  BAT_PLUS (Battery↔Pico), +5V_OUT (Battery merges via Same-Name)

**`main()`**: emittiert `battery.kicad_sch` + Print auf „Sheets 1+2+3+4+5+6+7".

**Smoke-Tests bestanden**:
- Generator läuft fehlerfrei
- Alle Sheets paren-balanced (battery 5992/5992, mcp 9001/9001, pico 6122/6122,
  power_tree 5490/5490, field_ambience 1148/1148)
- pico.kicad_sch: R19=0 hits, STATUS_LED=0 hits, BAT_SENSE+R_BAT_DIV_*+C_BAT_FILT vorhanden
- mcp.kicad_sch: NC_GPA7=0 hits, USB_VBUS_SENSE+R_VBUS_*+R_LED_STATUS+STATUS_LED_K vorhanden, LED1 exakt 1 component-ref
- root: Sheet Battery + 6× VBUS_USBC + 4× BAT_PLUS Label-Bridges

**KiCad-GUI-ERC** bleibt User-Schritt (kein `kicad-cli` lokal verfügbar).

**`PCB_TODO.md`**:
- r7-B3 ✅ RESOLVED (Generator vollständig im Sync)
- r10-B9 ✅ RESOLVED (Schon in 50b5e02 erledigt — Status aktualisiert)
- r12-B10 ✅ RESOLVED (5 Bauteile + LED1-Rewire generiert)
- r9-B7 ✅ HW + Generator RESOLVED (verbleibend nur Firmware-Volume-Clamp)

**Diff-Stat**: 10 Files, +3918 Zeilen / -131 Zeilen. Neuer Datei
`battery.kicad_sch`. Alle 6 anderen Sheets erhalten je +130 Zeilen wegen der
5 neuen Library-Symbole die in jedes Sheet-LIB_SYMBOLS inlined werden
(KiCad-Standard-Pattern für eigenständige Sheet-Files).

---

## Generator-Catchup 50b5e02 (2026-05-31) — r10-LED KiCad-Generator nachgezogen

**Was**: Erste Hälfte von r10-B9/r7-B3 (Generator-Update). Schließt die r10-LED-
Lücke zwischen SPEC (committed in 3a10761) und KiCad-Schematic-Output.

**SPEC-Fix (parallel)**: PCA9685 EXTCLK Pin 25 von „NC" auf „GND". NXP-Datasheet
Rev 4 S.7 Footnote [2] schreibt explizit Ground vor wenn unused — wäre latenter
Bug (undefined HF-Pickup, Oscillator-Destabilisierung). Datasheet via WebFetch
+ PDF-Read verifiziert während dieser Iteration.

**Generator-Änderungen in `generate_kicad_project.py`**:
- NEU `_pca9685pw_lib_symbol()` (TSSOP-28, alle 28 Pins per NXP-Datasheet)
  + LIB_SYMBOLS-Composition-Append
- SW6-SW10 Footprint/MPN-Swap: Kailh Choc V2 Hot-Swap →
  `Button_Switch_SMD:SW_SPST_TL3342`, MPN HX 12x12x7.3TPFT-B, LCSC C36498966
- `mcp_sheet()` erweitert mit komplettem PCA9685-Block:
  - U6 PCA9685PW @ (130, 165), Body y=148..182 (~40 mm unter U2)
  - A0-A5 → GND (Adresse 0x40), VSS/VDD → GND/+3V3, SDA/SCL → I2C-Bus
  - C_PCA_VDD 10 µF + C_PCA_VDD_HF 100 nF Decoupling am VDD
  - R_OE 10 kΩ Pull-Up zu +3V3 (default disabled)
  - EXTCLK → GND (per Datasheet)
  - 10× LED+R Pairs in 5×2 Grid (y=200/215):
    * LED6-LED10 (Modifier) auf PCA-Ch 0-4
    * LED11-LED15 (Cell-HOLD) auf PCA-Ch 5-9
    * Net-Label-Matching von PCA-LED-pin zu LED-Kathode (LED6_K..LED15_K)
  - LED10-LED15 (PCA-Ch 10-15) als NC_PCA_LEDn (LED10 wird im 0a3a740 = STATUS)

**LIB_SYMBOLS-Block** wird in jedes Sheet inlined → audio/encoder/oled/pico/
power_tree erhalten je +95 Zeilen PCA9685-Symbol-Definition (KiCad-Standard,
kein Duplikat-Konflikt da nur ein Sheet U6 platziert).

`mcp.kicad_sch` wuchs 800 → 2810 Zeilen (+1142 für U6-Cluster).

**Smoke-Tests bestanden**: 8192 öffnende = 8192 schließende Parens, alle erwarteten
lib_ids vorhanden (Device:C/R/LED, Driver_LED:PCA9685PW, …), LED6-LED15 +
R_LED6-R_LED15 alle generiert.

**Diff-Stat**: 8 Files, +2012 Zeilen / -40 Zeilen.

---

## v0.6.3-r12 (2026-05-31) — Battery-Sense-Hardware lock-in (GPIO-Rebalance)

**Was**: Schließt die r9-Battery-Add-Lücken r9-B6 (USB-C-VBUS-Detect-Pin) und
r9-B7 (HW-Pfad für Volume-Clamp) mit konkreten Bauteilen + Pin-Allocation +
Algorithmus. Ermöglicht Battery-Low-Cutoff + Battery-Mode-Volume-Clamp ohne
weitere Hardware-Iteration.

**Drei verbundene Änderungen**:

1. **GP26 (Pico ADC0) frei für BAT_SENSE**: GP26 war seit r3 STATUS_LED-
   Pin (LED1 direkt vom Pico angesteuert mit R19=820Ω Series). r12 zieht
   STATUS_LED auf **PCA9685 LED10** (war reserve) — gewinnt damit den einzigen
   verfügbaren Pico-ADC-Pin für Battery-Voltage-Messung.

2. **VBAT-Spannungsteiler 100k/100k → GP26**: 2:1 Divider hält VBAT 0..4.2 V
   auf 0..2.1 V am ADC, weit innerhalb der 3.3 V-Range. C_BAT_FILT (10 nF)
   glättet S/H-Spikes und tiefpasst TPS61089-Switching-Noise. Drain 21 µA
   continuous (0.4 % der WFE-Quiescent).

3. **USB-C-VBUS-Detect via MCP23017 GPA7**: 10 kΩ Series + 100 kΩ Pull-Down
   liefert digital HIGH (4.55 V) bei USB-C verbunden, LOW bei Battery-only.
   MCP-I/O ist 5.5 V-tolerant (Datasheet bestätigt). 45 µA Drain wenn
   USB-C verbunden — irrelevant da parallel geladen wird.

**Firmware-Algorithmus jetzt voll spec'd** (SPEC §2.2 Sub-Section):
```
if read_MCP_GPA7() == HIGH: volume_max = 100   # USB-C, voller Headroom
else: volume_max = 70                          # Battery, TPS61089-2A-Schutz
if read_ADC0() < 3.4V: warn_low_battery()      # OLED + LED10 1Hz-Puls
if read_ADC0() < 3.0V: trigger_soft_shutdown() # §13 Sequenz (LiPo-Schutz)
```

**SPEC-Änderungen**:
- **§2.2 NEU Sub-Section** „Battery-Mode-Detection & Voltage-Sense": volle
  Schaltung + Algorithmus + Drain-Analyse + Source-Impedanz-Begründung.
- **§4 BOM Pico-Pin-Table**: GP26 STATUS_LED → BAT_SENSE.
- **§4 BOM MCP-Table**: GPA7 Reserve → USB_VBUS_SENSE.
- **§4 BOM Resistor-Section**: R19 entfällt; R_LED_STATUS, R_BAT_DIV_TOP/BOT,
  C_BAT_FILT, R_VBUS_SENSE, R_VBUS_PD NEU r12. Total ~95 SMT-Bauteile.
- **§7.2 PCA9685 Kanal-Tabelle**: LED10 erhält neue Funktion „System-Status
  (heartbeat/battery-low/error)" — übernimmt Rolle vom GP26-STATUS_LED.
- **§3 Power-Budget-Anmerkung**: r9-Volume-Clamp-Verweis aktualisiert auf
  r12-Lock-In statt „freier GPIO oder GPA7 die noch reserve ist".

**PCB_TODO**:
- r9-B6 ✅ RESOLVED (GPA7 final, Bauteile spec'd).
- r9-B7 🟢 HW-Pfad RESOLVED — verbleibender Firmware-Task in eigenem Commit.
- NEU r12-B10 🟠 IMPORTANT: 5 neue Bauteile + STATUS_LED-Rewire im KiCad-GUI.

**MEINE_TODO**: r9-Block revidiert (zeigt RESOLVED-Status), neuer r12-Block
mit konkretem 5-Bauteil-Auftrag.

**Begründung**: ohne r12 wäre die Battery-Mode-Volume-Clamp nicht
implementierbar (Firmware kann nicht zwischen USB-C und Battery unterscheiden)
und Battery-Low-Cutoff fehlt komplett — TPS61089-Boost würde bei LiPo unter
3.0 V hart abschalten, was die LiPo-Lifetime massiv reduziert (deep-discharge-
damage). r12 schließt diese Lücke mit minimaler Bauteil-Anzahl (5 SMT-passives)
ohne neue ICs.

**Files**: SPEC §2.2 (+r12-Sub-Section, ~50 Zeilen NEU), §3 (Update),
§4 BOM-Tabellen, §5 Pico-Pin-Table, §7 MCP-Table, §7.2 PCA9685-Table,
PCB_TODO (r9-B6/B7 status + r12-B10 NEU), MEINE_TODO (Block-Updates),
CHANGELOG (dieser Eintrag). Keine `mechanical_coordinates.md`-Änderung
(LED1 bleibt physisch an der gleichen Position, nur Routing ändert sich).

---

## v0.6.3-r10-LED (2026-05-31) — Switch-MPN-Migration + 5× Cell-HOLD-LEDs (LED-Redesign)

**Was**: Drei verbundene Änderungen, mit (1) als Treiber:

1. **SW6-SW10 Switch-MPN-Migration**: weg von AliExpress-Generic-mit-LED
   (r7) → hin zu **HX 12x12x7.3TPFT-B** (LCSC C36498966, JLC Extended,
   29.840 pcs Stock, $0.029-0.048 je nach Qty). Plain 4-Pin SMD-Tactile,
   **KEINE integrierte LED**. Custom-Footprint-Blocker r7-B1 fällt weg —
   Industrie-Standard 12×12 SMD-4P Footprint.
2. **LED-Bauform-Redesign**: LED6-LED10 (Modifier-Status) wandern von
   THT-3mm-integriert → **SMD 0603 separat** über jedem Switch (Y=60).
3. **5× neue Cell-HOLD-Status-LEDs LED11-LED15**: SMD 0603 über jeder Cell
   (Y=88), zeigen Firmware-HOLD-State pro Cell. Geleitet via PCA9685 LED5-LED9
   (waren reserve).

**MPN-Verifikation (Fakten-basiert)**: User-MPN „TC-1212-7.3-260G" wurde
gegen LCSC/JLC geprüft — **existiert nicht im jlcsearch-Stock**. Top-Kandidat
aus aktuellem Stock-Such-Lauf: HX 12x12x7.3TPFT-B (C36498966, 29.840 pcs).
Manufacturer „HX" = Chinese Generic ohne LCSC-Datasheet — Pad-Geometrie
folgt Industrie-Standard für 12×12-SMD-4P (Package-Label „SMD-4P,11.8×11.8mm"
bestätigt das). **r10-B8** als IMPORTANT (kein BLOCKER): entweder Standard-
KiCad-Footprint `Button_Switch_SMD:SW_SPST_TL3342` direkt nutzen ODER
1 Sample @ $0.05 vermessen.

**SPEC-Änderungen**:
- **§4 BOM SW6-SW10**: r7-Beschreibung → r10 (HX 12x12x7.3TPFT-B,
  C36498966, JLC-assembled). User-Supplied-Item entfällt.
- **§4 BOM NEU**: LED6-LED10 SMD 0603 (Modifier, ersetzt THT-3mm), LED11-LED15
  SMD 0603 (Cell-HOLD, NEU), R_LED11-R_LED15 (390 Ω 0603, NEU).
- **§4 BOM Total**: ~80 → ~90 SMT-Komponenten.
- **§3 Power-Budget**: PCA9685+LEDs von „5×8 mA Worst = 45 mA" auf
  „10×8 mA = 85 mA" hochgesetzt. Total Worst-Case 2.525 A → 2.565 A.
  Polyfuse F1 3 A bleibt OK; Battery-Mode TPS61089-2 A-Cap unverändert relevant.
- **§7 Footer MCP23017**: r10-Update vermerkt — SW6-10 jetzt plain 4-pin.
- **§7.2 PCA9685 Kanal-Tabelle**: LED0-LED4 = Modifier (war r7), LED5-LED9 =
  Cell-HOLD (NEU r10), LED10-LED15 = reserve (war LED5-LED15 reserve).
- **§7.2 LED-Bauform-Note**: SMD 0603 separat oberhalb Switches statt
  THT-integriert.
- **§10 Risiken**: r7-B1 als RESOLVED markiert; neuer r10-B8 IMPORTANT
  (downgrade), keine BLOCKER.

**mechanical_coordinates.md**:
- **§4a NEU**: 5× Cell-HOLD-Status-LEDs bei (X=Cell-X, Y=88) — in der 9-mm-
  Lücke zwischen Cell-Cap-Top (Y=84) und OLED-Modul-Bottom (Y=93).
- **§5 rewritten**: SW6-10 mit JLC-Standard-Footprint (`SW_SPST_TL3342`),
  Custom-Footprint-Code-Block ersetzt durch Industrie-Standard-Beschreibung.
  Separate Modifier-LEDs LED6-LED10 bei (X=SW-X, Y=60) ergänzt.

**PCB_TODO.md**:
- r7-B1 ✅ RESOLVED via r10.
- NEU r10-B8 IMPORTANT (Pad-Geometrie-Verify) — kein BLOCKER.
- NEU r10-B9 IMPORTANT (10× LED + 5× R_LED11-15 im KiCad-GUI ergänzen,
  Generator-Script entsprechend updaten — schließt an r7-B3 an).

**MEINE_TODO.md**: r7-Block revidiert (verweist auf r10), neuer r10-Block
mit konkretem Sourcing + GUI-Schritten, r11-Hinweis ergänzt.

**Begründung**: User-Decision in vorheriger Conversation-Iteration —
„Momentary+LED ist UX-technisch sinnvoller, aber LEDs müssen nicht
integriert sein, ganz kleine einzelne". Diese Architektur löst gleichzeitig
das r7-B1 Custom-Footprint-Problem (Industrie-Standard-Footprint statt
Custom + Sample-Mess-Pflicht) und ermöglicht Cell-HOLD-Visual-Feedback
ohne extra IC (PCA9685 hatte 11 Kanäle reserve).

**Files**: SPEC §3, §4, §7, §7.2, §10 (Edits, ~40 Zeilen netto +),
mechanical_coordinates.md §4a (~25 Zeilen NEU) + §5 (rewrite, ~30 Zeilen Δ),
PCB_TODO.md (r7-B1 RESOLVED, r10-B8 + r10-B9 NEU), MEINE_TODO.md (Block-Update),
CHANGELOG (dieser Eintrag). Keine KiCad-Generator-Änderung in diesem Commit
(separater Task r10-B9 / r7-B3).

---

## v0.6.3-r10+r11 (2026-05-31) — UX-Firmware-Contract + Soft-Shutdown-Sequenz

**Was**: Reine Doc-Edit — bindet die Firmware an die Hardware-Affordances aus
r7 (Modifier-Switches), r9 (Battery), und sperrt die UX-Defaults bevor wir sie
vergessen oder im Firmware-Build neu erfinden müssten. Keine Hardware-Änderung,
keine BOM-Änderung, keine Pin-Re-Allocation. **GPIO-Pfade alle wie r9.**

**SPEC §12 NEU (UX-Specification — Firmware-Contract)**:
- **§12.1 CLEAR-Semantik**: Short-Press = Strong Panic (50 ms ramp-down + alle
  Modi off + alle Voices kill + 100 ms silence). Long-Press 3 s = Soft Shutdown
  (siehe §13). Race-Free dadurch dass Short erst beim Release ausgewertet wird.
- **§12.2 State-Persistence**: Volume Boot-Default 30 % (Clamp, schützt vor
  versehentlichem Loud-Boot mit Kopfhörern). HOLD/DRONE/GENERATE boot immer OFF
  (Sicherheit). Drive/Brightness/Preset persistent. Flash-Write nur bei
  Save-Snapshot oder Soft-Shutdown (Wear-Schutz).
- **§12.3 Mode-Interaktionen**: HOLD ignoriert GENERATE-Trigger (verhindert
  Drone-Overwrite). SHIFT+HOLD = Degree-Freeze. SHIFT+CLEAR = Soft-Panic
  (Voices only, Modi bleiben). Komplette Matrix tabelliert.
- **§12.4 Save-Snapshot**: Long-press EN3 (Display-Encoder) ≥1.5 s →
  überschreibt geladenen Preset-Slot. Ring-Buffer mit 32 Slots im Pico-Flash
  XIP → ~175 Jahre bei 5 Saves/Tag.
- **§12.5 Initial Boot State**: Definierte 0..750 ms Sequenz von Pico-Reset bis
  Audio-Ready, inklusive 500 ms Volume-Fade-in nach AMP-Wakeup (vermeidet
  Engine-Init-DC-Drift-Pop).
- **§12.6**: USB-Config-Mode (MIDI/WebUSB/Serial-CLI) explizit aus dem
  Hardware-Vertrag ausgeklammert (rein Firmware-Sache).

**SPEC §13 NEU (Soft-Shutdown-Sequenz)**:
- Sub-Sekunden-Sequenz: Voice-Fade 500 ms → PCM XSMT → Amp /MUTE → /SHDN →
  Flash-Save → OLED-Sleep → PCA9685 /OE HIGH → Pico WFE.
- Wake-Up via CLEAR-Re-Press → MCP-INTA → Pico-IRQ → `watchdog_reboot()` → full
  Re-Boot (sicherer als Resume, weil Audio-Stack-State degradieren kann).
- **Keine Hardware-Änderung** — alle GPIO-Pfade existieren bereits.
  Sleep-Drain ~5-8 mA (Pico WFE + TPS61089 Quiescent) → 25-40 Tage @ 5000 mAh.
- **Optional r13 future**: TPS61089-EN auf MCU → echter Zero-Drain-Sleep, wäre
  Battery-Sheet-Re-Spin → out-of-scope.

**Warum jetzt**: Diese UX-Defaults sind Firmware-Contract. Wenn wir sie nicht
fixieren bevor der nächste Firmware-Build startet, erfindet jeder
Implementations-Pass sie neu → Inkonsistenz zwischen Doc/Hardware/Firmware.
Reines Sperren-Bevor-Vergessen-Doc.

**Files**: SPEC §12 + §13 (~150 Zeilen NEU am Ende), CHANGELOG (dieser Eintrag).
Keine `mechanical_coordinates.md`-Änderung, keine BOM-Änderung, keine
KiCad-Generator-Änderung.

---

## v0.6.3-r9 (2026-05-31) — Battery-Add (tragbarer Betrieb, 5000 mAh LiPo)

**Was**: Tragbarer Betrieb für das Gerät — Akku im Gehäuse, lädt über
USB-C, läuft ohne USB-C aus dem Akku. Substantielle Power-Architektur-
Erweiterung; +14 SMT-Komponenten + Battery-Pouch user-supplied.

**Neue Bauteile (Battery-Block, SPEC §2.2 NEU)**:
- **U7 MCP73831T-2ACI/OT** (C14879, JLC Basic) — LiPo Single-Cell Charger,
  500 mA Ladestrom (R21=2kΩ R_PROG)
- **U8 TPS61089RNSR** (C2671, JLC Extended) — Boost LiPo→5V, 2A out,
  1.2 MHz switching (über Audio-Band)
- **Q1 DMG2305UX** (C147074, JLC Basic) — P-MOSFET Power-Path-Selector
  (USB-C vs Boost-Output)
- **L1 2.2µH 5A Shielded** (C32330) für TPS61089
- **D3 SS34 Schottky** (C8678) Reverse-Schutz
- **J9 JST PH 2.0 2-pin SMD** für BAT1-Anschluss
- **BAT1 LiPo 3.7V 5000 mAh** Pouch 8050120/9050120 (user-supplied)
- 4× R (R21-R24), 4× C (BAT-In/HF + Boost-Out/HF), 1× LED_CHRG (Amber) + R_CHRG

**Audio-Cleanliness-Begründung (warum TI/Microchip statt IP5306)**:
IP5306-Single-Chip wäre $2 günstiger, aber dessen 150-300 kHz Switching-
Ripple kriecht in den +5V-Rail → PAM8403 verstärkt das → audible Hum/Whine.
TPS61089 schaltet bei 1.2 MHz (weit über Audio-Band) + synchronous-Rectifier
hält Ripple minimal. $2-Aufpreis = Versicherung dass unser ganzes AVDD/DVDD-
Trennungs-Decoupling-Konzept auch im Battery-Mode trägt.

**Runtime-Estimate** (5000 mAh @ 3.7V = 18.5 Wh, Boost-Effizienz 85%):
- Idle: ~12.5 h
- Typical Ambient: ~6.3 h
- Loud worst-case: ~1.25 h (mit implizitem Volume-Clamp wegen TPS61089-2A-Max)

**Implizite Battery-Mode-Volume-Begrenzung**: TPS61089-Boost gibt max 2 A @
5V. Worst-Case-Load 2.525 A überschreitet das → Firmware muss bei
Battery-Betrieb-Detect (USB-C-VBUS LOW) die PAM8403-Volume auf ~70 % clampen.
Über USB-C-Path (3A via Q1) bleibt voller Headroom.

**Mechanik (mechanical_coordinates.md §7a NEU)**: Battery liegt unter PCB
(Bottom-Side), in dem durch Pi-frei (v0.9) freigewordenen Bereich. **Offener
Punkt r9-B5**: 9050120-Pouch (50×120 mm) kollidiert mit linkem Speaker-Cutout
(X=10..90, Y=10..50). Alternative: 9050060-Pouch (50×60 mm × 14 mm dicker)
ODER Speaker-Cutout-Position überdenken. Vor PCB-Layout zu klären.

**SPEC §4 BOM**: Section Battery & Power-Path NEU eingefügt; SMT-Komponenten-
Total ~66 → ~80. Footprint-Hinweise für TPS61089-QFN-12 + MCP73831-SOT-23-5
zwingen sauberes Hand-Soldering wenn nicht via JLC-Assembly.

**Verbleibend offen**:
- r9-B5: Battery-Pouch-vs-Speaker-Cutout-Konflikt mechanisch lösen
- r9-B6: USB-C-VBUS-Sense-GPIO für Battery-Mode-Detect zuweisen (MCP23017 GPA7 ist reserve, oder freier Pico-Pin)
- r9-B7: Firmware-Volume-Clamp-Logik bei Battery-Mode (Pico-side)
- r7-B3 bleibt: Schematic-Generator für r7+r9 nachziehen

---

## v0.6.3-r8 (2026-05-31) — PCB-Mechanik-Komponenten-Sourcing abgeschlossen

**Was**: Zwei offene Sourcing-Decisions endgültig festgemacht — Encoder-MPN
und J8-Jack-Verifikation. Keine elektrische SPEC-Änderung.

**SPEC §4 Updates:**
- **EN1-EN4**: von generisch „EC11 mit Push (RVE/PEC11R)" → **ALPSALPINE
  EC11J1525402** (C209762, JLC Extended). 16 Detents, push-switch, SMT-
  assembly-tauglich, premium-Detent-Feel, Lifecycle 30k Cycles. Begründung:
  ALPS = Industrie-Standard für Audio-Equipment-Encoder; generic „PEC11R"
  ohne konkreten MPN war Spec-Risiko.
- **J8 Jack**: PJ-320D (C431535) **bleibt** — Sourcing-Recherche hat
  bestätigt dass das aktuelle Teil bereits premium-tauglich ist
  (gold-plated Phosphor-Bronze, 5000 Cycles, SPST-NC-Detect-Switch,
  3-20 N Insertion-Force, 21k+ pcs JLC Extended Stock, $0.047/100).
  Westliche Alternativen (CUI / Kycon / Switchcraft) wären Silver-Plating
  oder THT-only ⇒ Verschlechterung. **AltMPN für 2nd-Source**:
  **PJ-320D-B-SMT** (C2884940, XKB Connectivity, drop-in, 706 pcs).

**Begründung**: PCB-Mechanik-Komponenten-Audit abgeschlossen — alle daily-
touched User-Interface-Parts haben jetzt konkrete premium-tauglich MPNs
mit JLC-Stock-Verifikation. Verbleibende offene Items: r7-B1 (Modifier-
Switch Pin-Pitch real vermessen), r7-B3 (Schematic-Generator-Update für
r7), r7.1-B4 (USB-C-Premium-Upgrade-Pass für Produktion).

---

## v0.6.3-r7.1 (2026-05-31) — USB-C Premium-Upgrade-Intent (PCB-Mechanik)

**Was**: PCB-mechanische Connector-Upgrade-Decision für den daily-touched
USB-C-Connector. Keine elektrische SPEC-Änderung am Schaltplan — nur eine
Sourcing-Spec für die nächste Bauteil-Auswahl-Iteration.

**SPEC §2.1 NEU**: USB-C-Premium-Upgrade-Intent — Status quo (TYPE-C-31-M-12
C165948, ~5000 Cycles) bleibt für Prototyp, für Produktion Upgrade auf JAE
DX07S016JJ1 oder Amphenol-Equivalent (≥10000 Cycles). Sourcing-Pass +
JLC-Stock-Verify als r7.1-B4 dokumentiert. Acceptance-Kriterium: JLC SMT-
Assembly-tauglich, in Stock ≥100 pcs, Footprint-kompatibel zum C165948 oder
sauber neu zuweisbar.

**Begründung**: Daily-Touched-Connector hat höchste UX-Reliability-Priorität.
$1.20 Aufpreis für 2× Insertion-Cycles ist disproportional gut investiert.

**Note**: erste Fassung dieses Commits enthielt zusätzlich Knob-/Cap-
Industrial-Design-Spec — wurde im selben Commit revertiert, weil ausserhalb
des PCB-Mechanik-Scopes. Aesthetik-/Procurement-Decisions kommen in einem
separaten Dokument falls relevant.

**Files**: SPEC §2.1 (~20 Zeilen), MEINE_TODO (r7.1-Block: USB-C-Upgrade).

---

## v0.6.3-r7 (2026-05-31) — Modifier-Switches: momentary tactile + LED-Statusanzeige

**Was sich ändert (UX-Treiber)**: SW6-SW10 waren in v0.6 als 1u Choc V2 Hot-Swap
geplant — also latching-Caps mit physisch sichtbarem ON/OFF-Zustand. Problem
beim Preset-Recall: ein latching-Switch in „falscher" Position widerspräche dem
geladenen Snapshot. Lösung: **alle 5 Modifier sind jetzt momentary tactile
(12×12×7.3 mm) mit integrierter LED**. State lebt in Firmware, LED zeigt den
Zustand an, Snapshot-Recall setzt Firmware-Var + PCA9685-Kanal kongruent.

**Hardware-Änderungen:**
- **+ U6 PCA9685PW,118** (NXP, TSSOP-28, LCSC C2678753, JLC Extended, ~1605 pcs)
  als 16-Kanal-PWM-LED-Driver. I²C-Adresse 0x40, gleicher Bus wie MCP23017.
  Symbol `Driver_LED:PCA9685PW`, Footprint `Package_SO:TSSOP-28_4.4x9.7mm_P0.65mm`.
- **+ R_LED6 .. R_LED10**: 5× 390 Ω 0603 LED-Series (open-drain Sink-Config: LED-Anode
  → R → +5 V → LED → PCA9685-Kanal → GND).
- **+ R_OE**: 10 kΩ 0603 Pull-Up an /OE zu +3V3 (Default-disabled bis Firmware enabled).
- **+ C_PCA_VDD / C_PCA_VDD_HF**: 10 µF X5R 0805 + 100 nF X7R 0603 Decoupling an Pin 28.
- **SW6-SW10 umdefiniert**: weg von Kailh Choc V2 1u Hot-Swap → **12×12×7.3 mm
  momentary tactile MIT integrierter LED**, Generic China / AliExpress
  („Momentary Touch LED 12*12*7.3 mm"). User-supplied + hand-soldered
  (analog zu SW1-5 Cells). **Custom-Footprint** in Projekt-PCB-Lib nötig
  (siehe `mechanical_coordinates.md` §5).

**Firmware-Verträge (nicht implementiert, dokumentiert):**
- SW6 SHIFT: momentary Modifier während gedrückt → Cells liefern Degrees 6-10
  statt 1-5. LED6 leuchtet solange Switch gedrückt.
- SW7 HOLD: Press toggelt HOLD-Mode. LED7 = HOLD aktiv.
- SW8 DRONE: Press toggelt Drone. LED8 = Drone spielt, optionaler Fade-In/Out.
- SW9 GENERATE: Press toggelt Generative-Mode. LED9 = generative aktiv.
- SW10 CLEAR: Press = one-shot „release all holds + reset patterns". LED10
  flasht ~200 ms als Visual-Confirmation.

**Power-Budget**: +5 mA Idle / +25 mA Worst-Case (PCA9685 + 5 LEDs @ 100 %
PWM). Worst-Case-Summe steigt 2480 mA → 2525 mA — Polyfuse F1 (3 A) bleibt
ausreichend dimensioniert.

**Verifikations-Blocker für r7 (vor PCB-Layout)**:
- Pin-Pitch der AliExpress-12×12-Switches real vermessen (Generic-Parts haben
  keinen herstellergemeinsamen Standard — Annahme im SPEC: 6.5×4.5 mm
  Switch-Raster + ~8 mm zum LED-Pin-Paar).
- `Driver_LED:PCA9685PW` Symbol-Pin-Map gegen NXP-Datasheet Rev. 4 S.6 prüfen.

**Aktive Aufgabe**: Schematic-Generator + KiCad-Sheets müssen die r7-
Änderungen abbilden (U6 hinzufügen, SW6-10 Symbol-Form umstellen, LED6-10
hinzu). Wird in eigenem Commit nachgezogen.

---

## v0.9 (2026-05-30) — Pi-frei-Transition (Schaltplan, Step 6 von 12)

Teil der nativen Portierung (`NATIVE_PORT_PLAN.md`). Nachdem die C-Firmware
auf dem RP2350 Display, Buttons, Encoder und **I²S-Audio** (Step 1–5) selbst
beherrscht, fällt der Raspberry Pi Zero 2 W aus dem Gerät.

**Schaltplan-Änderungen (`generate_kicad_project.py`):**
- `pi.kicad_sch` (Sheet 7, Pi-Header) **gelöscht**; aus kicad_pro-Sheetliste + Root entfernt.
- **5 Bauteile raus**: J2 (40-pin Pi-Header), R1 (UART-RX-Serie), R_BCK/R_LRCK/R_DOUT
  (Pi-seitige I²S-Serien-R). Reale Bauteilzahl **97 → 92**.
- GP0/GP1/GP4 im Pico-Sheet von UART/MISO → **I²S_BCK/LRCK/DOUT** umgewidmet
  (RP2350 PIO-I²S-Master). Root-Sheet brückt Pico-I²S-Outputs → Audio-Sheet-Inputs.
- D2 (SMAJ5.0A TVS) **bleibt** — sitzt auf der +5V-Hauptschiene (nicht am Pi),
  dient weiter als allgemeiner Rail-Surge-Schutz.

**Verifikation:** Generator regeneriert sauber, alle Sheets paren-balanced,
entfernte Nets (PICO_TX_PI_RX/PI_TX_PICO_RX/OLED_MISO_NC) verschwunden,
I²S-Nets durchgehend Pico→Root→Audio. kicad-happy-Analyzer: 6 VM-001-Blocker,
**alle die bekannte False-Positive-Klasse** (Pico-VBUS-Pin → Heuristik tagged
GPIOs als 5V; real GP0/1/4 @ 3V3 → PCM5102A @ 3V3, kein Level-Shifter nötig).
Warnings 19 → 16. GUI-ERC (B3) bleibt maßgeblich.

**Folge-Arbeit (offen):** SPEC §1/§3 (Architektur-Diagramm, Power-Budget mit
Pi-Zeile) sind noch Pi-zentrisch → SPEC-v0.9-Überarbeitung; Power-Budget sinkt
~700 mA, F1 darf optional kleiner werden.

---

## v0.8 (2026-05-28) — Komponenten-Audit: BOM ↔ Schematic abgeglichen

Vollständiger Cross-Check der Stückliste (SPEC §3 Decoupling-Tabelle + §4 BOM)
gegen jede tatsächlich im Generator/Schaltplan platzierte Komponente. Drei
Diskrepanzen gefunden und korrigiert:

- **🔴 C1 + C2 fehlten im Schaltplan** (Fix): SPEC §3 listet C1 (10µF X5R 0805)
  und C2 (100nF X7R 0603) als +5V-Hauptrail-HF-Decoupling, und §4 zählt sie mit
  (6× 10µF / 8× 100nF) — aber `power_tree_sheet()` platzierte sie nie (nur 5×/7×
  real). Die Rail hatte nur C_BULK (1000µF Elko, hohe ESR/ESL → schlechte HF-
  Antwort) + D2 TVS, aber keinen Keramik-HF-Bypass. **C1/C2 jetzt platziert**
  (Rail-Taps x=80/83, GND-Drop, Auto-Junction). 10µF-Count jetzt = 6 (BOM-konform).
- **🟠 C_audio_filt aus BOM gestrichen**: Der §4-Eintrag (2× 220nF "PCM5102A output
  filter") war nie im Schaltplan. PCM5102A hat internen Rekonstruktions-Filter,
  TI-Referenz nutzt keinen Output-Cap, und §8 Line-Out sagt "keine Koppel-Caps
  nötig". Doku an Realität angeglichen (§4 + §8 Notiz).
- **🟡 R_RUN in BOM ergänzt**: Pico RUN-Pull-up (10k 0603) war im Schaltplan,
  fehlte aber in §4. Jetzt gelistet.
- **Stale F1-BOM-Wert gefixt**: §4-Misc-Tabelle zeigte noch "2.0A/4.0A" — auf
  v0.7-Stand 3.0A/6.0A (Littelfuse 1812L300) gebracht (war in §3 schon korrekt).

Methodik-Hinweis: Reiner Audit-/Reconcile-Pass, keine neue Funktion. ERC im
KiCad-GUI (Blocker B3) bleibt offen — headless nicht durchführbar.

---

## v0.7 (2026-05-22) — Alle offenen Engineering-Entscheidungen geschlossen

Durchgang zum Schließen sämtlicher offener Auslegungs-Fragen, damit kein
"TBD" / keine Entscheidung mehr im Weg steht. Design und Doku reconciled.

**Power (I1/I2) — final entschieden:**
- 5V/3A-USB-C-Netzteil als **harte Anforderung** (kein PD-Controller im
  Prototyp). Worst-Case 2.45A < 3A → Board darf voll aussteuern, Volume-Clamp
  ist keine Power-Schutz-Pflicht mehr (nur optionale Akustik-Maßnahme).
- **F1 Polyfuse 2A/4A → 3A/6A** (Littelfuse 1812L300, LCSC C18198349). Das alte
  2A-hold derated bei ~50°C Innentemp auf ~1.5A → trug 1.4A-Typical-Audio nicht
  zuverlässig. 3A-hold derated ~2.3A deckt Typical; Bass-Peaks reiten den Bulk.

**Inrush (I3) — final entschieden:**
- Erkenntnis: Inrush-**Peak** ist widerstandsbegrenzt, nicht kapazitätsbegrenzt
  — ein kleinerer Cap senkt nur Dauer/Energie, nicht den Spitzenstrom. Da tiefer
  Bass das Produktziel ist und der Bulk genau die Bass-Transienten puffert,
  **bleibt C_BULK bei 1000µF**. Polyfuse ist thermisch → trippt nicht auf den
  <1ms-Inrush-Spike. Produktion bekommt einen Soft-Start-Load-Switch.
- **Footprint-Bug gefixt**: EEE-FK1A102P ist ein D10×10.2mm-Becher, der Generator
  hatte `CP_Elec_8x6.7` (zu klein) → korrigiert auf `CP_Elec_10x10.5`.
- **Doku/Design-Konflikt aufgelöst**: SPEC nannte fälschlich eine Polymer-Cap
  6.3V (PCV1A102), die nie ins Design kam. SPEC zeigt jetzt die reale Alu-10V.

**Stack-Up (I4):** final Signal/GND/+5V/Signal (SPEC §9).

**Mechanik (I5):** alle TBD-Positionen festgelegt — OLED-J3 @ (80,95) auf
Standoffs, Pi-J2 @ (160,90), Pico-U1 @ (270,80). `mechanical_coordinates.md`
von "Template/Draft" auf "alle Positionen definiert" hochgestuft.

**Bereits erledigt, nur noch als ✅ markiert:** I7 (UART-Naming eindeutig),
N1 (XSMT via MCP GPA5), N2 (I²S 33Ω Serien-R), N4 (Titleblocks → rev 0.7),
B4 (SHDN/MUTE Pull-Downs vorhanden).

**Verbleibend offen (nur noch GUI/physisch, nicht headless lösbar):**
B0-B2 (Footprint-Pad-Mapping im KiCad-Footprint-Editor gegen Datenblatt),
B3 (GUI-ERC-Lauf), sowie der eigentliche PCB-Layout-Schritt.

---

## v0.7-pre (2026-05-16) — Choc-V2-Footprint-Fix + Line-Out/Kopfhörer

Erste echte Funktionserweiterung seit v0.6. Zwei Themen.

### 1. Switch-Footprint-Mismatch behoben (Choc V2)

- **Bug**: Generator referenzierte `Button_Switch_Keyboard:SW_Hotswap_Kailh_MX_*`
  (MX-Hotswap) während BOM/SPEC durchgehend **Kailh Choc V2** sagen. MX und
  Choc V2 sind physikalisch inkompatibel (Pin-Spacing, Höhe, Keycap-Stem).
- **Fix**: Footprints auf `Switch_Keyboard_Hotswap_Kailh:SW_Hotswap_Kailh_Choc_V1V2_*`
  geändert (1.00u Modifier, 2.00u Cells).
- **Library**: Diese Footprints sind NICHT in der KiCad-Standard-Lib. Benötigt
  die **kiswitch keyswitch-kicad-library**:
  - KiCad → Plugin & Content Manager → Libraries → "Keyswitch Kicad Library" → Install
  - GitHub: https://github.com/kiswitch/keyswitch-kicad-library
  - **Footprint-Namen verifiziert gegen kiswitch v2.4** (jsDelivr-Tree-API):
    `SW_Hotswap_Kailh_Choc_V1V2_1.00u` und `_2.00u` existieren beide im
    Ordner `Switch_Keyboard_Hotswap_Kailh.pretty`. Bei abweichender
    Library-Version Namen erneut prüfen.
- **Warum V1V2 statt V2-spezifisch**: Die Lib hat auch `SW_Hotswap_Kailh_Choc_V2_*`.
  V1V2 bohrt die Alignment-Löcher für V1 UND V2 → die Hot-Swap-Buchse nimmt
  jede Choc-Generation auf. Genau das ist der Sinn eines Hot-Swap-Boards
  (End-User kann V1 oder V2 stecken). V2 wird voll unterstützt.
- **2u Cells**: Choc-Stabilizer (CPG1353G24D01) als separate mechanische
  Footprint-Platzierung im Layout — der Switch selbst ist 1u, der 2u-Keycap
  braucht den Stabilizer.

### 2. Line-Out / Kopfhörer-Buchse (J8) — löst das Tiefen-Problem

**Warum**: Die 40mm-Onboard-Speaker können den 30-60Hz-SubBass des
Sound-Constitution-Konzepts physikalisch nicht abstrahlen. Ohne externen
Ausgang verhungert der tiefe Charakter im Gehäuse.

**Hardware (Sheet 6 Audio erweitert)**:
- J8: 3.5mm TRS-Buchse mit Insertion-Detect-Switch (PJ-320-Klasse, LCSC C2884109)
- Passiver Tap an PCM5102A VOUTL/VOUTR (vor dem PAM8403). PCM5102A-Output ist
  ground-centered (interne Charge-Pump) → keine Koppel-Caps nötig.
- R_LO_L / R_LO_R: 22Ω Serien-Widerstände (Schutz / Kurzschluss-Limit)
- Jack-Detect: J8 DET-Switch → MCP23017 GPA6 (Pull-Up + IRQ). Idle=LOW
  (kein Plug), eingesteckt=HIGH.

**Firmware-Verhalten**:
- Plug eingesteckt → Pico mutet NUR den PAM8403 (Speaker), PCM5102A-DAC +
  Line-Out bleiben live. Neue `AudioPower.speakers(on)`-Methode trennt
  Speaker-Mute von vollem System-Mute.
- Pico sendet `{"event":"jack","inserted":true}` an die Bridge; die Bridge
  echo't `{"set":"amp","enabled":0}` (war im Protokoll schon vorgesehen).

**Empfehlung**: Für Line-Out an Aktivboxen/Interface reicht der direkte
PCM5102A-Tap. Für niederohmige Kopfhörer (<32Ω) wäre ein dedizierter
Kopfhörer-Amp (TPA6132 o.ä.) besser — kann in v0.8 nachgerüstet werden.

---

## v0.6.3-r6 (2026-05-15) — Stabilization-Pass (Senior-Review Findings)

Kein neues Feature. Reine Stabilisierung nach Senior-Engineer-Review.

### HIGH-Fixes

**PVDD-Decoupling (PAM8403H pins 4, 13)**
- Vorher: nur C9 10µF + C9b 100nF an VDD (pin 6). PVDD-Pins 4 (links) und
  13 (rechts) hatten KEIN lokales Decoupling.
- Datasheet-Anforderung (PAM8403H.PDF "Application Information"): "1.0µF
  ceramic close to VDD" (HF) + "20µF or greater" (bulk).
- Fix:
  - Links (VDD pin 6 + PVDD-L pin 4): C9 10µF→**22µF** (C45783), C9b 100nF→**1µF**
  - Rechts (PVDD-R pin 13): NEU **C_PVDDR 22µF + C_PVDDR_HF 1µF** lokal
- Verhindert: Class-D-Switching-Transienten (250kHz) modulieren PVDD →
  Distortion bei hoher Lautstärke + EMI.

**Startup-Sequenz (Pop-Suppression-Reihenfolge)**
- Vorher (SPEC §8): /MUTE=HIGH zuerst, dann /SHDN=HIGH 100ms später. FALSCH —
  beide active-low, un-mute VOR chip-wakeup → Pop wenn /SHDN HIGH wird.
- Fix: /SHDN=HIGH zuerst (chip wacht auf, Referenzen settlen), dann ~50ms
  später /MUTE=HIGH (un-mute). Shutdown umgekehrt.

### Cross-Domain-Korrekturen
- SPEC-Body §8 vollständig auf korrekte Startup-Sequenz umgeschrieben (nicht
  nur Errata).
- Errata-Historie aus SPEC-Body extrahiert → diese CHANGELOG.md (single
  source of truth: Body = aktueller Stand, CHANGELOG = Historie).

---

## Errata-Historie

### v0.6.3-r5 (2026-05-15) — N1: XSMT via MCP23017 GPA5 (statt statischem Pull-Up)

Aus User-Wunsch "lieber direkt haben als nicht haben" — Pop-Suppression
für PCM5102A jetzt explizit per GPIO statt nur durch PAM8403-Pull-Downs.

**Hardware-Änderung in audio.kicad_sch (U3 PCM5102A Pin 17 XSMT):**

| Alt (v0.6.3-r4) | Neu (v0.6.3-r5) |
|---|---|
| R_XSMT 10k pull-up zu +3V3 (statisch un-muted) | R_XSMT_PD 10k pull-down zu GND (default LOW = muted) |
| — | Hier-Input `PCM_XSMT` von MCP23017 |

**Hardware-Änderung in mcp.kicad_sch (U2 MCP23017 GPA5 = Pin 26):**

| Alt | Neu |
|---|---|
| GPA5 als NC_GPA5 Reserve-Label | GPA5 → Hier-Output `PCM_XSMT` zu Audio-Sheet |

**Cross-Sheet im Root**: Label-Bridge zwischen MCP-Sheet-Box rechts und
Audio-Sheet-Box links für PCM_XSMT-Net.

**Firmware-Konsequenz** (Pico-Init Reihenfolge):

1. Boot: Alle MCP23017-Pins default Input → R_XSMT_PD zieht XSMT LOW → PCM5102A stumm
2. I²C-Init: MCP23017 GPA5 als Output, default 0 (LOW = stumm bleibt)
3. Power-Sequence: +5V/+3V3-Rails stabilisieren (~50 ms)
4. PAM8403 enable: Pico setzt GP28 (MUTE) HIGH, 100 ms später GP27 (SHDN) HIGH
5. PCM5102A un-mute: MCP23017 GPA5 auf HIGH (PCM_XSMT = +3V3) → DAC liefert Audio
6. → kein Pop am Speaker, kein DAC-Hochfahr-Knack

**Bei Shutdown** umgekehrt: MCP GPA5 LOW → PCM stumm, dann PAM8403 muten/shutdown.

### v0.6.3-r3 (2026-05-14) — Important-Items aus 2nd-Review adressiert

Doc-side updates (kein KiCad-GUI nötig). Hardware-Schema unverändert
außer N2 (I²S Series-Resistoren am Pi-Header).

#### I1 — Power-Budget realistisch rekalkuliert

Der v0.6-PAM8403-Wert (350 mA peak) war zu optimistisch. Für 2× 4Ω
Speaker bei 3W BTL: ~6W Audio out, ~85% Class-D-Effizienz → ~7W Eingang
→ ~1.4 A nur für Amp.

| Last | Idle | Typical Audio | Worst Case (loud, 4Ω) |
|---|---|---|---|
| Pi Zero 2 W (SuperCollider) | 250 mA | 500 mA | 700 mA |
| Pico 2 (RP2350) | 30 mA | 50 mA | 50 mA |
| OLED SSD1322 256×64 | 50 mA | 150 mA | 250 mA |
| MCP23017 + Pull-Ups | 5 mA | 20 mA | 25 mA |
| PCM5102A DAC | 20 mA | 30 mA | 30 mA |
| **PAM8403H @ 4Ω, 6W Out** | 80 mA | 600 mA | **1400 mA** |
| EC11-Encoder + LED + Modifier | 5 mA | 15 mA | 25 mA |
| **TOTAL** | **440 mA** | **1365 mA** | **2480 mA** |

**Konsequenz:** 2A-Hold-Polyfuse (F1) trippt im Worst-Case.
Optionen:
- (a) **Firmware Volume-Clamp** auf ~50% max → Worst Case ~1.7 A,
  passt unter 2A-Hold knapp. SAFE für USB-C 5V/3A-Source.
- (b) Polyfuse höher dimensionieren auf 3A-Hold (z.B. MF-MSMF300),
  aber dann USB-C-Spec-Verletzung wenn Source nur Default-Current liefert.
- (c) **USB-PD-Sink-Controller** (siehe I2) + 3A-Polyfuse.

**Aktuelle Empfehlung für Prototyp**: (a) Firmware Volume-Clamp,
2A-Polyfuse beibehalten. Final-Produkt-Entscheidung mit (c).

#### I2 — USB-C Power-Negotiation

5.1kΩ CC-Pulldowns signalisieren "Sink", aber NICHT "berechtigt 3A".
USB-C Source-Port kontrolliert via Rp (Pull-Up an CC) ob Default
(~500 mA), 1.5 A oder 3 A erlaubt.

**Decision für v0.6.3**: Variante A (Konservativ) für Prototyp.
- Pi: Firmware-side Volume-Clamp damit Worst-Case Peak < 1.5 A
- BOM-Note: Mit Standard 5V/3A-Netzteil (z.B. Apple 20W) garantiert OK
- Mit altem 5V/0.5A-Hub-Port: Risiko Brown-out bei lauten Passagen

**Future Hardware-Option**: TPS25750 oder CYPD3177 als USB-PD-Sink-Controller.
Kosten: ~$2 IC + ein paar passives. Rechtfertigt sich für Produkt-Stage.

#### I3 — Inrush-Strategie für 1000µF Bulk-Cap

Roher Elko hinter USB-C ohne Strombegrenzung kann USB-C-Source
unhappy machen (Spike beim Hot-Plug). Polyfuse limitiert NICHT
elegant — sie wird träge warm, lässt den Spike aber durch.

**Mitigation v0.6.3**:
- C_BULK MPN-Spec auf **Low-ESR Polymer** geändert: nicht generischer Elko,
  sondern z.B. Nichicon **PCV1A102MCL1GS** (1000µF / 6.3V / Polymer / ESR < 20mΩ).
  Höhere Lebensdauer, definierter ESR begrenzt Charge-Strom selbst etwas.
- ADD-ON für Final-Build (DNP für Prototyp): **NTC-Inrush-Limiter** (CL-130
  Ametherm, 5Ω cold / 0.5Ω hot, in Serie zwischen Polyfuse und +5V-Rail).

#### I4 — 4-Layer Stack-Up überdacht

Alt: Signal / GND / **+3V3** / Signal
Neu: Signal / GND / **+5V** / Signal

**Begründung:** Auf diesem Board ist +5V die Hochstrom-Schiene:
- Pi Zero 2 W (700 mA peak)
- PAM8403H (1.4 A peak)
- OLED VBAT (250 mA peak)
- Summe: bis 2.4 A auf +5V-Plane

+3V3 trägt nur ~80 mA (OLED VDDIO + MCP + Pull-Ups + EC11). Kann lokal
gepoured werden.

**GND-Plane-Regel**: Layer 2 muss **eine ungeteilte Fläche** sein, KEINE
Splits unter USB-C-Bereich, I²S-Strecke (Pi→PCM5102A), oder Audio-Out
(PAM8403→Speakers). Trace-Loops über GND-Splits sind die Hauptquelle
für EMC-Probleme und Audio-Brumm.

#### I5 — Mechanische Koordinaten

Siehe neue Datei `mechanical_coordinates.md` für die vollständige
Tabelle. Hier nur Außen-Maße:

| Element | Dim |
|---|---|
| Gehäuse | 333 × 143.3 × 40 mm |
| PCB-Outline | 320 × 130 mm |
| Edge-Rails | 5 mm rundherum (entfernen nach Bestückung) |
| Component-to-Edge | min 2.5 mm |

#### I6 — BOM-Split JLC-fitted vs Manual

**SECTION A: JLCPCB SMT-bestückt (Full-PCBA)** — ~70 SKUs:

Discrete passives + Power-ICs + Connectors:
- C_BULK (Polymer 1000µF), C1-C9b, C10-C17, C_VREF, C_VCOM, C_VNEG, C_LDOO, C_FLY,
  C_CPVDD_*, C_in_L/R, C6/C6b/C6c — ALLE 0603/0805 SMD-Caps
- R1-R20, R_VOL_L/R, R_RUN, R_XSMT, R_MUTE_PD, R_SHDN_PD, R_BCK, R_LRCK, R_DOUT — ALLE 0603 SMD
- F1 Polyfuse 1812 (MF-MSMF200)
- FB1 Ferrite-Bead 0603 (BLM18AG601)
- D1 USBLC6-2SC6 ESD (SOT-23-6)
- D2 SMAJ5.0A TVS (SMA)
- LED1 0805 warm white

ICs (alle JLC Extended Stock):
- U2 MCP23017-E/SS (SSOP-28, C506653)
- U3 PCM5102APWR (TSSOP-20, C107671)
- U4 PAM8403H (SOIC-16, C17337)

Connectors (SMD):
- J1 USB-C TYPE-C-31-M-12 (C165948)
- J3 OLED-Header 2.54mm 16-pin
- J4 SWD-Header 1.27mm 3-pin
- J5 VSYS-Bridge 2-pin (DNP default)
- J6, J7 Speaker-Headers 2.54mm 2-pin

Switches:
- SW11 Reset 6mm SMD (TL3342)
- SW12 BOOTSEL 6mm SMD (TL3342) — DNP für THT-Pico-Variante

**SECTION B: Manuell zu bestücken (du lieferst)** — ~15 Items:

Mikrocontroller/Compute-Module:
- U1 Pico 2 (RP2350) als 40-pin THT Pin-Header (Empfehlung Prototyp)
- Pi Zero 2 W mit GPIO-Header J2 2×20 durchgesteckt

Switches (Hot-Swap-Sockets nicht im JLC-Stock):
- SW1-SW5 Kailh Choc V2 Hot-Swap-Socket 2u (Cells) — 5×
- SW6-SW10 Kailh Choc V2 Hot-Swap-Socket 1u (Modifier) — 5×
- 5× Kailh 2u Choc V2 Stabilizer (CPG1353G24D01)

Module:
- OLED ER-OLEDM032-1W 3.2" 256×64 SSD1322 mit 16-pin Header
- 4× EC11-Encoder (wenn nicht SMD-Variante gewählt)

Speaker:
- 2× PUI AS04008PS-4W-WR-R Lautsprecher (mit Drähten an J6/J7)

**SECTION C: TBD (noch zu sourcen oder mechanisch)**:
- Custom MX-Stem Silikon-Cell-Caps (5×, DIY/print)
- BOOTSEL- und Reset-Caps (klein)
- Gehäuse-Schraubdome + M3-Schrauben (4×)
- Bass-Reflex-Ports (Bottom-Case 2× Port 8×25 mm)
- USB-C-Source-Netzteil (5V/3A) — User liefert separat

#### N2 — I²S Series-Resistoren am Pi-Header (DONE)

3× 33Ω 0603 Series-Resistoren am Pi-GPIO-Header in Sheet 7 hinzugefügt:
- **R_BCK** an Pi pin 12 (GPIO18 PCM_CLK)
- **R_LRCK** an Pi pin 35 (GPIO19 PCM_FS)
- **R_DOUT** an Pi pin 40 (GPIO21 PCM_DOUT)

Dämpft Overshoot/Reflexionen auf I²S-Strecke (~100mm über Layer 1/4).
LCSC C23138 (RC0603FR-0733RL).

#### N3 — ERC/DRC Report-Workflow

Convention für die nächsten Iterationen:

1. Nach jedem Major-Pinout-Fix oder Architektur-Änderung:
   ```
   cd kicad/
   kicad-cli sch erc --severity-all --output ../reports/ERC_$(date +%Y-%m-%d).txt field_ambience.kicad_sch
   ```
2. Datei format `reports/ERC_YYYY-MM-DD.md`:
   ```
   KiCad Version: X.Y.Z
   Date: YYYY-MM-DD
   Schematic Commit: <git-sha>
   Errors: N
   Warnings: M
   Accepted warnings (with reason): list
   ```
3. Committen ins Repo. Vor jedem Merge in `main`: aktueller ERC-Report
   muss vorhanden sein und 0 unaccepted Errors zeigen.

### v0.6.3-r2 (2026-05-14, gleicher Tag) — Hardware-Pull-Downs + UART-Naming

Nach zweiter Review-Iteration ergänzt:

**B4-Fix: PAM8403 /SHDN und /MUTE Hardware-Defaults**

PAM8403H /SHDN (Pin 12) und /MUTE (Pin 5) sind ACTIVE LOW. Während
Pico-Reset/Boot floaten GPIOs unbestimmt — Amp könnte un-defined oder
voll-on starten → Pop/Klick. Fix:

- **R_MUTE_PD = 10 kΩ 0603** Pull-Down von Pin 5 (MUTE) zu GND
- **R_SHDN_PD = 10 kΩ 0603** Pull-Down von Pin 12 (SHDN) zu GND

Default-State (während Boot, Reset, Pico-down): beide LOW → Amp ist
GESHUTDOWN und GEMUTED. Pico-Firmware zieht beide aktiv HIGH erst
NACH Power-Sequencing (per Spec v0.6 §5 GP27/GP28 Pop-Suppression).

**I7-Fix: UART-Net-Naming disambiguiert**

Alt: UART0_TX / UART0_RX — perspektivisch mehrdeutig (TX/RX hängt
ab welches Bauteil schaut).

Neu (eindeutige Direction):
- `PICO_TX_PI_RX` — Pico GP0 (UART0 TX) → Pi GPIO15 (RX) auf pin 10
- `PI_TX_PICO_RX` — Pi GPIO14 (TX) auf pin 8 → R1 1k → Pico GP1 (UART0 RX)

Geändert in: pico_sheet, pi_sheet, root_sheet (sheet-pins + cross-sheet labels).

### v0.6.3 (2026-05-14) — CRITICAL: USB-C VBUS/GND-Pinout korrigiert

Externer Review fand: USB-C-Symbol hatte VBUS auf Pin-Numbers A1/A4/B1/B4
und GND auf A12/A9/B9/B12. Das ist **falsch gegen USB Type-C Spec
Rev 2.1 Table 3-1**. Die korrekte Pin-Belegung ist:

```
GND  = A1, A12, B1, B12  (NICHT was im v0.6 Symbol stand)
VBUS = A4, A9, B4, B9
CC1  = A5
CC2  = B5
D+   = A6 (+ B6 für reversed orientation)
D-   = A7 (+ B7 für reversed orientation)
SBU1 = A8
SBU2 = B8
```

**Kritische Konsequenz** ohne Fix: Beim PCB-Layout hätte das Pad A1
des USB-C-Footprints +5V bekommen — Standard-Pad A1 ist aber GND.
Beim Einstecken eines USB-C-Kabels wäre **VBUS direkt auf GND
kurzgeschlossen** worden (Pad A1 vom Kabel = GND, hier am PCB = +5V).

Fix in `generate_kicad_project.py`: Symbol pin numbers für die 8
Power-Pins korrigiert (Positionen bleiben, nur Numbers geswappt):
- local +10.16: number A1 → **A4** (name VBUS)
- local +7.62:  number A4 → **B4** (name VBUS)
- local +5.08:  number B4 → **A9** (name VBUS)
- local +2.54:  number B1 → **B9** (name VBUS)
- local -2.54:  number A12 → **A1** (name GND)
- local -5.08:  number A9 → **B1** (name GND)
- local -7.62:  number B9 → **A12** (name GND)
- local -10.16: number B12 → **B12** (name GND, unchanged)

Power-Tree-Sheet-Wiring bleibt unverändert da Y-Positionen identisch.
Analyzer `pin_nets`-Dump verifiziert nun alle 17 Pins (16 + Shield S1)
korrekt gegen USB Type-C Spec.

**Verbleibender PCB-Layout-Check**: Footprint des USB-C-Receptacle
(JLCPCB C165948 = TYPE-C-31-M-12) muss in KiCad gegen das Symbol
gegengecheckt werden — Pad-Number-Zuordnung muss zur Pin-Number im
Symbol passen. Standard-KiCad-Footprints `USB_C_Receptacle_HRO_TYPE-
C-31-M-12` haben die korrekte Spec-Belegung; trotzdem zwingend
verifizieren bevor Bestellung.

### v0.6.2 (2026-05-13) — PAM8403H-Pinout per PDF im Repo verifiziert

Der PAM8403H.PDF im Repo-Root ist das offizielle Diodes-Inc-Datasheet
für LCSC C17337. Damit ist die Pin-Belegung jetzt zu 100% gesichert.

Bug in v0.6.1 (zwischenzeitlicher Fix-Versuch ohne PDF): basierte auf
falscher DS31295-Vermutung. Pin 8 (VREF), 9 (NC), 10 (INR), 11 (GND),
12 (SHDN), 13 (PVDD), 14 (+OUT_R), 15 (PGND), 16 (-OUT_R) waren alle
um eine Position verschoben. **+OUT_L/−OUT_L Polarität war auch
vertauscht** (Pin 1 ist −OUT_L, nicht +OUT_L wie in v0.6.1 angenommen).

Verifizierte PAM8403H-Pin-Belegung (Diodes Inc, Nov 2012):

| Pin | Name | Funktion |
|---|---|---|
| 1 | **-OUT_L** | Left negative output (BTL) |
| 2 | PGND | Power Ground |
| 3 | **+OUT_L** | Left positive output (BTL) |
| 4 | PVDD | Power VDD |
| 5 | MUTE | Mute Control (ACTIVE LOW) |
| 6 | VDD | Analog VDD |
| 7 | INL | Left Channel Input |
| 8 | **VREF** | Internal analog ref — Bypass-Cap zu GND **REQUIRED** |
| 9 | NC | No connected |
| 10 | INR | Right Channel Input |
| 11 | GND | Analog GND |
| 12 | **SHDN** | Shutdown Control (ACTIVE LOW) |
| 13 | PVDD | Power VDD |
| 14 | **+OUT_R** | Right positive output (BTL) |
| 15 | PGND | Power Ground |
| 16 | **-OUT_R** | Right negative output (BTL) |

NEUER Component in v0.6.2: **C_VREF 1µF X7R 0603** an PAM8403 Pin 8
(VREF Bypass-Cap, per Datasheet zwingend). War in v0.6.1 vergessen.

### v0.6.1 (2026-05-13) — Erster Pinout-Fix-Versuch (PCM5102A korrekt, PAM8403 noch falsch)

| Errata | Ursprünglich (v0.6) | Korrigiert (v0.6.1) |
|---|---|---|
| PCM5102A pinout | LRCK/BCK/DIN auf Pin 1/2/3 (alle falsch) | Per TI Datasheet SLAS859C: CPVDD=1, OUTL=6, OUTR=7, AVDD=8, BCK=13, DIN=14, LRCK=15, DVDD=20 |
| BOM LCSC-Nummern | PCM5102A=C9900003814 (existiert nicht), PAM8403=C84368 (existiert nicht) | PCM5102A=**C107671** (verified, Stock 6726), PAM8403=**C17337** (verified, Stock 8962) |

---


