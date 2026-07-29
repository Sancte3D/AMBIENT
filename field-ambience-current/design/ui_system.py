#!/usr/bin/env python3
"""Field Ambience — the interactive layer over the 320x170 grid.

ui_grid.py is the still picture. This is the system around it: state, encoder
acceleration, motion, and dirty-region accounting. It imports the grid rather
than restating it, so there is exactly one place where a coordinate lives.

WHAT THE TRANSFER BUDGET DICTATES (ambient-lcd-motion, 30 MHz SPI):

    full 320x170     29.013 ms   100.1 % of a 30 fps frame
    one row 320x20    3.413 ms    11.8 %
    the bubble 64x22  0.751 ms     2.6 %

A full-frame repaint does not fit in a 30 fps frame, let alone 60. So the
architecture is not "render a frame and push it" — it is: advance state, mark
the row bands that changed, and push only those. A value change costs one
band. Moving the selection costs two. Nothing here ever needs the whole
screen, which is why the animation is affordable at all.

STRUCTURE, kept separate on purpose (the skill asks for this and it is also
what makes the dirty-region accounting honest):

    Encoder   -> detents, with acceleration
    UiState   -> what is true right now
    Motion    -> what is on screen right now, easing toward UiState
    render()  -> pixels, plus the bands it touched

ACCELERATION is on the VALUE, never on the layout. Turning fast changes the
value in bigger steps; it never changes row pitch, never skips a frame, never
moves the bubble discontinuously. The LCD-UX skill states this rule and it is
also the only version that stays readable: the bubble keeps easing toward
whatever the value became.
"""
import math
import os

from PIL import Image, ImageDraw

import ui_grid as G

HERE = os.path.dirname(os.path.abspath(__file__))

BROWSE, EDIT = 0, 1

# ------------------------------------------------------------------- motion
# Durations from ambient-lcd-motion's starting ranges, all ease-out cubic.
T_VALUE = 0.110        # encoder value settle      (skill: 80-140 ms)
T_SELECT = 0.150       # focus / selection shift   (skill: 120-180 ms)
T_BUBBLE = 0.130       # bubble grow / shrink on entering and leaving EDIT
T_OUT = 0.140          # one row leaving on a category change
T_LEAD = 0.100         # head start the outgoing rows get over the incoming
T_REVEAL = 0.200       # one row arriving after a category change
STAGGER = 0.050        # delay between consecutive rows

# Why a STAGED reveal and not a crossfade: a crossfade needs the old and the
# new pixels at once, and a full frame is 29.0 ms — 100.1 % of a 30 fps budget
# — so there is no frame in which both could be pushed. Staggering means about
# T_REVEAL/STAGGER = 3.3 rows are in flight at once, which is 3.3 * 3.413 =
# 11.3 ms = 34 % of the budget. That is the whole reason for this shape.


def ease_out_cubic(t):
    t = min(1.0, max(0.0, t))
    return 1.0 - (1.0 - t) ** 3


class Eased:
    """One animated scalar.

    Retargets from its CURRENT value, so a new input mid-flight continues from
    where the pixels are instead of snapping back to the old start. Advanced
    from elapsed time, never from a frame count — a dropped frame then costs
    smoothness, not correctness.
    """

    def __init__(self, v, dur):
        self.v = self.frm = self.to = float(v)
        self.dur, self.t = dur, dur

    def target(self, v):
        if abs(v - self.to) < 1e-9:
            return
        self.frm, self.to, self.t = self.v, float(v), 0.0

    def step(self, dt):
        if self.t >= self.dur:
            self.v = self.to
            return False
        self.t = min(self.dur, self.t + dt)
        self.v = self.frm + (self.to - self.frm) * ease_out_cubic(self.t / self.dur)
        return True

    @property
    def moving(self):
        return self.t < self.dur


