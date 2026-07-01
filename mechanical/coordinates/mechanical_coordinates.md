# Field Ambience PCB — Mechanical Coordinates

**Stand: v0.7-r18.16 (2026-06-13).** Single Source of Truth für PCB-Layout
(Phase 6) und Frontpanel-/Enclosure-CAD. Ersetzt vollständig die r18.8-§0-Skizze
und die r9/r10/r14/r15-Pico-Ära-Sektionen.

> **Auflösungs-Konvention.** PCB-Ursprung = Bottom-Left-Corner (0, 0), X nach
> rechts, Y nach oben (KiCad-PCB-Editor-Konvention — anders als Schematic
> Y-DOWN!). Alle Maße in mm, alle Bauteilpositionen sind Body-Center außer
> ausdrücklich anders markiert (Edge-Connectors). Z wird auf der PCB-
> Topside-Oberfläche bei 0 gemessen.

## Geltende Quellen

| Quelle | Was sie festlegt |
|---|---|
| ADR-0011 | Z-Stack-Up, 8 mm Top-Komponenten-Zone, 19.6 mm Außenhöhe ohne Knöpfe |
| ADR-0012 | Encoder = EC11E THT, alle 4 gleiche Höhe, Knopf Ø 19–20 × 8–10 mm |
| ADR-0013 (abgelöst r18.73/74) | Cell-Switches jetzt digital am MCP23017, aber echter Kailh-Choc-Hotswap-Keyswitch (nicht das kleine Modifier-Tactile); Hall/Gateron nur noch dokumentierte Option |
| ADR-0007 | Speaker = Dust-Mesh-Aussparung (Saati Acoustex), kein sichtbares Lochmuster |
| ADR-0008 | LED-XOR (Cell-LEDs gelb/grün, Modifier Shift=grün/Hold=gelb/Drone/Generate/Clear=weiß) |
| `mechanical/3d_models/MANIFEST.md` | Body-Höhen aller Z-/Panel-kritischen Teile (STEP-Modelle) |
| `field_ambience_pcb_SPEC_v0.7.md` §4 | BOM-Status (LCSC-IDs, Footprints) |
| IMG_9713 | Industrial-Design-Bezugsbild |

---

## 1. Gehäuse-Außen + PCB-Outline

| Element | Maß | Notiz |
|---|---|---|
| Gehäuse Außenmaß | **260 × 110 × 21.6 mm** (ohne Encoder-Knöpfe) | Knöpfe addieren ~10 mm → ~32 mm Gesamt. **r18.17b: +2 mm gegenüber 19.6 mm — der reale Speaker-Treiber ist 11.5 mm tief (nicht 9 mm), siehe §7** |
| PCB-Outline | **252 × 102 × 1.6 mm**, 4-Layer FR4 | 4 mm Bezel auf jeder Seite zwischen PCB-Edge und Gehäuse-Außen |
| Edge-Keepout (PCBA-Rail-Bereich) | 3 mm rundherum, „weich" | Für Pick&Place; in Phase 6 final |
| Bauteil-zu-Edge | ≥ 2 mm | DRC-Regel; ausgenommen Edge-Connectors (J1, Audio-Jack) |
| Tilt | 0° | Wie OP-1-Field, kein Kick75-Tilt |
| Frontpanel-Material | ABS oder Polycarbonat-Spritzguss, 2.5 mm | ADR-0011 |
| Bottom-Case-Material | gleiches Material, 2.5 mm | ADR-0011 |

**Warum 260 × 110**: Felder von OP-1-Field (200 × 100) zu groß für unsere
Anordnung mit Cells + Modifier-Reihe + 4 Eckencodern + 2 Speaker-Kammern;
260 × 110 ist die kleinste rechteckige Hülle, in die das IMG_9713-Layout mit
≥ 12 mm Pitch zwischen Cells und ≥ 8 mm Pitch zwischen Modifier-Buttons
unterbringbar ist und in die beide Speaker-Kammern noch das Mindestvolumen
(≥ 15 cm³ pro Seite, sealed) bekommen. Final-Maß = Industrial-Design-Sprint.

## 2. Z-Stack-Up (aus ADR-0011)

