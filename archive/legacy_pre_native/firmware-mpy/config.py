"""Field Ambience — Pico 2 (RP2350) Pin Mapping & Constants.

Per SPEC v0.6.3-r6 §5. These constants MUST match the KiCad schematic.
Changing a pin here without updating the schematic (and vice versa) is a
guaranteed hardware-firmware mismatch bug.

Verified against:
- Pico 2 / RP2350 datasheet pin functions
- field_ambience_pcb_SPEC_v0.6.md §5 (Pico pin allocation)
- MCP23017 §7 (button + XSMT assignment)
"""

# ---------------------------------------------------------------------------
# Firmware identity (sent in the "hello" event so the bridge can log it)
# ---------------------------------------------------------------------------
FW_VERSION = "fam-pico-1.0"

# ---------------------------------------------------------------------------
# UART0 → Raspberry Pi Zero 2 W (115200 baud, 8N1)
# Pico GP0 (TX) → Pi GPIO15 (RX, pin 10)
# Pico GP1 (RX) ← Pi GPIO14 (TX, pin 8) via R1 1k series
# ---------------------------------------------------------------------------
UART_ID = 0
PIN_UART_TX = 0
PIN_UART_RX = 1
UART_BAUD = 115200

# ---------------------------------------------------------------------------
# I²C1 → MCP23017 (400 kHz)
# ---------------------------------------------------------------------------
I2C_ID = 1
PIN_I2C_SDA = 2
PIN_I2C_SCL = 3
I2C_FREQ = 400_000
MCP23017_ADDR = 0x20          # A0=A1=A2=GND

# ---------------------------------------------------------------------------
# SPI0 → OLED SSD1322 (256×64, 4-wire SPI, up to 10 MHz)
# ---------------------------------------------------------------------------
SPI_ID = 0
PIN_OLED_SCK = 6              # SPI0 SCK
PIN_OLED_MOSI = 7            # SPI0 TX
PIN_OLED_MISO = 4            # SPI0 RX (unused by SSD1322, but reserved)
PIN_OLED_CS = 5
PIN_OLED_DC = 8
PIN_OLED_RES = 9
SPI_BAUD = 8_000_000

# ---------------------------------------------------------------------------
# 4× EC11 rotary encoders (A, B, push-switch). All Pico GPIO with RC debounce
# (100nF + 10k = 1ms) on the PCB; firmware adds quadrature decode.
#   index 0 = Drive, 1 = Brightness, 2 = Display(menu), 3 = Volume
# ---------------------------------------------------------------------------
#
# The `dir` field (+1 or -1) flips the rotation sense. The quadrature decoder
# is mathematically correct, but which physical direction reads as "+1" depends
# on the EC11 wiring and panel mounting — only verifiable on real hardware.
# If a knob feels backwards on first bring-up, flip its `dir` from 1 to -1.
ENCODERS = (
    # (name,      pin_A, pin_B, pin_SW, encoder_id, dir)
    ("drive",     10,    11,    12,     1,          1),   # EN1
    ("bright",    13,    14,    15,     2,          1),   # EN2
    ("display",   16,    17,    18,     3,          1),   # EN3 = menu controller
    ("volume",    19,    20,    21,     4,          1),   # EN4
)

# ---------------------------------------------------------------------------
# Misc Pico GPIO
# ---------------------------------------------------------------------------
PIN_MCP_INT = 22             # ← MCP23017 INTA (active low, MIRROR=1)
PIN_STATUS_LED = 26          # warm-white status LED via R19 820Ω
PIN_AMP_nSHDN = 27           # → PAM8403 /SHDN  (HIGH = chip enabled)
PIN_AMP_nMUTE = 28           # → PAM8403 /MUTE  (HIGH = un-muted)

# ---------------------------------------------------------------------------
# MCP23017 GPIO bit assignments (16-bit: GPA0..7 = bits 0..7, GPB0..7 = 8..15)
# ---------------------------------------------------------------------------
# GPA0-4 = CELL1-5 (inputs, internal pull-up, active low)
MCP_CELL_BITS = (0, 1, 2, 3, 4)      # bit index → cell id 1..5
# GPA5 = PCM_XSMT (output; HIGH = PCM5102A un-muted)
MCP_XSMT_BIT = 5
# GPA6 = JACK_DETECT (input; HIGH = headphone/line plug inserted)
# Set JACK_DETECT_ACTIVE_HIGH=False if your jack's switch polarity is inverted.
MCP_JACKDET_BIT = 6
JACK_DETECT_ACTIVE_HIGH = True
# GPB0-4 = modifier switches (inputs, pull-up, active low)
#   GPB0=SHIFT(id1) GPB1=HOLD(id2) GPB2=DRONE(id3) GPB3=GENERATE(id4) GPB4=CLEAR(id5)
MCP_MOD_BITS = (8, 9, 10, 11, 12)    # bit index → mod id 1..5

# Modifier IDs (must match field_ambience_bridge.py PicoBridge constants)
MOD_SHIFT = 1
MOD_HOLD = 2
MOD_DRONE = 3
MOD_GENERATE = 4
MOD_CLEAR = 5

# ---------------------------------------------------------------------------
# Power-sequencing timing (ms) — per SPEC v0.6.3-r6 §8 corrected order
# ---------------------------------------------------------------------------
PWR_RAIL_SETTLE_MS = 50      # wait after +5V/+3V3 stable before waking amp
PWR_SHDN_TO_MUTE_MS = 50     # wait after /SHDN HIGH before un-muting

# If no message from the Pi for this long, assume Pi crashed → mute audio.
PI_WATCHDOG_MS = 5000
