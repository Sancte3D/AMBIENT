"""EC11 quadrature encoder bank (MicroPython, polled).

Handles 4 encoders, each with A/B quadrature phases + a push switch.
Hardware RC debounce (100nF + 10k = 1ms) handles contact bounce; firmware
adds a quadrature state machine and per-detent accumulation.

EC11 produces 4 quadrature transitions per mechanical detent. We accumulate
sub-steps and emit ±1 per full detent so the host sees one step per click.

poll() returns a list of (encoder_id, delta) and (encoder_id, "push", down)
events to emit. Call it frequently (≥500 Hz) from the main loop.
"""

from machine import Pin

# Quadrature transition table indexed by (prev_AB << 2) | curr_AB.
# Valid CW transitions → +1, CCW → -1, invalid/no-change → 0.
_QUAD = (0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0)
_DETENT = 4  # sub-steps per mechanical click


class EncoderBank:
    def __init__(self, encoder_defs):
        """encoder_defs: iterable of (name, pin_a, pin_b, pin_sw, enc_id, dir).
        `dir` is +1 or -1 to flip rotation sense (see config.py note)."""
        self.encs = []
        for name, pa, pb, psw, eid, direction in encoder_defs:
            a = Pin(pa, Pin.IN, Pin.PULL_UP)
            b = Pin(pb, Pin.IN, Pin.PULL_UP)
            sw = Pin(psw, Pin.IN, Pin.PULL_UP)
            prev_ab = (a.value() << 1) | b.value()
            self.encs.append({
                "name": name, "id": eid, "dir": direction,
                "a": a, "b": b, "sw": sw,
                "prev_ab": prev_ab, "accum": 0,
                "sw_state": 1,            # 1 = released (pull-up), 0 = pressed
                "sw_stable": 1, "sw_count": 0,
            })

    def poll(self):
        """Return a list of event tuples:
          ("enc", enc_id, delta)         for each completed detent
          ("push", enc_id, down_bool)    for each debounced switch edge
        """
        events = []
        for e in self.encs:
            # --- rotary ---
            ab = (e["a"].value() << 1) | e["b"].value()
            if ab != e["prev_ab"]:
                idx = (e["prev_ab"] << 2) | ab
                e["accum"] += _QUAD[idx]
                e["prev_ab"] = ab
                if e["accum"] >= _DETENT:
                    e["accum"] = 0
                    events.append(("enc", e["id"], e["dir"]))
                elif e["accum"] <= -_DETENT:
                    e["accum"] = 0
                    events.append(("enc", e["id"], -e["dir"]))

            # --- push switch (simple counted debounce) ---
            raw = e["sw"].value()
            if raw == e["sw_stable"]:
                e["sw_count"] = 0
            else:
                e["sw_count"] += 1
                if e["sw_count"] >= 3:        # 3 consecutive polls confirm
                    e["sw_stable"] = raw
                    e["sw_count"] = 0
                    # raw 0 = pressed (active low) → down=True
                    events.append(("push", e["id"], raw == 0))
        return events
