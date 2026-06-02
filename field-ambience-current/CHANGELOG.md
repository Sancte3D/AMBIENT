# Field Ambience PCB — CHANGELOG

Vollständige Änderungshistorie der PCB-Spec und des KiCad-Schematic.
Die Spec-Body selbst (`field_ambience_pcb_SPEC_v0.6.md`) beschreibt
**immer den aktuellen Stand** — diese Datei trackt wie wir dahin kamen.

Aktuelle Rev: **v0.6.3-r14** (Acoustic-v2: Sealed + Top-Firing, Passivradiator-Plan
verworfen, 4Ω→8Ω Impedanz-Fix). Generator und SPEC im Sync. Pi-frei (v0.9) bleibt
der maßgebliche Audio-Stand.

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


