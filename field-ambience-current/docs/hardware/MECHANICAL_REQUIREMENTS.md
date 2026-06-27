# Mechanical Requirements — Gehäuse & PCB-Constraints

Diese Datei listet **alle mechanisch-fixierten Bauteile** mit ihren
Constraint-Maßen + Gehäuse-Anforderungen, damit das Case-CAD (3D-Print) und
das PCB-Layout gleichzeitig entstehen können, ohne sich gegenseitig zu blocken.

Adressat: PCB-Designer (Bauteilplatzierung Schritt 7 des PCB-Workflows) +
3D-CAD-Designer (Gehäuse-Modellierung). Beide brauchen die selben Maße.

> **Status:** PROPOSED — die Maße sind aus Datenblättern + bestehenden ADRs
> abgeleitet. **UNVERIFIED**-Markierungen wo eine Datenblatt-Quelle fehlt.
> Bevor die Mechanik final ist, müssen die UNVERIFIED-Items mit dem realen
> Bauteil in der Hand bestätigt werden (Anti-Guess, CLAUDE.md).

---

## 0. Globale Constraints

| Constraint | Wert | Quelle |
|---|---|---|
| Gerät handheld, 1-Hand-Bedienung | ja | PROJECT_STATUS §2 |
| Max. PCB-Außenmaße | **TBD — User entscheidet** | hängt am Gehäuse-Form-Faktor |
| Vorgeschlagen: ~120 × 80 mm (Stack-Mate-OP-1-Klasse) | Vorschlag | basiert auf 4 Encoder + 5 Cells + Display + Speakers passen müssen |
| PCB-Dicke | 1,6 mm | ADR-0018 (JLCPCB Default 4-Layer) |
| Layer-Stack | 4-Layer | ADR-0018 |
| Mounting (Schraube) | M3 × 4 Eckbohrungen, Ø 3,2 mm Drill | Standard, Gehäuse muss Pillars/Standoffs liefern |
| Gehäuse-Material | 3D-Print PLA/PETG (Single-Color OP-1-Look) | Cost-Trade-Off, kein Alu-CNC (Cost) |
| Bottom-Plate abnehmbar | ja, für Service-Bohrungen `SW11` (Reset) + `SW_BOOT` (DFU) + Akku-JST-Zugang | ADR-0008/Service-Strategie |

---

## 1. Mechanisch-fixierte Bauteile (must-place-first)

Diese Bauteile **müssen** vor allen anderen platziert sein — sie sind durch das
Gehäuse oder die Bedienflächen positionell fest.

### 1.1 Display `LCD-Modul` (Waveshare 1.9″ 170×320 ST7789V2)