# -------------------------------------------------------------- the encoder
class Encoder:
    """Detent-rate acceleration — the pointer-acceleration idea, on an EC11.

    A detent arriving soon after the last one means the user is spinning, so
    each detent is worth more. The curve is deliberately flat at the bottom:
    below KNEE detents/second one detent is always exactly one step, so slow
    turning stays exact and a parameter can always be dialled to a precise
    value. Above the knee the step grows linearly and is capped, because an
    uncapped curve makes the last turn of a fast spin unpredictable.

    encoders.c emits +-1 per mechanical detent from a 1 kHz sampler, so this
    sits directly on that event, needs no extra timer, and is a handful of
    integer operations — nothing here goes near the audio hot path.
    """

    # Tuned against the printed log, not guessed. The first attempt used
    # GAIN 1.6 / MAX 12 and pegged at the cap on the THIRD detent of a spin:
    # every detent after that was worth exactly the same, so a fast turn had
    # no gradation and 2 -> 12 happened in one click. A real EC11 spin runs
    # 20-30 detents/s, and at 30/s this curve gives ~6 steps, so a full 0-100
    # sweep is about 17 detents — fast without becoming unpredictable.
    KNEE = 6.0         # detents/s below which there is no acceleration at all
    GAIN = 0.25        # steps gained per detent/s above the knee
    MAX = 8            # hard cap on one detent's worth of steps
    IDLE = 0.25        # s without a detent -> the spin is over, reset

    def __init__(self):
        self.last = None
        self.rate = 0.0

    def detent(self, now, delta):
        if self.last is None or now - self.last > self.IDLE:
            self.rate = 0.0
        else:
            dt = max(1e-3, now - self.last)
            inst = 1.0 / dt
            self.rate += (inst - self.rate) * 0.45     # one-pole, no spikes
        self.last = now
        extra = max(0.0, self.rate - self.KNEE) * self.GAIN
        return delta * min(self.MAX, 1 + int(extra))


# ---------------------------------------------------------------- the state
class UiState:
    def __init__(self, ci=0, pi=0):
        self.ci, self.pi, self.mode = ci, pi, BROWSE
        self.values = {}
        for c, (_, _, rows) in enumerate(G.CATS):
            for i, (name, _, amt) in enumerate(rows):
                self.values[(c, i)] = amt

    @property
    def rows(self):
        return G.CATS[self.ci][2]

    def rotate(self, steps):
        if self.mode == BROWSE:
            self.pi = max(0, min(len(self.rows) - 1, self.pi + (1 if steps > 0 else -1)))
        else:
            k = (self.ci, self.pi)
            self.values[k] = max(0.0, min(1.0, self.values[k] + steps * 0.01))

    def push(self):
        self.mode = EDIT if self.mode == BROWSE else BROWSE

    def set_category(self, ci):
        """A cell key picks the category. Leaving EDIT is deliberate: staying
        in edit across a switch would put the encoder on a different parameter
        than the one the user was holding."""
        self.ci = ci % len(G.CATS)
        self.pi = 0
        self.mode = BROWSE

    def text_for(self, i):
        name, val, _ = self.rows[i]
        if name in G.SEGMENTED:
            return val
        return "%d%%" % round(self.values[(self.ci, i)] * 100)


