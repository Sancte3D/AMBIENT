# ADR-0016: Power & Sleep Architecture

**Status:** PROPOSED → AMENDED 2026-06-27 → **IMPLEMENTED r18.81 (2026-07-02): U_PWR + SW_PWR + R_PWR_PD + C_PWR_SW (+ C_UPWR_IN VIN-Bypass per TI-DS) sind jetzt im power_tree-Schematic verdrahtet; +5V_RAIL traegt das globale +5V-Flag.**
**Date:** 2026-06-22 (urspr.) / 2026-06-27 (Amendment)

> **Amendment 2026-06-30 (r18.73 — Cells digital, ADR-0013 abgelöst):** Die unten
> dominierende Prämisse „die 5 Hall-Sensoren (~17,5 mA) sind der echte Idle-
> Stromfresser" **gilt nicht mehr** — die Hall-Sensoren wurden entfernt, die Cells
> sind jetzt digitale Switches am MCP23017. Damit entfällt die Begründung für
> einen **dedizierten Hall-Load-Switch (`U9`/`HALL_VDD`, Variante C unten)**; das
> ist obsolet. **Unverändert gültig** bleibt der **Haupt-Aus-Schiebeschalter
> (`SW_PWR` + Load-Switch `U_PWR`)** aus dem Amendment 2026-06-27 — er gated die
> gesamte 3V3-Domäne (MCU + MCP + 2× PCA9685/LEDs + LCD), Lader davor. Wo unten
> „Halls" als 3V3-Last steht: ersatzlos streichen; der Rest der Last-Liste bleibt.

## Amendment 2026-06-27 — Haupt-Power-Gate per Schiebeschalter („dunkel, aber lädt")

User-Entscheidung: **ein physischer Schiebeschalter als echtes Aus** — Gerät im
Aus dunkel, **lädt aber bei eingestecktem USB weiter.** Der Schalter sitzt
bewusst **NICHT** im Akku-Hochstrompfad, sondern auf einer **Enable-Leitung**
(signal-level, µA).

**Topologie (entschieden, single-sheet `power_tree`):**
- Neuer **Load-Switch `U_PWR`** (TPS22918-Klasse, 5,5 V/2 A) auf dem **LDO-
  Eingang**: `+5V_RAIL → U_PWR(VIN→VOUT) → +5V_SW → AP7361C-LDO-VIN`.
  Gated damit die **gesamte 3V3-Domäne** (MCU + die 17,5 mA Hall-Sensoren +
  beide PCA9685/LEDs + LCD) auf einmal. Der Class-D-Amp (an +5V) geht über
  seinen `AMP_nSHDN`-Pulldown selbst in Shutdown → dunkel.
- **Schiebeschalter `SW_PWR`** steuert `U_PWR.ON`: `+5V_RAIL → SW_PWR → ON`,
  `R_PWR_PD` (100 k) zieht `ON` low (Default AUS). EN-Referenz = `+5V_RAIL`
  (immer da, sobald USB **oder** Akku) — nicht +3V3 (das gibt's erst nach dem
  Switch).
- **Lader (U7 MCP73831) bleibt unberührt** — hängt am Akku/USB, nicht hinter
  `U_PWR` → **lädt im Aus weiter.** ✓ User-Anforderung erfüllt.

**Verhältnis zur ursprünglichen Variante C:** Das ist ein **Haupt-Aus**, nicht
der Hall-only-Soft-Sleep von unten. Der Hall-Gate/STANDBY-Soft-Sleep (PE2/
`LSW_EN`, firmware-gesteuert) kann **zusätzlich** später als Auto-Sleep-Schicht
kommen — orthogonal. Für „echtes Aus" reicht jetzt `SW_PWR`.

**Neue Teile (final, gewählt):** `U_PWR` **TPS22918DBVR** SOT-23-6 (C131941) ·
`SW_PWR` **MST-12D18G3** Right-Angle SMD Slide-Switch (C49023766, FP
`field_ambience:SW_MST-12D18_SlideSwitch_RA` im Repo) · `R_PWR_PD` 100 k 0603
(C25803) · `C_PWR_SW` 10 µF 0805 (C15850). Alles in `power_tree_sheet`.

### Drop-in-Spec (Pin-Ebene — beim Schaltbild-Aufbau in ~10 min einzeichnen)

**`U_PWR` = TPS22918DBVR (SOT-23-6), Pinout DS-verifiziert:**

