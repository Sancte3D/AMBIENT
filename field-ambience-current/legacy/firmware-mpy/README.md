# Field Ambience — Pico 2 Firmware (MicroPython)

Hardware-I/O-Firmware für den Pico 2 (RP2350). Liest Buttons/Encoder,
treibt das OLED, steuert die Audio-Power-Sequenz, und spricht über UART
mit dem Raspberry Pi (der SuperCollider + die Python-Bridge fährt).

## Architektur

```
Buttons (MCP23017) ─I²C─┐
4× EC11 Encoder ────────┤→ Pico 2 ─UART(JSON)→ Pi-Bridge → OSC → SuperCollider
OLED SSD1322 ──SPI──────┘     │
PAM8403 /SHDN /MUTE ←─────────┘  (Power-Sequencing)
PCM5102A XSMT ←── MCP23017 GPA5
```

Der Pico ist bewusst "dumm": er liest Hardware → schickt Events; empfängt
fertige Display-Inhalte → zeichnet sie. Die Menü-Logik liegt in der Bridge
(`../field_ambience_bridge.py`).

## Dateien

| Datei | Zweck |
|---|---|
| `main.py` | Entry-Point, Main-Loop, UART-Protokoll, Power-Sequencing |
| `config.py` | Pin-Mapping (muss zum KiCad-Schematic SPEC §5 passen) |
| `mcp23017.py` | I²C-Treiber für den Button-Expander |
| `ssd1322.py` | OLED-Treiber (256×64, 4-bit greyscale, framebuf) |
| `encoders.py` | 4× Quadratur-Decoder + Switch-Debounce |

## Flashen

1. **MicroPython auf den Pico 2 flashen** (einmalig):
   - Pico 2 mit gedrücktem BOOTSEL-Button per USB-C anstecken → erscheint als
     USB-Massenspeicher `RP2350`
   - MicroPython-UF2 für RP2350 von https://micropython.org/download/RPI_PICO2/
     herunterladen
   - UF2 auf das `RP2350`-Laufwerk ziehen → Pico rebootet mit MicroPython

2. **Firmware-Dateien kopieren** (mit `mpremote` oder Thonny):
   ```bash
   pip install mpremote
   cd firmware
   mpremote cp config.py mcp23017.py ssd1322.py encoders.py main.py :
   mpremote reset
   ```
   `main.py` startet automatisch beim Boot.

   Alternativ mit **Thonny** (GUI): alle 5 Dateien öffnen → "Save as" →
   "Raspberry Pi Pico" → unter gleichem Namen speichern.

## Erstes Bring-up (Reihenfolge wichtig)

1. Nur Pico 2 bestückt (ohne restliche Hardware): Firmware bootet, OLED bleibt
   leer (kein OLED), Status-LED blinkt (wartet auf Pi). Über USB-Serial-REPL
   siehst du ggf. `MCP23017 not found` — normal ohne MCP.
2. Mit OLED: Splash "FIELD AMBIENCE / waiting for Pi..." sollte erscheinen.
   Wenn das Bild horizontal verschoben ist → `_COL_START` in `ssd1322.py`
   anpassen (±1 Spalte = ±4 Pixel).
3. Mit MCP23017 + Buttons: Tasten drücken → über USB-Serial-REPL siehst du
   die JSON-Events (wenn kein Pi dranhängt). Z.B. `{"event":"cell","id":1,"down":true}`.
4. Encoder drehen: `{"event":"enc","id":1,"delta":1}`. **Wenn ein Knopf
   verkehrt herum zählt** → in `config.py` das `dir`-Feld dieses Encoders von
   `1` auf `-1` setzen.
5. Mit Pi + Bridge + SuperCollider: Pico erkennt den Pi an der ersten
   `{"set":"display",...}`-Message, fährt die Audio-Power-Sequenz hoch
   (/SHDN → 50ms → /MUTE + XSMT), Status-LED wird dauerhaft an.

## Protokoll (JSON-Zeilen über UART, 115200 8N1)

**Pico → Pi:**
```json
{"event":"hello","version":"fam-pico-1.0"}
{"event":"cell","id":1,"down":true}        // Cell 1-5
{"event":"mod","id":1,"down":true}         // 1=SHIFT 2=HOLD 3=DRONE 4=GENERATE 5=CLEAR
{"event":"enc","id":1,"delta":1}           // Encoder 1=Drive 2=Bright 3=Display 4=Volume
{"event":"push","id":3,"down":true}        // Encoder-Switch
{"event":"log","msg":"..."}
```

**Pi → Pico:**
```json
{"set":"display","key":"C","mode":"Ion","prog":"Slow","vibe":"Warm",
 "chord":"C E G B","param":"KEY","value":"C","bar_pct":50,"mode_ui":"nav"}
{"set":"amp","enabled":0}                  // 0 = mute+shutdown (z.B. Jack-Detect)
```

## Sicherheit / Power-Sequencing

- **Boot-Default**: Amp aus (/SHDN LOW), gemuted (/MUTE LOW), DAC stumm
  (XSMT LOW). Hardware-Pull-Downs (R_SHDN_PD, R_MUTE_PD, R_XSMT_PD) halten
  das auch wenn die Firmware abstürzt.
- **Enable-Sequenz** (Pi erkannt): /SHDN HIGH → 50ms → /MUTE HIGH + XSMT HIGH.
- **Watchdog**: Wenn der Pi >5s keine Message schickt (Crash/Reboot), wird
  Audio wieder gemutet bis der Pi zurück ist.
- **Exception-Handler**: Crash in `main()` → letzter Versuch /SHDN+/MUTE LOW.

## Hinweise

- Der Pico 2 muss auf dem PCB so verbaut sein, dass die USB-Daten (D+/D-) den
  BOOTSEL-Flash-Pfad bedienen (SPEC v0.6 Variante A). Beim Prototyp mit
  Pin-Header-THT: Pico-Modul-eigener USB-Anschluss oder SWD via J4.
- Jack-Detect (`{"event":"jack",...}`) ist im Protokoll vorgesehen, aber das
  Gerät hat (Stand SPEC v0.6) keine Kopfhörerbuchse → kein Jack-Pin alloziert.
  Der `{"set":"amp",...}`-Command funktioniert trotzdem (z.B. für künftige
  Erweiterung).
