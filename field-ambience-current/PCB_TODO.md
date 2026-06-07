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

## 🔴 BLOCKER v0.7-r18 (MCU-Migration, NEU 2026-06-07)

### r18-B1: STM32H743 Schematic-Sheet ersetzt pico.kicad_sch
**Status**: 🔴 BLOCKER — Phase 3 der MCU-Migration. Siehe `NATIVE_PORT_PLAN.md`
Step 13.3. Vorher Phase 2 (HAL-Abstraktion).
**Fix-Pfad**: `generate_kicad_project.py` erweitern; STM32H743VIT6 Symbol +
LQFP-100 Footprint; SWD-Header, BOOT0-Pull-Down, NRST, HSE 8 MHz Crystal +
Load-Caps; VDD/VDDA-Decoupling nach ST AN3318.

### r18-B2: power_tree.kicad_sch erweitern für H743
**Status**: 🔴 BLOCKER — Phase 3. AP7361A LDO aktivieren (war DNP), VCAP-Caps,
VDDA-Ferrit-Filter, Reset-Sequencing-Schaltung.

### r18-B3: Firmware HAL-Layer einziehen
**Status**: 🔴 BLOCKER für H743-Firmware — Phase 2. HAL-Header
(`audio_hal.h`, `gpio_hal.h`, etc.) abstrakte Interfaces; Pico-Treiber in
`src/hal_pico/` archivieren; CMake `-DTARGET=pico2|h743|host` Schalter.
**Gate**: Pico-Build muss grün bleiben als Regressions-Anker.

### r18-B4: Profiling-Acceptance-Gate vor PCB-Layout
**Status**: 🔴 BLOCKER — Phase 5. Cycle-Count-Messung via DWT->CYCCNT auf
echter H743-Hardware. Acceptance < 40 % Block-Zeit Worst-Case (1.1 M Cycles
von 2.8 M Budget @ 480 MHz / 256 Frames / 44.1 kHz). 1-Stunden-Dauerlauf
ohne Underrun. **Vor diesem Gate: KEIN PCB-Layout.**

---

## 🔴 BLOCKER für PCB-Layout (r7/r10-spezifisch, 2026-05-31)

### r7-B1: 12×12×7.3 Modifier-Switch Pin-Pitch verifizieren — RESOLVED durch r10
**Status**: ✅ RESOLVED via r10. r7 forderte Vermessung eines AliExpress-
Generic-Custom-Footprints. Mit r10 wechseln SW6-SW10 auf **HX 12x12x7.3TPFT-B**
(C36498966, JLC Extended, 29.840 pcs Stock) — plain 4-pin SMD-Tactile mit
Industrie-Standard-12×12-Footprint. Kein Custom-Footprint, kein Sample-
Vermessen mehr nötig. **Verbleibende Verifikation jetzt unter r10-B8**.

### r10-B8: HX-Switch Pad-Geometrie verifizieren (downgraded von r7-B1)
**Status**: 🟠 IMPORTANT (kein PCB-Layout-Blocker mehr). HX 12x12x7.3TPFT-B
Datasheet ist bei LCSC nicht hinterlegt. Industrie-Standard für 12×12 SMD-4P
ist seit 2010er Jahren stabil (6.5×4.5 mm Pad-Raster, 1.0×1.5 mm Pads), und
das Package-Label „SMD-4P,11.8×11.8mm" matcht den Standard. Optionen:
(1) Standard-KiCad-Footprint `Button_Switch_SMD:SW_SPST_TL3342` 1:1 nehmen — **empfohlen**,
(2) 1 Sample @ $0.05 bestellen + Caliper-Vermessung als Sicherheits-Pass,
(3) Alternative mit verfügbarem Datasheet wählen (z.B. KH-12X12X7H-SMT
C18186471, 328 pcs Stock — weniger, aber Datasheet wahrscheinlicher).

### r10-B9: 10× SMD-0603-LEDs + 5× R_LED11-15 ins Schematic — RESOLVED
**Status**: ✅ RESOLVED via r10-LED-Gen Commit (50b5e02). Alle 10 LEDs
(LED6-LED15) + 10 R_LED6-R_LED15 + PCA9685 (U6) + Decoupling + /OE-Pull-Up
sind im mcp_sheet generiert. PCA-Kanal-Mapping LED0-LED9 → schematic-Refs
LED6-LED15 verdrahtet via Net-Label-Matching (LED6_K..LED15_K).
PCB-Layout-Positionen bleiben User-Schritt (siehe mechanical_coordinates.md
§4a + §5).