| Pin | Name | Verbindung |
|---|---|---|
| 1 | VIN | **`+5V_RAIL`** (der Knoten, der heute U5/LDO-VIN speist) |
| 2 | GND | GND |
| 3 | ON | **`PWR_ON`-Knoten** (von `SW_PWR` + `R_PWR_PD`, s.u.) |
| 4 | CT | ~~NC~~ **r18.81-Korrektur (TI SLVSD76C):** Pin 4 heisst **CT** (Slew-Rate-Cap); floaten ist DS-erlaubt (= schnellster Anstieg) — floatet im Schematic (no_connect-Pin), optionaler CT-Cap = Soft-Start nachruestbar |
| 5 | QOD | ~~no-connect (interne Quick-Discharge)~~ **r18.81-Korrektur:** floaten wuerde die Quick-Discharge laut DS DEAKTIVIEREN — QOD ist deshalb **an VOUT gebunden** (DS-Option 2, interner R entlaedt +5V_SW im Aus aktiv) |
| 6 | VOUT | **`+5V_SW`** |

**Reroute:** `U5` (LDO) **VIN** von `+5V_RAIL` → **`+5V_SW`** umhängen. Das ist
die *einzige* Änderung an einem bestehenden Netz.

**`SW_PWR` (MST-12D18G3, SPDT als SPST genutzt):** Common-Pol → `PWR_ON`;
ein Throw → `+5V_RAIL` (zweiter Throw = NC). **`R_PWR_PD`** 100 k von `PWR_ON`
→ GND (Default AUS beim Einschalten). **`C_PWR_SW`** 10 µF von `+5V_SW` → GND.

**Verhalten:** Schalter AN → `PWR_ON`=+5V_RAIL → U_PWR leitet
`+5V_RAIL→+5V_SW→LDO→+3V3` (gesamte Logik an). Schalter AUS → `PWR_ON` low →
`+5V_SW` tot → 3V3-Domäne dunkel. **Lader U7 hängt vor U_PWR (an VBUS/Akku) →
lädt im Aus weiter.** Verbrauch im Aus = nur U_PWR/Boost-Quiescent.

> Der untenstehende Variante-C-Text bleibt als History/Option für den späteren
> Firmware-Auto-Sleep stehen.

---

## Kontext

User-Frage (2026-06-22): *„brauchen wir eigentlich einen on off switch? oder
wie löst man das intelligent? weil das macht die pcb komplexer."*

Heute (r18.45) hat die PCB:
- **kein** dediziertes Power-Switch-Bauteil
- `SW_BOOT` als USB-DFU-Boot-Taster (Service-Bohrung im Bottom-Case)
- `SW11` als Reset-Taster (Service-Bohrung)
- Akku-Speisung dauerhaft am Boost (TPS61089) + LDO (AP7361A-33ER)
- 5× DRV5056A4 Hall-Sensoren (Cells) — laut Datenblatt **3.5 mA pro Stück
  typisch** im aktiven Zustand → **~17.5 mA permanent** alleine durch die
  Cells. Das ist Faktor 100× über STM32H7-STANDBY und der eigentliche
  Sleep-Strom-Killer.

Ohne Sleep-Architektur: ~25–30 mA Idle-Strom → bei 2000 mAh Akku
≈ 70 Stunden bis leer. Das ist zu wenig für "in der Tasche", aber zu viel
fürs "permanente Lagern".

## Optionen

| Variante | Panel-Bauteil | PCB-Komplexität | Idle (Sleep) | Lagern |
|---|---|---|---|---|
| A. **Hard slide switch** an VBAT | 1× Slide-Switch sichtbar | trivial | 0 µA | Switch off |
| B. **Soft latch P-MOSFET + Button** | dedizierte Taste | +5 Bauteile | <100 µA bei MCU+Halls aus | OK |
| C. **No switch, MCU STANDBY + Load-Switch für Halls** | nichts neues | +3 Bauteile | <100 µA | JST trennen |
| D. **No switch, naive STANDBY (Halls bleiben an)** | nichts | 0 | ~20 mA | Akku leer in 4 Tagen |

## Entscheidung

**Variante C.**

Begründung — in Reihenfolge des Wichtigseins:
1. **Kein neues sichtbares Bauteil** — keine zusätzliche Panel-Bohrung, kein
   Gehäuse-Slot, der versehentlich in der Tasche geschaltet wird (klassisches
   Problem bei Geräten mit Slide-Switch).
