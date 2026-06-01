# Field Ambience PCB — Mechanical Coordinates

PCB-Layout-Constraints für die kommende Layout-Phase (Sheet 2 in KiCad
PCB-Editor). **Status (v0.7): alle Positionen definiert.** Die X/Y-Werte
sind verbindliche Layout-Vorgaben; die CAD-/Gehäuse-Validierung bestätigt
sie nur noch (keine offenen Platzierungs-Entscheidungen mehr).

---

## 1. Außen-Geometrie

| Element | Wert |
|---|---|
| Gehäuse Außen | 333 × 143.3 × 40 mm |
| PCB-Outline | 320 × 130 mm, 1.6 mm dick |
| Gehäuse-Bezel rundherum | 6.5 mm |
| Edge-Rails (für PCBA, werden gebrochen) | 5 mm rundherum |
| Component-to-Edge | min 2.5 mm |
| Top-Plate Material | PC oder ABS, 2.5 mm dick |
| Bottom-Case Material | PC oder Aluminium, mit Speaker-Grille-Pattern |
| Tilt | 0° (Kick75 hat 6°, wir nicht) |

PCB-Ursprung: Bottom-Left-Corner = (0, 0). X nach rechts, Y nach oben
(KiCad-PCB-Editor-Konvention — anders als Schematic-Y-DOWN!).

---

## 2. USB-C-Stecker (J1) — TOP-Edge zentriert

| Parameter | Wert | Quelle |
|---|---|---|
| X (Mitte) | 160 mm | von left edge |
| Y (Edge) | 130 mm | Top-Edge |
| Z-Offset | Stecker-Front bündig mit Top-Edge oder 0.5 mm Inset |
| Edge-Cutout | TYPE-C-31-M-12 Standard-Footprint |
| USB-Stecker-Tiefe | ~6.5 mm Insertion |

---

## 3. OLED-Display (J3 + Modul ER-OLEDM032-1W)

OLED-Modul hat Active-Area 80×22 mm, Modul-Außenabmessungen ~100.5×33.5 mm.

| Parameter | Wert |
|---|---|
| Active-Area X-Mitte | ~80 mm |
| Active-Area Y-Mitte | ~110 mm |
| Active-Area Größe | 80 × 22 mm |
| Top-Plate Cutout-Größe | 82 × 24 mm (1 mm Toleranz) |
| J3 Header X | 80 mm (mittig unter Modul-Unterkante) |
| J3 Header Y | 95 mm |
| J3 Orientierung | Liegender 2.54mm-Header, Pins Richtung -Y (Board-Mitte) |

**Entscheidung (v0.7)**: Option (a) — Modul auf 8mm-Standoffs über dem PCB,
J3 als liegender Pin-Header an der Modul-Unterkante (Y=95). Modul-Außenmaße
~100.5×33.5mm spannen X 30..130.5, Y 93.25..126.75. Standoffs an den 4
Modul-Ecken. Begründung: hält die Active-Area auf einheitlicher Höhe mit der
Front-Plate (8mm Component-Height-Zone unter Display) und lässt unter dem
Modul Platz für Routing.

---

## 4. Cell-Switches SW1-SW5 (2u Hot-Swap, Kailh Choc V2)

5 Switches in horizontaler Reihe.

| Switch | X (Mitte) | Y (Mitte) | Notes |
|---|---|---|---|
| SW1 (CELL1) | 67.5 mm | 75 mm | links außen |
| SW2 (CELL2) | 105 mm | 75 mm | |
| SW3 (CELL3) | 142.5 mm | 75 mm | mitte |
| SW4 (CELL4) | 180 mm | 75 mm | |
| SW5 (CELL5) | 217.5 mm | 75 mm | rechts außen |

Spacing: 37.5 mm zwischen Cells (2u Cap = ~37.5 mm). Stabilizer-Position
zusätzlich zur Switch-Position: je 5.95 mm links und rechts vom Switch-Center
(2u Choc V2 Stabilizer-Hole-Spacing).