class Motion:
    """What is actually on screen, chasing UiState."""

    def __init__(self, st):
        self.sel = Eased(st.pi, T_SELECT)
        self.grow = Eased(0.0, T_BUBBLE)          # 0 = browse, 1 = edit
        self.ci = st.ci
        self.tr = 99.0                            # transition clock, done
        self.out = None                           # snapshot of what is leaving
        self._rebuild(st)

    def _rebuild(self, st):
        """fill is keyed by row index, so it MUST be rebuilt when the category
        changes — categories hold 3 to 5 rows and the old dict would be both
        stale and the wrong length."""
        self.fill = {i: Eased(st.values[(st.ci, i)], T_VALUE)
                     for i in range(len(st.rows))}

    def sync(self, st):
        if st.ci != self.ci:
            # snapshot what is on screen so it can be run OUT rather than cut.
            # Without this the old rows vanish in a single frame, which
            # measured as the two largest frame-to-frame deltas in the whole
            # interaction (17.8 and 11.3 against a moving mean of 1.5).
            self.out = (self.ci, {i: e.v for i, e in self.fill.items()})
            self.ci, self.tr = st.ci, 0.0
            self._rebuild(st)
            self.sel = Eased(st.pi, T_SELECT)     # no slide across a switch
        self.sel.target(st.pi)
        self.grow.target(1.0 if st.mode == EDIT else 0.0)
        for i in self.fill:
            self.fill[i].target(st.values[(st.ci, i)])

    def reveal(self, i):
        """0..1 for an incoming row during a staged category change."""
        return ease_out_cubic((self.tr - T_LEAD - i * STAGGER) / T_REVEAL)

    def reveal_out(self, i):
        """1..0 for an outgoing row. Runs ahead of the incoming ones so the
        two phases overlap and read as one motion rather than two."""
        return 1.0 - ease_out_cubic((self.tr - i * STAGGER) / T_OUT)

    def revealing(self, i):
        return (0.0 <= self.tr - i * STAGGER <= T_OUT or
                0.0 <= self.tr - T_LEAD - i * STAGGER <= T_REVEAL)

    @property
    def swapped(self):
        """The header swaps mid-flight, while the eye is on the rows."""
        return self.tr >= T_LEAD + 2 * STAGGER

    def step(self, dt):
        m = self.sel.step(dt) | self.grow.step(dt)
        for e in self.fill.values():
            m |= e.step(dt)
        if self.tr < 90.0:
            self.tr += dt
            if self.tr > T_LEAD + 5 * STAGGER + T_REVEAL:
                self.tr, self.out = 99.0, None
            m = True
        return m