### r7-B2: PCA9685 Symbol-Pin-Map verifizieren — RESOLVED
**Status**: ✅ RESOLVED während r10-LED-Gen-Commit (50b5e02). NXP-Datasheet
Rev 4 (16-April-2015) als PDF gefetcht via WebFetch + Read; Pin-Map auf S.6
Table 3 + Fig. 2 verifiziert. Pin-Belegung im generierten
`_pca9685pw_lib_symbol()` matcht 1:1 dem Datasheet (Pins 1-14 links A0-A4 +
LED0-LED7 + VSS; Pins 15-28 rechts LED8-LED15 + ~OE + A5 + EXTCLK + SCL + SDA
+ VDD). Zusätzlich SPEC-Fix in derselben Iteration: EXTCLK (Pin 25) von „NC"
auf „GND" korrigiert (Datasheet S.7 Footnote [2] verlangt GND wenn unused).

### r7.1-B4: USB-C Premium-Upgrade verifizieren
**Status**: 🟠 IMPORTANT (kein PCB-Layout-Blocker, aber vor Produktions-Charge
zu klären). Aktuell: TYPE-C-31-M-12 C165948 Generic, ~5000 Cycles. Ziel:
Premium-Equivalent ≥10000 Cycles, JLC SMT-Assembly-tauglich, in Stock.
**Action**: Sourcing-Pass für JAE DX07S016JJ1, Amphenol 12401x, GCT USB4055
via LCSC-API + JLC-Stock-Check. Wenn Premium-Equivalent gefunden + Footprint-
kompatibel (Drop-in oder kleine Footprint-Anpassung): SPEC §4 update, sonst
C165948 + Soft-Mount-Reinforcement-Plan (Epoxy am Connector-Body).

### r9-B5: Battery-Pouch-vs-Speaker-Cutout-Konflikt
**Status**: 🔴 BLOCKER für PCB-Layout. 9050120-Pouch (50×120 mm) kollidiert
mit linkem Speaker-Cutout (X=10..90, Y=10..50). Drei Lösungswege:
(a) kleinerer Pouch 9050060 (50×60 mm × 14 mm dicker, immer noch 5000 mAh),
(b) Speaker-Cutouts auf andere Achse verlegen (z.B. obere/untere Kante statt
links/rechts), (c) PCB-Größe vergrößern.
**Empfehlung**: (a) — 14 mm Dicke passt noch in 40-mm-Gehäuse.

### r9-B6: USB-C-VBUS-Sense GPIO — RESOLVED durch r12
**Status**: ✅ RESOLVED via r12. GPA7 fix verdrahtet als USB_VBUS_SENSE
(10kΩ Series von VBUS + 100kΩ Pull-Down zu GND, MCP-Pin 5.5V-tolerant).
Siehe SPEC §2.2 + §7.

### r9-B7: Firmware Volume-Clamp bei Battery-Mode — RESOLVED (HW + Generator)
**Status**: ✅ RESOLVED (HW-Pfad + Schematic-Generator). Hardware-Detect-Pfad
ab r12 spec'd UND im KiCad-Schematic verdrahtet (Commit r12-Sense-Gen):
`if read_MCP_GPA7() == HIGH: vol_max=100; else: vol_max=70`. Battery-Low-
Cutoff via GP26/ADC0 (<3.4V Warning, <3.0V Soft-Shutdown). Volle Algorithmus-
Spec in SPEC §2.2 (r12-Sub-Section). Firmware-Implementation als eigener
Commit nach erstem Audio-Build (kein PCB-Blocker mehr).

### r12-B10: 5 neue passive Bauteile + STATUS_LED-Rewire — RESOLVED
**Status**: ✅ RESOLVED. Im Generator vollständig umgesetzt (r12-Sense-Gen Commit):
- NEU: R_BAT_DIV_TOP/BOT (je 100kΩ 0603), C_BAT_FILT (10nF), R_VBUS_SENSE (10kΩ),
  R_VBUS_PD (100kΩ), R_LED_STATUS (390Ω) — alle im pico.kicad_sch + mcp.kicad_sch
- WEG: R19 (820Ω), STATUS_LED-Label am GP26 — bestätigt durch grep-Check 0 hits
- REWIRE: LED1 hängt jetzt an PCA9685 LED10 (Pin 17, rechts) mit
  R_LED_STATUS-Series — STATUS_LED_K Net-Label matched zwischen LED1-Kathode
  und PCA-LED10-Pin