```
   ┌────────────────────────────────────────────────┐
8–10mm Encoder-Knopf (Aluminium, außerhalb Gehäuse) │
   ├────────────────────────────────────────────────┤
2.5 mm Top-Panel (ABS/PC)                           │  Aussparungen siehe §5
   ├────────────────────────────────────────────────┤
12  mm Encoder-Schaft (durch Bohrung Ø 7 mm)        │  Innenraum
       Andere Komponenten ≤ 8 mm hoch in dieser Zone│  (siehe §7 Höhen-Zonen)
   ├────────────────────────────────────────────────┤
12  mm Above-PCB-Raum (PCB-top → Panel-innen):      │
        - Encoder-Body 7 mm                         │
        - Cell-Switch + Cap-Stack ~8 mm             │
        - Speaker-Treiber 11.5 mm (hängt v. Panel)  │  ← treibt die 12-mm-Höhe
        - USB-C 3.2 mm, LCD-Modul 3.5 mm            │
        - Polymer-Cap 2.0 mm (ADR-0011)             │
   ├════════════════════════════════════════════════┤
1.6 mm PCB                                          │
   ├────────────────────────────────────────────────┤
 3  mm PCB-Standoff (M2.5 oder integrierter Boss)   │
   ├────────────────────────────────────────────────┤
2.5 mm Bottom-Panel                                 │
   └────────────────────────────────────────────────┘
   Gehäuse-Innenhöhe  16.6 mm
   Gehäuse-Außenhöhe  21.6 mm  (ohne Knöpfe)
   Gesamt mit Knöpfen ~32 mm
```

> **r18.17b-Änderung:** Above-PCB-Raum 10 → **12 mm**, weil der reale
> Cloth-Cone-Mini-Treiber (CMS-402811-28SP primär / PUI AS04008PS sekundär,
> r18.18) ist **11.5 mm tief** (Datenblatt-Korrektur, war
> fälschlich 9 mm angenommen). Bei 10 mm Raum wäre der von der Top-Platte
> hängende Treiber 1.5 mm in die PCB-Ebene kollidiert. 12 mm → 0.5 mm
> Luft. Allgemeine Top-Komponenten bleiben ≤ 8 mm; nur Encoder-Schaft
> (durchstoßend) und Speaker-Treiber nutzen die volle Höhe.
> **Alternative ohne +2 mm Höhe** (falls 19.6 mm zwingend): lokaler
> Ø-24-mm-PCB-Relief-Cutout je Speaker für den Magnet-Boss — verworfen,
> weil ohne Treiber-Mechanik-Zeichnung (Datenblatt-PDF aktuell 503) die
> Magnet-Boss-Geometrie unbekannt ist; +2 mm Höhe ist die robuste,
> geometrie-unabhängige Lösung.

## 3. Top-Side Komponenten (alle Body-Center)

> **Bauteilkürzel:** „Planung" = aus IMG_9713 + Z-/Pitch-Constraints abgeleitet,
> in Phase 6 final fixiert. „Final" = unverhandelbar (Pinout, Bohrungen,
> Edge-Cutouts). Spalte „Höhe" siehe `mechanical/3d_models/MANIFEST.md`.

### 3.1 Encoder-Eckcluster (alle EC11E THT, Push nur EN3, ADR-0012)

| Ref | X (mm) | Y (mm) | Funktion | Knopfdurchmesser | Höhe Schaft+Body |
|---|---|---|---|---|---|
| EN1 | **22** | **88** | Drive (smooth) | Ø 20 mm Alu | 7 mm Body + 12 mm Schaft = 19 mm |
| EN2 | **52** | **88** | Brightness (smooth) | Ø 20 mm Alu | s. o. |
| EN3 | **200** | **88** | Display (push + detent) | Ø 20 mm Alu | s. o. |
| EN4 | **230** | **88** | Volume (smooth) | Ø 20 mm Alu | s. o. |

**Pitch innerhalb der Paare**: 30 mm Mitte-zu-Mitte → 10 mm Lücke zwischen
benachbarten Ø-20-Knöpfen, ergonomisch greifbar mit zwei Fingern.
**Pitch Paar-zu-Display**: 28 mm Mitte-Encoder zu Display-Kante.

