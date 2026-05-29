# Meine TODO — Field Ambience PCB

Stand: **nach Merge PR #3** (v30-Menü + v0.8 PCB-Reconcile + SC-Reverb-Unify).
Schaltplan ist fertig + validiert, Stückliste mit der SPEC abgeglichen.
Was hier steht, muss **ich selbst** in der KiCad-GUI / am CAD / beim Bestellen
machen — das kann Claude headless nicht erledigen.

Reihenfolge von oben nach unten abarbeiten. **Priorität: Komponenten-
Vollständigkeit (Abschnitt 0) zuerst.**

---

## 0. Komponenten-Vollständigkeit — der wichtigste Check

Stand des Audits (diese Session, headless geprüft):

- [x] **Schaltplan-BOM = SPEC §3/§4 abgeglichen** — C1/C2 (+5V-Rail-HF) ergänzt,
  C_audio_filt (nie verbaut) gestrichen, R_RUN dokumentiert, F1 auf 3A/6A korrigiert.
- [x] **Alle 97 Bauteile haben einen Footprint zugewiesen** (Namen verifiziert).
- [x] **C1 + C2 LCSC-Nummern ergänzt** (JLCPCB-Basic, in den Generator eingetragen):
  - C1 = **C15850** — Samsung `CL21A106KAYNNNE`, 10 µF 25V X5R 0805
  - C2 = **C14663** — Yageo `CC0603KRX7R9BB104`, 100 nF 50V X7R 0603
  - → **Jedes der 97 Bauteile hat jetzt Footprint + LCSC/MPN. Keine Lücke mehr.**
- [x] **Bestehende LCSC-Nummern auf Stock + Preis geprüft** (jlcsearch, 2026-05-29).
  Ergebnis: 22 von 23 Nummern lösen sauber auf, ~$3,62 SMT-Bauteilkosten/Board.
  **Punkte, die DU vor der Bestellung beachten musst:**
  - 🔴 **J8 Line-Out-Jack `C2884109` nicht in der Teile-DB gefunden** (evtl. veraltet).
    Vor Bestellung gegen die echte gekaufte Buchse fixieren. SMD-Kandidat:
    `C431535` (PJ-320D SMD). Hängt eh an der Footprint-Verifikation (Abschnitt 2, B0b).
  - 🟠 **U2 MCP23017 `C506653`: nur 357 Stück Lager** (Extended). Für 1 Prototyp ok,
    aber knapp — vor Bestellung Verfügbarkeit prüfen, ggf. alternativen Lieferanten.
  - 🟡 U3 PCM5102A `C107671` (6,7k) und U4 PAM8403H `C17337` (9k): niedrig-ish,
    für Prototyp ausreichend; für Serie im Auge behalten.
  - ✅ **R1 von Extended → Basic getauscht**: war Yageo `C22548`, jetzt Uni Royal
    `C21190` (0603WAF1001T5E) — gleiche Familie wie alle anderen Rs, spart eine
    Extended-Setup-Gebühr, 15,8M Lager. (im Generator erledigt)
  - Verbleibende Extended-Parts (unvermeidbar, weil ICs/Spezialteile): U2, U3, U4,
    J1 USB-C, D1 USBLC6, D2 TVS, FB1 Ferrit, F1 Sicherung.
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
