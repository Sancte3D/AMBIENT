"""Field Ambience — Pico 2 (RP2350) main firmware.

Role: hardware I/O bridge between the physical panel and the Raspberry Pi
(which runs SuperCollider + the Python bridge). The Pico:
  - reads 10 buttons via MCP23017 (I²C, interrupt-driven)
  - reads 4 EC11 encoders (polled quadrature)
  - drives the SSD1322 256×64 OLED (SPI)
  - exchanges JSON-line messages with the Pi over UART
  - owns the audio power-sequencing (PAM8403 /SHDN, /MUTE; PCM5102A XSMT)

Protocol (matches field_ambience_bridge.py PicoBridge):
  Pico → Pi:  {"event":"hello","version":...}
              {"event":"cell","id":1..5,"down":bool}
              {"event":"mod","id":1..5,"down":bool}
              {"event":"enc","id":1..4,"delta":±1}
              {"event":"push","id":1..4,"down":bool}
              {"event":"log","msg":...}
  Pi → Pico:  {"set":"display", ...display fields...}
              {"set":"amp","enabled":0|1}

Boot state is audio-OFF + muted. Audio is enabled only after the Pi proves
it is alive (first valid command received), following the SPEC §8 corrected
sequence: /SHDN HIGH first, settle, then /MUTE HIGH + XSMT HIGH.
"""

import json
from machine import Pin, I2C, SPI, UART
from time import sleep_ms, ticks_ms, ticks_diff

import config as cfg
from mcp23017 import MCP23017
from ssd1322 import SSD1322
from encoders import EncoderBank


# ---------------------------------------------------------------------------
# Audio power sequencing (SPEC v0.6.3-r6 §8)
# ---------------------------------------------------------------------------
class AudioPower:
    def __init__(self, mcp):
        self.mcp = mcp
        self.n_shdn = Pin(cfg.PIN_AMP_nSHDN, Pin.OUT, value=0)  # 0 = shutdown
        self.n_mute = Pin(cfg.PIN_AMP_nMUTE, Pin.OUT, value=0)  # 0 = muted
        self.enabled = False
        self.speakers_on = False
        # XSMT held LOW by the MCP latch (set in mcp.init) + R_XSMT_PD.

    def enable(self):
        """Full enable: wake amp, un-mute Class-D + DAC. Correct anti-pop order."""
        if self.enabled:
            return
        self.n_shdn.value(1)             # 1: wake PAM8403, references settle
        sleep_ms(cfg.PWR_SHDN_TO_MUTE_MS)
        self.n_mute.value(1)             # 2: un-mute Class-D output
        self.mcp.set_xsmt(True)          # 3: un-mute PCM5102A DAC
        self.enabled = True
        self.speakers_on = True

    def disable(self):
        """Full shutdown: mute DAC + Class-D, shut down chip. (watchdog/power-off)"""
        self.mcp.set_xsmt(False)         # 1: mute DAC first
        self.n_mute.value(0)             # 2: mute Class-D
        sleep_ms(cfg.PWR_SHDN_TO_MUTE_MS)
        self.n_shdn.value(0)             # 3: shut down chip
        self.enabled = False
        self.speakers_on = False

    def speakers(self, on):
        """Toggle ONLY the PAM8403 speaker amp. The PCM5102A DAC (and thus the
        line-out tap) stays live. Used for jack-detect: plug headphones in →
        speakers off, line-out keeps playing."""
        if on == getattr(self, "speakers_on", False):
            return
        if on:
            self.n_shdn.value(1)
            sleep_ms(cfg.PWR_SHDN_TO_MUTE_MS)
            self.n_mute.value(1)
        else:
            self.n_mute.value(0)         # mute Class-D
            sleep_ms(cfg.PWR_SHDN_TO_MUTE_MS)
            self.n_shdn.value(0)         # shut down speaker amp (DAC stays on)
        self.speakers_on = on