### 3.2 Display (J3, ST7789 1.9″ 320×170 IPS via 1×8-Header)

| Parameter | Wert |
|---|---|
| Active-Area X-Mitte | **126** mm |
| Active-Area Y-Mitte | **84** mm |
| Active-Area Größe | 40 × 22 mm (Datenblatt Adafruit 5394) |
| Modul-Außenmaß | 50 × 28 mm (über 4 M2-Standoffs) → belegt Y 70…98 (4 mm Bezel zur Top-Edge) |
| J3 X-Mitte | **126** mm |
| J3 Y | **70** mm (unter Modul-Bottom-Edge) |
| J3 Orientierung | Liegender 2.54 mm 1×8-Header, Pins Richtung −Y |
| Standoff-Höhe | 3.5 mm (Modul-Topface bündig mit 7-mm-Encoder-Body-Zone) |

**Frontpanel-Aussparung**: 42 × 24 mm rechteckiger Cutout, mittig auf Active-Area.

### 3.3 Modifier-Reihe (5× HX 12×12, ADR-0008-Farben)

5 identische Buttons, alle momentary, Pitch 14 mm Mitte-zu-Mitte (→ 2 mm Lücke
zwischen 12-mm-Bodies, daumengerecht), Reihe zentriert unter dem Display.

| Ref | X (mm) | Y (mm) | Funktion | Zugeordnete LED (XOR-Farbe) |
|---|---|---|---|---|
| SW6  | **98**  | **58** | Shift    | LED6 (grün) |
| SW7  | **112** | **58** | Hold     | LED7 (gelb) |
| SW8  | **126** | **58** | Drone    | LED8 (weiß) |
| SW9  | **140** | **58** | Generate | LED9 (weiß) |
| SW10 | **154** | **58** | Clear    | LED10 (weiß, flash-on-press) |

**Modifier-LED-Reihe** (LED6–LED10, jeweils 8 mm über zugehörigem Switch):
gleiche X, **Y = 66 mm**, SMD 0603, Light-Pipe Ø 1.5 mm im Frontpanel.
(4 mm Lücke zur Display-Modul-Bottom-Edge bei Y = 70.)

**Frontpanel-Aussparungen**: 12.5 × 12.5 mm pro Switch + Ø-2-mm-Bohrung pro
Modifier-LED.

### 3.4 Cell-Reihe (5× Kailh Choc V1/V2 Hot-Swap, digital — r18.74, ADR-0013 abgelöst)

> **r18.73 → r18.74 (User-UX-Korrektur):** r18.73 hatte die Cells auf dasselbe
> kleine THT-Tactile wie die Modifier gesetzt — fühlte sich dann identisch zu
> einem simplen Modifier-Knopf an, kein "Tastatur"-Gefühl mehr. **r18.74:**
> Cells bleiben elektrisch digital (gleiche Netze GPA0–GPA4), bekommen aber
> einen **echten Kailh-Choc-Keyswitch über Hotswap-Socket** zurück (~3mm
> Hub, klickt von vorne rein, kein Löten des Switches, tauschbar).

5 identische Cell-Sites, **Hotswap-Socket SMD auf dem PCB gelötet** (15×15 mm
Switch-Envelope, Kailh-Choc-Standard) — kein Hall-Sensor mehr, aber (anders als
r18.73) auch keine kleine Tactile-Taste mehr; stattdessen ein echter,
tauschbarer Keyboard-Switch. Plate optional (Choc-Hotswap-Builds sind oft
plateless). X/Y-Raster unverändert (Reihe zentriert unter der Modifier-Reihe).

| Ref | X (mm) | Y (mm) | Cap (Plan) |
|---|---|---|---|
| Cell 1 (SW1) | **82**  | **26** | 3D-Druck-Cap, clippt auf Choc-Stem |
| Cell 2 (SW2) | **104** | **26** | dito |
| Cell 3 (SW3) | **126** | **26** | dito |
| Cell 4 (SW4) | **148** | **26** | dito |
| Cell 5 (SW5) | **170** | **26** | dito |

