# Audit — externes Fertigungspaket „Field Ambience Source files" (2026-07-30)

**Geprüftes Paket:** `Field Ambience Source files.zip`
(`SCHEMATICS.zip` = KiCad-9.0.4-Projekt „FIELD AMBIENCE", `GERBERS.zip`,
`FIELD_AMBIENCE_BOM.xlsx`, `FIELD_AMBIENCE_CPL.xlsx`, Handoff-PDF, 2 Screenshots)

**Prüfmethode:** vollständiges Parsen von `.kicad_sch` (302 Symbole) und
`.kicad_pcb` (195 Footprints / 239 Netze / 1462 Segmente / 166 Vias), Netzliste
aus den Pad-Netzzuweisungen der Leiterplatte rekonstruiert, Pin-Zuordnungen
gegen die im Repo bereits verifizierten Pinouts in
`kicad/generate_kicad_project.py` und gegen `firmware-c-next/src/hal_h743/`
gespiegelt, Geometrie-Checks (Zonen-Clearance, Bauteil-Abstände) numerisch,
BOM/CPL/PCB-Designator-Abgleich, Gerber-/Drill-/Job-File-Inspektion.

**Ergebnis in einem Satz:** **Nein — so nicht fertigen.** Das Paket ist formal
sauber (BOM/CPL/PCB 1:1 konsistent, keine floatenden Netze, keine
Pour-Kurzschlüsse), hat aber **7 Blocker**, davon zwei, die den 5-V-Zweig beim
ersten Einschalten zerstören können.