| Maß | Wert | Quelle |
|---|---|---|
| Modul-PCB-Außenmaße (LxBxH) | ≈ 47,6 × 23,4 × 5,2 mm | **UNVERIFIED** — Waveshare-Doku nochmal prüfen |
| Active Area (LxB) | ≈ 39,1 × 21,2 mm | Waveshare 1.9″-Spec |
| 8-Pin-Header-Position | Lange Seite, Pin-Pitch 2,54 mm | siehe `J3` Pinout im Walkthrough §6 |
| Pin-Reihenfolge (gegen J3) | **UNVERIFIED — gegen geliefertes Modul prüfen** vor finaler Layout-Bestätigung | r18.22 Note |
| Gehäuse-Fenster-Cutout | Active Area + ≤ 1 mm Toleranz pro Seite (LxB ≈ 41 × 23 mm) | Standard |
| Tiefe vom PCB | 5,2 mm Modul-Höhe + ~3 mm J3-Pin-Header → **min. 8 mm Vertical-Clearance** zwischen PCB-Top und Gehäuse-Front-Innenseite | Stack-Up |
| **Display für diesen PCB-Rev EINGEFROREN auf 1.9″** (r18.64, User „1,9 zoll reicht safe") | verwende die 1.9″-Maße oben | nicht layout-blockierend |
| **ADR-0015 PROPOSED:** Pivot auf 2.0″ 240×320 → **Rev-B** | Maße noch zu verifizieren | BOM UNVERIFIED, kein Blocker mehr |

### 1.2 USB-C Connector `J1` (TYPE-C-31-M-12)

| Maß | Wert | Quelle |
|---|---|---|
| Body (LxBxH) | ≈ 7,3 × 8,9 × 3,2 mm | Datasheet |
| Lage | **An Gehäuse-Kante** (kürzeste USB-Buchse mit D+/D- Spec USB 2.0) | Standard-Constraint |
| Cutout im Gehäuse | 9 × 4 mm rechteckig (Schienen-Toleranz +0,2 mm) | herstellertypisch |
| PCB-Edge-Abstand vom Connector-Anschlag | 0 mm — Buchse muss bündig oder leicht aus dem Gehäuse heraus | Steckbarkeit |
| Footprint | `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12` | BOM §3 |
| Cycles-Klasse | ~5k Insertion-Cycles | r18.19 (akzeptiert) |

### 1.3 3.5 mm Audio-Jack `J8` (PJ-320D, mit Insertion-Detect)

| Maß | Wert | Quelle |
|---|---|---|
| Body (LxBxH) | ≈ 14,5 × 6,1 × 5,1 mm | EasyEDA-CAD vendored r18.19 |
| Lage | An Gehäuse-Kante (gegenüber oder seitlich von USB-C) | Bedienbarkeit |
| Cutout | Ø 6,5 mm Kreisbohrung + 14 mm Body-Schlitz | Standard 3,5 mm Klinke |
| Footprint | `field_ambience:Jack_3.5mm_PJ-320D_SMT` (Custom) | BOM §4 |
| Insertion-Detect-Pin → MCP23017 GPIO | für Auto-Mute der Speaker beim Einstecken | ADR-0007 |

### 1.4 Encoder × 4 `EN1..EN4` (ALPS EC11E18244AU)

| Maß | Wert | Quelle |
|---|---|---|
| Body (BxHxT) | 12 × 12 × 7,5 mm (Body, ohne Schaft) | ALPS-Datasheet |
| Schaftlänge | 20 mm (Flat-Shaft, "20 mm" Suffix B4) | ALPS-Datasheet |
| Cutout im Gehäuse (Top-Plate) | Ø 7,3 mm pro Encoder (Standard) | herstellertypisch |
| Knopf-Höhe über Top-Plate | ~10 mm (3D-gedruckte Knöpfe Ø 19–20 mm × 8–10 mm) | BOM §6 r18.21 |
| Pitch zwischen Encodern | ≥ 22 mm Center-to-Center (Daumen passt zwischen 2 Knöpfen) | ergonomisch |
| Anordnung 4× nebeneinander | Bündel oben oder seitlich, je nach Gehäuse-Konzept | User-Entscheidung |
| Mounting | Through-Hole, 5,1 mm hohe Lötseite — PCB-Underside-Clearance | Datasheet |

### 1.5 Cell-Switches × 5 `Cell 1..5` (Gateron Low Profile Magnetic Jade)

| Maß | Wert | Quelle |
|---|---|---|
| Switch-Plate-Cutout | 14 × 14 mm pro Switch | BOM §7 (Gateron-Standard MX-Footprint) |
| Switch-Höhe über Plate | ~6,5 mm | Gateron-LP-Spec |
| Magnet-Stem-Hub | 0,1 – 3,3 mm analog | Gateron-LP-Magnetic |
| Hall-Sensor-Position | DRV5056A4 (SOT-23) **direkt unter dem Magnet-Stem auf PCB-Unterseite** (oder Top, je nach Stack-Up) | ADR-0013 + Generator-Kommentar Z. 3350 |
| Pitch zwischen Cells | 19 mm (Standard-MX-Spacing) | Gateron-LP |
| Plate-Mounted, NICHT PCB-Mounted | bedeutet: Cells sitzen auf einer separaten Plate über dem PCB; Hall sieht den Magnet durch's PCB | ADR-0013 |
| Plate-Material | 3D-gedruckte Plate ODER 1,5 mm FR-4 Plate (Stretch-Goal) | TBD |

### 1.6 Modifier-Buttons × 5 (Mini-Tact, oben)

| Maß | Wert | Quelle |
|---|---|---|
| Switch-Body | XUNPU TS-1088-AR02016, ~3,9 × 2,9 × 2,0 mm | BOM §7 |
| Cap-Höhe (3D-Print über Top-Plate) | ~3 mm | User-CAD |
| Cutout im Gehäuse | Ø 3,5 mm pro Button | Standard |
| Pitch | ≥ 6 mm — Finger trifft 1 Button auf einmal | ergonomisch |
| Lage | Top-Plate, oberhalb oder neben den Cells | User-Entscheidung |
| LED-Anordnung daneben | jede Modifier-LED links/rechts des zugeordneten Buttons | siehe Walkthrough §4 |

### 1.7 Speaker × 2 `J7` (PUI AS04008PS 8 Ω 40 mm)

| Maß | Wert | Quelle |
|---|---|---|
| Driver-Durchmesser | 40 mm | PUI-Datasheet |
| Driver-Tiefe | ~5 mm | PUI-Datasheet |
| Gehäuse-Schallaustritt (Grill-Pattern) | Ø 25–35 mm offener Bereich + Dust-Mesh (ADR-0007) | ADR-0007 |
| **Akustische Kammer pro Speaker** | **min. ~5 cm³ luftdicht** hinter jedem Driver, kritisch für "nicht Plastik-Spielzeug"-Sound | Walkthrough §3 |
| Pitch zwischen Speakern | Stereo-Spreizung, ≥ 50 mm Center-to-Center | psycho-akustisch |
| Anschluss | 2-Pin-Header pro Speaker (BTL-Pärchen) | BOM §4 |

### 1.8 LiPo-Akku 2000 mAh `LiPo-Pouch`

| Maß | Wert | Quelle |
|---|---|---|
| Pouch-Maße (LxBxH) | 50 × 37 × 9,4 mm | r18.21 Spec |
| Lage | **Bottom-Case-Slot**, NICHT auf der PCB | BOM §3 |
| JST-PH-Stecker `J_BAT` | im Inneren erreichbar (für Lagerung-Trennung, ADR-0016) | ADR-0016 |
| Slot-Maße (Gehäuse) | 52 × 39 × 10 mm minimum (Akku + Kabel-Knick-Reserve) | Pouch + 5 % |

### 1.9 Service-Buchten (Bottom-Plate)

| Element | Loch-Position | Bedeutung |
|---|---|---|
| `SW11` Reset | kleines Loch in Bottom-Plate (Ø ~2 mm), Büroklammer-Zugang | manueller MCU-Reset |
| `SW_BOOT` BOOT0 | gleiches Pattern, neben `SW11` | USB-DFU-Flash (Long-Press = Sleep ab ADR-0016) |
| LiPo-JST-Zugang | Slot oder herausnehmbares Bottom-Plate-Stück | Lagerungs-Trennung |
| Status-LED-Sichtbarkeit (`STATUS_LED`, ein PCB-LED am MCU) | optional Lichtleiter ins Bottom-Plate | Power-Indicator |

---

## 2. Layout-Order-Vorgabe (für PCB-Designer)

In dieser Reihenfolge platzieren. *Erst diese Liste durch, dann kommt die
restliche elektrische Platzierung* (ADR-0018 Routing-Regeln).

1. **Display `J3`** — Top-Center oder Top-Edge je nach Gehäuse-Konzept
2. **USB-C `J1`** — Kante (links oder rechts)
3. **3.5 mm Jack `J8`** — andere Kante als `J1`, weg von Switching-Reglern
4. **5 Cells (Switch-Plate-Cutouts + Hall-Sensoren drunter)** — Mittelbereich,
   Standard-MX-Pitch 19 mm
5. **5 Modifier-Buttons + 5 Modifier-LEDs** — Top-Plate-Pattern
6. **4 Encoder** — Top-Plate-Reihe
7. **Speaker-Header × 2** — Flanken, Schallaustritt-Gehäuse-Öffnungen drüber
8. **LiPo-JST `J_BAT`** — Innen, neben Bottom-Plate-Zugang
9. **Service-Buttons `SW11` + `SW_BOOT`** — Bottom-Plate-Side
10. **Mounting-Bohrungen × 4** — Eckpunkte, M3 × 3,2 mm

---

## 3. Was beim Gehäuse-CAD beachten

- **Speaker-Kammern** = der mit Abstand wichtigste Punkt fürs Klangerlebnis
  (Walkthrough §3). Plane-CAD-Wände um jeden Driver, luftdicht (max 5 cm³).
- **Encoder-Schaft** kommt ~10 mm über die Top-Plate raus → die Knöpfe (eigene
  3D-Prints, Ø 19–20 mm × 8–10 mm) sitzen darauf.
- **Display-Fenster** = Active-Area + 0,5 mm Toleranz pro Seite. Bezel-Look
  optional, aber Cutout MUSS Active-Area komplett zeigen — ein abgeschnittener
  Pixel-Rand ist hässlich.
- **Cell-Plate** = entweder eine 2. PCB als Plate (FR-4 1,5 mm, 14 × 14 mm
  Cutouts) oder direkt in den Gehäuse-Top-3D-Print gefräst.
- **Bezel + Front-Symmetrie**: User-Geschmack, aber Encoder-Knöpfe links neben
  Display, Cells unter Display sind ergonomisch ok.

---

## 4. UNVERIFIED-Liste (zu klären vor PCB-Bestellung)

1. **PCB-Außenmaße** — User-Entscheidung treffen
2. **LCD-Modul-Pin-Order** (Waveshare-Vendor-Variation) gegen tatsächlich
   geliefertes Modul (PCB_FOOTPRINT_RISK_AUDIT §6)
3. **2.0″-Display-Pivot-Maße** falls ADR-0015 D1 angenommen wird
4. **Encoder-Pitch + Knopf-Design** — abhängig vom Layout-Konzept
5. **Akustik-Kammer-Volumen** — empirisch im 3D-Druck-Prototyp ermitteln,
   nicht aus Datasheet (Driver-Performance hängt vom konkreten Plastikteil ab)

---

## 5. Related

- ADR-0011 — Enclosure-Thickness
- ADR-0013 — Cell-Magnetic-Hall (Position-Constraint Hall ↔ Magnet)
- ADR-0015 — 2.0″-Display-Pivot (PROPOSED, betrifft Position-Maße)
- ADR-0016 — Power & Sleep (Service-Buchten + JST-Zugang)
- ADR-0018 — PCB Layer-Stack (4-Layer, 1,6 mm)
- SCHEMATIC_WALKTHROUGH.md — Bauteile-Liste + Refdesigns + Funktion
- `PCB_FOOTPRINT_RISK_AUDIT.md` — alle MECH_CRITICAL-Footprints + offene
  Verifikationen
- KICAD_BLUEPRINT.md — Layout-Workflow-Anleitung