Footprint = `Switch_Keyboard_Hotswap_Kailh:SW_Hotswap_Kailh_Choc_V1V2_Plated_1.00u`
— vendored (MIT, `keyswitch-kicad-library`), community-verified, akzeptiert
sowohl Choc V1 (CPG1350) als auch V2 (CPG1353). **Der Hotswap-Socket selbst hat
keine saubere Hersteller-/LCSC-Teilenummer** — Keyboard-Markt-Ware (z.B.
Chosfox "Kailh Choc PG1350 Hot Swap Socket" ~$1.45/10 Stk) — vor Bestellung
konkreten Vendor-Listing + Footprint-Maße gegen Kailh-Zeichnung verifizieren.
Der Switch, der reinklickt (nicht gelötet): reales verifiziertes Beispiel Kailh
Choc V1 rot/linear, LCSC **C400229**. V1-Stem (~3,4mm) ≠ V2-Stem (~5mm) — eine
Version konsistent wählen und den 3D-Cap-Stem danach designen.

**Cell-LED-Reihe** (LED11–LED20, 2 LEDs pro Cell, gelb + grün, XOR pro ADR-0008):

| Cell | LED-Y (mm) | LED-X-Pärchen (mm) |
|---|---|---|
| 1 | **40** | 78 (gelb), 86 (grün) |
| 2 | **40** | 100 (gelb), 108 (grün) |
| 3 | **40** | 122 (gelb), 130 (grün) |
| 4 | **40** | 144 (gelb), 152 (grün) |
| 5 | **40** | 166 (gelb), 174 (grün) |

(8 mm Pitch innerhalb des Pärchens; 4 mm Innenabstand am Cell-Center.)

**Frontpanel-Aussparungen**: Per Cell eine Cap-Fensteröffnung passend zur
3D-gedruckten Cell-Cap (Square-Head-Plunger ~7,3 mm hoch; exakte Fenstergröße
folgt aus dem Cap-Design — *UNVERIFIED, beim Cap-CAD final festlegen*) +
Ø-2-mm-Bohrung pro LED, Light-Pipe-Stab Ø 1.5 mm.

### 3.5 USB-C-Stecker J1 (TYPE-C-31-M-17, C283540)

Edge-mounted an der **Bottom-Edge des PCBs** (Y = 0), zentriert.

| Parameter | Wert |
|---|---|
| X-Mitte | **126** mm |
| Y (Connector-Face) | **0** mm |
| Body-Tiefe | 7.35 mm in das PCB (Y-Ausdehnung 0 → 7.35) |
| Höhe über PCB | 3.2 mm (STEP-verifiziert, MANIFEST.md) |
| Gehäuse-Cutout | M-17 Standardprofil im Bottom-Bezel |

### 3.6 Audio-Klinke + MIDI-Klinke (J8, J9 = PJ-320D, C431535)

Beide an der **Left-Edge** des PCBs (X = 0), Top-Side-mounted.

| Ref | X (Body) | Y (Mitte) | Funktion |
|---|---|---|---|
| J8 | **3 mm** in PCB | **88 mm** | Line-Out / Kopfhörer 3,5 mm TRS Stereo, mit Insertion-Detect |
| J9 | **3 mm** in PCB | **66 mm** | MIDI-Out (TRS-A) — DNP bis ADR-0004 entschieden |

Jack-Buchsenfront sitzt **bündig mit PCB-Edge** (X = 0); Body ragt nach +X.
Gehäuse hat seitliche Bohrungen Ø 6 mm in Höhe der Y-Positionen.
Z-Body-Höhe 5.0 mm — passt in 8-mm-Top-Zone.

### 3.7 SWD-Header J4 (TC2030-IDC 3-Pin, Service-Only)

| Parameter | Wert |
|---|---|
| X | **245** mm |
| Y | **15** mm |
| Orientierung | THT, 3 Pin in Linie, kein Cap → kein Frontpanel-Cutout |

### 3.8 Boot-/Reset-Buttons (SW11 Reset + SW_BOOT BOOT0)

Beide Service-Only, **kein** Frontpanel-Cutout — Druck via Pin-Spitze durch
2-mm-Bohrung in der Bottom-Plate.

