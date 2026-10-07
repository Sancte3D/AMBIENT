#!/usr/bin/env python3
"""Field Ambience — the reference layout SYSTEM, applied to the real panel.

Two corrections from the reference feedback drive this file.

CORRECTION 1 — the white rounded outline is the SCREEN, not a UI element.
    Every earlier pass here drew it as a card inside the display with a margin
    around it. It is the visible display area itself. So: the gradient is
    full-bleed, nothing is inset from a "card", and the reference's internal
    paddings are the SCREEN's paddings. That single misreading is what made
    the earlier layouts wrong at the whole-surface level, not in details.

CORRECTION 2 — build from one coordinate system, not from guessed positions.
    Every number below is a fraction of the panel, named once and reused:
    one shared left axis for the breadcrumb, the title and every track; a
    fixed two-column grid; a constant row pitch; fixed panel padding. Nothing
    is nudged individually, which is exactly what went wrong before.

Ratios are taken from the reference measurements (1019 x 513 panel) and
re-expressed as fractions so the same code renders at any size:

    DESIGN  1019 x 513   the system at the size it was designed for
    DEVICE   320 x 170   the actual ST7789V2 panel

The reference spec gives vertical padding both as "40 px" and as "7.8 %",
which disagree when a CSS percentage resolves against width. Vertical
fractions here resolve against HEIGHT, which reproduces the measured 40 px.

Fixed row pitch, not space-between, is also the right call for this product
and not only for the reference: a category holds 3 to 5 parameters, and with
a fixed pitch the rows do not jump when you switch category.
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = os.path.join(HERE, ".fonts")

# ------------------------------------------------- the one coordinate system
# horizontal: fractions of panel WIDTH
PAD_L, PAD_R = 0.077, 0.088
TRACK_W = 0.572                  # slider column
COL_GAP = 0.046                  # track -> label column
SEG_FIRST, SEG_GAP = 0.1354, 0.0137   # segmented row: active capsule, gaps
PILL_W = 0.0765                  # value capsule on the active track
BADGE_W = 0.0736

# vertical: fractions of panel HEIGHT
PAD_T, PAD_B = 0.078, 0.078
HEAD_Y = 0.0838                  # cap-top of the breadcrumb
TITLE_Y = 0.2359                 # cap-top of the title
ROW_0 = 0.3860                   # centre of the first slider row
ROW_PITCH = 0.1004               # constant; never derived from row count
TRACK_H = 0.0721
BADGE_H = 0.0585

# type, fractions of panel HEIGHT
SZ_HEAD, SZ_TITLE, SZ_LABEL, SZ_SMALL = 0.0526, 0.0780, 0.0604, 0.0351

ACCENT = (0x00, 0xFF, 0x99)
INK = (0x14, 0x16, 0x1A)

# Reference colourway: several soft radial gradients in one pink/coral/purple
# family, as stops along a single scalar field (see ui_glass.py for why 1-D
# matters once this has to survive a 16-entry palette).
STOPS = [(0.00, "#FFA82F"), (0.16, "#FF662F"), (0.34, "#FF00A8"),
         (0.58, "#FFDDF3"), (0.92, "#BCD1FE")]

CATS = [
    ("FIELD", "Crystal Ocean",
     [("Drive", "36%", .36), ("Echo", "72%", .72), ("Granular", "29%", .29),
      ("Key", "D", .25), ("Noise", "88%", .88)]),
    ("AIR", "Open Sea",
     [("Space", "72%", .72), ("Shimmer", "45%", .45), ("Echo", "28%", .28),
      ("Blur", "15%", .15), ("Age", "33%", .33)]),
    ("HARMONY", "Fjords",
     [("Key", "D", .25), ("Tuning", "Just", .50), ("Bass", "Fifth", .75)]),
]
SEGMENTED = {"Key", "Tuning", "Bass", "Voice", "Synth", "FX", "Cell", "World"}


def rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def over(fg, bg, a):
    return tuple(round(bg[i] + (fg[i] - bg[i]) * a) for i in range(3))


class Panel:
    """One panel at one size. All geometry comes from the fractions above."""

    def __init__(self, w, h):
        self.W, self.H = w, h
        self.im = Image.fromarray(self._gradient()).convert("RGB")
        self.d = ImageDraw.Draw(self.im, "RGBA")

        # the shared axes — computed ONCE, used by everything
        self.x_left = PAD_L * w                     # breadcrumb, title, tracks
        self.x_label = self.x_left + TRACK_W * w + COL_GAP * w
        self.x_right = w - PAD_R * w                # badge right edge
        self.track_w = TRACK_W * w
        self.track_h = TRACK_H * h

    # ---------------------------------------------------------- background
    def _gradient(self):
        w, h = self.W, self.H
        yy, xx = np.mgrid[0:h, 0:w].astype(np.float64)
        x = (xx + .5) / w - 0.50
        y = ((yy + .5) / h - 0.52) * (h / float(w)) * 1.85
        r = np.sqrt(x * x + y * y)
        r = r * (1.0 + 0.09 * np.sin(np.arctan2(y, x) * 3.0 + 0.7))
        r = np.clip(r / 0.46, 0.0, 1.0)
        stops = [(p, rgb(c)) for p, c in STOPS]
        out = np.zeros((h, w, 3))
        out[:] = stops[-1][1]
        for (p0, c0), (p1, c1) in zip(stops, stops[1:]):
            t = np.clip((r - p0) / (p1 - p0), 0, 1)
            t = t * t * (3 - 2 * t)
            m = (r >= p0) & (r <= p1)
            for k in range(3):
                out[..., k] = np.where(m, c0[k] + (c1[k] - c0[k]) * t, out[..., k])
        out[r < stops[0][0]] = stops[0][1]
        return np.uint8(np.round(out))

    # ------------------------------------------------------------ drawing
    def font(self, frac, weight="text"):
        """Reference ratio, with a hard floor.

        A 3.18x reduction turns the reference's 18 px value type into 6 px.
        Measured on this panel, 6-7 px is three grey smudges: no hinting, stems
        between pixels. 9 px is the floor for anything that must be read, so
        the small size deviates from the ratio ON PURPOSE and only downward-
        clamped. Everything else keeps the reference proportion exactly.
        """
        px = max(9, round(frac * self.H))
        return ImageFont.truetype(os.path.join(FONTS, weight + ".ttf"), px)

    def text(self, x, y_mid, s, f, fill, right=False):
        anchor = "rm" if right else "lm"
        self.d.text((x, y_mid), s, font=f, fill=fill, anchor=anchor)

    def capsule(self, x, y_mid, w, h, fill, alpha=1.0):
        if alpha < 1.0:
            fill = tuple(fill) + (round(alpha * 255),)
        self.d.rounded_rectangle([x, y_mid - h / 2, x + w, y_mid + h / 2],
                                 radius=h / 2, fill=fill)

    def row_y(self, i):
        return (ROW_0 + i * ROW_PITCH) * self.H


def screen(w, h, ci, pi):
    cat, title, rows = CATS[ci]
    p = Panel(w, h)
    white = (255, 255, 255)
    track_col = white          # drawn at 22 % — the reference track is a pale
    track_a = 0.34             # wash, clearly weaker than the fill

    f_head = p.font(SZ_HEAD)
    f_title = p.font(SZ_TITLE, "title")
    f_label = p.font(SZ_LABEL)
    f_small = p.font(SZ_SMALL)

    # header — breadcrumb left on the shared axis, badge right on the content
    # edge, both centred on ONE row (not aligned to each other's box tops)
    head_mid = HEAD_Y * h + p.font(SZ_HEAD).size * 0.5
    p.text(p.x_left, head_mid, cat.title(), f_head, white)
    bw, bh = BADGE_W * w, BADGE_H * h
    p.capsule(p.x_right - bw, head_mid, bw, bh, ACCENT)

    # title — same left axis, never its own margin
    p.text(p.x_left, TITLE_Y * h + f_title.size * 0.5, title, f_title, white)

    for i, (name, val, amt) in enumerate(rows):
        y = p.row_y(i)
        on = (i == pi)

        if name in SEGMENTED:
            # a fixed grid, not a track with things drawn on top of it
            first, gap = SEG_FIRST * w, SEG_GAP * w
            rest = (p.track_w - first - 3 * gap) / 3.0
            p.capsule(p.x_left, y, first, p.track_h, ACCENT)
            if on:
                # a segmented row has no travelling pill, so the active
                # capsule carries the value — same mechanism, same place
                p.d.text((p.x_left + first / 2, y), val, font=f_small,
                         fill=INK, anchor="mm")
            x = p.x_left + first + gap
            for _ in range(3):
                p.capsule(x, y, rest, p.track_h, track_col, track_a)
                x += rest + gap
        else:
            p.capsule(p.x_left, y, p.track_w, p.track_h, track_col, track_a)
            fill_w = max(p.track_h, p.track_w * (amt or 0))
            p.capsule(p.x_left, y, fill_w, p.track_h, ACCENT)
            if on:
                # the value capsule sits at the RIGHT END of the fill and
                # overlaps it seamlessly — it is not centred over the whole bar
                pw = max(PILL_W * w, p.track_h * 1.6)
                px = p.x_left + max(pw, fill_w) - pw
                p.capsule(px, y, pw, p.track_h, ACCENT)
                p.d.text((px + pw / 2, y), val, font=f_small, fill=INK,
                         anchor="mm")

        # second column: fixed axis, left aligned, centred on its own row
        # one column, one weight. There is no right-hand value column in the
        # reference — the value lives in the capsule on the active row only.
        p.text(p.x_label, y, name, f_label, white)
    return p.im


def ensure_fonts():
    import urllib.request
    os.makedirs(FONTS, exist_ok=True)
    base = "https://cdn.jsdelivr.net/fontsource/fonts/inter@latest/"
    for name, url in (("text.ttf", base + "latin-400-normal.ttf"),
                      ("title.ttf", base + "latin-600-normal.ttf")):
        dst = os.path.join(FONTS, name)
        if not (os.path.exists(dst) and os.path.getsize(dst) > 10000):
            urllib.request.urlretrieve(url, dst)


SHOTS = [("01_field", 0, 0), ("02_air", 1, 0), ("03_harmony", 2, 1)]


def main():
    ensure_fonts()
    out = os.path.join(HERE, "out", "panel")
    os.makedirs(out, exist_ok=True)

    for tag, ci, pi in SHOTS:
        # the system at the size it was designed for
        screen(1019, 513, ci, pi).save(os.path.join(out, "design_%s.png" % tag))
        # and at the size we actually have, integer-scaled for inspection only
        dev = screen(320, 170, ci, pi)
        dev.save(os.path.join(out, "device_%s_1x.png" % tag))
        dev.resize((320 * 6, 170 * 6), Image.NEAREST).save(
            os.path.join(out, "device_%s_6x.png" % tag))

    # what the reference type sizes actually become on this panel
    print("panel 1019x513 -> 320x170 is a %.2fx reduction" % (1019 / 320.0))
    for nm, frac in (("breadcrumb", SZ_HEAD), ("title", SZ_TITLE),
                     ("label", SZ_LABEL), ("value/badge", SZ_SMALL)):
        print("  %-12s reference %2.0f px  ->  device %4.1f px"
              % (nm, frac * 513, frac * 170))
    print("wrote %d design + %d device frames to %s"
          % (len(SHOTS), len(SHOTS), out))


if __name__ == "__main__":
    main()