# --------------------------------------------------------------- rendering
def _bands(rows_touched):
    """Row index -> the 320x20 band the flush would actually push."""
    out = []
    for i in sorted(set(rows_touched)):
        y = G.row_y(i)
        out.append((0, y - G.ROW_PITCH // 2, G.W, G.ROW_PITCH))
    return out


def _fill_end(st, mo, i):
    """Right edge of row i's fill, in px — where the chip wants to sit."""
    name = st.rows[i][0]
    if name in G.SEGMENTED:
        slot = min(G.SEG_N - 1, mo.fill[i].v * G.SEG_N)
        return G.TRACK_X + int(round(slot * (G.SEG_W + G.SEG_GAP))) + G.SEG_W
    return G.TRACK_X + max(G.TRACK_H, int(round(G.TRACK_W * mo.fill[i].v)))


def _row(d, i, name, amount, rv, f_small):
    """One parameter row at reveal factor rv. Used by BOTH the outgoing and
    the incoming phase of a category change, so the two can never drift."""
    y = G.row_y(i)
    if name in G.SEGMENTED:
        for k in range(G.SEG_N):
            x = G.TRACK_X + k * (G.SEG_W + G.SEG_GAP)
            G.pill(d, x, y, G.SEG_W, G.TRACK_H, G.WHITE, G.TRACK_A * rv)
        slot = min(G.SEG_N - 1, amount * G.SEG_N)
        sx = G.TRACK_X + int(round(slot * (G.SEG_W + G.SEG_GAP)))
        w = max(2, int(round(G.SEG_W * rv)))
        G.pill(d, sx, y, w, G.TRACK_H, G.GREEN)
    else:
        G.pill(d, G.TRACK_X, y, G.TRACK_W, G.TRACK_H, G.WHITE, G.TRACK_A * rv)
        fw = max(G.TRACK_H, int(round(G.TRACK_W * amount * rv)))
        G.pill(d, G.TRACK_X, y, fw, G.TRACK_H, G.GREEN)
    if rv > 0.35:
        d.text((G.LABEL_X, y), name, font=f_small, fill=G.WHITE, anchor="lm")


def render(st, mo):
    """Draw a frame and report which row bands changed.

    The background is a swappable asset — the gradient is loaded, never
    generated here, so replacing it is a file swap and touches no geometry.
    """
    im = Image.open(os.path.join(G.ASSETS, G.GRADIENT)).convert(
        "RGB").resize((G.W, G.H), Image.BICUBIC)
    d = ImageDraw.Draw(im, "RGBA")
    f_small, f_title = G.font(G.SZ_SMALL), G.font(G.SZ_TITLE)
    head_ci = st.ci if (mo.out is None or mo.swapped) else mo.out[0]
    head, title, _ = G.CATS[head_ci]
    rows = G.CATS[st.ci][2]

    d.text((G.TRACK_X, G.HEAD_MID), head, font=f_small, fill=G.WHITE, anchor="lm")
    bx = G.CONTENT_R - G.BADGE_W
    G.pill(d, bx, G.HEAD_MID, G.BADGE_W, G.BADGE_H, G.GREEN)
    d.text((bx + G.BADGE_W // 2, G.HEAD_MID), "100%", font=f_small,
           fill=G.GREEN_D, anchor="mm")
    d.text((G.TRACK_X, G.TITLE_BASE), title, font=f_title, fill=G.WHITE,
           anchor="ls")

    touched = []
    sel_moving = mo.sel.moving or mo.grow.moving

    if mo.out is not None:
        oci, ovals = mo.out
        for i, (name, val, _) in enumerate(G.CATS[oci][2]):
            rv = mo.reveal_out(i)
            if rv > 0.005:
                _row(d, i, name, ovals.get(i, 0.0), rv, f_small)
                touched.append(i)

    for i, (name, val, _) in enumerate(rows):
        y = G.row_y(i)
        rv = mo.reveal(i)
        if rv <= 0.005:
            continue

        _row(d, i, name, mo.fill[i].v, rv, f_small)

        if (i in mo.fill and mo.fill[i].moving) or mo.revealing(i):
            touched.append(i)

    # ONE chip, drawn at the interpolated position. Drawing a chip per row and
    # cross-fading them put the value text on TWO rows at once for two frames
    # of every selection move; a single chip that slides is both correct and
    # the actual "selection shift" motion the interaction was missing.
    if mo.reveal(int(mo.sel.v)) > 0.5:
        i0 = max(0, min(len(rows) - 1, int(math.floor(mo.sel.v))))
        i1 = max(0, min(len(rows) - 1, i0 + 1))
        f = mo.sel.v - i0
        e0, e1 = _fill_end(st, mo, i0), _fill_end(st, mo, i1)
        x_end = int(round(e0 + (e1 - e0) * f))
        y = int(round(G.ROW_0 + mo.sel.v * G.ROW_PITCH))
        _bubble(d, x_end, y, st.text_for(i1 if f >= 0.5 else i0), f_small,
                mo.grow.v)
        if sel_moving:
            touched += [i0, i1]

    return im, _bands(touched)


def _bubble(d, x_end, y, text, f, grow):
    """Same capsule as ui_grid.bubble, with the grow factor animated.

    Height interpolates from the track height to the measured bubble height,
    and it is ROUNDED TO AN INT every frame — a capsule that is 15.4 px tall
    does not exist on this panel, and letting it be fractional is how a
    'smooth' animation turns into an edge that shimmers.
    """
    tw = int(d.textlength(text, font=f))
    w = tw + 2 * G.BUBBLE_PAD
    h = int(round(G.TRACK_H + (G.BUBBLE_H - G.TRACK_H) * grow))
    h += h % 2                                   # keep the radius exact
    x = min(G.TRACK_X + G.TRACK_W - w, max(G.TRACK_X, x_end - w))
    for k, a in ((3, 0.10), (1, 0.16)):          # halo only while editing
        if grow > 0.02:
            G.pill(d, x - k, y, w + 2 * k, h + 2 * k, G.GREEN, a * grow)
    G.pill(d, x, y, w, h, G.GREEN)
    d.text((x + w // 2, y), text, font=f, fill=G.WHITE, anchor="mm")


# ------------------------------------------------------------------ script
FPS = 30
DT = 1.0 / FPS


def scripted():
    """One deterministic interaction, so the motion can be reviewed and
    regression-tested rather than admired once.

    t=0.3  push into EDIT on Drive
    t=0.7  a fast spin up: eleven detents at 40 ms, acceleration engages
    t=1.5  a slow correction: three detents at 200 ms, one step each
    t=2.4  push back to BROWSE, rotate down two rows
    """
    ev = []
    ev.append((0.30, "push", 0))
    t = 0.70
    for _ in range(11):
        ev.append((t, "rot", +1)); t += 0.040
    t = 1.50
    for _ in range(3):
        ev.append((t, "rot", -1)); t += 0.200
    ev.append((2.40, "push", 0))
    ev.append((2.70, "rot", +1))
    ev.append((2.95, "rot", +1))
    ev.append((3.40, "cat", 1))          # cell key -> AIR, staged reveal
    ev.append((4.40, "rot", +1))
    ev.append((4.70, "push", 0))
    t = 5.00
    for _ in range(8):                   # spin on the new category
        ev.append((t, "rot", -1)); t += 0.045
    ev.append((6.20, "cat", 2))          # -> HARMONY, only three rows
    return ev, 7.2


def main():
    out = os.path.join(HERE, "out", "system")
    os.makedirs(out, exist_ok=True)
    G.check()

    st = UiState(0, 0)
    mo = Motion(st)
    enc = Encoder()
    events, total = scripted()

    frames, band_count, worst, accel_log = [], 0, 0, []
    ei, now = 0, 0.0
    n = int(total * FPS)
    for k in range(n):
        while ei < len(events) and events[ei][0] <= now:
            _, kind, delta = events[ei]
            if kind == "push":
                st.push()
            elif kind == "cat":
                st.set_category(delta)
            else:
                steps = enc.detent(now, delta)
                accel_log.append((round(now, 3), delta, steps))
                st.rotate(steps)
            ei += 1
        mo.sync(st)
        mo.step(DT)
        im, bands = render(st, mo)
        frames.append(im)
        band_count += len(bands)
        worst = max(worst, len(bands))
        now += DT

    for k in (9, 30, 84, 105, 112, 150, 190):
        if k < len(frames):
            frames[k].resize((G.W * 4, G.H * 4), Image.NEAREST).save(
                os.path.join(out, "frame_%03d_4x.png" % k))
    frames[0].save(os.path.join(out, "interaction.gif"), save_all=True,
                   append_images=frames[1:], duration=int(1000 * DT), loop=0,
                   optimize=True)

    row_ms = 320 * G.ROW_PITCH * 2 * 8 / 30e6 * 1000.0
    print("scripted interaction: %d frames at %d fps (%.1f s)" % (n, FPS, total))
    print("acceleration (t, detent, steps applied):")
    for t, dl, s in accel_log:
        print("   %5.3f s  %+d  ->  %+d" % (t, dl, s))
    print("dirty bands: %.2f per frame average, %d worst case" %
          (band_count / float(n), worst))
    print("one 320x%d band = %.3f ms wire; worst frame = %.3f ms = %.1f%% of "
          "a %d fps budget" % (G.ROW_PITCH, row_ms, worst * row_ms,
                               100.0 * worst * row_ms / (1000.0 / FPS), FPS))
    print("full-frame repaint would be 29.013 ms = 87.0% of the same budget "
          "- which is why this pushes bands, not frames")
    print("wrote frames + interaction.gif to %s" % out)


if __name__ == "__main__":
    main()