| Ref | X (mm) | Y (mm) | Footprint |
|---|---|---|---|
| SW11 (Reset) | **240** | **30** | field_ambience:SW_TS1088_SMD |
| SW_BOOT | **245** | **72** | field_ambience:SW_TS1088_SMD |

> r18.17b: SW_BOOT von (245, 45) → **(245, 72)** verschoben — die alte Position
> lag im rechten Speaker-Treiber-Keepout (Y 34…66, §7). Y=72 ist außerhalb
> (2.5 mm zum Keepout-Rand), 4.5 mm unter J_BAT (245, 80). SW11 (240, 30) und
> J4 (245, 15) liegen schon unterhalb des Keepouts.

### 3.9 Audio-/Power-IC-Cluster (Layout-Constraint aus ADR-0010 + §7)

Alle Höhen ≤ 4 mm wenn der Cluster in der Speaker-Treiber-Zone (X 8–48 oder
204–244, Y 22–72) liegt; sonst ≤ 8 mm. **Power-Cluster wandert wegen L1
(4.5 mm hoch) in den linken Mittenbereich, außerhalb der Speaker-Treiber-
Zone.**

**Power-/Audio-/MCU-Insel** liegt komplett im Y-Streifen **34…51 mm** (zwischen
Cell-Body-Top Y=33 und Modifier-Body-Bottom Y=52). Das hält alles aus den
Speaker-Treiber-Zonen heraus (X 8…48 und X 204…244 wären nur ≤ 4 mm hoch, der
Boost-Inductor L1 ist aber 4.5 mm — siehe §7).