**Top-Plate Cutout**: ~17×18 mm pro Cell-Pos für 2u-Cap-Profil.

---

## 4a. Cell-HOLD-Status-LEDs LED11-LED15 (NEU r10)

5 SMD-0603-LEDs, je eine über jeder Cell. Sichtbarer Indikator dass Cell N im
HOLD-Modus sustained spielt (Firmware-State, gespeist via PCA9685 LED5-LED9).

| LED | X (Mitte) | Y (Mitte) | Funktion | PCA9685-Kanal |
|---|---|---|---|---|
| LED11 (CELL1 HOLD) | 67.5 mm | 88 mm | über SW1 | LED5 |
| LED12 (CELL2 HOLD) | 105 mm | 88 mm | über SW2 | LED6 |
| LED13 (CELL3 HOLD) | 142.5 mm | 88 mm | über SW3 | LED7 |
| LED14 (CELL4 HOLD) | 180 mm | 88 mm | über SW4 | LED8 |
| LED15 (CELL5 HOLD) | 217.5 mm | 88 mm | über SW5 | LED9 |

**Y=88 Begründung**: Cells bei Y=75 mit 2u-Cap-Profil (Höhe ~18 mm) belegen
Y=66..84. OLED-Modul-Bottom bei Y=93. Die 9 mm-Lücke Y=84..93 ist freie
Front-Plate-Zone — LED bei Y=88 ist mittig in dieser Lücke, ungeniert sichtbar
beim Cell-Spielen ohne dass die Hand sie verdeckt.

**Top-Plate-Cutout pro LED**: 3 × 3 mm transparentes Fenster ODER 2-mm-Bohrung
mit Light-Pipe (1.5 mm Acrylstab). Erste Iteration: Bohrung-Variante (einfacher
Fabrikations-Schritt).

**Pad-Geometrie**: SMD-0603-Standard, 0.95×1.0 mm Pads, 0.8 mm Spacing.
Footprint: `LED_SMD:LED_0603_1608Metric`. R_LED11-15 (390 Ω 0603) sitzt
direkt neben jeder LED auf der PCB (≤2 mm Distanz für saubere LED-Anode-
Routing).

---

## 5. Modifier-Switches SW6-SW10 (12×12×7.3 mm plain SMD-Tactile, r10)

5 Switches in horizontaler Reihe. **r10-Wechsel**: weg vom AliExpress-Generic
mit integrierter LED (r7) → hin zu **HX 12x12x7.3TPFT-B** (LCSC C36498966)
JLC Extended SMD-Tactile mit **4 Pins, KEINE integrierte LED**. State lebt
in Firmware, LED zeigt Zustand über **separate SMD-0603-LEDs** direkt über
jedem Switch (siehe LED-Tabelle unten + SPEC §7.2).

| Switch | X (Mitte) | Y (Mitte) | Label | LED-Ref (r10) | LED X/Y |
|---|---|---|---|---|---|
| SW6 (SHIFT) | 95 mm | 50 mm | | LED6 (SMD 0603) | 95 / 60 |
| SW7 (HOLD) | 120 mm | 50 mm | | LED7 | 120 / 60 |
| SW8 (DRONE) | 145 mm | 50 mm | | LED8 | 145 / 60 |
| SW9 (GENERATE) | 170 mm | 50 mm | | LED9 | 170 / 60 |
| SW10 (CLEAR) | 195 mm | 50 mm | | LED10 | 195 / 60 |

Spacing: 25 mm Mitte-zu-Mitte (bleibt) → 13 mm Lücke zwischen 12 mm
Switch-Bodies (ergonomisch erreichbar mit Daumen während Cell-Spiel).

**LED-Y=60 Begründung**: Modifier-Switch-Body bei Y=50 erstreckt sich Y=44..56
(12 mm Body). LED bei Y=60 sitzt 4 mm über der Body-Oberkante — direkt
sichtbar, kein Konflikt mit Switch-Mechanik. Y=60 liegt unter Cell-Cap-Bottom
(Y=66) → kein Cell-Konflikt.