2. **Der echte Stromfresser im Idle sind die Halls, nicht der MCU.** Den MCU
   in STANDBY zu bringen löst nur 1 % des Problems; die Halls müssen mit. Ein
   Load-Switch-IC (z. B. **TI TPS22918DBVR**, SOT-23-6, ~10–15 ct) gated den
   3V3-Pfad zu den 5 Hall-Sensoren über einen MCU-GPIO.
3. **Existierender `SW_BOOT`-Taster bekommt Dual-Funktion** — short = Wake,
   long-press 3 s = Sleep. Boot in den DFU-Modus bleibt erhalten (BOOT0-Pin
   wird beim Reset gelesen — der MCU sieht den langen Druck *nach* dem Boot,
   also beißt sich nichts).
4. **Wake-Sources im STANDBY:** `SW_BOOT`-Taster (WKUP-Pin), Encoder-Drehung
   (separater WKUP-Pin), USB-Plug (VBUS-Detect über `USB_VBUS`-Pin). Cells
   können *nicht* wecken, weil ihre Versorgung im Sleep gegated ist — das ist
   eine bewusste Entscheidung (sonst kein Sleep möglich).
5. **Für echtes Lagern (Wochen+):** Akku-JST trennen. Der JST-PH-Stecker ist
   im Inneren erreichbar — gewollte Friction-Schwelle, damit nicht
   versehentlich "weg".

## Hardware-Delta zur Umsetzung

**Neuer Block (3 Bauteile):**

| Ref | Teil | Footprint | Funktion |
|---|---|---|---|
| `U9` | TPS22918DBVR | SOT-23-6 (KiCad-Standard) | Load-Switch 3V3 → HALL_VDD |
| `R_LSW_EN` | 100 kΩ 0603 | KiCad-Standard | Pull-Down auf `LSW_EN` (default OFF beim Boot) |
| `C_HALL` | 1 µF X7R 0603 | KiCad-Standard | Lokale Ausgangs-Cap auf HALL_VDD |

**Schematic-Änderung im Generator:**
- Neuer Subblock im `power_tree.kicad_sch`-Sheet (oder im `mcp.kicad_sch`, je nach Layoutfluss)
- Hall-Sensoren ziehen Versorgung jetzt von Net `HALL_VDD` statt direkt `+3V3`
- `LSW_EN` als neuer Net vom MCU-GPIO (freier Pin — wird in der MCU-Pin-Allocation gewählt)

**Firmware-Delta:**
- `power.c` (neu): `power_enter_sleep()` / `power_check_wake_sources()` über STM32 HAL
- `main_h743.c`: Long-press 3 s auf `SW_BOOT` → `power_enter_sleep()`
- Auto-Sleep nach N Minuten ohne Input (N = 5 default, im Menü als "Sleep timer" exposed)
- Beim Wake: `HAL_GPIO_WritePin(LSW_EN, HIGH)`, dann Hall-Init nach Spec-Zeit (TPS22918 enable-to-ON-Delay ~1 ms typ).

## Consequences

**Positiv:**
- 100× weniger Idle-Strom (~25 mA → <100 µA realistisch)
- Akku-Lebensdauer im Sleep: 2000 mAh / 0.1 mA ≈ **800 Tage** auf dem Papier
  (real begrenzt durch LiPo-Selbstentladung ~2–3 %/Monat → ~6–9 Monate praktisch)
- Kein neues Panel-Bauteil, kein hässlicher Switch
- BOM-Erhöhung: 1 IC + 1 R + 1 C ≈ 20 ct

**Negativ:**
- 1 zusätzliche MCU-GPIO belegt (LSW_EN) — Pin-Allocation muss prüfen, ob frei
- Hall-Sensoren brauchen ~1 ms Settling nach Wake — User merkt nicht, weil
  Tap-Erkennung sowieso mehrere ms läuft
- Cells können nicht wecken (siehe oben — bewusst)

**Bewusst nicht geändert:**
- `SW11` (Reset) bleibt — orthogonale Funktion
- Charging-Pfad (MCP73831) bleibt **immer** aktiv unabhängig vom Sleep-State,
  damit USB-Plug-Wake funktioniert und das Gerät am Strom liegend immer lädt

## Implementation specs (r18.57 — bereit für Generator-Emission)

