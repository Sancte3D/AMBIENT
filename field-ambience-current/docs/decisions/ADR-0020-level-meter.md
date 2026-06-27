# ADR-0020: Live-Level-Meter — 8 LEDs via 2. PCA9685 (U10)

**Status:** ACCEPTED (User-Direktive, 2026-06-27)
**Date:** 2026-06-27

## Kontext

User-Wunsch: eine **Reihe kleiner LEDs als Live-Pegel-Anzeige** — beim Abspielen
soll man den Audio-Pegel live ausschlagen sehen (wie der dB-Meter in After
Effects). Stil: **OP-1 Field — wenige LEDs, blau + weiß**, minimalistisch.

## Entscheidung

- **8 LEDs** in einer Reihe als Level-Meter: **6 blau** (Pegel, `LED_VU1–6`) +
  **2 weiß** (Peak, `LED_VU7–8`). Vorwiderstand je 390 Ω an +5V-Anode.
- Getrieben von einem **zweiten PCA9685 (`U10`)** auf Channels 0-7.
  Begründung: U6 (1. PCA9685) ist mit 16/16 Kanälen voll (5 Modifier + 10 Cell +
  1 Backlight) — kein Kanal frei. U10 ist dasselbe Teil (C2678753), gleicher
  TSSOP-28, gleiches Treiber-Pattern.
- **I²C-Adresse 0x41** (A0=+3V3, A1-A5=GND) am **gleichen I²C-Bus** wie U6 (0x40)
  und MCP23017 (0x20). Lokale Labels `I2C_SDA`/`I2C_SCL` joinen den bestehenden
  Bus → **keine neuen MCU-Pins, keine Root-/PINMAP-Änderung.**
- **Firmware-getrieben:** der STM32 erzeugt/verarbeitet das Audio bereits — die
  Firmware rechnet RMS/Peak pro Frame aus dem Buffer und schreibt die PWM-Werte
  per I²C an U10. Damit ein **echtes Live-Meter** (nicht nur Knopfstellung); beim
  Volume-Drehen kann dieselbe Reihe die Lautstärke zeigen.
- **Kein analoger Meter-IC** (LM3915-Klasse): der bräuchte einen Analog-Abgriff
  des Ausgangs; das Signal liegt aber digital im MCU → firmware-getrieben ist
  hier richtig und billiger.

## Warum nicht MCU-GPIO direkt

8 freie GPIOs gäbe es (~50 frei). Aber: das verteilt die Änderung über
NETS + stm32h743_sheet + root_sheet + PINMAP (4 Stellen, leicht zu desynchen)
und gäbe ohne Timer-Zuordnung nur An/Aus. Der 2. PCA9685 ist **self-contained**
(nur `mcp_sheet`), spiegelt ein erprobtes Block-Muster (U6) und gibt 16-Kanal-
Hardware-PWM für weiche Übergänge. +1 IC (~$1), ~$ vernachlässigbar.

## Offen

- **Blaue 0603-LED LCSC:** `NEEDS-VERIFY` (Anti-Guess — nicht erfunden). Steht in
  der NO-LCSC-Liste von `export_jlc_bom.py`; vor JLC-Order eine verifizierte
  blaue 0603 (z.B. KENTO KT-0603B) fixieren. Weiß = C965808 (verifiziert).
- **PCB-Position der LED-Reihe:** Layout-Entscheidung (User: „kann man später
  definieren"). Unter Display / über Volume / seitlich — siehe
  `MECHANICAL_REQUIREMENTS.md` bei Layout-Start.
- **Firmware:** Level-Berechnung (RMS/Peak) + U10-I²C-Treiber + Mapping auf 8
  Segmente (log/dB-Skala). Eigener Schritt, kein PCB-/HW-Blocker.
- **GUI-ERC:** der neue U10-Block muss wie das ganze Board einmal durch KiCad-9-
  ERC (Blocker B3) — `kicad-cli` ist nicht in der Build-Umgebung.

## Related
- `kicad/generate_kicad_project.py` — `mcp_sheet()` U10-Block (r18.66)
- BOM_MASTER §4 (U10) + §9 (VU-LEDs)
- ADR-0008 — LED-System (Status-LEDs, getrennt vom Meter)
