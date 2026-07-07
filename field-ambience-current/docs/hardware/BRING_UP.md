# Hardware Bring-Up Runbook — Field Ambience Prototyp

Systematischer Erst-Einschalt-Plan für die 5 Prototyp-Boards. **Reihenfolge
ist Sicherheit** — jede Stufe erst freigeben, wenn die vorige besteht. Nie
den MCU in einen kurzgeschlossenen Rail booten, nie den Speaker vor dem
Line-out testen.

> Quelle der Wahrheit für Pins/Nets: `PINMAP.md`, `bom_overview.html`,
> `SCHEMATIC_WALKTHROUGH.md`. Firmware-Boot: `src/hal_h743/main_h743.c`.

## Sicherheits-Grundregeln
- **Strombegrenzt einschalten** (Labornetzteil oder USB-Meter mit Limit).
  Steigt der Strom schlagartig → sofort trennen, Kurzschluss suchen.
- **Line-out (J8) vor Speaker** testen. Speaker leise anfangen.
- **Akku-Polung** vor dem ersten Stecken prüfen: **Pad 1 = + (BAT_PLUS)**
  (verifiziert r19.2) — Multimeter am Adafruit-Stecker, siehe
  `SOURCING_COMPLIANCE.md §2`. Verpolung tötet Boost + Charger sofort.
- Erst USB-Pfad, dann Akku-Pfad — nie beim ersten Mal beides gleichzeitig.

## Schnellreferenz
| Rail | Soll | Woher |
|---|---|---|
| `+5V_RAIL` (USB) | ~4,7 V | VBUS → F1 → D3B (SS34, ~0,3 V Drop) |
| `+5V_RAIL` (Akku) | ~4,97 V | TPS61089 Boost (121k-Divider) |
| `+5V_SW` | = +5V_RAIL | TPS22918 U_PWR, geschaltet von SW_PWR |
| `+3V3` | 3,30 V | AP7361C LDO (ganze 3V3-Domäne) |

| I²C @ 400 kHz | Adresse |
|---|---|
| MCP23017 (Cells GPA0-4, XSMT GPA5, Jack GPA6) | **0x20** |
| PCA9685 U6 (15 LEDs + Backlight) | **0x40** |

| Debug | Pin |
|---|---|
| SWD J4 (Tag-Connect TC2030) | SWDIO PA13 · SWCLK PA14 · SWO PB3 |
| NRST | Pin 14 (SW11 + 10k PU + 100 nF) |
| BOOT0 | Pin 94 (SW_BOOT + Pull-**down** → Flash-Boot) |
| HSE | Y1 8 MHz Crystal |

---

## Stufe 0 — Vor dem Strom (Ohmmeter, unbestromt)
- Sichtprüfung: Bestückung vollständig, Polung von Elkos/Dioden/USB-C/JST,
  keine Lötbrücken (besonders unter dem LQFP-100 + den QFN/SOT-Reglern).
- **Kurzschluss-Check** (Ohmmeter gegen GND, muss NICHT ~0 Ω sein):
  `+5V_RAIL`, `+5V_SW`, `+3V3`, `BAT_PLUS`.
- SW_PWR in **OFF**. Akku **nicht** gesteckt.

## Stufe 1 — USB an, Gerät AUS (strombegrenzt)
- USB-C rein, Limit ~200 mA. Erwartung (SW_PWR off, PWR_ON per 100k PD low):
  - `+5V_RAIL` ≈ 4,7 V (über D3B von VBUS).
  - `+5V_SW` = 0 V, `+3V3` = **0 V** (LDO aus — Load-Switch sperrt).
  - Charger MCP73831 aktiv; ohne Akku flackert/ruht STAT-LED (orange).
  - Ruhestrom klein, **kein Bauteil warm**. Wird etwas heiß → trennen.

## Stufe 2 — Power-On (SW_PWR ON)
- SW_PWR → ON: PWR_ON high → TPS22918 durch → AP7361C → 3V3-Domäne.
- Messen: `+5V_SW` ≈ +5V_RAIL; **`+3V3` = 3,30 V ±3 %**. Ripple sauber.
- LDO-Temperatur beobachten (~1,5–2,2 W, **kein Lüfter** — Kupfer + Vias
  müssen die Wärme tragen).
- Bei Fehler: LDO-Enable/Eingang, TPS22918 ON-Pin (PWR_ON), SW_PWR-Kontakte
  durchklingeln (r18.85: physisches Teil vor Fab prüfen).

## Stufe 3 — Akku- + Boost-Pfad
- USB trennen. Akku stecken (**Polung!**). SW_PWR ON.
- Boost TPS61089: `BAT_PLUS` 3,7 V → `+5V_RAIL` ≈ **4,97 V**, unter Last
  stabil (r18.80: Loop-Comp gefixt — kein Oszillieren). Mit Scope auf Ripple/
  Schwingen prüfen.
- USB wieder dazu: Charger lädt (STAT-LED orange) **während** der Boost
  läuft — Akkubetrieb + Laden gleichzeitig ist Absicht (ADR-0016).

## Stufe 4 — MCU-Lebenszeichen (SWD, noch keine Firmware)
- Tag-Connect TC2030 an J4, ST-Link/Debugger.
- **MCU-ID lesen** (STM32H743 erkennen). Nur ob der Kern antwortet.
- Bei „not found": `+3V3` am MCU, **NRST high** (SW11 nicht gedrückt),
  **BOOT0 LOW** (für Flash-Boot — verpolter Pull würde in den System-
  Bootloader booten; PINMAP sagt Pull-down, gegen das reale Board prüfen),
  HSE-Crystal (8 MHz) schwingt.

