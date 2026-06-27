# ADR-0019: LED-System — Smart RGB (SK6812-Mini-V5) statt PCA9685 + diskrete LEDs

> **❌ SUPERSEDED / VERWORFEN (r18.64, 2026-06-23).** Dieser ADR basierte auf
> einer falschen Annahme: die LEDs sollten **nie** die World-Farbe spiegeln /
> RGB sein. Tatsächlich sind die LEDs **feste Mono-Status-Anzeigen**:
> 5 Modifier-LEDs (Shift=gelb, Hold=grün, Generate/Drone/Clear=weiß; Clear nur
> Klick-Blitz) + 10 Cell-LEDs (je 2 pro Cell, gelb+grün). Das ist exakt das
> bestehende **PCA9685 + 15-diskrete-LED-Design**, das im Generator bleibt.
> SK6812 wird nicht umgesetzt; PCA9685 bleibt. Das Dokument bleibt zur
> Nachvollziehbarkeit, ist aber **nicht** umzusetzen.

**Status:** ~~PROPOSED~~ → **SUPERSEDED / REJECTED (r18.64)** — diskretes
Mono-LED-System (PCA9685 + 15 LEDs) wird beibehalten.
**Date:** 2026-06-23
**Supersedes:** *(nichts — der bestehende PCA9685-Pfad bleibt unverändert)*

## Kontext

User-Frage (2026-06-23): *„wegen LEDs frage; wie kommunizieren die untereinander
wer wann wie ausleuchtet. Ich will nicht pro LED 3 resistoren, am besten sind
die schon integriert, und die kommunikation muss laufen ohne probleme, ohne
delay etc. wie macht man das am besten, oder machen die LEDs dann schon?"*

Ist-Stand (r18.60):
- `U6 PCA9685PW` treibt 16 PWM-Kanäle über I²C
- 10 davon belegt: 5 Cell-LEDs + 5 Modifier-LEDs
- Kanal 12 = Backlight-FET-Gate (Q2, ADR-0007 / Walkthrough §6)
- 5 Reserve-Kanäle
- Pro LED: 1 Strom-Begrenzungs-Widerstand (10 Widerstände gesamt)
- Refresh ~1500 Hz interner PWM, I²C-Update typisch alle 16 ms

Probleme:
1. **Layout-Overhead:** 10 PWM-Leitungen vom PCA9685 zu den LEDs an
   verschiedenen Positionen → komplexe Trace-Topologie
2. **Visuelle Einschränkung:** monochrome LEDs; die Akzentfarbe pro World
   (ADR-0015 Step 1) kann nicht auf den LEDs gespiegelt werden
3. **Komponentenzahl:** PCA9685 (~$1.50) + 10 R = realer Stückkosten-Anteil
   ohne klaren Mehrwert vs. moderne Smart-LEDs

## Optionen

| Option | Vorteile | Nachteile |
|---|---|---|
| A. **PCA9685 bleibt** (heute) | bewährt, niedriger Spitzenstrom, einfaches Sleep | viele Traces, kein RGB, keine World-Farb-Mirror-Möglichkeit |
| B. **WS2812B** klassisch (5050) | Standard, billig, jeder kann das | großes Package (5×5mm), mid-density |
| C. **SK6812-Mini-V5** ⭐ | klein (3,4×3,4mm), RGB(W) optional, WS2812-Protokoll-kompatibel, gut bei JLC bestückbar | etwas teurer als WS2812 (~5-15ct) |
| D. **APA102 (DotStar)** | echte SPI, kein Bit-Timing nötig, höhere Refresh | 2 Datenleitungen (Data+Clk), seltener bei JLC |

## Entscheidung

**Option C: SK6812-Mini-V5 (RGB)** für die 10 User-LEDs (5 Cells + 5 Modifier),
in Daisy-Chain. PCA9685 entfällt komplett.

### Begründung

1. **Vereinfachung schlägt alles**: 1 Datenleitung + 1 +5V + 1 GND statt
   PCA9685 + I²C-Pull-Ups + 10 PWM-Traces + 10 Widerstände.
2. **RGB für freie Visual-Coherence**: jede Cell + Modifier-LED kann die
   World-Akzentfarbe spiegeln (Tokyo blau, Coast aqua, Drive violett, After
   Hours amber). Klingt klein, ist gross — UI-Design wird konsistent.
3. **Latenz ist kein Problem**: 10 LEDs × 24 Bit × 1.25 µs/Bit = 300 µs
   serieller Stream + 50 µs Reset-Pause = **350 µs Frame-Update**. Atomic
   latch — alle LEDs übernehmen gleichzeitig. 60 fps trivial, 1 kHz möglich.