Diese Sektion macht das Hardware-Delta so konkret, dass entweder ich es
generator-seitig emittieren kann ODER der User es in 30 min in KiCad-GUI
direkt einzeichnet. Beides funktioniert.

### Pin-Allocation final

| Signal | MCU-Pin | LQFP-Pin-Nr. | Begründung |
|---|---|---|---|
| `LSW_EN` (load-switch enable, MCU-out) | **PE2** | 1 | bisher unbelegt (SPEC §5 verifiziert), nahe PE3/4/5/6 (SAI-Block), kurzer Trace zu U9 möglich; nicht auf einem WKUP-Pin nötig (LSW_EN ist Output) |
| `SW_BOOT` (wake-Eingang im Sleep) | **PA0** ODER bestehender BOOT0 (Pin 94) | 22 / 94 | STM32H743 WKUP1 sitzt auf PA0 — ist heute frei. **TODO:** entscheiden ob (a) SW_BOOT physisch an PA0 statt BOOT0 für Wake-Kapabilität, oder (b) zweiter parallel-Tap auf PA0 zusätzlich zum BOOT0-Pull-Up. (a) ist sauberer aber Schematic-Re-Wire; (b) ist ein zusätzlicher Pin + Pull-Up. **Empfehlung: (a)** — SW_BOOT bekommt eine zweite Funktion (Wake + DFU) über den gleichen physischen Taster, MCU liest beide Pins beim Boot |

### U9 Symbol + Footprint

| Property | Wert | Quelle |
|---|---|---|
| MPN | **TPS22918DBVR** | TI 1-A Single-Channel Load Switch |
| LCSC | **C131941** (verfügbar, Stock typ. 50k+) | LCSC product detail TBD |
| Package | SOT-23-6 (DBV) | TI DS — siehe DS0008 |
| Footprint | `Package_TO_SOT_SMD:SOT-23-6` (KiCad-Standard) | KiCad 9 lib |
| Symbol | `Power_Switch:TPS22918DBV` (KiCad-9 lib) — **UNVERIFIED**, ggf. generic `Power_Switch:Load_Switch_Generic_6pin` oder eigener Symbol-Eintrag in `field_ambience.kicad_sym` | KiCad-Lib |

**Pinout SOT-23-6** (gegen TI-Datasheet TPS22918 Page 3 verifiziert):

| Pin | Name | Funktion | Anschluss in unserem Design |
|---|---|---|---|
| 1 | VIN | Source | +3V3 (vom AP7361A LDO-Output) |
| 2 | GND | — | GND |
| 3 | ON | Enable input (active high) | `LSW_EN` (vom MCU PE2) |
| 4 | NC | not connected | floating (oder GND-Tie für EMI) |
| 5 | QOD | Quick-Output-Discharge (auto on ON-falling) | NC oder GND-Tie |
| 6 | VOUT | Switched output | `HALL_VDD` (zur Hall-Sensoren-Versorgung) |

### Schematic-Position-Vorschlag

Auf **Sheet 1 power_tree.kicad_sch**, unterhalb des bestehenden AP7361A-LDO-
Blocks und neben dem Bulk-Cap. Konkrete KiCad-Koordinaten:

```
  U9       (x=70, y=140)   — SOT-23-6 body, rotation 0
  R_LSW_EN (x=58, y=140)   — 100 kΩ 0603 Gate-Pull-Down (LSW_EN→GND)
  C_HALL   (x=85, y=140)   — 1 µF X7R 0603 lokale Output-Cap auf VOUT

  Hier-Labels (rechte Seite, x=115):
    LSW_EN     (y=140) shape="input"    — MCU GPIO PE2 → ON pin
    HALL_VDD   (y=144) shape="output"   — VOUT → mcp_sheet
```

### Schematic-Änderung im `mcp_sheet`

Im `mcp_sheet.kicad_sch` (Sheet 4) die fünf DRV5056A4 Hall-Sensoren von
`+3V3` umrouten auf neuen Net `HALL_VDD`:

```python
# Existing (Generator Z. ~3340, 5× pro Cell):
#   wires.append(wire(hall_x, vcc_y, +3V3_rail_x, vcc_y))
# Neu:
#   wires.append(wire(hall_x, vcc_y, HALL_VDD_rail_x, vcc_y))
```

