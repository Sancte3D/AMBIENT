# KiCad-Quickstart für Field Ambience PCB

Du bist Audio-/Elektronik-Person, kein KiCad-Daily-User. Dieses Doc führt
dich durch die 3 verbleibenden Blocker (B1, B2, B3) in Klick-für-Klick-
Anleitung, oder du nutzt das Auto-Skript `scripts/run_erc.sh`.

---

## Schritt 0: KiCad installieren

Du brauchst KiCad **9.0 oder neuer**. Stand 2026: Version 9.x ist aktuell.

- **macOS**: https://www.kicad.org/download/macos/ — Universal-DMG runterladen, installieren wie jede andere App
- **Windows**: https://www.kicad.org/download/windows/ — Installer-EXE runterladen
- **Linux**: `sudo apt install kicad` (Ubuntu/Debian) oder Distro-Package

Nach Installation: KiCad ist im Programme-Ordner. Beim ersten Start fragt
es nach Libraries — einfach Defaults akzeptieren ("Yes" zu allem).

---

## Schritt 1: Projekt öffnen

1. KiCad starten (das Programm "KiCad" im Programme-Ordner, NICHT eeschema
   direkt)
2. Im KiCad-Hauptfenster: **File → Open Project**
3. Navigiere zu deinem Repo: `field-ambience-current/kicad/`
4. Wähle `field_ambience.kicad_pro` und klicke "Open"

Im KiCad-Projektfenster siehst du jetzt zwei wichtige Buttons links:
- **Schematic Editor** (zweites Icon von oben, sieht aus wie ein Schaltplan)
- **PCB Editor** (drittes Icon, sieht aus wie eine Platine)

---

## Schritt 2: Schematic öffnen + visuell prüfen

1. Klicke auf **Schematic Editor**
2. Es öffnet sich `field_ambience.kicad_sch` (Root-Sheet)
3. Du siehst 7 Rechteck-Boxen — jede ist ein Sub-Sheet:
   - PowerTree, Pico, OLED, MCP23017, Encoders, Audio, PiHeader

4. Doppelklick auf eine Box öffnet das jeweilige Sub-Sheet.
5. Wenn KiCad ROT/ORANGE Warnungen unten zeigt, ist das normal — wir
   prüfen das gleich systematisch via ERC.

**Was du visuell checken kannst** (nicht zwingend, nur als sanity-check):
- Power-Tree: USB-C-Symbol links, Polyfuse F1 oben, Bulk-Cap, +5V/GND-Flags
- Audio: PCM5102A links, PAM8403H rechts, 2 Speaker-Header noch weiter rechts

---

## Schritt 3 (Blocker B3): ERC laufen lassen — der wichtigste Schritt

### Option A (einfach): Auto-Skript

In einem Terminal/Shell (macOS: Terminal.app, Windows: PowerShell, Linux: bash):

```bash
cd /pfad/zu/deinem/repo/field-ambience-current
bash scripts/run_erc.sh
```

Das Skript ruft `kicad-cli sch erc` auf und speichert den Report in
`reports/ERC_YYYY-MM-DD.md`. Es zeigt am Ende: errors=N warnings=M.

Wenn errors=0: Schematic ist ERC-clean ✓ — Blocker B3 ist erledigt.
Wenn errors>0: Schau in den Report, fix oder bring zurück zu mir.

### Option B (KiCad GUI):

1. Im Schematic Editor: Menü **Inspect → Electrical Rules Checker**
2. Im ERC-Fenster: Button **"Run ERC"** unten links
3. Warte ~5-30 Sekunden
4. Liste mit Findings erscheint:
   - Rote Punkte = Errors (müssen gefixt werden)
   - Gelbe Punkte = Warnings (sollten geprüft werden)
5. **Export Report**: Button "Save..." → speichere als `reports/ERC_2026-05-XX.txt`

---

## Schritt 4 (Blocker B1, B2): Footprint-Check

Wenn ERC clean ist, prüfe ob KiCad die Footprints findet:

1. Im Schematic Editor: Menü **Tools → Update PCB from Schematic**
2. Es öffnet sich ein Dialog
3. Klicke "Update PCB"
4. KiCad öffnet den PCB-Editor und versucht alle Footprints zu laden

**Wenn KiCad meckert** "Footprint not found":
- Die fehlenden Footprint-Library-Refs in der Liste notieren
- Zurück zu mir mit dem Fehlertext → ich fixe das im Generator

**Wenn alles lädt**: PCB-Editor zeigt einen Haufen ungroupierter Bauteile.
Das ist erwartet — du hast noch kein Layout. Aber das beweist dass Symbol
↔ Footprint matchen.

### Konkrete Footprint-Sanity-Checks (B1 + B2)

Im PCB-Editor:

**USB-C Footprint (J1)**:
1. Suchen: Menü **Edit → Find** → tippe "J1" → Enter
2. KiCad markiert den USB-C-Footprint
3. Doppelklick darauf → Footprint-Dialog öffnet
4. Tab **"Pads"** klicken
5. Suche in der Pad-Liste nach **Pad "A1"** — die Spalte "Net" muss "GND" sein
6. Suche **Pad "A4"** — Net muss "+5V_USB" sein
7. Wenn beides stimmt: **B2 ✓ erledigt**

