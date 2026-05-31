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

## 5. Modifier-Switches SW6-SW10 (12×12×7.3 mm momentary tactile MIT LED, r7)

5 Switches in horizontaler Reihe. **r7-Wechsel**: weg von Kailh Choc V2
Hot-Swap (1u, latching-Caps) → hin zu 12×12×7.3 mm momentary tactile mit
integrierter LED (Generic China / AliExpress „Momentary Touch LED"). State
lebt in Firmware, LED zeigt Zustand (siehe SPEC §7.2).

| Switch | X (Mitte) | Y (Mitte) | Label | LED-Ref |
|---|---|---|---|---|
| SW6 (SHIFT) | 95 mm | 50 mm | | LED6 (Anode unter Switch) |
| SW7 (HOLD) | 120 mm | 50 mm | | LED7 |
| SW8 (DRONE) | 145 mm | 50 mm | | LED8 |
| SW9 (GENERATE) | 170 mm | 50 mm | | LED9 |
| SW10 (CLEAR) | 195 mm | 50 mm | | LED10 |

Spacing: 25 mm Mitte-zu-Mitte (bleibt) → 13 mm Lücke zwischen 12 mm
Switch-Bodies (ergonomisch erreichbar mit Daumen während Cell-Spiel).

**Top-Plate-Cutout**: **12.5 × 12.5 mm** pro Switch (war 14×14 für Choc-Caps).
Das Cap des Tactile-Switches sitzt direkt im Cutout; Plate-Dicke bestimmt
Cap-Travel-Höhe.

**Switch-Höhe ab PCB**: 7.3 mm Body + ~3-5 mm Cap = 10.3-12.3 mm. Liegt im
bestehenden 15 mm-Component-Height-Budget (Y=40..90, siehe §9).

**Custom-Footprint** (im Projekt-PCB-Lib, NICHT in KiCad-Standard):

```
        Top View (Switch, viewed from above PCB)
        
        ┌─────────────────────────┐  ← 12.0 mm × 12.0 mm Body
        │  ●1              ●4     │
        │                         │
        │       (plunger)         │  ← Square plunger, ~5×5 mm,
        │                         │      nimmt custom caps auf
        │  ●2              ●3     │
        │                         │
        │   ✦LED+        ✦LED-    │  ← LED-Anode/Kathode (Pin 5/6)
        └─────────────────────────┘
```

Pin-Pitch-**Annahme** (verifizieren mit realem AliExpress-Part vor Footprint-
Finalize!):
- Switch-Pins (1↔2, 3↔4 sind je intern verbunden, 1+2 ↔ 3+4 ist das Switch-
  Element): **Raster 6.5 mm horizontal × 4.5 mm vertikal**
- LED-Pins (5 = Anode, 6 = Kathode): **Raster ~5 mm horizontal**, ca. 8 mm
  vertikal unterhalb der Switch-Pins
- Pad-Größe: 1.4 mm Bohrung, 2.4 mm Ring (THT Standard für 1.0 mm Pin-Dicke)

**r7 BLOCKER**: AliExpress-Generic-Parts haben KEINEN herstellergemeinsamen
Pin-Pitch-Standard. Vor PCB-Layout-Freigabe:
1. 1 Stück bestellen (Lieferzeit ~3-4 Wochen)
2. Pins mit Messschieber vermessen (0.1 mm Genauigkeit)
3. Custom-Footprint in `kicad/libraries/field_ambience.pretty/` final fixieren
4. ERST DANN PCB-Routing finalisieren

Verkabelung pro Switch im Schematic:
- Switch-Pin 1+2 → MCP23017 GPB(n-6) (mit internem Pull-Up)
- Switch-Pin 3+4 → GND
- LED-Anode (Pin 5) → R_LEDn (390 Ω) → +5 V
- LED-Kathode (Pin 6) → PCA9685 LEDn-Output (open-drain Sink)

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

## 7. Lautsprecher (J6, J7 + PUI AS04008PS Treiber)

Down-firing, Bottom-Case-Mount. Speaker-Mitte muss mit PCB-Cutout
übereinstimmen.

| Lautsprecher | X (Mitte) | Y (Mitte) | PCB-Cutout |
|---|---|---|---|
| Links (J6 = SPK_L) | 50 mm | 30 mm | 41 mm Durchmesser |
| Rechts (J7 = SPK_R) | 270 mm | 30 mm | 41 mm Durchmesser |

Mount: 4× M2-Schrauben pro Speaker am Bottom-Case. Bass-Reflex-Ports:
2× 8×25 mm hinten im Bottom-Case (Tuning ~80Hz).

---

## 8. Pi Zero 2 W (auf Unterseite, durchgesteckter Header)

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
