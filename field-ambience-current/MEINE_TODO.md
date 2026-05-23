# Meine TODO — Field Ambience PCB

Stand: v0.7 (2026-05-22). Schaltplan ist fertig + validiert (0 funktionale
Fehler). Alle Engineering-Entscheidungen sind getroffen. Was hier steht, muss
**ich selbst** in der KiCad-GUI / am CAD / beim Bestellen machen — das kann
Claude headless nicht erledigen.

Reihenfolge von oben nach unten abarbeiten.

---

## 1. KiCad vorbereiten (einmalig, ~5 Min)

- [x] ~~kiswitch-Library installieren~~ — **erledigt**: die Choc-V2-Footprints
  sind ins Repo eingebunden (`kicad/libraries/`) + in `kicad/fp-lib-table`
  registriert. Sie lösen beim Öffnen automatisch auf, kein PCM-Schritt nötig.
- [ ] Projekt öffnen: `kicad/field_ambience.kicad_pro`
- [ ] Prüfen dass alle Sheets ohne Fehler laden (7 Sheets)

---

## 2. Footprints prüfen (GUI, ~30 Min) — Blocker B0-B2

Für jedes Bauteil: Footprint-Editor öffnen, Pad-Nummern gegen Datenblatt-Pinout
vergleichen. (Claude hat nur die Footprint-*Namen* verifiziert, nicht das
Pad-Mapping — dafür braucht es die installierten Libs.)

- [ ] **Choc-Hotswap-Sockets** (SW1-10): Pad-Mapping der kiswitch-Buchse gegen
  Switch-Pins. Footprint = `SW_Hotswap_Kailh_Choc_V1V2_1.00u` / `_2.00u`
- [ ] **J8 Line-Out-Jack**: echte PJ-320 (C2884109) gegen Platzhalter-Footprint
  `Connector_Audio:Jack_3.5mm_CUI_SJ-3523-SMT_Horizontal`.
  **Wichtig**: Detect-Switch-Polarität prüfen — falls invertiert,
  `JACK_DETECT_ACTIVE_HIGH = False` in `firmware/config.py` setzen.
- [ ] **U4 PAM8403H** SOIC-16: Pad 1..16 gegen Datenblatt (PDF liegt im Repo)
- [ ] **J1 USB-C**: Pad A1→GND, A4→VBUS etc. gegen USB-Type-C-Spec

---

## 3. ERC in der GUI laufen lassen (~10 Min) — Blocker B3

- [ ] KiCad → Tools → Electrical Rules Checker → Run
- [ ] Report exportieren nach `reports/ERC_2026-MM-DD.txt`
- [ ] Alle Errors fixen; Warnings entweder fixen oder als "acceptable" notieren
  (Die 3 VM-001-Meldungen aus Claudes Analyzer sind False-Positives —
  KiCads eigener ERC ist die maßgebliche Instanz.)

---

## 4. Mechanik / CAD (~halber Tag)

Positionen sind in `mechanical_coordinates.md` festgelegt. Hier nur noch
ausführen/bestätigen:

- [ ] CAD-Modell (FreeCAD/Fusion) mit Gehäuse + allen Komponenten bauen
- [ ] Component-Body-Heights gegen Gehäuse-Innenraum (~25mm) validieren
- [ ] Board-Outline als DXF exportieren (mit Speaker-Cutouts + 6 Mounting-Holes)
- [ ] Front-Plate-Cutouts dimensionieren (USB-C, OLED, Cells, Modifier, Encoder)

---

## 5. PCB-Layout (der große Brocken, ~1-2 Tage)

Es gibt noch **kein `.kicad_pcb`** — das ist reine Handarbeit in der GUI.

- [ ] DXF als Edge.Cuts-Layer importieren
- [ ] Bauteile auf die Koordinaten aus `mechanical_coordinates.md` platzieren
- [ ] 4-Layer-Stack-Up setzen: **Signal / GND / +5V / Signal**
- [ ] Routen. GND-Plane NICHT zerschneiden unter USB, I²S, Audio
- [ ] **Produktions-Verbesserung optional**: Soft-Start-Load-Switch
  (TPS22810-Klasse) am +5V-Eingang gegen Inrush — für Prototyp nicht nötig
- [ ] DRC laufen lassen, Report nach `reports/DRC_2026-MM-DD.txt`

---

## 6. BOM / Bestellung

- [ ] **Sourcing-Reste klären** (reine Beschaffung, keine Design-Entscheidung):
  - C_BULK EEE-FK1A102P: exakte LCSC-C-Nr. beim Bestellen bestätigen (extended part)
  - BOOTSEL-Switch-Caps (5×)
  - Custom-Cell-Caps (Silikon, MX-Stem)
  - Gehäuse-Schrauben / Schraubdome
- [ ] **Selbst beschaffen** (nicht JLC-SMT): Pico-2-Modul, 10× Choc-Hotswap-Sockets,
  5× Choc-Stabilizer, 2× Speaker, 4× EC11-Encoder, OLED-Modul, Pi Zero 2 W
- [ ] JLCPCB-BOM + CPL exportieren (siehe `bom`-Workflow)
- [ ] **Netzteil**: dediziertes 5V/3A-USB-C-Netzteil besorgen (NICHT PC-USB-Port!)

---

## Referenz / Hintergrund

- Entscheidungs-Begründungen: `CHANGELOG.md` (v0.7)
- Vollständige Spec: `field_ambience_pcb_SPEC_v0.6.md`
- Alle erledigten/offenen Punkte mit Status: `PCB_TODO.md`
- Mechanische Koordinaten: `mechanical_coordinates.md`