| Block | Zone (X, Y) | Höhe | Begründung |
|---|---|---|---|
| PCM5102A | X 60–70, Y 40–46 | 1.2 mm | Kurzer Weg zu J8 (Audio-Klinke links, X=3) und zum PAM8403 |
| PAM8403 Class-D | X 75–95, Y 40–46 | 1.8 mm | Direkt neben C_BULK-Polymer (ADR-0010 „kürzester Polygon-Loop") |
| C_BULK Polymer 470 µF + 220 µF MLCC | X 75–95, Y 46–50 | 2.0 mm | Bass-Transient-Pfad, Layout-Constraint ADR-0010 |
| TPS61089 Boost VQFN-HR | X 100–104, Y 40–43 | 1.0 mm | Power-Eingangsseite |
| L1 Boost-Inductor (4.5 mm) | X 100–108, Y 44–50 | 4.5 mm | Direkt neben TPS61089; **OK** weil hier Y > 33 (außerhalb Cell-Body) und X außerhalb Speaker-Zonen |
| AP7361C-33Y5 LDO | X 110–115, Y 40–43 | 1.5 mm | Nach Boost, vor MCU |
| Y1 Crystal HC-49/US-SMD | X 134–146, Y 39–45 | 4.2 mm | **Rechts neben** STM32, auf gleicher Höhe wie OSC-Pins; AN2867 ≤ 5 mm zum nächsten OSC-Pin (Trace-Routing in Phase 6) |
| STM32H743VIT6 LQFP-100 | X 118–132, Y 35–49 | 1.4 mm | Zentral unter Modifier-Reihe (1 mm Lücke zu SW8); kurzer Routing zu Display (oben) + Cells (unten) + Modifier-Reihe (direkt darüber) |
| MCP23017 | X 153–163, Y 41–47 | 2.0 mm | Nahe Modifier-LEDs (kurze GPIO-Stubs) |
| PCA9685 + 220-Ω-LED-Rs | X 165–187, Y 41–47 | 1.0 mm | Nahe Cell-LED-Reihe |

**Speaker-Lötpunkte J6/J7** (Same Sky CMS-402811-28SP Cloth-Cone primär /
PUI AS04008PS Treated-Paper sekundär, beide Footprint 40 × 28.3 mm, 11.5 mm
tief, **Löt-Eyelets — keine Kabel ab Werk**; Hand-Assembly, kein PCB-Mount,
kein JLC-Bestücken): zwei 2-Pin-Pads je Speaker am Rand des Keepouts
(X ≈ 7 / X ≈ 49 bzw. 203 / 245, Y ≈ 50), maximal flach (≤ 0.5 mm). Draht vom
Pad zum Treiber-Eyelet wird von Hand gelötet. Siehe ADR-0007 für Wahl
Cloth- statt Papier-Konus (r18.18).

### 3.10 Battery JST-PH 2.0 J_BAT (S2B-PH-SM4-TB, C295747)

Top-Side-mounted, **außerhalb** der Speaker-Treiber-Zone (Y > 72), damit der
6-mm-Connector nicht im 5-mm-Höhenlimit unter dem hängenden Speaker-Treiber
endet (§7).

| Parameter | Wert |
|---|---|
| X-Mitte | **245** mm |
| Y-Mitte | **80** mm |
| Höhe | 6 mm (STEP-verifiziert) |
| Battery-Pouch | Liegt **unter** dem PCB im Bottom-Case (siehe §4) |

### 3.11 LCD-Backlight-Driver Q2 (2N7002, SOT-23)

| Parameter | Wert |
|---|---|
| X | **155** mm |
| Y | **70** mm |
| Hinweis | Rechts neben dem Display-Modul-Right-Edge (X=151), neben J3-Receptacle; BLK-Pfad ~5 mm |

---

## 4. Bottom-Side / Battery-Kammer

Bottom-Side ist im Prototyp **leer von SMD-Komponenten** (vereinfacht Reflow
auf nur einer Seite + Hand-Lötung der THT-Teile). Im Bottom-Case unter dem PCB
liegt die LiPo-Pouch-Batterie.

| Element | Lage / Maß | Notiz |
|---|---|---|
| Battery LiPo (5000 mAh) | Pouch 50 × 60 × 9 mm „9050060" | Liegt im rechten Hälfte des Bottom-Case unter PCB |
| Battery-Y-Bereich | Y = 30 … 90 mm | Außerhalb der Speaker-Kammern (§5) |
| Battery-X-Bereich | X = 190 … 250 mm | Unter Boost-/Power-Cluster, nahe J_BAT |
| Battery-Anschluss | 100-mm-JST-PH-Kabel zu J_BAT (§3.10) | |

> Größere Pouches (z. B. 8050120, 5000 mAh in 8 × 50 × 120 mm) passen NICHT
> bei diesem Outline ohne Konflikt mit der linken Speaker-Kammer. Falls
> mehr Kapazität gewünscht: Bottom-Case zusätzlich 2 mm tiefer (→ Außenhöhe
> 21.6 mm) ODER zweiten kleineren Pouch auf der linken Seite.

---

## 5. Frontpanel-Aussparungen (Top-Panel-CAD)

Sammelübersicht aller Cutouts/Bohrungen im 2.5-mm-Top-Panel, X/Y in PCB-
Koordinaten (Top-Panel und PCB sind kongruent angeordnet).

| Element | Geometrie | X-Mitte | Y-Mitte |
|---|---|---|---|
| Encoder-Bohrung EN1 | Ø 7 mm | 22 | 88 |
| Encoder-Bohrung EN2 | Ø 7 mm | 52 | 88 |
| Encoder-Bohrung EN3 | Ø 7 mm | 200 | 88 |
| Encoder-Bohrung EN4 | Ø 7 mm | 230 | 88 |
| Display-Cutout | 42 × 24 mm Rechteck | 126 | 84 |
| Modifier-Cutouts SW6–SW10 | je 12.5 × 12.5 mm | 98, 112, 126, 140, 154 | 58 |
| Modifier-LED-Bohrungen LED6–LED10 | je Ø 2 mm | gleiche X wie SW6–10 | 66 |
| Cell-Cutouts (Switch-Frame) | je 14 × 14 mm | 82, 104, 126, 148, 170 | 26 |
| Cell-LED-Bohrungen LED11–LED20 | je Ø 2 mm | siehe §3.4 | 40 |
| Dust-Mesh-Aussparung links | oval **36 × 24 mm** mit 0.3-mm-Mesh-Inset | 28 | 50 |
| Dust-Mesh-Aussparung rechts | oval **36 × 24 mm** | 224 | 50 |

> **Cutout-Sizing r18.17b:** Der reale Treiber-Footprint ist **40 × 28.3 mm**
> (Datenblatt-Korrektur, war fälschlich 40 × 40). Ein 50 × 30-Cutout wäre
> GRÖSSER als der Treiber → kein Material für den Rahmen-Rim zum Abdichten.
> 36 × 24 mm lässt **2 mm Rim-Seat** rundum (Treiber-Rim 40×28.3 presst gegen
> Panel-innen um die Öffnung). Mesh deckt die Öffnung staubdicht.
> Outline-Check: Top-Panel 260 × 110, PCB 252 × 102 mittig → Panel-X-Bereich
> −4 … 256. Mesh-L X = 28 ± 18 = 10 … 46, Mesh-R X = 224 ± 18 = 206 … 242 —
> beide gut innerhalb Panel + ≥ 10 mm zu jeder Außenkante.

**Speaker-Kammer-Volumen** (geschlossen, sealed): Treiber-Footprint
40 × 28.3 mm, Treiber-Tiefe 11.5 mm. Brutto-Footprint-Volumen ≈ 15 cm³;
durch Kammerwände im Bottom-Case auf ~20–30 cm³ erweiterbar (Wände etwas
weiter als der Treiber-Footprint). Für einen Sealed-F0=450-Hz-Mid-Range-
Treiber unkritisch (ADR-0011/SPEC §8: Tiefbass kommt ohnehin nur über
Line-Out; die Kammer ist nur Roll-Off-Punkt, kein Klang-Charakter).

---

## 6. Mounting Holes (Top-Plate-zu-Bottom-Case-Verschraubung)

4 Mounting-Holes, M2.5, plattiert auf der PCB.

| Hole | X (mm) | Y (mm) | Spec |
|---|---|---|---|
| MH1 | **12**  | **6**   | M2.5, Ø-Bohrung 2.7 mm, Pad Ø 6 mm |
| MH2 | **240** | **6**   | s. o. |
| MH3 | **12**  | **96**  | s. o. |
| MH4 | **240** | **96**  | s. o. |

Holes versetzt zu den Edges (≥ 3 mm zur PCB-Edge), zu den Encoder-Bodies
(Encoder bei X 22/52/200/230, Y 88 — MH3/MH4 ≥ 7 mm horizontaler Abstand zum
nächsten Encoder-Body-Rand) und zur Audio-Klinke (J8 X 0…6, MH1/MH3 bei X=12
→ 6 mm zum J8-Body). Standoff im Bottom-Case 3 mm hoch → trifft den 3-mm-
Standoff-Z-Slot aus §2.

---

## 7. Component-Height-Zones (DRC-Vorgabe für Layout)

| Bereich | Z-Limit über PCB | Quelle |
|---|---|---|
| Globale Top-Zone | ≤ 8 mm | ADR-0011 (above-PCB-Raum 12 mm, 4 mm Reserve) |
| Ausnahme „Encoder-Schaft" | bis 19 mm (durchstoßend) | EC11E-Schaft + Knopfdurchgang |
| **Speaker-Treiber-Footprint L** (X 7…49, Y 34…66, = 42×32 um Treiber 40×28.3) | **Bauteil-Keepout** (nur Traces/Vias, Bauteile ≤ 0.4 mm) | Treiber hängt 11.5 mm vom Panel → nur 0.5 mm über PCB. Direkt darunter keine Bauteile. |
| **Speaker-Treiber-Footprint R** (X 203…245, Y 34…66) | **Bauteil-Keepout** | s. o. |
| Bottom-Side | ≤ 1 mm (Reserve, Prototyp blank) | Vereinfacht Reflow auf eine Seite |

C_BULK-Polymer (470 µF, ~2 mm) erfüllt die 8-mm-Zone (ADR-0011 r18.12); der
alte 10.5-mm-Alu-Elko ist retired.

**r18.17b-Vereinfachung:** Mit dem auf 12 mm angehobenen Above-PCB-Raum (§2)
ist die einzige Speaker-Höhen-Restriktion der **42 × 32 mm Bauteil-Keepout
direkt unter jedem Treiber-Footprint** (nur 0.5 mm Luft zum hängenden
Treiber). Außerhalb dieses Keepouts gilt überall die normale 8-mm-Zone — die
frühere „≤ 4 mm in 40×50-Zone"-Regel (auf der 9-mm-Treiber-Fehlannahme
basierend) entfällt. L1 Boost (4.5 mm) und JST-PH (6 mm) bleiben dennoch
außerhalb der Keepouts platziert (siehe §3.9/§3.10), brauchen aber keine
Sonderbehandlung mehr.