# ---------------------------------------------------------------------------
# OLED rendering — draws the display dict the bridge sends us.
# ---------------------------------------------------------------------------
class Display:
    def __init__(self, oled):
        self.oled = oled
        self.last = {}

    def splash(self, line1, line2=""):
        o = self.oled
        o.fill(0)
        o.text(line1, 4, 24, 0x0F)
        if line2:
            o.text(line2, 4, 36, 0x08)
        o.show()

    def render(self, d):
        """d = display dict from the bridge ({"set":"display", ...})."""
        o = self.oled
        o.fill(0)

        # Header row: KEY / MODE / PROG / VIBE (8x8 font, 32 cols)
        key = d.get("key", "?")
        mode = d.get("mode", "?")
        prog = d.get("prog", "?")
        vibe = d.get("vibe", "?")
        o.text("KEY:%s MODE:%s" % (key, mode), 0, 0, 0x0F)
        o.text("PRG:%s VIBE:%s" % (prog, vibe), 0, 10, 0x0A)

        # Chord readout
        chord = d.get("chord", "")
        o.text("CHD:" + chord[:27], 0, 22, 0x0C)

        o.hline(0, 33, 256, 0x06)

        # Selected parameter + value (larger emphasis via position)
        param = d.get("param", "")
        value = d.get("value", "")
        mode_ui = d.get("mode_ui", "nav")
        marker = "=" if mode_ui == "edit" else ">"
        o.text("%s %-10s %10s" % (marker, param, value), 0, 40, 0x0F)

        # Progress bar for the selected parameter
        pct = max(0, min(100, int(d.get("bar_pct", 0))))
        bar_w = int(252 * pct / 100)
        o.rect(0, 52, 254, 9, 0x06)
        if bar_w > 0:
            o.fill_rect(1, 53, max(1, bar_w), 7, 0x0F)

        # Mode indicator bottom-right is implicit in marker; show audio state
        o.show()
        self.last = d