**PAM8403H Footprint (U4)**:
1. **Edit → Find** → "U4" → Enter
2. Doppelklick auf U4-Footprint
3. Tab "Pads":
   - Pad 1 → Net: "SPK_L-"
   - Pad 3 → Net: "SPK_L+"
   - Pad 14 → Net: "SPK_R+"
   - Pad 16 → Net: "SPK_R-"
   - Pad 4, 6, 13 → "+5V"
   - Pad 12 → "AMP_SHUTDOWN"
   - Pad 5 → "AMP_MUTE"
4. Wenn alle Pads die korrekten Nets haben: **B1 ✓ erledigt**

Falls KiCad andere Pad-zu-Net-Belegung zeigt:
- Footprint ist falsch gewählt → eventuell anderen Footprint suchen
- Oder Pin-Reihenfolge im Symbol stimmt nicht zum Footprint → Symbol fixen

---

## Schritt 5: Report ins Repo committen

Nach erfolgreichem ERC + Footprint-Check:

```bash
cd /pfad/zu/deinem/repo
git add reports/ERC_2026-MM-DD.txt
git commit -m "ERC clean — PCB-Layout cleared"
git push
```

Damit ist die elektrische Phase fertig. Nächster Schritt ist PCB-Layout
(component placement + routing) — wenn du das nicht selbst machen willst,
gibt's Layout-Services die das gegen Bezahlung machen.

---

## Was tun bei Errors

### ERC error "Pin not connected" auf einem NC-Label
Das ist OK — NC = absichtlich nicht verbunden. In KiCad: Rechtsklick auf
den Pin → "Place no-connect flag" (oder Taste **Q**). Macht den ERC-Warner weg.

### ERC error "Hierarchical labels mismatch"
Etwas hat ein hier-label inside einem Sheet, aber kein passender Pin auf
dem Sheet-Symbol außen. → Zurück zu mir.

### ERC error "Power input pin not driven"
Ein Power-Pin (VDD, VBUS, etc.) hat keine Source. Meistens fehlt ein
Power-Flag. → Zurück zu mir.

### Footprint not found
Die KiCad-Standard-Library muss installiert sein (bei normalem KiCad-
Install enthalten). Wenn ein Library-Path zerschossen ist: Menü
**Preferences → Manage Symbol Libraries / Footprint Libraries** → "Add
Existing" → KiCad-Default-Pfad finden.

---

## Faktorisiert: Was die einzelnen Pinouts sind

Wenn du im PCB-Editor unsicher bist, hier die ECHTEN Pin-Belegungen die
KiCad zeigen muss:

### USB-C (J1) per USB Type-C Spec Rev 2.1:
| Pad | Net |
|---|---|
| A1, A12, B1, B12 | GND |
| A4, A9, B4, B9 | +5V_USB |
| A5 | (CC1, R2 pulldown) |
| B5 | (CC2, R3 pulldown) |
| A6, B6 | USB_DP |
| A7, B7 | USB_DN |
| A8 | NC_SBU1 |
| B8 | NC_SBU2 |
| S1 | GND (Shield) |

### PAM8403H (U4) per Diodes Inc DS31295 (PAM8403H.PDF im Repo):
| Pad | Net |
|---|---|
| 1 | SPK_L- |
| 2 | GND (PGND) |
| 3 | SPK_L+ |
| 4 | +5V (PVDD) |
| 5 | AMP_MUTE |
| 6 | +5V (VDD) |
| 7 | (INL, via R_VOL_L + C_in_L) |
| 8 | (VREF, via C_VREF zu GND) |
| 9 | NC_U4_9 |
| 10 | (INR, via R_VOL_R + C_in_R) |
| 11 | GND |
| 12 | AMP_SHUTDOWN |
| 13 | +5V (PVDD) |
| 14 | SPK_R+ |
| 15 | GND (PGND) |
| 16 | SPK_R- |

### PCM5102A (U3) per TI Datasheet SLAS859C:
| Pad | Net |
|---|---|
| 1 | +3V3 (CPVDD) |
| 2, 4 | (CAPP, CAPM via C_FLY) |
| 3 | GND (CPGND) |
| 5 | (VNEG via C_VNEG zu GND) |
| 6 | PCM_VOUTL |
| 7 | PCM_VOUTR |
| 8 | +3V3 (AVDD, via FB1) |
| 9 | GND (AGND) |
| 10 | GND (DEMP tied LOW) |
| 11 | GND (FLT tied LOW) |
| 12 | GND (SCK tied LOW = 3-wire mode) |
| 13 | I2S_BCK |
| 14 | I2S_DOUT |
| 15 | I2S_LRCK |
| 16 | GND (FMT tied LOW = I²S mode) |
| 17 | (XSMT via R_XSMT zu +3V3) |
| 18 | (LDOO via C_LDOO zu GND) |
| 19 | GND (DGND) |
| 20 | +3V3 (DVDD) |

---

## Plan B: Wenn dir das alles zu viel ist

1. **Install KiCad** + öffne das Projekt mal nur visuell
2. Schick mir Screenshots vom Schematic
3. Lass mich aus der Ferne kucken ob optisch was komisch aussieht
4. Beauftrag ggf. einen KiCad-Freelancer für PCB-Layout (Upwork, Fiverr,
   lokaler Elektronik-Bekannter) — Material schick ihm den ganzen Repo

PCB-Layout-Aufwand für dieses Board: ~4-8 Stunden für einen geübten
KiCad-User. Routing ist nicht trivial (Audio, USB-C, 4-Layer-Stack).