4. **JLC-Stock + Bestückbar**: SK6812-Mini-V5 ist Standard-Teil.
5. **Bauteil-Reduktion**: U6 + 10 Widerstände + I²C-Trace-Routing → 0.
   BOM-Kostenneutral oder leicht günstiger.

### Wie die LEDs „kommunizieren" (Klärung)

Sie kommunizieren **nicht** peer-to-peer. Das Daisy-Chain-Protokoll funktioniert
streng seriell:

```
MCU.MOSI ─► LED1.DIN ─► LED1.DOUT ─► LED2.DIN ─► LED2.DOUT ─► … LED10
```

1. MCU sendet einen 240-Bit-Stream (10 × 24 Bit RGB) auf einer einzigen
   Datenleitung mit präzisem 800-kHz-Timing.
2. **LED N verschluckt die ersten 24 Bit** (interpretiert als ihre eigenen
   RGB-Werte), und gibt **den Rest unverändert weiter** über ihren Data-Out-Pin
   zum nächsten LED.
3. Nach dem Stream + einer **50-µs-Reset-Pause** latchen alle 10 LEDs
   ihre neuen Werte **gleichzeitig**.

Es gibt keinen Bus-Konflikt, keine Adressierung, keine Race-Conditions —
die Position in der Kette **ist** die Adresse.

## Hardware-Delta

### Entfällt

| Ref | Teil | Sheet |
|---|---|---|
| `U6` | PCA9685PW | Sheet 4 `mcp_sheet` |
| Pull-Ups + Decoupling von U6 | — | Sheet 4 |
| 10 Strom-Begrenzungs-Widerstände der heutigen LEDs | — | Sheet 4 |

### Kommt rein

| Ref | Teil | Zweck | Footprint |
|---|---|---|---|
| `D_LED1..10` | **Opsco SK6812-Mini-V5** (10×) | RGB-Smart-LED in Daisy-Chain | `LED_SMD:LED_SK6812_PLCC4_3.4x3.4mm_P2.5mm` (KiCad-Standard) — **UNVERIFIED, gegen reales Modul prüfen** |
| `C_LED1..10` | 100 nF X7R 0603 pro LED | Lokales Decoupling am V+ Pin (Hersteller-Empfehlung) | 0603 |
| `R_DIN` | 470 Ω 0603 | Serien-Widerstand zwischen MCU-MOSI und LED1.DIN — Reflection-Dämpfung auf langer Daisy-Chain-Trace | 0603 |
| `U_BL_FET` | bestehender `Q2` 2N7002 bleibt | Backlight-FET wandert von PCA9685-Gate auf MCU-TIM-PWM-Pin | unverändert |
| (optional) Load-Switch | TPS22918DBVR | gated +5V zur LED-Kette im Sleep — analog ADR-0016. Sonst ziehen die 10 LEDs ~10 mA Ruhestrom dauerhaft | SOT-23-6 |

### MCU-Pin-Allocation

- **`LED_DATA`** (WS2812-Bitstream-Output) → **neue Pin-Reservierung** auf
  einem TIM/SPI-MOSI-fähigen Pin. Empfohlen: ein bisher unbelegter PA1/PA2 mit
  TIM2_CH2/CH3 — der STM32H7-Standard-Pattern ist *TIM_CH + DMA + Bit-Pattern-
  Table* (kein CPU-Polling, kein Bit-Bang). **TODO:** finalen Pin gegen SPEC §5
  prüfen.
- **`LCD_BL_PWM`** (Backlight) → wandert von PCA9685-Kanal 12 auf einen
  MCU-TIM-PWM-Pin. STM32H7 hat reichlich TIM-Kanäle frei; Kandidat: PE5/PE6
  oder PA8/PA9 (TIM1).
- I²C-Bus bleibt — wird weiterhin von MCP23017 genutzt.

### Stromverbrauch

| Szenario | Strom |
|---|---|
| Alle 10 LEDs Vollweiß (60 mA/Stück × 3 Farben) | 600 mA peak |
| Typische Animation (single colour, ~20 mA/LED) | 200 mA |
| Idle (alle off, aber +5V noch da) | ~10 mA (1 mA/LED Quiescent) |
| Sleep (Load-Switch off) | <1 µA |

→ **Load-Switch ist Pflicht** wenn ADR-0016 Sleep-Strom-Budget von <100 µA
gehalten werden soll. 2. Instance von TPS22918 ist trivial.

### Stromversorgung