- NEUE NETS: BAT_SENSE (im pico_sheet), USB_VBUS_SENSE (im mcp_sheet),
  BAT_PLUS (Battery → Pico hier-Bridge), VBUS_USBC (Power-Tree → Battery + MCP)

### r12-B11: TPS61089-Package-Decision — RESOLVED via RNR-Refactor
**Status**: ✅ RESOLVED. Option 1 (RNR-Refactor) gewählt für JLC-soviel-wie-
möglich-Strategie. Umfasste:
- `_tps61089_lib_symbol()` komplett neu — 11 Pins (FSW, VCC, FB, COMP, GND, VOUT,
  EN, ILIM, VIN, BOOT, SW) per TI Datasheet SLVSD38C Table 6-1 + Fig. 6-1
- battery_sheet U8-Block komplett rewritten — alle Pin-Connections updated für
  neuen Pinout (VIN+EN auf RIGHT statt LEFT, FB auf LEFT, neue Pins FSW/COMP/VCC/
  BOOT/ILIM verkabelt)
- 6 zusätzliche externe Bauteile im battery_sheet:
  - **C_VCC** 1µF X7R 0603 (C15849 Basic) — interne LDO-Decoupling
  - **R_FSW** 360k 0603 1% (C23146 Preferred) — Fsw ~1.21 MHz (über Audio-Band)
  - **R_ILIM** 20k 0603 1% (C4184 Basic) — Stromlimit ~4A peak Inductor
  - **C_BOOT** 100nF X7R 0603 (C14663 Basic) — high-side gate driver bootstrap
  - **R_COMP** 22k 0603 1% (C31850 Basic) — Type-II loop-comp Series
  - **C_COMP** 1nF X7R 0603 (C1588 Basic) — Type-II loop-comp Series-Cap
- MPN von TPS61089RNSR auf **TPS61089RNR**, LCSC auf **C165129** (Extended,
  ~2.775 pcs Stock, $0.41), Footprint auf
  `Package_DFN_QFN:VQFN-11-1EP_2.6x2.6mm_P0.5mm_EP0.85x1.5mm_HotRod`

**Analyzer-Verify nach Refactor**: 6 Errors (unchanged VM-001 false-positives),
Warnings 22 → 21, Trust 79.8% → 81.4%. Critical-nets sind korrekt verbunden:
- BAT_PLUS: 8 pins inkl. U8.9 (VIN), U8.7 (EN), L1.1
- BOOST_OUT: 5 pins (U8.6 VOUT, R23.1, C_BOOST_OUT, C_BOOST_HF, D3.2 Anode)
- BOOST_FB: 3 pins (U8.3 FB + R23.2 + R24.1 — Divider korrekt verdrahtet)
- U8_SW_NODE: 4 pins (U8.11 SW + L1.2 + C_BOOT.2 + R_FSW.1)
- U8_BOOT_NODE: 2 pins (U8.10 BOOT + C_BOOT.1)

**Side-Bug aufgefangen**: Im ersten Refactor-Pass ging eine R23-Wire vertikal
durch R23+R24-Body und shortete den FB-Divider. Analyzer fing das (BOOST_FB
hatte 6 statt 3 pins inkl. U8.6 VOUT). Fix: Label-only-Connect statt
durchgehende vertikale Wire.

### r12-B12: Wrong-LCSC-Codes Audit — RESOLVED 2026-06-01
**Status**: ✅ RESOLVED. JLC-Pricing-Audit hat 9 weitere falsche LCSC-Codes
aufgedeckt — Generator-LCSCs matchten nicht mit ihren MPNs (würde bei
JLC-SMT-Assembly **falsche Teile** bestücken). Fixes alle via Generator-sed-Pass:

| Ref | MPN | War (falsch) | Korrigiert |
|---|---|---|---|
| R_BAT_DIV_TOP/BOT, R_VBUS_PD | 100kΩ 0603 1% | C22796 (=130Ω) | **C25803** Basic |
| R23 Boost-FB-Top | 200kΩ 0603 1% | C22810 (=15Ω 5%) | **C25811** Basic |
| R24 Boost-FB-Bot | 39kΩ 0603 1% | C25090 (=0402 210Ω) | **C23153** Basic |
| R_LED6-15, R_LED_STATUS | 390Ω 0603 1% | C23289 (=22µF Elko) | **C23151** Basic |
| U7 MCP73831 | MCP73831T-2ACI/OT | C14879 (=nicht-existent) | **C424093** Ext |
| Q1 DMG2305UX-7 | DMG2305UX-7 | C147074 (=2512-R) | **C150470** Ext |
| L1 SWPA6045S2R2MT | 2.2µH 6×6 | C32330 (=nicht-existent) | **C83455** Ext |
| J9 JST PH-2 SMD | S2B-PH-SM4-TB | C146061 (=nicht-existent) | **C295747** Ext |
| LED6-15, LED1 | warm-white 0603 | C72043 (=nicht-existent) | **C965808** XL-1608UWC Ext |

