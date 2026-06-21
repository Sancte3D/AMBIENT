# legacy_pico2/ — Pico-2-Ära-Schematics (bis v0.9 / r17)

**Archiviert 2026-06-11 (r18.5, H7-Migration Phase 3).** Diese Sheets werden
vom Generator NICHT mehr geschrieben und sind NICHT Teil des aktuellen
Projekts (`field_ambience.kicad_pro` referenziert sie nicht mehr):

| Datei | War | Ersetzt durch |
|---|---|---|
| `pico.kicad_sch` | Sheet 2: Raspberry Pi Pico 2 (RP2350-Modul) | `stm32h743.kicad_sch` (STM32H743VIT6 Bare-Chip per SPEC v0.7 §5) |
| `oled.kicad_sch` | Sheet 3: ER-OLEDM032 256×64 SSD1322, 16-Pin | `lcd.kicad_sch` (1.9" ST7789 320×170, 8-Pin-SPI per SPEC §6) |

Die Generator-Funktionen `pico_sheet()` / `oled_sheet()` existieren noch im
Code (als LEGACY markiert, nicht aufgerufen) — Historie + Diff-Referenz.