# ---------------------------------------------------------------------------
# UART JSON line protocol
# ---------------------------------------------------------------------------
class Link:
    def __init__(self, uart):
        self.uart = uart
        self._rx = b""

    def send(self, obj):
        try:
            self.uart.write(json.dumps(obj) + "\n")
        except Exception:
            pass  # never let a serial hiccup crash the main loop

    def poll_lines(self):
        """Return a list of parsed JSON objects received since last call."""
        out = []
        n = self.uart.any()
        if n:
            self._rx += self.uart.read(n)
            while b"\n" in self._rx:
                line, self._rx = self._rx.split(b"\n", 1)
                line = line.strip()
                if not line:
                    continue
                try:
                    out.append(json.loads(line))
                except Exception:
                    pass  # ignore malformed lines
        # Guard against unbounded growth if no newline ever arrives
        if len(self._rx) > 1024:
            self._rx = b""
        return out


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    led = Pin(cfg.PIN_STATUS_LED, Pin.OUT, value=0)

    # --- buses ---
    uart = UART(cfg.UART_ID, baudrate=cfg.UART_BAUD,
                tx=Pin(cfg.PIN_UART_TX), rx=Pin(cfg.PIN_UART_RX),
                timeout=0, timeout_char=2)
    link = Link(uart)

    i2c = I2C(cfg.I2C_ID, sda=Pin(cfg.PIN_I2C_SDA), scl=Pin(cfg.PIN_I2C_SCL),
              freq=cfg.I2C_FREQ)

    spi = SPI(cfg.SPI_ID, baudrate=cfg.SPI_BAUD, polarity=0, phase=0,
              sck=Pin(cfg.PIN_OLED_SCK), mosi=Pin(cfg.PIN_OLED_MOSI),
              miso=Pin(cfg.PIN_OLED_MISO))

    # --- peripherals ---
    oled = SSD1322(spi,
                   cs=Pin(cfg.PIN_OLED_CS, Pin.OUT, value=1),
                   dc=Pin(cfg.PIN_OLED_DC, Pin.OUT, value=0),
                   res=Pin(cfg.PIN_OLED_RES, Pin.OUT, value=1))
    disp = Display(oled)
    disp.splash("FIELD AMBIENCE", "waiting for Pi...")

    mcp = MCP23017(i2c, cfg.MCP23017_ADDR)
    mcp_ok = mcp.probe()
    if mcp_ok:
        mcp.init_field_ambience()
    else:
        link.send({"event": "log", "msg": "MCP23017 not found at 0x20"})

    mcp_int = Pin(cfg.PIN_MCP_INT, Pin.IN, Pin.PULL_UP)

    amp = AudioPower(mcp)
    amp.disable()  # explicit: boot muted + shutdown

    encoders = EncoderBank(cfg.ENCODERS)

    # --- button state mirror (active-low; True = pressed) ---
    cell_state = [False] * 5
    mod_state = [False] * 5
    jack_state = [False]  # mutable holder so the nested fn can update it

    def _jack_inserted(val):
        bit = (val >> cfg.MCP_JACKDET_BIT) & 1
        # HIGH (bit=1) = plug inserted when JACK_DETECT_ACTIVE_HIGH
        return (bit == 1) if cfg.JACK_DETECT_ACTIVE_HIGH else (bit == 0)

    def read_buttons():
        if not mcp_ok:
            return
        val = mcp.read_ports()  # 16-bit GPB<<8|GPA, also clears INT
        # Cells: GPA0-4 → bits 0-4. Pressed = bit low.
        for i, bit in enumerate(cfg.MCP_CELL_BITS):
            pressed = ((val >> bit) & 1) == 0
            if pressed != cell_state[i]:
                cell_state[i] = pressed
                link.send({"event": "cell", "id": i + 1, "down": pressed})
        # Modifiers: GPB0-4 → bits 8-12. id 1..5.
        for i, bit in enumerate(cfg.MCP_MOD_BITS):
            pressed = ((val >> bit) & 1) == 0
            if pressed != mod_state[i]:
                mod_state[i] = pressed
                link.send({"event": "mod", "id": i + 1, "down": pressed})
        # Jack-detect: GPA6. Plug in → mute speakers locally + notify Pi.
        inserted = _jack_inserted(val)
        if inserted != jack_state[0]:
            jack_state[0] = inserted
            amp.speakers(not inserted)   # plug in → speakers OFF (line-out stays)
            link.send({"event": "jack", "inserted": inserted})

    # Send hello so the bridge logs our firmware version.
    link.send({"event": "hello", "version": cfg.FW_VERSION})

    last_rx_ms = ticks_ms()
    last_led_ms = ticks_ms()
    led_on = False
    pi_alive = False

    # Read initial button state without emitting (avoid phantom press at boot)
    if mcp_ok:
        val = mcp.read_ports()
        for i, bit in enumerate(cfg.MCP_CELL_BITS):
            cell_state[i] = ((val >> bit) & 1) == 0
        for i, bit in enumerate(cfg.MCP_MOD_BITS):
            mod_state[i] = ((val >> bit) & 1) == 0
        jack_state[0] = _jack_inserted(val)

    while True:
        now = ticks_ms()

        # --- encoders (poll fast) ---
        for ev in encoders.poll():
            if ev[0] == "enc":
                link.send({"event": "enc", "id": ev[1], "delta": ev[2]})
            else:  # push
                link.send({"event": "push", "id": ev[1], "down": ev[2]})

        # --- buttons (only when MCP signals an interrupt) ---
        if mcp_ok and mcp_int.value() == 0:
            read_buttons()

        # --- incoming commands from the Pi ---
        for msg in link.poll_lines():
            last_rx_ms = now
            if not pi_alive:
                pi_alive = True
                amp.enable()              # Pi is up → safe to enable audio
                led.value(1)              # solid LED = running
            kind = msg.get("set")
            if kind == "display":
                try:
                    disp.render(msg)
                except Exception:
                    pass
            elif kind == "amp":
                # The bridge's amp command controls ONLY the speaker amp
                # (e.g. jack-detect echo). The PCM5102A DAC + line-out stay
                # live. Full mute is reserved for the watchdog / power-off.
                amp.speakers(bool(msg.get("enabled")))

        # --- watchdog: Pi silent too long → mute for safety ---
        if pi_alive and ticks_diff(now, last_rx_ms) > cfg.PI_WATCHDOG_MS:
            amp.disable()
            pi_alive = False
            disp.splash("FIELD AMBIENCE", "Pi link lost...")

        # --- status LED: blink while waiting, solid when running ---
        if not pi_alive:
            if ticks_diff(now, last_led_ms) > 300:
                last_led_ms = now
                led_on = not led_on
                led.value(1 if led_on else 0)

        sleep_ms(1)  # ~1 kHz loop; encoders + buttons stay responsive


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        # Last-resort: try to mute the amp and show the error.
        try:
            Pin(cfg.PIN_AMP_nSHDN, Pin.OUT, value=0)
            Pin(cfg.PIN_AMP_nMUTE, Pin.OUT, value=0)
        except Exception:
            pass
        raise
