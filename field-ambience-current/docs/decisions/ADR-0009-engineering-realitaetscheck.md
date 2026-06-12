# ADR-0009: Engineering-Realitätscheck nach IMG_9713-Sprint

**Status:** ACCEPTED (Engineering-Review 2026-06-11)
**Date:** 2026-06-11

## Kontext

Nach drei aufeinanderfolgenden Design-Sprints (r18.5 Migration, r18.6
Verifikation, r18.7 Gate-Skip, r18.8 IMG_9713, r18.9 Schematic-Implementation)
verlangte der User einen ehrlichen Engineering-Review: Welche Entscheidungen
machen Sinn, welche nicht. Schwerpunkt-Fragen:

- USB-C-Stecker-Qualität
- Flashen ohne Pico-Modul
- Display-Funktionsfähigkeit nach Migration
- Component-Z-Höhen
- Doku-vs-Code-Drift

Dieses ADR fixiert die Befunde und Maßnahmen.

## Befunde + Entscheidungen

### 🔴 1. BOOT0-Button gefehlt (kritisch, gefixt in r18.10)

**Befund:** Bis r18.9 war BOOT0 nur mit 10-k-Pulldown beschaltet. USB-DFU-
Flash setzt BOOT0=HIGH beim Reset voraus. Ohne externen Pull-up-Schalter
ist USB-DFU **physikalisch nicht möglich**. SPEC-§1-Diagramm behauptete
fälschlich "USB-DFU oder ST-Link".

**Fix r18.10:** SW_BOOT (SMD-Tactile TS-1185A-C-A / C720477) plus
R_BOOT_SW 1 kΩ in Reihe nach +3V3. Bedienung: SW_BOOT halten → NRST tippen
→ loslassen → STM32 ist im System-Memory-Boot (USB-DFU-Modus).

**Warum 1 kΩ in Reihe und nicht direkter Pull-up:** SW_BOOT gegen +3V3
würde mit dem 10-k-Pulldown einen 330-µA-Querstrom-Pfad bilden, der
allein OK ist — aber falls SW_RESET gleichzeitig gedrückt wird (Edge-
Case: User-Fehler), entsteht ein definiertes Strom-Limit von 3 mA über
R_BOOT_SW. Ohne R_BOOT_SW könnte das +3V3-Rail kurzzeitig zusammenbrechen.

**ST-Link via SWD-Header bleibt** als Primär-Programmer (zuverlässiger,
empfohlen für Daily-Dev). USB-DFU ist Backup für End-User-Service-Updates.

### 🟠 2. USB-C-Stecker upgegradet (r18.10)

**Befund:** TYPE-C-31-M-12 (C165948) = Generic-China, ~5000 Insertion-
Cycles, JLC-Basic. Für ein Produkt mit täglicher USB-Verbindung knapp.

**Fix r18.10:** Drop-In auf TYPE-C-31-M-17 (C165935) = gleiche Korean-
Hroparts-Bauform, **10 k Insertion-Cycles**, JLC-Basic. Footprint
HRO_TYPE-C-31-M-12 bleibt drop-in laut HRO-Tabelle. Vor Fab-Order in
Phase 6 visuell gegen M-17-Drawing prüfen.

**Premium-Pfad (post-Prototyp):** GCT USB4135-03-A oder Amphenol
12401610E4-2A — beide ~10 k Cycles, höherer Anpressdruck, anderer Footprint
(eigenes FP nötig). Nicht für Spin-1, sinnvoll für Serie.

### 🟢 3. Display funktioniert nach Pico-Migration

**Befund:** Display-Pfad in r18.5 sauber umgebaut: ST7789 hängt jetzt an
**SPI1** (PA5 SCK, PA7 MOSI) statt Pico-PIO, plus PA6 CS, PC4 DC, PC5 RES.
Backlight: PCA9685-Kanal 15 → Q2 2N7002 → BLK-Pin. Alle Netze verdrahtet
und im Crossref bestätigt (lcd.kicad_sch + mcp.kicad_sch + root).

**Bewertung:** Besser als Pico-Stand. SPI1 ist Hardware-SPI mit DMA-Support
(SPI1_TX → DMA1/DMA2 Stream), perfekt für die ~27-kB-Framebuffer-Blits.

### 🟠 4. Z-Höhen — keine Show-Stopper, ein Layout-Constraint

Höchste Komponenten (absteigend, gegen Datenblatt verifiziert):

| Komponente | Höhe | Anmerkung |
|---|---|---|
| EN1-4 EC11J Encoder | ~19 mm | bestimmt Gehäuse-Mindesthöhe + Frontplatten-Auslass |
| C_BULK 1000 µF Elko 10×10.5 | 10.5 mm | Layout: nicht unter LCD-Modul platzieren |
| J3 LCD-Header 2.54 mm | 8.5 mm | Standoff für LCD-Modul drauf |
| USB-C TYPE-C-31-M-17 | 7.5 mm | + 7 mm Mate-Höhe = 14.5 mm Aussparung |
| SW6-10 HX 12×12×7.3 | 7.3 mm | Modifier-Buttons, Frontplatten-Cap drauf |
| J9 JST PH 2.0 | 6.0 mm | LiPo-Batterie-Anschluss |
| J6/J7 Speaker | ~5 mm | + Dust-Mesh +0.3 mm Inset |
| Y1 HC-49/US-SMD | 4.2 mm | weniger als Connector — OK |
| Silicon-Cap über FSR | ~3 mm | inkl. FSR-Pad ~3.5 mm |

