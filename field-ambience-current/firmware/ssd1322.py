"""SSD1322 256×64 OLED driver (MicroPython, 4-wire SPI, 4-bit greyscale).

Targets the ER-OLEDM032-1W (3.2", 256×64) module per SPEC §6.
Uses framebuf.GS4_HMSB which exactly matches the SSD1322 GDDRAM layout
(2 pixels per byte, high-nibble = left pixel), so the framebuffer can be
streamed directly with no per-pixel conversion.

The visible 256-pixel window maps to controller column addresses 0x1C..0x5B
(each column address spans 4 horizontal pixels). This offset is correct for
the common 256×64 SSD1322 modules; if your specific module shows the image
shifted horizontally, adjust _COL_START.
"""

import framebuf
from time import sleep_ms

# Column address window for a 256-px-wide panel (4 px per column address).
_COL_START = 0x1C
_COL_END = 0x5B            # 0x1C + (256/4) - 1
_ROW_START = 0x00
_ROW_END = 0x3F            # 64 rows


class SSD1322:
    WIDTH = 256
    HEIGHT = 64

    def __init__(self, spi, cs, dc, res):
        self.spi = spi
        self.cs = cs
        self.dc = dc
        self.res = res
        # 256*64 px @ 4bpp = 8192 bytes
        self.buffer = bytearray(self.WIDTH * self.HEIGHT // 2)
        self.fb = framebuf.FrameBuffer(
            self.buffer, self.WIDTH, self.HEIGHT, framebuf.GS4_HMSB
        )
        self.cs.value(1)
        self.dc.value(0)
        self.reset()
        self.init_display()

    # ---- low-level ----
    def _cmd(self, c):
        self.dc.value(0)
        self.cs.value(0)
        self.spi.write(bytes((c,)))
        self.cs.value(1)

    def _data(self, buf):
        self.dc.value(1)
        self.cs.value(0)
        self.spi.write(buf)
        self.cs.value(1)

    def reset(self):
        self.res.value(1)
        sleep_ms(10)
        self.res.value(0)
        sleep_ms(50)
        self.res.value(1)
        sleep_ms(50)

    def init_display(self):
        self._cmd(0xFD); self._data(b"\x12")          # unlock command lock
        self._cmd(0xAE)                                # display off
        self._cmd(0xB3); self._data(b"\x91")          # clock divide / osc freq
        self._cmd(0xCA); self._data(b"\x3F")          # multiplex ratio = 64
        self._cmd(0xA2); self._data(b"\x00")          # display offset
        self._cmd(0xA1); self._data(b"\x00")          # display start line
        self._cmd(0xA0); self._data(b"\x14\x11")      # remap + dual COM
        self._cmd(0xB5); self._data(b"\x00")          # GPIO disabled
        self._cmd(0xAB); self._data(b"\x01")          # function: internal VDD
        self._cmd(0xB4); self._data(b"\xA0\xFD")      # display enhancement A
        self._cmd(0xC1); self._data(b"\x9F")          # contrast current
        self._cmd(0xC7); self._data(b"\x0F")          # master contrast = max
        self._cmd(0xB1); self._data(b"\xE2")          # phase length
        self._cmd(0xD1); self._data(b"\x82\x20")      # display enhancement B
        self._cmd(0xBB); self._data(b"\x1F")          # pre-charge voltage
        self._cmd(0xB6); self._data(b"\x08")          # pre-charge period
        self._cmd(0xBE); self._data(b"\x07")          # VCOMH
        self._cmd(0xA6)                                # normal display
        self._cmd(0xA9)                                # exit partial display
        self.show()                                    # clear before on
        self._cmd(0xAF)                                # display on

    def contrast(self, value):
        self._cmd(0xC1); self._data(bytes((value & 0xFF,)))

    def show(self):
        # Set column + row window, then stream the whole framebuffer.
        self._cmd(0x15); self._data(bytes((_COL_START, _COL_END)))
        self._cmd(0x75); self._data(bytes((_ROW_START, _ROW_END)))
        self._cmd(0x5C)                                # write RAM
        self._data(self.buffer)

    # ---- drawing convenience (delegates to framebuf) ----
    def fill(self, gs):
        self.fb.fill(gs & 0x0F)

    def text(self, s, x, y, gs=0x0F):
        self.fb.text(s, x, y, gs & 0x0F)

    def hline(self, x, y, w, gs=0x0F):
        self.fb.hline(x, y, w, gs & 0x0F)

    def rect(self, x, y, w, h, gs=0x0F):
        self.fb.rect(x, y, w, h, gs & 0x0F)

    def fill_rect(self, x, y, w, h, gs=0x0F):
        self.fb.fill_rect(x, y, w, h, gs & 0x0F)