---

## 8. PCB-Edge-Konnektoren (Übersicht)

| Edge | Belegung |
|---|---|
| Top (Y = 102) | nur Speaker-Kammer-Aussparungen im Panel; PCB-Edge frei |
| Bottom (Y = 0) | J1 USB-C zentriert (§3.5) |
| Left (X = 0) | J8 Audio + J9 MIDI vertikal gestapelt (§3.6) |
| Right (X = 252) | frei (Service-Side, SWD-/Boot-Buttons sitzen 5–10 mm einwärts) |

---

## 9. Layout-Reihenfolge für Phase 6 (Empfehlung)

1. **Edge.Cuts**: 252 × 102 mm Rechteck, 4 Mounting-Holes (§6), USB-C-Edge-Cut.
2. **Critical-Path-Placement** (in dieser Reihenfolge platzieren, weil sie die
   maximalen Layout-Constraints erzeugen):
   1. STM32H743 LQFP-100 (§3.9, mittig)
   2. HSE-Crystal + Caps direkt an OSC-Pins
   3. PCM5102A + PAM8403 + C_BULK-Stack (ADR-0010-Constraint)
   4. TPS61089 Boost + L1
   5. AP7361C-33Y5 LDO
3. **Panel-Placement** (Positionen fix aus §3): 4× Encoder, ST7789-Header,
   5× Modifier-Buttons, 5× Cell-Sites, alle LEDs.
