"""MCP23017 16-bit I/O expander driver (MicroPython).

Field Ambience usage:
- GPA0-4: cell switches (input, pull-up, IRQ-on-change)
- GPA5:   PCM_XSMT output (HIGH = PCM5102A un-muted)
- GPB0-4: modifier switches (input, pull-up, IRQ-on-change)
- INTA mirrored (covers both ports), active-low, to Pico GP22

Register map assumes IOCON.BANK = 0 (power-on default).
"""

# Register addresses (BANK=0)
_IODIRA = 0x00
_IODIRB = 0x01
_IPOLA = 0x02
_GPINTENA = 0x04
_GPINTENB = 0x05
_INTCONA = 0x08
_INTCONB = 0x09
_IOCON = 0x0A
_GPPUA = 0x0C
_GPPUB = 0x0D
_GPIOA = 0x12
_GPIOB = 0x13
_OLATA = 0x14


class MCP23017:
    def __init__(self, i2c, addr):
        self.i2c = i2c
        self.addr = addr
        self._olata = 0x00  # output latch shadow for port A

    def _w(self, reg, val):
        self.i2c.writeto_mem(self.addr, reg, bytes((val & 0xFF,)))

    def _r(self, reg):
        return self.i2c.readfrom_mem(self.addr, reg, 1)[0]

    def probe(self):
        """Return True if the device ACKs on the bus."""
        try:
            return self.addr in self.i2c.scan()
        except OSError:
            return False

    def init_field_ambience(self):
        """Configure for Field Ambience:
        GPA0-4 input+pullup, GPA5 output, GPB0-4 input+pullup, INTA mirror.
        """
        # IODIR: 1=input, 0=output.
        #   Port A: GPA0-4 input(1), GPA5 output(0), GPA6-7 input(1) → 0b1101_1111
        self._w(_IODIRA, 0b11011111)
        #   Port B: GPB0-4 input, rest input → all input
        self._w(_IODIRB, 0xFF)

        # Pull-ups on the switch inputs (GPA0-4) + jack-detect (GPA6) + GPB0-4.
        # GPA6 (bit 6) = line-out jack-detect: idle (no plug, switch closed)
        # pulls to GND via the jack switch; plug inserted (switch open) → pull-up.
        self._w(_GPPUA, 0b01011111)   # GPA0-4 + GPA6
        self._w(_GPPUB, 0b00011111)

        # Input polarity normal (we treat pressed = logic 0 in software)
        self._w(_IPOLA, 0x00)

        # Interrupt-on-change for switches + jack-detect
        self._w(_GPINTENA, 0b01011111)   # GPA0-4 + GPA6
        self._w(_GPINTENB, 0b00011111)
        # INTCON 0 = compare against previous value (interrupt-on-change mode)
        self._w(_INTCONA, 0x00)
        self._w(_INTCONB, 0x00)

        # IOCON: MIRROR=1 (bit6) so INTA reflects both ports; INTPOL=0 (active
        # low, bit1); ODR=0 (push-pull). SEQOP left enabled (auto-increment).
        self._w(_IOCON, 0b01000000)

        # PCM_XSMT (GPA5) default LOW = PCM5102A muted at boot. The R_XSMT_PD
        # pull-down also holds it low, but set the latch explicitly.
        self._olata = 0x00
        self._w(_OLATA, self._olata)

    def read_ports(self):
        """Read GPIOA and GPIOB. Returns 16-bit value (GPB<<8 | GPA).
        Reading also clears the interrupt condition.
        """
        a = self._r(_GPIOA)
        b = self._r(_GPIOB)
        return (b << 8) | a

    def set_xsmt(self, on):
        """Drive GPA5 (PCM_XSMT): on=True → HIGH (un-muted)."""
        if on:
            self._olata |= (1 << 5)
        else:
            self._olata &= ~(1 << 5)
        self._w(_OLATA, self._olata)