**Konflikt-Risiko:** C_BULK 10.5 mm steht über LCD-Header (8.5 mm). Beide
unter der Frontplatte. Layout muss C_BULK an eine Stelle ohne LCD-Modul-
Überdeckung platzieren (z. B. unter Speaker-Bereich oder zwischen MCU und
Battery).

**Gehäuse-Mindest-Innenhöhe:** ~22-25 mm (Encoder dominieren), Frontpanel-
Tiefe dazu.

### 🟠 5. BOM-Drift SPEC §4 ↔ Generator r18.9 (gefixt in r18.10)

**Befund:** SPEC §4 listete weiterhin:
- 5× Kailh Choc V2 Hot-Swap-Sockets
- 5× Choc V2 Stabilizer
- alte LED-Tabelle (LED11-LED15 als 1× pro Cell)

Generator r18.9 hat:
- Choc/Stabilizer komplett entfernt
- 5× J_CELL + 5× R_CELL + 5× C_CELL + 5× FSR (extern)
- 10× Cell-LEDs (5 gelb + 5 grün) + 5× Modifier-LEDs (3 Farben)

**Fix r18.10:** SPEC §4 BOM-Tabelle synchronisiert. mechanical_coordinates
§4 als veraltet markiert.

### 🟠 6. LED-Farben-Stock-Realität

**Befund:** ADR-0008 wählt 3 Farben (weiß/gelb/grün). Generator r18.9
nutzt 4 verschiedene MPN-Slots: XL-1608UWC-04 (weiß, C965808, JLC-Basic),
XL-1608UYC-04 (gelb, TBD-VERIFY), XL-1608UGC-04 (grün, TBD-VERIFY).

**JLC-Realität:** Weiß ist Basic, Gelb/Grün sind Extended → pro nicht-Basic-
Part eine $3 Setup-Fee. Für 5 Stück Prototyp = $6 Mehrkosten, marginal.
Für Serien-Produktion: Stock-Verfügbarkeit der XINGLIGHT-Serie sichern oder
auf JLC-Basic-Alternativen wechseln.

**Optionale Optimierung (post-Prototype):** Bi-Color-LED Gelb-Grün
(KingBright APHF1608-Klasse, 3-Pin Common-Cathode) für die 10 Cell-LEDs —
spart 5 PCA-Kanäle und 5 LED-Refs. Aktuell **nicht** nötig, weil 16/16
PCA-Budget passt.

### 🟠 7. FSR-Anschluss: 2.54-mm-Header ist suboptimal (Layout-Punkt)

**Befund:** r18.9 nutzt 5× Conn_01x02 (2.54 mm THT-Header) für die FSR-
Pads. Das sind 10 Lötlöcher mitten auf der Platine, mechanisch klobig,
braucht Platz unter dem PCB.

**Empfehlung (nicht in r18.10 implementiert):** Ein einzelner **FFC/FPC-
Connector** (10-Pin, 1.0-mm-Pitch, ZIF) mit Flexkabel zur FSR-Bank. Spart
Platz, ist SMT-bestückbar, professioneller Anschluss-Standard für Velocity-
Pads.

**Entscheidung:** vertagt bis Mechanik-Phase (Silicon-Cap-Frame-Design).
Wer FSR-Bauform fixiert, fixiert auch Anschluss-Art. SPEC §5.6a hält
beides offen.

### 🟢 8. Was bewusst NICHT geändert wird

- **STM32H743 statt Pico 2** — ADR-0002 bleibt richtig
- **ABLS-HSE-Crystal mit GM=2.97 Worst-Case** — ADR-0008-Y1 bleibt richtig
- **AP7361C SOT-89-5 Pinout** — verifiziert ADR-0006-LDO
- **MCP-XSMT-Pop-Suppression** — bleibt für DAC-Mute-Sequenz wichtig, FSR-
  Cells triggern das nicht
- **C_BULK 1000 µF Polymer-Bypass-Strategie** — Inrush-Frage in r12 geklärt
- **Dust-Mesh-Speaker** — Industrial-Design-Entscheidung, akustisch -1 dB ist
  marginal und durch Volume-Calibration kompensierbar
- **Cell-LED-XOR-Logik** — passt sowohl konzeptuell als auch hardware-budget

## Consequences

**Positive:**
- Board ist tatsächlich flashbar (USB-DFU via SW_BOOT, plus ST-Link backup)
- USB-C verdoppelt sich in Lebensdauer ohne Footprint-Change
- BOM und Schematic synchron
- Z-Höhen-Konflikt früh erkannt → Layout-Constraint, nicht Re-Design

**Negative:**
- SW_BOOT + R_BOOT_SW = 2 zusätzliche SMD-Komponenten, marginal teuer
- FSR-Anschluss-Frage offen, vertagt
- Bi-Color-LED-Optimierung offen, vertagt

## Related

- ADR-0006 — Cell-Action FSR (Anschluss-Optimierung offen)
- ADR-0008 — LED-XOR (Bi-Color-Optimierung offen)
- ADR-0002 — MCU-Migration
- SPEC §5.8 — BOOT0/NRST/SWD-Beschaltung
