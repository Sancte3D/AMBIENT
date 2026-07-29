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
        self.fill = {i: Eased(st.values[(st.ci, i)], T_VALUE)
                     for i in range(len(st.rows))}

    def sync(self, st):
        self.sel.target(st.pi)
        self.grow.target(1.0 if st.mode == EDIT else 0.0)
        for i in self.fill:
            self.fill[i].target(st.values[(st.ci, i)])

    def step(self, dt):
        m = self.sel.step(dt) | self.grow.step(dt)
        for e in self.fill.values():
            m |= e.step(dt)
        return m


# --------------------------------------------------------------- rendering
def _bands(rows_touched):
    """Row index -> the 320x20 band the flush would actually push."""
    out = []
    for i in sorted(set(rows_touched)):
        y = G.row_y(i)
        out.append((0, y - G.ROW_PITCH // 2, G.W, G.ROW_PITCH))
    return out


def render(st, mo):
    """Draw a frame and report which row bands changed.

    The background is a swappable asset — the gradient is loaded, never
    generated here, so replacing it is a file swap and touches no geometry.
    """
    im = Image.open(os.path.join(G.ASSETS, G.GRADIENT)).convert(
        "RGB").resize((G.W, G.H), Image.BICUBIC)
    d = ImageDraw.Draw(im, "RGBA")
    f_small, f_title = G.font(G.SZ_SMALL), G.font(G.SZ_TITLE)
    head, title, rows = G.CATS[st.ci]

    d.text((G.TRACK_X, G.HEAD_MID), head, font=f_small, fill=G.WHITE, anchor="lm")
    bx = G.CONTENT_R - G.BADGE_W
    G.pill(d, bx, G.HEAD_MID, G.BADGE_W, G.BADGE_H, G.GREEN)
    d.text((bx + G.BADGE_W // 2, G.HEAD_MID), "100%", font=f_small,
           fill=G.GREEN_D, anchor="mm")
    d.text((G.TRACK_X, G.TITLE_BASE), title, font=f_title, fill=G.WHITE,
           anchor="ls")

    touched = []
    sel_moving = mo.sel.moving or mo.grow.moving
    for i, (name, val, _) in enumerate(rows):
        y = G.row_y(i)
        # focus is a continuous distance, so the selection can be mid-flight
        # between two rows without either of them flickering
        focus = max(0.0, 1.0 - abs(mo.sel.v - i))

        if name in G.SEGMENTED:
            for k in range(G.SEG_N):
                x = G.TRACK_X + k * (G.SEG_W + G.SEG_GAP)
                if k == 0:
                    G.pill(d, x, y, G.SEG_W, G.TRACK_H, G.GREEN)
                else:
                    G.pill(d, x, y, G.SEG_W, G.TRACK_H, G.WHITE, G.TRACK_A)
            if focus > 0.5 and st.mode == EDIT:
                d.text((G.TRACK_X + G.SEG_W // 2, y), val, font=f_small,
                       fill=G.WHITE, anchor="mm")
        else:
            G.pill(d, G.TRACK_X, y, G.TRACK_W, G.TRACK_H, G.WHITE, G.TRACK_A)
            fw = max(G.TRACK_H, int(round(G.TRACK_W * mo.fill[i].v)))
            G.pill(d, G.TRACK_X, y, fw, G.TRACK_H, G.GREEN)
            if focus > 0.01:
                # The chip is on the SELECTED row at all times — browse mode
                # otherwise has no focus indicator at all, and "the value being
                # changed is slightly bigger" only means anything if there is a
                # normal size to be bigger than. grow scales height 14 -> 16
                # and fades the halo in; focus scales it so a mid-flight
                # selection cannot show two chips at full size.
                _bubble(d, G.TRACK_X + fw, y, st.text_for(i),
                        f_small, mo.grow.v * focus, focus)

        # A band is dirty only when something in it actually moved this
        # frame. Marking the focused row every frame would make the average
        # look like 1.0 forever and hide the real cost.
        if (i in mo.fill and mo.fill[i].moving) or (sel_moving and focus > 0.01):
            touched.append(i)
        d.text((G.LABEL_X, y), name, font=f_small, fill=G.WHITE, anchor="lm")

    return im, _bands(touched)


def _bubble(d, x_end, y, text, f, grow, focus=1.0):
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
    if focus > 0.45:
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
    return ev, 3.6


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

    for k in (9, 24, 30, 45, 60, 84):
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