Plus: in `mcp_sheet` eine neue Hier-Label `HALL_VDD` als `shape="input"` an
der linken Sheet-Seite anlegen (zur Verbindung mit Sheet 1).

### Schematic-Änderung im `stm32h743_sheet`

`LSW_EN` als neue Hier-Label am Sheet-Output (rechts), verbunden zu Pin 1
(PE2). Ein einzelner Wire vom PE2-Pin zur Hier-Label.

### Top-Level-`root_sheet`-Wiring

Die drei neuen Hier-Labels (`LSW_EN`, `HALL_VDD`) müssen am Top-Sheet als
**inter-sheet wires** zwischen den drei Sheets gezogen werden:

- `LSW_EN`: stm32h743 (out) → power_tree (in)
- `HALL_VDD`: power_tree (out) → mcp (in)

### BOM-Eintrag (r18.57 nach Implementation)

In `BOM_MASTER.md` §3 Power-Tree neue Zeile (DIRECT NACH dem AP7361A-LDO):

```markdown
| **U9** TPS22918DBVR SOT-23-6 (Load-Switch +3V3 → HALL_VDD; ADR-0016 sleep-gate) | TI TPS22918DBVR | C131941 (LCSC) | Package_TO_SOT_SMD:SOT-23-6 | KiCad-Standard | Standard-Lib-3D |
```

Plus `R_LSW_EN` (100 kΩ 0603) und `C_HALL` (1 µF X7R 0603) als reguläre
Passives in der bestehenden 0603-Sammlung mit Refdesignation.

### Implementation-Auswahl

Zwei Wege das Hardware-Delta einzuziehen — beide gleichwertig:

**(a) User in KiCad-GUI** (~30 min, sichtbarer Verifikations-Loop):
1. `power_tree.kicad_sch` öffnen, U9 + R + C + Hier-Labels nach obigen
   Koordinaten platzieren
2. `mcp.kicad_sch` öffnen, 5 Hall-VCC-Wires von `+3V3` auf `HALL_VDD`
   umrouten, neue Hier-Label am Sheet-Eingang
3. `stm32h743.kicad_sch` öffnen, PE2 mit Hier-Label `LSW_EN` versehen
4. Top-Sheet `field_ambience.kicad_sch` Inter-Sheet-Wires zeichnen
5. ERC ausführen — sollte 0 Errors für die neuen Nets zeigen

**(b) Ich im Generator** (in Folge-PR, wenn der User die Generator-Änderung
will): `generate_kicad_project.py` Python-Emission für U9-Block in
`power_tree_sheet()`, Hall-Wire-Re-Route in `mcp_sheet()`, PE2-Label in
`stm32h743_sheet()`, plus inter-sheet wiring in `root_sheet()`. Erfordert
4 Funktion-Anpassungen ~+80 Zeilen Python.

**Empfehlung:** (a) zuerst — schneller, der User sieht beim KiCad-Layout
sowieso die Sheets, und die Generator-Emission wird dann im nächsten
Generator-Update einfach als "diese KiCad-GUI-Änderung nachvollziehen"
kodiert mit gesicherten Koordinaten.

## Offene Items (entkoppelt)

1. **Load-Switch-PN final wählen:** TPS22918 als Default; LCSC-Stock prüfen
   vor Bestellung. Alternativen: FPF2895 (Fairchild) oder TPS22965 (kleiner)
   falls TPS22918 NRND wird.
2. **WKUP-Pin-Routing prüfen:** STM32H743 hat 6 WKUP-Pins (WKUP1..6) — der
   `SW_BOOT`-Taster muss an einen davon liegen, damit Wake aus STANDBY
   funktioniert. Empfehlung oben: PA0 = WKUP1.
3. **PE2-Default-State beim Boot:** Pull-Down (R_LSW_EN 100kΩ) garantiert
   `LSW_EN=LOW` ⇒ Halls OFF bis Firmware sie aktiv schaltet. MCU muss
   nach Boot `LSW_EN=HIGH` setzen damit Cells reagieren (1ms Settling).

## Related

- ADR-0013 — Cell-Velocity (Hall-Sensoren als Eingabe; jetzt sleep-gated)
- SPEC §5 — MCU-Pin-Allocation (muss aktualisiert werden um `LSW_EN`)
- BOM_MASTER §3/§7 — Power-Tree (`U9` einfügen)
- `field-ambience-current/firmware-c-next/src/hal_h743/` — neue `power.c`
