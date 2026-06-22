# ADR-0016: Power & Sleep Architecture — kein dedizierter On/Off-Switch

**Status:** PROPOSED
**Date:** 2026-06-22

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

## Offene Items (vor Implementierung)

1. **MCU-Pin-Allocation:** freien GPIO für `LSW_EN` festlegen. Kandidat: ein
   bisher unbelegter PE2/PB0/PB1 — gegen SPEC §5 prüfen.
2. **Load-Switch-PN final wählen:** TPS22918 ist mein Default; Alternativen
   sind FPF2895 (Fairchild) oder TPS22965 (kleinerer Footprint). Entscheidung
   nach LCSC-Stock-Check.
3. **WKUP-Pin-Routing prüfen:** STM32H743 hat 6 WKUP-Pins (WKUP1..6) — der
   `SW_BOOT`-Taster muss an einen davon liegen, damit Wake aus STANDBY
   funktioniert.

## Related

- ADR-0013 — Cell-Velocity (Hall-Sensoren als Eingabe; jetzt sleep-gated)
- SPEC §5 — MCU-Pin-Allocation (muss aktualisiert werden um `LSW_EN`)
- BOM_MASTER §3/§7 — Power-Tree (`U9` einfügen)
- `field-ambience-current/firmware-c-next/src/hal_h743/` — neue `power.c`