## Stufe 5 — Firmware flashen + Boot
- `field_ambience_h743.bin`/`.hex` via SWD flashen.
- Boot (`main_h743.c`): Caches → Clock 480 MHz (PLL3 → SAI 44,1 kHz) →
  OLED-Splash → mcp/enc → dsp/brain/engine → audio.
- **Erwartung: Display zeigt das Menü** (World-Slot). Schwarz? → SPI/J3-
  **Pin-Order** (per Kabel Draht-für-Draht, s. Sourcing-Doc), Backlight (Q2/
  LCD_BLK_PWM), 3V3 am Modul.
- **Async-DMA-Flush validieren (r19.12):** `oled_show_async()` streamt den
  Framebuffer per SPI1-TX-DMA (DMA2_Stream3, Prio 6) zeilen-pipelined, damit
  der Main-Loop während der ~29 ms nicht mehr blockiert. **Bench-Check:** Bild
  sauber/vollständig, kein Tearing/Sparkle/versetzte Zeilen, kein
  abgeschnittenes letztes Byte (CS-High im TxCplt). **Wenn es zickt**
  (Zeilen-Re-Enable-Glitch, CS-Timing): Fallback = im Refresh-Block von
  `main_h743.c` `oled_show_async()` → `oled_show()` (blocking, bewährt)
  tauschen — eine Zeile, sonst identisch. Alternativ Zeilen zu Chunks bündeln
  (weniger DMA-Re-Enables). Der Konverter (`oled_convert_row`) ist
  host-getestet; nur DMA-/Panel-Timing ist hier offen.

## Stufe 6 — I²C-Bus
- I²C-Scan: **0x20** (MCP23017) + **0x40** (PCA9685 U6) müssen ACKen.
- Fehlt eins: SDA/SCL-Pull-ups, Verdrahtung, 3V3 am IC.
- PCA9685 treibt die 15 Status-LEDs + Backlight — beim Boot ansprechbar.

## Stufe 7 — Eingaben
- **Cells** (Kailh Choc, GPA0-4): drücken → MCP-INT → LED + Ton-Trigger.
- **Modifier** (GPB): DRONE · HOLD · SHIFT · GENERATE · CLEAR.
- **4 Encoder** (EC11): drehen → Menü-Nav / DRIVE / BRIGHT / VOLUME; Push.

## Stufe 8 — Audio (die Mute-Sequenz ist heikel)
- **Line-out J8 zuerst** (Scope/Kopfhörer). SAI1 (PE4 LRCK / PE5 BCK /
  PE6 DOUT) → PCM5102A → J8. Ton sauber?
- Mute-Reihenfolge (SPEC §8.3): beim Start `/SHDN` (PB14) → `/MUTE` (PB15)
  → `XSMT` (MCP GPA5) lösen; beim Stop rückwärts. Kein Pop.
- **Jack-Detect** (MCP GPA6): Klinke rein → PAM8403 gemutet (Speaker aus),
  Line-out bleibt live. Raus → Speaker an.
- **Speaker (PAM8403) zuletzt**, leise anfangen.

## Stufe 9 — Batterie-UI + Laden
- `BAT_SENSE` (PA3, 100k:100k): Menü zeigt Akkustand plausibel.
- USB rein → USB-Glyph + Laden. SW_PWR off → 3V3 weg, **Charger lädt
  weiter** (ADR-0016).

## Stufe 10 — Abnahme
- Alle 4 Welten, alle 11 Menü-Slots, GENERATE-Autoplay.
- **Dauerlauf** ein paar Minuten: Temperatur (LDO/Boost), Stabilität, in
  Stille keine CPU-/Audio-Spikes (FTZ ist an — sollte sauber sein).
- Line-out + Speaker gegen die Host-Demos gegenhören.
- **WCET-/Block-Size-Sweep (P0.2):** das Worst-Case-Szenario aus
  `test/test_blocksize_sweep.c` (Drone + alle FX max + Trigger-Storm) auf dem
  Board fahren und pro Blockgröße (512→256→128→64) `audio_profiler_state()`
  auslesen: **`max_cycles`, `deadline_miss_count`, `clip_count`**. Gate
  (REALTIME_AUDIO_RULES §1): `peak_load < 0,60`, `deadline_miss_count == 0`.
  Host-seitig ist die Szene bereits als **stabil + bounded + 0 % Clipping bei
  jeder Blockgröße** verifiziert — offen ist nur die echte Zyklenzahl. Erst
  wenn 128 (oder 64) die Deadline hält, `AUDIO_BUFFER_FRAMES` senken.

---

## Wenn eine Stufe fehlschlägt
Nicht weiter — erst die Stufe grün. Häufige Erstfehler bei diesem Design:
- **3V3 fehlt trotz SW_PWR ON** → TPS22918 PWR_ON-Pfad / SW_PWR-Kontakt.
- **Boost schwingt** → sollte mit r18.80-Comp gefixt sein; Bestückung der
  Comp-R/C prüfen.
- **Display schwarz** → J3-Pin-Order (Vendor-Variation, per Kabel gemappt).
- **I²C tot** → Pull-ups / ein IC ohne 3V3.
- **Audio-Pop** → Mute-Sequenz-Reihenfolge in der Firmware vs Hardware.
- **Kein SWD** → BOOT0-Pegel / NRST / Crystal.
