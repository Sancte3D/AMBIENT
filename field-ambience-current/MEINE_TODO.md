# Meine TODO — Field Ambience PCB

Stand: **nach Merge PR #3** (v30-Menü + v0.8 PCB-Reconcile + SC-Reverb-Unify).
Schaltplan ist fertig + validiert, Stückliste mit der SPEC abgeglichen.
Was hier steht, muss **ich selbst** in der KiCad-GUI / am CAD / beim Bestellen
machen — das kann Claude headless nicht erledigen.

> 🆕 **r7 (2026-05-31)**: Modifier-Switches SW6-10 umgestellt auf
> **12×12×7.3 mm momentary tactile mit integrierter LED** (AliExpress) +
> **PCA9685 PWM-LED-Driver** (LCSC C2678753, JLC Extended) für LED-Status-
> anzeige. Custom-Footprint nötig — vor PCB-Layout:
> 1. 5× Switches bei AliExpress bestellen („Momentary Touch LED 12×12×7.3 mm")
> 2. Pin-Pitch mit Messschieber auf 0.1 mm vermessen (SPEC nimmt 6.5×4.5 mm an)
> 3. Custom-Footprint in `kicad/libraries/field_ambience.pretty/` finalisieren
> 4. Im KiCad-GUI: U6 PCA9685 hinzu (`Driver_LED:PCA9685PW` + TSSOP-28-Footprint),
>    LED6-10 (`Device:LED` THT 3 mm), R_LED6-10 (390 Ω), R_OE (10 kΩ),
>    C_PCA_VDD (10 µF), C_PCA_VDD_HF (100 nF).
> 5. I²C1 SDA/SCL Net zum PCA9685 routen (Adresse 0x40).
>
> Siehe `field_ambience_pcb_SPEC_v0.6.md` §7.2 und §4 für Details,
> `mechanical_coordinates.md` §5 für Pad-Geometrie.

> 🆕 **r7.1 (2026-05-31)** — USB-C-Premium-Upgrade (PCB-Mechanik):
> Für die Produktions-Charge JAE DX07S016JJ1 oder Amphenol-Equivalent prüfen
> (≥10000 Cycles vs. ~5000 beim Generic). Sourcing-Pass + JLC-Stock-Verify
> nötig. Prototyp läuft mit dem aktuellen C165948.

Reihenfolge von oben nach unten abarbeiten. **Priorität: Komponenten-
Vollständigkeit (Abschnitt 0) zuerst.**

---

## 0. Komponenten-Vollständigkeit — der wichtigste Check

> 🆕 **v0.9 (Pi-frei):** Der Raspberry Pi ist aus dem Schaltplan raus — die
> Audio-Engine läuft jetzt nativ auf dem RP2350 (siehe `NATIVE_PORT_PLAN.md`).
> Damit sind **J2, R1, R_BCK, R_LRCK, R_DOUT entfernt** → reale Bauteilzahl
> **97 → 92**. GP0/GP1/GP4 sind jetzt I²S zum PCM5102A. D2 (TVS) bleibt.
> Die Punkte unten beziehen sich auf den v0.8-Stand; sie gelten weiter,
> nur ohne die 5 Pi-Teile.

Stand des Audits (diese Session, headless geprüft):

- [x] **Schaltplan-BOM = SPEC §3/§4 abgeglichen** — C1/C2 (+5V-Rail-HF) ergänzt,
  C_audio_filt (nie verbaut) gestrichen, R_RUN dokumentiert, F1 auf 3A/6A korrigiert.
- [x] **Alle Bauteile haben einen Footprint zugewiesen** (Namen verifiziert; v0.9: 92 real).
- [x] **C1 + C2 LCSC-Nummern ergänzt** (JLCPCB-Basic, in den Generator eingetragen):
  - C1 = **C15850** — Samsung `CL21A106KAYNNNE`, 10 µF 25V X5R 0805
  - C2 = **C14663** — Yageo `CC0603KRX7R9BB104`, 100 nF 50V X7R 0603
  - → **Jedes der 97 Bauteile hat jetzt Footprint + LCSC/MPN. Keine Lücke mehr.**
- [x] **JLC-Bestellbarkeit jedes Teils verifiziert** (jlcsearch, 2026-05-29): für
  jede LCSC-Nummer geprüft, ob sie real existiert, lagernd ist UND zum gewollten
  Wert/Package passt. Dabei **7 echte Fehler gefunden und korrigiert** — die
  hätten ein falsch bestücktes Board produziert:
  - 🔴 **R_VOL_L/R**: LCSC zeigte auf **22 Ω 0402** statt 20 kΩ 0603 (falscher Wert
    *und* Package — hätte die Amp-Verstärkung zerstört). → **C4184** (echte 20 kΩ 0603, Basic).
  - 🔴 **R_LO_L/R**: zeigte auf **220 Ω** statt 22 Ω (10×). → **C23345** (echte 22 Ω, Basic).
  - 🔴 **R_BCK/R_LRCK/R_DOUT**: zeigte auf **330 Ω** statt 33 Ω (10×, I²S-Serien-R).
    → **C23140** (echte 33 Ω, Basic).
  - 🔴 **J8-Jack `C2884109` existierte nicht** → **C431535** (PJ-320D SMD, lagernd).
  - 🔴 **C_BULK** hatte nur Platzhalter-LCSC → **C46550395** (1000 µF 16V SMD, D10×10.5).
  - 🔴 **LED1** hatte keine LCSC → **C965818** (weiß 0805, lagernd).
  - 🟡 R1 Extended→Basic getauscht (C22548 → **C21190**), spart eine Setup-Gebühr.
  - ✅ Außerdem alle MPN-Texte an die echten LCSC-Teile angeglichen (waren Yageo-
    Nummern, JLC liefert Uni-Royal/Samsung — gleicher Wert; jetzt konsistent).
  - **Ergebnis: 25 distinkte LCSC-Teile, alle lagernd, 0 Wert-/Package-Fehler,
    14 Basic / 11 Extended.** SMT-Bauteilkosten ~$3,7/Board.
  **Verbleibende Punkte VOR Bestellung (kein Blocker, nur beobachten):**
  - 🟠 **U2 MCP23017 `C506653`: nur 357 Stück Lager**. Für 1 Prototyp ok, vor
    Bestellung Verfügbarkeit prüfen.
  - 🟡 U3 PCM5102A (6,7k) / U4 PAM8403H (9k): niedrig-ish, für Proto ausreichend.
  - 🟠 **J8 PJ-320D**: LCSC jetzt korrekt, aber Footprint im GUI gegen PJ-320D-Pads
    verifizieren (Abschnitt 2, B0b) — das war eh schon offen.
  - Unvermeidbare Extended-Parts (ICs/Spezialteile): U2, U3, U4, J1 USB-C, D1, D2,
    FB1, F1, C_BULK, J8, LED1.
  - **Echte Selbstbeschaffung** (JLC kann/stockt das nicht — by design): Pico-Modul,
    OLED-Modul, 2× Speaker, 10× Choc-Hotswap-Sockets, 4× EC11-Encoder, Pi-/SWD-/
    Speaker-Header. Die haben absichtlich keine LCSC-Nummer.
- [ ] **C_BULK** exakte LCSC-Nr. (EEE-FK1A102P, extended part) beim Bestellen fixieren.
- [ ] **TBD-Teile sourcen** (rein Beschaffung): BOOTSEL-Switch-Caps (5×),
  Custom-Cell-Caps (Silikon, MX-Stem), Gehäuse-Schrauben/Schraubdome.
- [ ] **Eigenbeschaffung bestätigen** (nicht JLC-SMT, „du lieferst"):
  Pico-2-Modul, 10× Choc-Hotswap-Sockets, 5× Choc-Stabilizer, 2× Speaker
  (PUI AS04008PS), 4× EC11-Encoder, OLED ER-OLEDM032-1W, Pi Zero 2 W.

> ⚠️ Footprint **zugewiesen** ≠ Footprint **Pad-Mapping verifiziert**. Letzteres
> ist Abschnitt 2 (GUI-Pflicht vor Layout).

---

## 1. KiCad vorbereiten (einmalig, ~5 Min)

- [x] ~~kiswitch-Library~~ — erledigt: Choc-V2-Footprints sind im Repo
  (`kicad/libraries/`) + in `kicad/fp-lib-table` registriert. Auto-Resolve beim Öffnen.
- [ ] Projekt öffnen: `kicad/field_ambience.kicad_pro`
- [ ] Prüfen dass alle 7 Sheets ohne Fehler laden

---

## 2. Footprint-Pad-Mapping prüfen (GUI, ~30 Min) — Blocker B0c/B1/B2

Für jedes kritische Bauteil: Footprint-Editor öffnen, Pad-Nummern gegen
Datenblatt-Pinout vergleichen. (Claude hat nur die Footprint-*Namen* verifiziert.)

- [ ] **Choc-Hotswap-Sockets** (SW1-10): Pad-Mapping der kiswitch-Buchse
  (`SW_Hotswap_Kailh_Choc_V1V2_1.00u` / `_2.00u`) gegen Switch-Pins.
- [ ] **J8 Line-Out-Jack**: echte PJ-320 (C2884109) gegen Platzhalter-Footprint
  `Connector_Audio:Jack_3.5mm_CUI_SJ-3523-SMT_Horizontal`.
  **Detect-Switch-Polarität prüfen** — falls invertiert,
  `JACK_DETECT_ACTIVE_HIGH = False` in `firmware/config.py` setzen.
- [ ] **U4 PAM8403H** SOIC-16: Pad 1..16 gegen Datenblatt (PDF im Repo).
- [ ] **J1 USB-C**: Pad A1→GND, A4→VBUS etc. gegen USB-Type-C-Spec.

---

## 3. ERC in der GUI laufen lassen (~10 Min) — Blocker B3

- [ ] KiCad → Tools → Electrical Rules Checker → Run
- [ ] **C1/C2-Netze gegenprüfen** (neu diese Session): beide sitzen auf der
  +5V-Rail (Tap x=80/86) gegen GND — auf Dangling-/PowerFlag-Warnungen achten.
- [ ] Report nach `reports/ERC_2026-MM-DD.txt` exportieren.
- [ ] Alle Errors fixen; Warnings fixen oder als „acceptable" notieren.
  (Die 3 VM-001-Meldungen aus Claudes Analyzer sind False-Positives.)

---

## 4. Mechanik / CAD (~halber Tag)

Positionen liegen in `mechanical_coordinates.md`. Hier nur ausführen/bestätigen:

- [ ] CAD-Modell (FreeCAD/Fusion) mit Gehäuse + allen Komponenten bauen
- [ ] Component-Body-Heights gegen Gehäuse-Innenraum (~25 mm) validieren
- [ ] Board-Outline als DXF exportieren (Speaker-Cutouts + 6 Mounting-Holes)
- [ ] Front-Plate-Cutouts dimensionieren (USB-C, OLED, Cells, Modifier, Encoder)

---

## 5. PCB-Layout (~1-2 Tage) — es gibt noch kein `.kicad_pcb`

- [ ] DXF als Edge.Cuts importieren
- [ ] Bauteile auf die Koordinaten aus `mechanical_coordinates.md` platzieren
- [ ] 4-Layer-Stack-Up: **Signal / GND / +5V / Signal**
- [ ] Routen. GND-Plane NICHT zerschneiden unter USB, I²S, Audio
- [ ] **C1/C2 dicht an die Rail-Einspeisung legen** (HF-Decoupling wirkt nur lokal)
- [ ] Optional Produktion: Soft-Start-Load-Switch (TPS22810-Klasse) gegen Inrush
- [ ] DRC laufen lassen, Report nach `reports/DRC_2026-MM-DD.txt`

---

## 6. BOM / Bestellung

- [ ] Abschnitt-0-Sourcing abgeschlossen? (LCSC-Stock/Preis, C1/C2, TBD-Teile)
- [ ] JLCPCB-BOM + CPL exportieren (siehe `bom`-Workflow)
- [ ] **Netzteil**: dediziertes 5V/3A-USB-C-Netzteil besorgen (NICHT PC-USB-Port!)

---

## 7. Software auf dem Pi verifizieren (kurz, vor erstem Sound)

- [ ] `field_ambience_v29o.scd` einmal in sclang laden — Reverb-Refactor (v0.8)
  wurde headless nur strukturell geprüft, nie kompiliert. Auf Fehler achten.
- [ ] SPACE/MODE/MOOD durchdrehen und hören, dass sich der Hall nicht mehr
  gegenseitig zurücksetzt (das war der gefixte Bug).
- [ ] v30-Menü am Gerät testen: PLAY-Macros scrollen/editieren, SETUP-Wechsel,
  GENERATE-Toggle (PROG/TEMPO erscheinen).

---

## Referenz / Hintergrund

- Entscheidungs-Begründungen: `CHANGELOG.md` (v0.7 + v0.8)
- Vollständige Spec: `field_ambience_pcb_SPEC_v0.6.md`
- Alle erledigten/offenen Punkte mit Status: `PCB_TODO.md`
- Mechanische Koordinaten: `mechanical_coordinates.md`