**Begründung der Bug-Klasse**: Mein ursprünglicher Generator hatte LCSCs
"aus dem Kopf" gewählt unter der Annahme dass UNI-ROYAL "0603WAFnnnn"-MPNs
sequentiell mappen zu LCSC-Codes — das stimmt **nicht**. Korrekturen via
jlcsearch-API-Lookup pro MPN-String. Effekt: **3 von 4 Battery-Block-
Spannungsteiler-Widerständen waren völlig falsch** (130Ω statt 100kΩ etc.) —
wäre bei sofortigem JLC-Order zum DOA-Prototyp geworden.

**Side-Effekt**: 4 Resistor-SKUs sind eigentlich Basic-Parts (kein $3 Setup-
Fee) statt Extended → spart $12 bei 5-Boards-Order. Aktualisierter
Extended-SKU-Count: 19 statt 23.

### r13-B1: Passivradiator MPN + T/S-Parameter — RESOLVED (verworfen in r14)
**Status**: ✅ RESOLVED 2026-06-02. PR-Konzept in r14 verworfen weil F0=380 Hz
(PUI-AS04008PS-Datenblatt) keine Reflex-Lösung erlaubt — ein PR ist genauso
ein Reflex-System wie ein Port und kann nicht weit unter F0 abgestimmt
werden. Sourcing entfällt komplett. Volle Begründung CHANGELOG r14.

### r13-B2: Top-Plate Cutout-Position PR — RESOLVED (entfällt mit r14)
**Status**: ✅ RESOLVED 2026-06-02. PR-Cutout entfällt mit r14. Stattdessen
sitzt an derselben X/Y-Position (50, 30) bzw (270, 30) jetzt der **Speaker-
Grille-Cutout** (38 mm Durchmesser, Top-Firing-Mount). Position-Verify
war für r13-PR-Cutout (51 mm) schon OK; 38 mm-Grille hat noch mehr Margin
zu Cells/OLED/Encoders → kein offener Punkt.

### r15-B1: MIDI Out TRS Type A — Schematic + BOM (offen, vor Schematic-Update)
**Status**: 🟠 IMPORTANT. r15-User-Direktive (2026-06-03): MIDI Out als
3.5-mm-Klinke statt USB-MIDI. SPEC + Mechanik + Plan dokumentiert; Schematic-
Eintrag und BOM-Add stehen noch aus.

**Action vor nächster KiCad-Generator-Update**:
- Neuer Net `MIDI_TX` auf Pico GP21 (war VOL_SW)
- Neuer Net `MCP_VOL_SW` auf MCP23017 GPB5 (war GPB5 reserve)
- Neue Bauteile in `audio.kicad_sch` (oder eigenes `midi.kicad_sch`-Sheet?):
  - J9 = PJ-320A (3.5-mm-TRS, **same MPN als J8** → LCSC C431535)
  - R_MIDI_TX = 220 Ω 0603 (z. B. Yageo RC0603FR-07220RL, JLC Basic)
  - R_MIDI_REF = 220 Ω 0603 (gleicher MPN)
- VOL_SW-Signal von Pico GP21 entfernen, MCP GPB5 hinzufügen
- Mechanik: J8 + J9 Edge-Cutouts in Top-Plate/Bottom-Case-CAD bei X=0,
  Y=75 (J9) bzw Y=90 (J8) — Layout-Verify im 3D-Rendering

**Beim Generator-Update** (`generate_kicad_project.py`): MIDI-Block zur
audio_sch-Generierung hinzufügen oder dediziertes midi_sch erstellen.
Root-Sheet ergänzen.