**Top-Plate-Cutout pro Switch**: **12.5 × 12.5 mm** für Cap. Plus pro LED:
3×3 mm transparentes Fenster oder 2-mm-Light-Pipe-Bohrung (gleiche Bauweise
wie Cell-LEDs §4a).

**Switch-Höhe ab PCB**: 7.3 mm Body + ~3-5 mm Cap = 10.3-12.3 mm. Liegt im
bestehenden 15 mm-Component-Height-Budget.

**Footprint** (JLC-Standard, KEIN Custom mehr — r7-B1 RESOLVED):

```
        Top View (Switch, viewed from above PCB)
        
        ┌─────────────────────────┐  ← 11.8 × 11.8 mm Body
        │  ●1              ●4     │
        │                         │
        │       (plunger)         │  ← Square plunger, ~5×5 mm,
        │                         │      nimmt custom caps auf
        │  ●2              ●3     │
        │                         │
        └─────────────────────────┘

        Pad-Raster (Industrie-Standard 12×12 SMD-4P):
        - Pins 1↔2 vertikal: 4.5 mm
        - Pins 1↔4 / 2↔3 horizontal: 6.5 mm
        - Pad-Größe SMD: 1.0 × 1.5 mm (Gull-Wing-Lötfläche)
```

**Pin-Verdrahtung**: Pin 1+2 sind intern eine Seite, Pin 3+4 die andere.
Schalt-Element ist 1+2 ↔ 3+4. Im Schematic: Pin 1 → MCP23017 GPB(n-6),
Pin 3 → GND. Pin 2, Pin 4 = NC (oder parallel zum Schalt-Partner für
Redundanz).

**LED-Schaltung pro Switch (separat, NICHT mehr integriert)**:
- LED6-LED10 SMD-0603 (Anode → R_LEDn 390 Ω → +5 V; Kathode → PCA9685 LED0-LED4)
- R_LED6-10 (390 Ω 0603) direkt neben jeder LED platziert, ≤2 mm Distanz

**r10-B8 SOURCING-PASS noch offen**: HX 12x12x7.3TPFT-B Datasheet nicht bei
LCSC verfügbar. Standard-12×12-SMD-4P-Footprint sollte stimmen, aber vor
PCB-Layout-Freigabe entweder:
1. 1 Sample bestellen ($0.05) und Pin-Pitch mit Caliper auf 0.1 mm verifizieren, ODER
2. JLC LCSC-API einen alternativen MPN mit verfügbarem Datasheet finden (z.B. KH-12X12X7H-SMT C18186471, $0.055, 328 pcs Stock — weniger Stock aber Datasheet potentiell vorhanden), ODER
3. Standard-KiCad-Footprint `Button_Switch_SMD:SW_SPST_TL3342` 1:1 nehmen (12×12-Industrie-Standard).