> **Wichtiger Hinweis vorab:** Dieses Layout ist **nicht** aus
> `kicad/generate_kicad_project.py` entstanden. Es ist ein eigenständiges,
> extern gezeichnetes Projekt mit EasyEDA-importierten Bibliotheken. Es fällt
> damit auch hinter mehrere bereits im Repo geschlossene Entscheidungen zurück
> (siehe § „Regressionen"). `PCB_LAYOUT_STATUS.md` behauptet weiterhin „Es
> existiert KEIN `.kicad_pcb`" (Stand 2026-06-11) — das ist überholt.

---

## 1 · Blocker — DO NOT FABRICATE

### B1 · L1 = 1,8 µH im 0402-Gehäuse als Boost-Speicherdrossel 🔴

- **Was:** `L1 = MLI1005F-1R8KT` (LCSC **C2885840**), Footprint `L0402`
  (Pads 0,54 × 0,54 mm), im Schaltknoten von U1 (TPS61089).
- **Warum kritisch:** U1 boostet VBAT (3,0–4,2 V) auf 5 V. Schon bei 500 mA
  Last am 5-V-Rail liegt der Spitzenstrom in der Drossel über 2 A. Ein
  0402-Vielschicht-Chip-Induktor ist für einige hundert mA spezifiziert und
  sättigt weit darunter; sein DCR liegt im Bereich mehrerer hundert mΩ.
- **Was ausfällt:** Der Wandler regelt nie aus, geht in Dauerstrombegrenzung,
  die Drossel wird thermisch zerstört. Der komplette 5-V-Zweig (Amp,
  Kopfhörer, 3,3-V-LDO → MCU) hängt daran.
- **Fix:** Repo-BOM verwenden: **SWPA6045S2R2NT, 2,2 µH, 6045, ~4 A
  (C36500)**. Dabei prüfen, ob die Typ-II-Kompensation (R3/C6) noch passt —
  sie wurde für 2,2 µH ausgelegt, nicht für 1,8 µH.

### B2 · Der gesamte Boost-Eingangsstrom läuft über den Schiebeschalter SW2 🔴

- **Was:** `VSYS → SW2.1 / SW2.2 → PWR_ON`, und `PWR_ON` speist **L1.1 **und**
  U1.9 (VIN)** **und** U1.7 (EN) **und** U5.3 (ON).
- **Warum kritisch:** SW2 = **SSSS811101** (C109335), ein ALPS-Signalschalter.
  LCSC führt ihn mit 12 V / 50 mA. Über ihn fließen hier 1,5–2 A.
- **Was ausfällt:** Kontakt verschweißt oder brennt weg — je nachdem bleibt das
  Gerät entweder dauerhaft an oder dauerhaft tot. Zusätzlich sitzt ein
  mechanisch klappernder Kontakt direkt im Schaltwandler-Eingangspfad
  (Bounce → Wandler-Restarts).
- **Fix:** Topologie aus **ADR-0016** übernehmen: VSYS geht **direkt** an
  U1.VIN und an das Eingangs-C. SW2 schaltet **nur** das Signal `PWR_ON`
  (= EN von U1 + ON von U5).

### B3 · `PWR_ON` hat keinen Pull-down 🔴

- **Was:** SW2 ist SPDT; der zweite Kontakt (Pad 3) ist unbeschaltet. Am Netz
  `PWR_ON` hängen nur C2, L1.1, U1.7 (EN), U1.9 (VIN), U5.3 (ON) — **kein
  Widerstand nach GND**.
- **Warum kritisch:** Bei geöffnetem Schalter ist EN des Boosts und ON des
  Load-Switch undefiniert (hochohmig).
- **Was ausfällt:** Das Gerät schaltet sich durch Störeinkopplung selbst ein
  oder schwingt im Ein-/Aus-Grenzbereich; im Aus-Zustand fließt kein
  definierter µA-Ruhestrom.
- **Fix:** `R_PWR_PD` (100 k) von `PWR_ON` nach GND — zusammen mit B2 in einem
  Rutsch.

### B4 · USB-C: D+/D− nur auf einer Steckerorientierung verdrahtet 🔴

- **Was:** `USBC1.B6 = DR+`, `USBC1.B7 = DR−`; **A6 (DP1) und A7 (DN1) sind
  unbeschaltet** (`unconnected-(USBC1-DP1-PadA6)` / `-DN1-PadA7`).
- **Warum kritisch:** Bei einer USB-2.0-Buchse müssen A6↔B6 und A7↔B7
  gebrückt werden, sonst funktioniert nur eine der beiden Steckrichtungen.
- **Was ausfällt:** USB (DFU-Flashen, Laden-Erkennung, Host-Kommunikation)
  enumeriert in ~50 % aller Steckvorgänge nicht. Das ist genau die Art Fehler,
  die im Bring-up tagelang als „defekte Platine" fehlgedeutet wird.
- **Fix:** A6→B6 und A7→B7 direkt an der Buchse verbinden.

### B5 · LED17 ist verpolt 🔴

- **Was:** `XL-1608UOC-06` hat laut Symbol **Pin 1 = K, Pin 2 = A**. Auf der
  Platine: `LED17.1 (K) → R45 → +5V`, `LED17.2 (A) → PCA9685 Kanal 9`.
- **Warum kritisch:** Alle anderen 16 LEDs sind korrekt A → R → +5 V und
  K → PCA-Senke (auch die baugleichen LED14–16, `XL-1608UWC-04`, mit
  identischer Pin-Konvention). LED17 ist als einzige gedreht.
- **Was ausfällt:** LED17 leuchtet nie. Kein Bodge ohne Leiterbahn-Schnitt.
- **Fix:** LED17 im Schaltplan drehen.

### B6 · PCA9685 treibt LEDs gegen +5 V, Firmware konfiguriert Totem-Pole 🔴

- **Was:** U11 läuft an 3,3 V (Pin 28), die LED-Anoden hängen über 390 Ω am
  **ungeschalteten +5 V**. `mcp23017_h743.c:197` setzt
  `PCA_MODE2 = PCA_MODE2_OUTDRV` (Totem-Pole).
- **Warum kritisch:** Die LEDn-Ausgänge des PCA9685 sind nur **im
  Open-Drain-Modus** 5,5-V-tolerant. Im Totem-Pole-Modus gilt
  Abs-Max = VDD + 0,5 V = 3,8 V; der 5-V-Pull-up speist im Aus-Zustand
  dauerhaft in den High-Side-Treiber und über dessen Body-Diode zurück ins
  3,3-V-Rail.
- **Was ausfällt:** Latch-up-Risiko am PCA9685, glimmende LEDs im Aus-Zustand,
  Rückspeisung ins 3,3-V-Netz.
- **Fix (eine der beiden):** `MODE2 = 0x00` (Open-Drain) in `pca_dev_init()`
  — dann ist die Beschaltung datenblattkonform; **oder** die LED-Anoden auf
  3,3 V legen (Vorwiderstände neu rechnen). Firmware-Fix ist der billigere Weg,
  muss aber vor dem Bring-up in `mcp23017_h743.c` landen.

### B7 · MIDI-Out-Buchse CN5 liegt auf den falschen Kontakten 🔴

- **Was:** CN2 und CN5 sind dasselbe Bauteil (`PJ-320D`, C431535) mit demselben
  Footprint. Bei **CN2** liegt GND auf **Pad 3**, Audio L/R auf Pad 1/2, der
  Schaltkontakt (Jack-Detect) auf Pad 4. Bei **CN5** liegt GND auf **Pad 1**,
  R50 (10 k → 3,3 V) auf Pad 2, R49 (220 Ω ← USART2_TX) auf **Pad 4**, und
  **Pad 3 ist unbeschaltet**.
- **Warum kritisch:** Zwei identische Bauteile mit widersprüchlicher
  Pin-Belegung — mindestens eine der beiden Buchsen ist falsch verdrahtet.
  Unabhängig davon ist das CN5-Netzwerk kein MIDI-TRS-Type-A-Ausgang: der
  „+"-Zweig braucht ~33 Ω gegen 3,3 V (CA-033), nicht 10 kΩ. Über 10 kΩ lässt
  sich die 5-mA-Stromschleife eines MIDI-Optokopplers nicht treiben.
- **Was ausfällt:** MIDI-Out tot; bei CN2 im schlechtesten Fall Audio auf dem
  Schaltkontakt statt auf Tip/Ring.
- **Fix:** Pad↔Kontakt-Zuordnung des PJ-320D gegen das Datenblatt festnageln
  (`PJ-320D Pad-/Kontakt-Zuordnung: UNVERIFIED — NEEDS HUMAN CHECK`), dann
  beide Buchsen konsistent neu verdrahten. MIDI: Tip ← 33 Ω ← USART2_TX,
  Ring ← 33 Ω ← 3,3 V, Sleeve = GND (Repo-Referenz r18.67 / ADR-0004).

---

## 2 · Wichtig — vor Fertigung klären

| # | Befund | Wirkung | Fix |
|---|--------|---------|-----|
| **I1** | **STM32 VBAT (Pin 6) unbeschaltet** | ST verlangt VBAT → VDD, wenn keine Backup-Zelle sitzt. Backup-Domain/RTC (X2 ist bestückt!) unzuverlässig, Startprobleme möglich | VBAT → 3,3 V + 100 nF |
| **I2** | **VREF+ (Pin 20) nur mit C43/C44 nach GND, nicht an VDDA** | ADC ohne Referenz; Pin floatet, solange die Firmware den VREFBUF nicht einschaltet | VREF+ an das VDDA-Netz (wie im Repo-Generator) |
| **I3** | **Encoder 3 und 4 auf Pins ohne Timer-Quadratur** — SW14 → PC4/PC5, SW15 → PB0/PB1 | PC4/PC5 haben auf dem H743 gar keine TIM-AF; PB0/PB1 nur CH3/CH4/CHxN — **kein TIMx_CH1/CH2**. Die Firmware fährt alle 4 Encoder im TIM-Encoder-Modus (TIM2/3/4/1). Zwei Encoder müssten auf EXTI/Polling → Schrittverluste beim schnellen Drehen + zusätzliche ISR-Last neben dem Audio-Hot-Path | Auf PA0/PA1, PC6/PC7, PD12/PD13, PA8/PA9 legen (oder TIM2/TIM3-Remaps PA15/PB3, PB4/PB5) |
| **I4** | **Pinmap-Kollisionen mit `hal_h743`** (Details § 4) | Board ist **nicht** drop-in für die aktuelle Firmware | Firmware-Port einplanen **oder** Layout angleichen |
| **I5** | **8 von 15 Status-LEDs hängen an einem anderen PCA9685-Kanal als ihr Netzname sagt** | `leds.c` schreibt „LED n → Kanal n" → falsche LED leuchtet | Mapping-Tabelle in Firmware **oder** Schaltplan korrigieren (§ 5) |
| **I6** | **Lagenaufbau weicht von SPEC §9 (Sig/GND/PWR/Sig) ab** | In1.Cu = 3,3-V-Pour, **zusätzlich mit 233 Signalsegmenten durchschnitten**; In2.Cu **ohne jede Fläche**, nur 151 Tracks. Einzige Masse sind die von Routing zerschnittenen F.Cu/B.Cu-Pours. Für 480-MHz-H7 + 2-MHz-Boost + Analogaudio das größte EMV-Risiko des Layouts | Durchgehende GND-Fläche auf einer Innenlage, Signale von der Plane-Lage runter |
| **I7** | **DRC im Projektfile faktisch abgeschaltet** | `clearance`, `annular_width`, `hole_clearance`, `padstack`, `*_courtyard`, `silk_*`, `starved_thermal` = **ignore**; `min_clearance = 0`, `min_track_width = 0`, `min_connection = 0` | Severities zurück auf `error`, DRC neu laufen lassen. **Ein DRC-Lauf mit diesem Projektfile beweist nichts.** |
| **I8** | **NPTH-Löcher als plattierte Löcher mit Ringbreite 0 exportiert** | Die NPTH-Bohrdatei ist **leer**; alle Befestigungs-/Ankerlöcher (CN2/CN5 1,0 mm, SW1/SW3–6 2,0 + 3,5 mm, SW2 0,9 mm, USBC1 0,75 mm) liegen in der PTH-Datei mit Paddurchmesser = Bohrdurchmesser | Pads in den Footprints auf `np_thru_hole` umstellen, Gerber neu exportieren. Sonst JLC-DFM-Rückfrage |
| **I9** | **Boost-Eingangs-C 6,2 mm von U1 entfernt** (C2, 22 µF; Ausgangsbank 4,6–9 mm) | Hot Loop viel zu groß für einen bis 2,4 MHz schaltenden Wandler → Überschwinger am SW-Knoten, EMV | Power-Stage neu platzieren (fällt mit B1/B2 ohnehin an) |
| **I10** | **HSE-Quarz X1 auf der Gegenseite, ~10 mm von PH0/PH1, mitten im Frontpanel** | Elektrisch: langer, viadurchsetzter Taktpfad. Mechanisch: 11,4 × 4,7 mm HC49-SMD zwischen den Tasten auf der Bedienfläche | X1 + C54/C55 direkt an PH0/PH1 auf die MCU-Seite |
| **I11** | **Keine Serienwiderstände auf dem LCD-SPI**, H1 sitzt 54,5 mm von der MCU plus Kabel | Ringing/EMV bei den Refresh-Raten, auf die die LCD-Motion-Arbeit zielt | 22–33 Ω in Serie auf SCK/MOSI/CS/DC nahe der MCU |
| **I12** | **H1-Pinreihenfolge unüblich** (1 = VCC, 2 = GND, 3 = DIN, 4 = CLK, 5 = CS, 6 = DC, 7 = RST, 8 = BL) | Gängige ST7789-Module sind GND, VCC, SCL, SDA, RES, DC, CS, BLK → direktes Aufstecken legt VCC auf GND. Zusätzlich setzt die Backlight-Schaltung (Low-Side-2N7002 an BL) voraus, dass BL die LED-**Kathode** ist; viele Module erwarten BL = aktiv-high-Enable | `Exaktes Display-Modul: UNVERIFIED — NEEDS HUMAN CHECK`, danach Pinout + Backlight-Pfad festziehen |
| **I13** | **Akku-NTC R13 sitzt auf der Platine, nicht an der Zelle** | R13 (NCP15XH103) liegt in der Power-Ecke neben Lader und Boost → BQ24074-TS misst Platinentemperatur. Unter Last droht ein Temperatur-Fault, der das Laden abbricht. Das Repo verwendet bewusst einen festen 10 k statt eines NTC | Festwiderstand 10 k **oder** NTC an die Zelle (Pack-NTC über CN1) |
| **I14** | **PCA9685 EXTCLK (Pin 25) floatet** | CMOS-Eingang, laut NXP nicht offen lassen | auf VSS legen |
| **I15** | **PSRAM-Exposed-Pad (U9 Pad 9) unbeschaltet** | Thermik/EMV | auf GND |

---

## 3 · Regressionen gegenüber dem Repo-Stand (r19.37 ff.)

| # | Board | Repo-Stand | Konsequenz |
|---|-------|-----------|------------|
| **R1** | `U8 = PAM8403DR` (C17337) | **PAM8406DR (C86270)**, r19.37 / ADR-0025 — PAM8403 ist NRND | Über die Lifecycle-Frage hinaus: der PAM8403 hat **feste +24 dB**. Der r19.37-Gain-Staging-Fix (RI 20 k → 174 k, +23 dB → +4,3 dB) **lässt sich auf diesem Board gar nicht anwenden** — der 5-V-BTL-Amp clippt analog weit unter DAC-Vollaussteuerung. Exakt der Fehler, der bereits gefunden und behoben war |
| **R2** | `C69/C70 = 1 µF` Eingangskoppel-C | **10 nF** (Speaker-HPF ~91 Hz, r19.37) | HPF liegt bei ~8 Hz; der 8-Ω-40-mm-Treiber bekommt die volle Bassenergie |
| **R3** | TPA6132A2 Gain = **0 dB** (G0 = H, G1 = L) | **−6 dB** laut ADR-0024 | Pegelplan neu beurteilen — zusammen mit R1 |
| **R4** | `U3 = AP7361C-33E-13`, **SOT-223** (C500795), verdrahtet als 1 = IN, 2 = GND, 3 = OUT, Tab = GND | **AP7361C-33Y5-13, SOT-89-5** (C460397); r18.6 hat eine SOT-223-Variante ausdrücklich verworfen, weil das Pinout abweicht | `AP7361C SOT-223 Pin-Reihenfolge: UNVERIFIED — NEEDS HUMAN CHECK`. Falsch = das komplette 3,3-V-Rail ist tot. **Muss vor Fertigung gegen das Diodes-Datenblatt geprüft werden** |
| **R5** | `U9 = APS6404L-3SQR-ZR`, USON-8 (C3040877) | APS6404L-3SQN-SN, SOIC-8 (C3028887) | Pin-Mapping geprüft und **korrekt**; nur BOM-Divergenz dokumentieren |
| **R6** | `U2.14 (TMR) → GND` = Safety-Timer **aus** | TMR = NC = 5-h-Default | Bewusst? Sonst NC lassen |
| **R7** | — | `PCB_LAYOUT_STATUS.md`: „Es existiert KEIN `.kicad_pcb`" (2026-06-11) | Repo-Doku ist überholt und muss nachgezogen werden |

---

## 4 · Firmware ↔ Board — Pinmap-Abgleich

Verglichen gegen `firmware-c-next/src/hal_h743/`.

**Stimmt überein:**

| Funktion | Firmware | Board |
|---|---|---|
| SAI1 FS / SCK / SD | PE4 / PE5 / PE6 | Pin 3 / 4 / 5 ✅ |
| QSPI CLK / BK2_NCS / IO0-3 | PB2 / PC11 / PE7–PE10 | Pin 36 / 79 / 37–40 ✅ |
| Amp /SHDN, /MUTE | PB14, PB15 | Pin 53, 54 ✅ |
| MIDI TX | PD5 (USART2) | Pin 86 ✅ |
| LCD SCK / MOSI / CS | PA5 / PA7 / PA6 | Pin 29 / 31 / 30 ✅ |
| Encoder 1, 2 | PA0/PA1 (TIM2), PC6/PC7 (TIM3) | ✅ (bei Enc 2 A/B getauscht → Drehrichtung invers) |
| MCP23017 GPA0–4 = Cells, GPA5 = XSMT, GPA6 = Jack, GPA7 = VBUS | `IODIRA 0xDF`, `GPPUA 0x5F` | ✅ bit-genau |
| Backlight = PCA9685 Kanal 15 → 2N7002 | ✅ | ✅ |

**Weicht ab:**

| Funktion | Firmware | Board | Anmerkung |
|---|---|---|---|
| LCD DC / RES | PC4 / PC5 | **PD15 / PD14** | PC4/PC5 sind auf dem Board jetzt Encoder 3 |
| MCP23017-Bus | I²C1 PB6/PB7 (gemeinsam mit PCA9685) | **I²C4 PD12/PD13** | PD12/PD13 sind in der Firmware TIM4 = Encoder 3 |
| Encoder 3 | PD12/PD13 (TIM4) | **PC4/PC5** | siehe I3 — keine Timer-Quadratur |
| Encoder 4 | PA8/PA9 (TIM1) | **PB0/PB1** | siehe I3 — keine Timer-Quadratur |
| Encoder-Push 1–3 | PE0 / PE1 / PE3 | über MCP GPB5–7 + PD4 | Port nötig |

Fazit: **kein Respin nötig, aber ein geplanter Firmware-Port** — mit der
Ausnahme I3, die Hardware ist.

---

## 5 · LED ↔ PCA9685-Kanal (Ist-Zustand)

| Netzname | tatsächlicher Kanal | LED |
|---|---|---|
| LD0–LD4 | 0–4 ✅ | LED6, LED4, LED5, LED3, LED7 |
| **LD5** | **11** ❌ | LED16 |
| LD6 | 6 ✅ | LED9 |
| **LD7** | **10** ❌ | LED10 |
| LD8 | 8 ✅ | LED11 |
| **LD9** | **7** ❌ | LED13 |
| **LD10** | **5** ❌ | LED8 |
| **LD11** | **14** ❌ | LED14 |
| **LD12** | **13** ❌ | LED15 |
| **LD13** | **12** ❌ | LED12 |
| **LD14** | **9** ❌ | LED17 (zusätzlich verpolt, siehe B5) |
| LD15 | 15 ✅ | Backlight-FET Q1 |

---

## 6 · Kosmetik / Dokumentationsschulden

Verstöße gegen `AI_READY_SCHEMATIC_STANDARD.md` — nicht fertigungskritisch,
aber sie machen den Schaltplan als maschinenlesbare Design-Datenquelle
unbrauchbar:

- **Netznamen mit Leerzeichen:** `LCD CS`, `LCD DC`, `LCD SCK`, `LCD RST`,
  `SCL MCP`, `SDA MCP`, `SCL PCA`, `SDA PCA`, `OUT DR+`, `OUT DR-`.
  Aktiv-low fehlt durchgängig: `SHDN` → `AMP_SHDN_N`, `MUTE` → `AMP_MUTE_N`.
- **Netzlabels widersprechen den echten Pins:** „PB0"/„PB1" auf Pin 35/34
  (vertauscht), „PC6"/„PC7" auf Pin 63/64 (vertauscht), „GPA5" auf MCP-Pin 25
  (= GPA4).
- **Falsche MPN-/Package-Felder im Schaltplan** — die BOM ist in allen Fällen
  richtig, es wird also nichts falsch bestellt, aber der Schaltplan lügt:
  - R10/R14: Wert „470 Ω", MPN-Feld `RC0603FR-07470KL` = **470 kΩ**
    (BOM: C144657 = 470 Ω ✅)
  - R8/R53: Wert „200 kΩ", MPN-Feld `0603WAF3900T5E` = **390 Ω**
    (BOM: C105574 = 200 k ✅)
  - R37/R38/R55/R56: Footprint-Feld `R0402`, MPN-Feld `RC0603FR-074K7L`
    (BOM: C99782 = 4,7 k 0402 ✅, Pads sind 0402 ✅)
  - C39-Familie: Bibliotheksname „10uF 0805" auf einem C1210-Land
- **PAM8403-Symbol:** Pin 12 heißt „SHND" (Tippfehler für SHDN);
  MCP23017-Symbol: Pin 12 heißt „SCK" (SPI-Name des MCP23**S**17, hier SCL).
- **BOM enthält H2–H5 (Bohrungen) als Positionen mit leerem LCSC-Feld** —
  der JLC-BOM-Upload läuft darauf in einen Fehler. Streichen oder DNP.
- **`copper_finish: "None"`** im Stackup, entsprechend `"Finish": "None"` im
  Gerber-Job-File. Auf ENIG/HASL setzen, damit Fertigungsdaten und Bestellung
  übereinstimmen.
- **Ungespeicherte Schaltplan-Änderungen im Archiv:**
  `_autosave-FIELD AMBIENCE.kicad_sch` (30.07. 14:23) ist **neuer** als
  `FIELD AMBIENCE.kicad_sch` (29.07. 22:39) und enthält zusätzlich die
  `Mechanical:MountingHole_Pad`-Symbole (H2–H5). Der gespeicherte Schaltplan
  ist also gegenüber der Platine um genau diese vier Bohrungen im Rückstand.
  Die `.kicad_pcb` ist identisch mit ihrem Autosave ✅.

---

## 7 · Was sauber ist (geprüft, keine Beanstandung)

- **BOM ↔ CPL ↔ PCB sind 1:1 konsistent:** 195 Designatoren in Platine und
  BOM, keine Karteileiche in beide Richtungen, alle Stückzahlen stimmen mit der
  Zahl der Designatoren überein, LCSC-Nummern überall gesetzt (Ausnahme:
  H2–H5), CPL enthält korrekt 191 Positionen (Bohrungen ausgenommen),
  Top/Bottom-Zuordnung in CPL und PCB deckungsgleich (69 top / 122 bottom).
- **Keine floatenden Netze:** alle 239 Netze haben ≥ 2 Pads.
- **Keine Pour-Kurzschlüsse:** trotz `connect_pads (clearance 0)` in der
  GND-Zone hält der Pour geometrisch gemessen **überall exakt 0,2 mm** Abstand
  zu jedem fremden Pad (583 Pad-Instanzen geprüft, Median 0,201 mm).
- **STM32-Entkopplung gut:** je 100 nF in 1,8–3,3 mm an allen fünf VDD-Pins;
  VCAP1/VCAP2 mit 2,2 µF in 2,8 / 3,2 mm.
- **BQ24074 korrekt beschaltet** (gegen die im Repo verifizierte
  TI-SLUS810N-Lesart): EN1 = LO / EN2 = HI = ILIM-Widerstandsmodus ✅,
  CE_N = GND ✅, **ITERM = NC ist der Datenblatt-Default (10 %) und damit
  richtig** ✅, ISET 1,13 k ≈ 0,79 A, ILIM 1,18 k ≈ 1,36 A, EP auf GND ✅,
  /CHG- und /PGOOD-LED korrekt gepolt ✅.
- **TPS61089:** FB-Teiler 332 k / 107 k → 4,94 V ✅; **R_FSW gegen den
  SW-Knoten ist korrekt** (TI SLVSD38C Table 6-1 — kein Fehler, obwohl es
  ungewöhnlich aussieht) ✅; BOOT-C vorhanden ✅.
- **TPS22918:** QOD an VOUT gebunden, CT offen — genau die ADR-0016-Absicht ✅.
- **TPA6132A2:** Ladungspumpen-Netzwerk vollständig (CPP/CPN/HPVDD/HPVSS),
  Eingänge single-ended mit IN+ auf GND, EP auf GND ✅.
- **PCM5102A / APS6404L / MCP23017 / PCA9685:** Pin-Zuordnungen gegen die
  Symbole geprüft, keine Vertauschung.
- **USB-ESD (USBLC6-2SC6) vorhanden, CC1/CC2 mit 5,1 k** ✅.
- **Board-Outline geschlossen**, 198,84 × 106,92 mm, 4 Lagen, 1,6 mm,
  Vias 0,6/0,3 mm, Innenlagen-Clearance 0,127 mm — alles innerhalb der
  JLC-Standardkapazität.

---

## 8 · Empfohlene Reihenfolge

1. **B1 + B2 + B3 zusammen** — Power-Stage neu zeichnen und neu platzieren
   (Drossel, Schalterpfad, Pull-down, Eingangs-C an den IC).
2. **B4, B5, B7** — reine Netzlisten-Korrekturen.
3. **B6** — Firmware-Einzeiler (`MODE2 = 0x00`), sofort machbar.
4. **R1 + R2 + R4** — Audio-Front-End auf den r19.37-Stand ziehen und das
   LDO-Pinout verifizieren.
5. **I1, I2, I14, I15** — vier kleine Netzlisten-Ergänzungen.
6. **I3 + I4** — Encoder-Pins gerade ziehen, danach Firmware-Port planen.
7. **I6** — Lagenaufbau auf SPEC §9.
8. **I7 + I8** — DRC-Severities zurücksetzen, NPTH korrigieren, Gerber neu
   exportieren, **erst dann** ist ein DRC-Lauf aussagekräftig.