4. **Edge-Connectors**: J1 USB-C, J8/J9 Audio.
5. **Service**: SW11, SW_BOOT, J4 SWD, J_BAT.
6. **Routing-Reihenfolge**:
   1. GND-Pour Top + Bottom (Quiet-AGND-Insel unter PCM5102A)
   2. Power-Pours (+3V3, +5V) — separate Layer 3 wenn 4-Layer
   3. Audio-Differential-Pairs (SAI → PCM5102A)
   4. HSE-Loop (kürzest)
   5. ADC-Cell-Leitungen mit Guard-Trace
   6. I²C/I²S, SPI-LCD
   7. GPIO-Stubs
7. **DRC + ERC + 3D-Viewer-Check** gegen `field_ambience.3dshapes/`.

---

## 10. Offene Punkte (Phase 6 / Industrial-Design-Sprint)

| Punkt | Beschluss durch |
|---|---|
| Finale Außenmaße (Gehäuse-Höhe ggf. ±2 mm) | Industrial Design |
| Knopf-Material + exakte Knopf-CAD (Ø 19 oder 20 mm) | Industrial Design |
| Cell-Cap-Profil (Custom-Cap auf HX-B3F-Square-Head) | Industrial Design + Muster-Test |
| HX B3F-4055 THT-Pin-Pattern vs Footprint (Modifier SW6–10) | GUI-ERC gegen Datenblatt prüfen |
| Kailh-Choc-Hotswap-Socket (Cells SW1–5): kein Hersteller-/LCSC-PN gefunden | Konkretes Vendor-Listing + Footprint-Maße gegen Kailh-Zeichnung verifizieren vor Bestellung |
| LP-Stabilizer-Notwendigkeit bei 2u in dieser Pitch | Erfahrungswert nach Muster-Druck |
| Plate-Material (Aluminium vs ABS-Spritzguss) | Mockup-Bau |
| Mesh-Lieferant (Saati Acoustex T-Klasse vs Alternative) | ADR-0007 follow-up |

Alle hier offenen Punkte verzögern Phase 6 NICHT — sie verfeinern die schon
festgelegten Cutout-Maße um max. 1 mm und können in der CAD-Iteration nach
dem ersten DRC-Pass eingearbeitet werden.