Empfehlung: (3) — Industrie-Standard ist seit 2010er-Jahren stabil, und HX
übernimmt nachweislich diesen Footprint (Package-Bezeichnung jlcsearch:
„SMD-4P,11.8×11.8mm" matcht Standard).

---

## 6. Encoder EN1-EN4 (EC11 mit Push, 4 Stück)

4 Encoder in horizontaler Reihe oben, neben dem Display.

| Encoder | X (Mitte) | Y (Mitte) | Funktion |
|---|---|---|---|
| EN1 | 195 mm | 110 mm | Drive |
| EN2 | 225 mm | 110 mm | Brightness |
| EN3 | 255 mm | 110 mm | Display (Menu) |
| EN4 | 285 mm | 110 mm | Volume |

Encoder-Shaft-Höhe: 20 mm ab PCB (EC11 H20mm). Top-Plate-Bohrung: 7 mm
Durchmesser (EC11 6.4mm Standard-Bushing).

---

## 7. Lautsprecher (J6, J7 + PUI AS04008PS Treiber) — r13 Acoustic-Refactor

Down-firing, Bottom-Case-Mount. Speaker-Mitte muss mit PCB-Cutout
übereinstimmen. **r13: Bass-Reflex-Ports entfernt → Sealed-Chamber + Top-Plate-
Passivradiator (siehe §7b).**

| Lautsprecher | X (Mitte) | Y (Mitte) | PCB-Cutout |
|---|---|---|---|
| Links (J6 = SPK_L) | 50 mm | 30 mm | 41 mm Durchmesser |
| Rechts (J7 = SPK_R) | 270 mm | 30 mm | 41 mm Durchmesser |

Mount: 4× M2-Schrauben pro Speaker am Bottom-Case. **Akustik-Kammer geschlossen
pro Kanal**, abgedichtet via Dichtung zwischen Treiber-Rahmen und Bottom-Case-
Cutout. Innen-Volumen pro Kammer ~80-120 cm³ (abhängig von Battery-Position +
PCB-Cutout-Geometrie, siehe §7a).

**Bass-Reflex-Ports ENTFERNT in r13** (waren früher: 2× 8mm × 25mm hinten,
~80Hz Tuning). Verworfen wegen Port-Chuffing bei Drone-Audio im flachen
Gehäuse. Akustik-Erweiterung via Passivradiator §7b.

---

## 7b. Passivradiator PR_L + PR_R (NEU r13, Top-Plate-Mount)

Ersetzt die in r13 verworfenen Bass-Reflex-Ports. Top-mounted, atmet sichtbar
(ästhetisches Bonus), wird durch Auflage-Fläche nicht blockiert. Tuning via
Mass-Loading der Membran (Klebegewichte auf Kunststoff-Membran nach erstem Build).

| Element | Wert |
|---|---|
| Anzahl | 2× (1 pro Kanal in geschlossener Akustik-Kammer) |
| Membran-Außendurchmesser | 45-55 mm (Kandidaten Dayton DMA45-PR oder Generic 50mm BT-Speaker-PR) |
| Sd (Membranfläche) | ≥ 13 cm² (1.5× über AS04008-Sd) |
| Xmax | ≥ 4 mm (höher als Treiber-Xmax) |
| Tuning Fb | ~85-100 Hz (via Mass-Loading getrimmt nach erstem Build) |

| PR | X (Mitte) | Y (Mitte) | Top-Plate-Cutout | Bottom-Case |
|---|---|---|---|---|
| PR_L (über Speaker L) | 50 mm | 30 mm | 51 mm Durchmesser | unverändert (Treiber unten) |
| PR_R (über Speaker R) | 270 mm | 30 mm | 51 mm Durchmesser | unverändert (Treiber unten) |

**X/Y matched mit Treiber-Position**: PR sitzt direkt über dem zugehörigen
Speaker → kürzester Akustik-Pfad zur geschlossenen Kammer → bestes Coupling.

**Top-Plate-Geometrie**: Cutout 51 mm Durchmesser. PR-Rand klebt/schraubt von
unten an die Top-Plate (durch Cutout sichtbar als „Atmen"-Membran). Optional
mit Schutzgitter / Stoff-Bespannung außen für Optik.

**Akustik-Kammer**: Innen geschlossen mittels Trennsteg zwischen L- und R-Kammer
(im Bottom-Case-Inlay). Kammer-Größe pro Kanal abhängig von Battery-Position
(siehe §7a Revision r13) — Ziel ~80-120 cm³.

**OFFENE PUNKTE r13**:
- **r13-B1 (Sourcing)**: konkreter PR-MPN finalisieren mit T/S-Parametern
  (Mms, Cms, Sd, Re) für Sealed-Box-Simulation in WinISD oder Hornresp.
  Kandidaten: Dayton DMA45-PR, Tang Band W2-1625S-P, oder Generic 50mm
  AliExpress „BT-Speaker Passive Radiator".
- **r13-B2 (Mechanik)**: 51 mm-Cutout-Position auf Top-Plate gegen
  Front-Plate-Bezel + OLED + Encoder-Cluster verifizieren. Y=30 mm bedeutet
  PR sitzt im Bottom-Bereich der Top-Plate, unterhalb Modifier-Switches (Y=50).
  Konflikt-Check: Cell-Switches bei Y=75 mit 2u-Cap-Body bis Y=84 → PR-Y=30
  ist 45 mm vom Cell-Bottom entfernt, kein Konflikt.

---

## 7a. Battery BAT1 (NEU r9, LiPo 5000 mAh)

LiPo-Pouch 8050120 oder 9050120 (8-9 mm × 50 mm × 120 mm), liegt **unter
dem PCB**, längs zur PCB-Längsachse (X) ausgerichtet. JST PH 2.0 2-pin
Connector J9 auf PCB-Unterseite, Plus-Pin polarisiert.

| Parameter | Wert |
|---|---|
| BAT1 Center X | 80 mm (linker Bereich, neben Speaker-Cutout-Bereich) |
| BAT1 Center Y | 65 mm (unter den Cells, unter dem Modifier-Bereich) |
| BAT1 Außenmaße | 8-9 × 50 × 120 mm (Z-Tiefe nach unten) |
| BAT1 Keepout (Unterseite) | 60 × 130 mm @ X=20..140, Y=0..130 |
| J9 (Battery-Connector) X | 25 mm |
| J9 (Battery-Connector) Y | 65 mm |
| J9 Z-Offset | Bottom-Side, vertical SMD |
| Z-Tiefe zusätzlich für Gehäuse | +9 mm Unterseite (geht in 40-mm-Gesamthöhe rein) |

**Vereinbarkeit mit Pi-frei-Stand (v0.9)**: Pi ist raus → Bottom-Side-Keepout
für Pi (X 125..195, Y 72..108) **frei**. Battery passt in den freigewordenen
Bereich rechts (X=20..140, Y=0..130) — Pi-Zone (X=125..195) bleibt für
zukünftige Erweiterung oder leer.

**Speaker-Cutout-Kompatibilität**: Speaker-Cutouts liegen X=10..90, Y=10..50
(links) und X=230..310, Y=10..50 (rechts). Battery-Zone Y=0..130 würde mit
linker Speaker-Cutout überlappen — Battery-Pouch **muss in Y-Position auf
Y=60..120 begrenzt** werden (also weiter oben) um Speaker freizuhalten.
Revidierte Battery-Center: **X=80, Y=80, Pouch-Footprint 50×60 mm**.
Bei 9050120-Pouch (50×120) passt das NICHT — dann Wahl: **kleinerer Pouch
9050060 = 5000 mAh in 50×60 mm² × 14 mm** (etwas dicker), ODER **Speaker-Cutouts
in eine andere Anordnung bringen**. **OFFENER PUNKT r9-B5 (Mechanik)**.

---

## 8. Pi Zero 2 W (auf Unterseite, durchgesteckter Header) — OBSOLET v0.9 (Pi-frei)

Pi liegt unter dem PCB, GPIO-Header J2 durch das PCB gesteckt. Pi-Modul
selbst nimmt ~65×30 mm ein.

| Parameter | Wert |
|---|---|
| J2-Header X | 160 mm (mittig, unter Display/Encoder-Bereich) |
| J2-Header Y | 90 mm |
| Pi Z-Offset | 0..-15 mm (Pi und Header zusammen ~12 mm dick) |
| Pi Keepout-Area | 70×35 mm unter PCB (5 mm Toleranz) → X 125..195, Y 72..108 |

**Entscheidung (v0.7)**: J2 @ (160, 90). Mittig platziert, damit der Pi unter
dem Display/Encoder-Cluster sitzt (dort ist die Top-Side ohnehin von Modul +
Encodern belegt, also keine Doppelnutzung). Keepout 70×35mm bleibt frei von
Bottom-Side-Komponenten. Kollidiert nicht mit Speaker-Cutouts (X<90 / X>230).

---

## 9. Pico 2 (RP2350, optional THT-Header oder SMD-castellated)

THT-Variante (Empfehlung Prototyp): Pin-Header durchgesteckt.

| Parameter | Wert |
|---|---|
| U1 Position X | 270 mm |
| U1 Position Y | 80 mm |
| Pico-Höhe | 5 mm Modul + 8 mm Header = 13 mm |

**Entscheidung (v0.7)**: U1 @ (270, 80). Pico-Modul ~51×21mm spannt X 244..296,
Y 69.5..90.5 — liegt vollständig in der 15mm-Height-Zone (Cell/Modifier-Bereich
Y=40..90), nicht in der 5mm-Encoder-Zone (Y≥95). Rechts neben den Cells, kurze
I²S-/I²C-Wege zum Audio-Block und MCP. SWD-Header J4 daneben platzieren.

---

## 10. Mounting-Holes

| Hole | X | Y | Spec |
|---|---|---|---|
| MH1 | 5 mm | 5 mm | M3, Bottom-Left |
| MH2 | 315 mm | 5 mm | M3, Bottom-Right |
| MH3 | 5 mm | 125 mm | M3, Top-Left |
| MH4 | 315 mm | 125 mm | M3, Top-Right |
| MH5 (opt) | 160 mm | 5 mm | M3, Bottom-Center für Verstärkung |
| MH6 (opt) | 160 mm | 125 mm | M3, Top-Center |

Hole-Diameter: 3.2 mm (für M3 mit etwas Spiel). Pad-Annular: 6 mm
für Schraubkopf-Sitz.

---

## 11. Component-Height-Zones (Gehäuse-Innenraum)

Gehäuse-Innenraum: 40 mm minus Top-Plate (2.5 mm) minus Bottom-Case
(2.5 mm) minus Bottom-Speaker-Volume (~10 mm) = ~25 mm nutzbar.

| Bereich | Max Component Height | Anmerkung |
|---|---|---|
| Unter Encoder (X=180..305, Y=95..125) | 5 mm | Encoder-Bushing belegt Vertical-Space |
| Unter Display (X=40..120, Y=95..125) | 8 mm | OLED-Modul auf Standoffs |
| Cells/Modifier-Bereich (Y=40..90) | 15 mm | Switches durchgesteckt durch PCB |
| Pi-Bereich (Bottom-Side, X=130..200) | unter-PCB 13 mm | Pi+Header durchgesteckt |
| USB-C-Bereich (Y=125..130) | edge-mounted | Front-Plate-Cutout |
| Speaker-Bereich (X=10..90 und X=230..310, Y=10..50) | 0 mm (Cutout) | PCB hat hier ein 41-mm-Loch |

---

## 12. PCB-Edge-Annahmen für Layout

| Edge | Belegung |
|---|---|
| Top (Y=130) | USB-C zentriert. Kein Routing innerhalb 3 mm zur Edge |
| Bottom (Y=0) | Edge-Rail 5 mm — keine Komponenten. Speaker-Cutouts unterbrechen |
| Left/Right (X=0/320) | Edge-Rails 5 mm |

---

## Status / TODOs für Layout-Phase

Alle Komponenten-Positionen sind festgelegt (✅). Verbleibend sind reine
Layout-/CAD-Ausführungsschritte (keine Entscheidungen mehr):

- ✅ OLED-J3-Position festgelegt: (80, 95), liegender Header, Option (a) Standoffs
- ✅ Pi-J2-Position festgelegt: (160, 90)
- ✅ Pico-U1-Position festgelegt: (270, 80)
- [ ] CAD-Modell erstellen (FreeCAD/Fusion) zur Bestätigung der Maße
- [ ] DXF-Export der Board-Outline (mit Speaker-Cutouts + Mounting-Holes)
- [ ] DXF-Import in KiCad als Edge.Cuts-Layer
- [ ] Komponenten in KiCad-PCB-Editor auf obige Koordinaten platzieren + routen