LEDs ziehen aus **+5V_RAIL** (existiert bereits via Boost `U8` + Power-Path `Q1`).
Realer Verbrauch im typischen Use-Case ist gemütlich (200 mA peak); +5V_RAIL
ist auf 2 A ausgelegt (Polyfuse F1 3 A).

## Firmware-Delta

### Entfällt

- `src/leds.c` aktueller Code: PCA9685-spezifisch (PWM-Werte pro Kanal über I²C)

### Kommt rein

- **`src/leds.c` rewrite**: rendert in einen 30-Byte-Buffer (10 LEDs × 3 RGB-Byte)
- **`src/hal_h743/led_chain_h743.c` (neu)**: WS2812-Output via TIM-DMA-Pattern
  - Each WS2812 bit = 1 timer period
  - Bit-Pattern-Table mapped per WS2812-Bit auf zwei PWM-Duty-Werte (0.4 / 0.8
    of period) → "0" oder "1"
  - DMA blastet die ganze Tabelle in einem Schuss; kein CPU-Polling
  - Frame-Rate 60 Hz reicht für UI-Animationen
- Per-Cell-Color-Logic: lese die aktuelle World-Akzentfarbe (ist heute schon
  in `oled_color.c`) und tönte die Cell-LEDs entsprechend; Modifier-LEDs
  behalten ihre semantische Farbe (gelb=HOLD, grün=DRONE etc.) aber
  helligkeits-modulieren mit dem World-Akzent.
- **Pico-Bench:** WS2812 auf RP2350 via PIO-Programm (Standard-Beispiel im
  Pico-SDK). Zwei Pin-Defines.

### Backward-Compat

Der existierende `leds.c`-State (welche LED soll wie hell sein) bleibt
identisch — nur die Output-Schicht ändert sich.

## Open Items (für die Folge-PRs)

1. **MCU-Pin final wählen** für `LED_DATA` (TIM-CH-Pattern, gegen SPEC §5
   prüfen).
2. **MCU-Pin final wählen** für `LCD_BL_PWM` (TIM-Pattern, freier Pin).
3. **SK6812-Mini-V5 LCSC-PN** + Footprint-Drawing verifizieren (Anti-Guess
   per CLAUDE.md — kein Footprint-Assignment ohne Datasheet-Match).
4. **Load-Switch für LED_VDD** entscheiden: 2. TPS22918 (gleich wie U9 in
   ADR-0016), oder gemeinsamer mit HALL_VDD (Group-Switch)?
5. **Modifier-LED-Farben** finalisieren: heute fix gelb/rot/weiß/grün; mit
   RGB-Smart-LEDs sind sie frei wählbar.

## Consequences

**Positiv:**
- 1 IC + 10 Widerstände raus → BOM-Vereinfachung
- 10 PWM-Traces → 1 Daisy-Chain
- RGB pro LED → World-Akzent-Spiegel, UI-Coherence
- Atomic-Latch-Update, kein Sync-Problem
- WS2812-Protokoll ist Industry-Standard, gut dokumentierter STM32-Pattern

**Negativ:**
- 2. Load-Switch (TPS22918) für saubere Sleep-Strom-Budget
- Spitzenstrom 600 mA (nur Vollweiß) vs heute 100 mA — Polyfuse fängt das,
  aber Bulk-Caps am +5V_RAIL evtl. neu dimensionieren
- WS2812-Bit-Timing ist Hardware-spezifisch (STM32H7 TIM-DMA-Pattern; Pico
  PIO-Programm). Mehr Aufwand als „I²C-Slave schreiben"
- PCB-Layout-Wechsel — die LED-Positionen bleiben, aber die Trace-Topologie
  ändert sich grundsätzlich

**Bewusst nicht entschieden:**
- Modifier-LED-Farben (heute gelb/rot/weiß/grün — bleibt offen)
- Cell-LED-Brightness-Curve (gamma) — gleich wie heute, just gamma-2 auf
  RGB statt single-channel

## Related

- ADR-0007 — Dust-Mesh-Speakers (Q2 Backlight-FET bleibt)
- ADR-0008 — Cell-LED-XOR-Shift-Hold (LED-Semantik unverändert)
- ADR-0015 — Display-Akzent-Farbe (RGB-LEDs werden auf gleiche Akzent-Tabelle
  zugreifen)
- ADR-0016 — Power/Sleep (LED-Chain braucht eigenen Load-Switch im Sleep)
- ADR-0018 — Layer-Stack (WS2812-Daten-Trace auf Top-Layer, kurz, mit GND-Plane
  drunter)
- `field-ambience-current/docs/hardware/SCHEMATIC_WALKTHROUGH.md` §4
  (MCP+PCA-Sheet) — wird nach PCA9685-Removal stark schrumpfen