### r14-B-impedance: PUI-AS04008PS 4Ω → 8Ω — RESOLVED 2026-06-02
**Status**: ✅ RESOLVED. Datenblatt-Audit (im Zuge der r14-Akustik-Analyse)
hat ergeben: PUI AS04008PS-4W-WR-R ist **8 Ω**, nicht 4 Ω. Spec sagte fälschlich
4 Ω in Power-Budget + Anmerkungen. In r14 korrigiert: SPEC §10 Power-Budget
mit r14-Anmerkung (Worst-Case-Strom halbiert, F1 + TPS61089 mehr Margin), §9
USB-C-PD-Sektion mit Hinweis dass Volume-Clamp nun noch unkritischer ist,
Datenblatt-URL als Quelle. Tabelle selbst nicht umgeschrieben (enthält noch
Pi-Reihe von vor Step-6-Pi-Removal — separate Reconciliation).

### r7-B3: KiCad-Schematic + Generator-Update für r7+r9 — RESOLVED
**Status**: ✅ RESOLVED via r10-LED-Gen (50b5e02) + r12-Sense+r9-Battery-Gen
(siehe folgender Commit). `generate_kicad_project.py` ist vollständig im Sync
mit allen SPEC-Revisionen r9/r10/r10-LED/r11/r12:
- U6 PCA9685 + 10 SMD-0603 LEDs + Decoupling + /OE-Pull-Up im mcp.kicad_sch
- SW6-10 mit JLC-Standard-Footprint (HX 12x12x7.3TPFT-B, C36498966)
- I²C-Bus geteilt zwischen MCP23017 (0x20) und PCA9685 (0x40) via
  Same-Name-Label-Matching
- NEU r12-Sense: pico_sheet GP26→BAT_SENSE, mcp_sheet GPA7→USB_VBUS_SENSE,
  LED1 auf PCA-Ch 10
- NEU r9-Battery: battery.kicad_sch mit U7 MCP73831 + U8 TPS61089 + Q1
  DMG2305UX + L1 + D3 + J9 + R21-R24 + Caps + LED_CHRG
- 5 neue Library-Symbole (MCP73831, TPS61089, DMG2305UX, Inductor, Schottky)
- root_sheet erweitert um Sheet 7 Battery + Inter-Sheet-Labels für
  VBUS_USBC / BAT_PLUS / +5V_OUT

---

## ✅ Behoben (auf PR #1 HEAD)

### v0.7 — Engineering-Entscheidungen final getroffen + Design/Doc reconciled
- **Choc-V2-Footprint-Fix**: MX→Choc V1V2 Hotswap (gegen kiswitch v2.4 verifiziert)
- **Line-Out/Kopfhörer J8** + Jack-Detect (MCP GPA6) + Firmware `speakers()`
- **I1/I2 final**: 5V/3A-Netzteil als harte Anforderung; **F1 auf 3A/6A** (Littelfuse
  1812L300, C18198349) hochgesetzt — 2A war zu klein für 2.45A Worst-Case
- **I3 final**: Inrush-Peak ist R-limitiert (nicht Cap-limitiert) → Bulk bleibt
  1000µF Alu Low-ESR (EEE-FK1A102P), Polyfuse trippt nicht auf <1ms-Spike.
  Die in r3 notierte Polymer-Cap-Idee (PCV1A102) wurde nie ins Design übernommen
  und ist hiermit verworfen. **Footprint-Bug gefixt** (D10 → CP_Elec_10x10.5).
- **I5 final**: alle TBD-Positionen (OLED-J3, Pi-J2, Pico-U1) in
  `mechanical_coordinates.md` festgelegt
- **N4**: alle Titleblocks auf rev 0.7 vereinheitlicht
- Verbleibend offen: nur noch GUI-Schritte (B0-B3) + der PCB-Layout-Schritt selbst

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

### ✅ B0. kiswitch-Library — ERLEDIGT (ins Repo eingebunden)
- Choc-V2-Hotswap-Footprints sind NICHT KiCad-Standard → daher vendored:
  `kicad/libraries/keyswitch-kicad-library/` (kiswitch v2.4, nur die nötigen
  `.pretty`-Ordner) + registriert in `kicad/fp-lib-table`. Lösen beim Öffnen
  automatisch auf, kein PCM-Install nötig.
- Referenz: `Switch_Keyboard_Hotswap_Kailh:SW_Hotswap_Kailh_Choc_V1V2_1.00u/_2.00u`
  (beide Dateien vorhanden + verifiziert).
- Verbleibend (→ B0c): Pad-Mapping der Hot-Swap-Buchse im Footprint-Editor
  gegen die Symbol-Pins sanity-checken.
- 2u Cells: Choc-Stabilizer (CPG1353, `Mounting_Keyboard_Stabilizer`) als
  separate Mechanik-Platzierung im Layout.

### B0b. J8 Line-Out-Jack-Footprint verifizieren (v0.7)
- `Connector_Audio:Jack_3.5mm_CUI_SJ-3523-SMT_Horizontal` ist ein KiCad-Standard-
  Platzhalter. Gegen die tatsächlich bestellte PJ-320-Buchse (C2884109) Pad-Mapping
  + Switch-Kontakt-Polarität verifizieren. Bei abweichender Detect-Polarität:
  `JACK_DETECT_ACTIVE_HIGH` in firmware/config.py umstellen.

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

### ✅ B4. PAM8403 /SHDN und /MUTE Hardware-Defaults — ERLEDIGT (v0.6.3)
- R_SHDN_PD + R_MUTE_PD (10k Pull-Downs) sind im Schaltplan vorhanden
  → Default LOW = Amp aus + gemuted während Pico-Boot. Firmware zieht in
  korrekter Anti-Pop-Sequenz aktiv HIGH. Nichts mehr offen.

---

## 🟠 IMPORTANT — Vor JLCPCB-Order erledigen

### ✅ Status v0.7 — alle Engineering-Entscheidungen getroffen

Die folgenden Punkte sind jetzt **entschieden und im Design/Doc umgesetzt**
(keine offenen Fragen mehr). Details unten + in SPEC §3 und CHANGELOG v0.7.

| # | Thema | Entscheidung |
|---|---|---|
| I1/I2 | Power-Budget + USB-Versorgung | **5V/3A-Netzteil als harte Anforderung** (Standard-USB-C-Charger). Kein USB-PD-Controller im Prototyp. Worst-Case 2.45A liegt unter 3A. |
| I1 | Polyfuse | **F1 auf 3A-hold / 6A-trip** hochgesetzt (Littelfuse 1812L300, C18198349). War 2A/4A = zu klein. |
| I3 | Inrush 1000µF | **Akzeptiert für Prototyp** — Inrush-Peak ist R-limitiert (Cap-Größe ändert nur Dauer), Polyfuse ist thermisch → trippt nicht auf <1ms-Spike. Bulk bleibt 1000µF (puffert Bass-Transienten = Produktziel). Produktion: Soft-Start-Load-Switch (TPS22810-Klasse). |
| I3 | Bulk-Cap Footprint | **Bug gefixt**: EEE-FK1A102P ist D10×10.2mm → Footprint von CP_Elec_8x6.7 auf CP_Elec_10x10.5 korrigiert. |
| I4 | 4-Layer Stack-Up | **Signal / GND / +5V / Signal** (SPEC §9 aktualisiert). +5V ist die Hochstrom-Schiene. |
| I5 | Mechanische Koordinaten | **Alle Positionen festgelegt** in `mechanical_coordinates.md` (OLED-J3, Pi-J2, Pico-U1 waren TBD → jetzt definiert). |
| I6 | BOM-Split | Vollständig dokumentiert (siehe unten). |
| I7 | UART Net-Naming | **Erledigt** — Nets heißen `PICO_TX_PI_RX` / `PI_TX_PICO_RX` (eindeutig). |

### Detail-Begründungen (Referenz):

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

### ✅ I7. UART Net-Naming disambiguiert — ERLEDIGT
- Nets heißen eindeutig `PICO_TX_PI_RX` (Pico GP0 → Pi GPIO15) und
  `PI_TX_PICO_RX` (Pi GPIO14 → Pico GP1 via R1 1k). Kein perspektivisches
  TX/RX mehr.

---

## 🟢 NICE-TO-HAVE — Quality-Improvements

### ✅ N1. XSMT per MCP-GPIO — ERLEDIGT
- PCM5102A XSMT wird von MCP23017 GPA5 gesteuert (PCM_XSMT-Netz), mit
  R_XSMT_PD Pull-Down (Default stumm). Explizite Pop-Suppression möglich.

### ✅ N2. I²S Series-Resistoren — ERLEDIGT
- R_BCK / R_LRCK / R_DOUT (je 33Ω 0603) sind in Sheet 7 am Pi-Header in
  Serie zu BCK/LRCK/DOUT eingefügt (pin_to_hier_via_r). Signal-Integrity
  über die 320mm-Traces verbessert.

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

### ✅ N4. Titleblock-Rev sync — ERLEDIGT
- Alle 8 Sheet-Titleblocks auf `(rev "0.7")` vereinheitlicht (waren gemischt
  0.6 / 0.6.3).

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
