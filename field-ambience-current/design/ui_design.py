#!/usr/bin/env python3
"""Field Ambience — light duo-menu, high-resolution DESIGN render.

Designed in device units (320x170) and drawn at 6x with real vector type, so
the proportions are the panel's but the rendering shows the design intent
rather than the 4-bit dither. Type stands in with Inter Light/ExtraLight;
the device bakes Helvetica Neue Light/Thin at the same optical weight.

Rules this render obeys:
  - English throughout
  - uniform row height -> nothing can overlap anything
  - contrast carries the hierarchy, not size (the Giopato move)
  - no category holds more than 5 parameters, so the screen never fills up
"""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = os.path.join(HERE, ".fonts")

# Inter (SIL OFL) stands in for Helvetica Neue in this DESIGN render only —
# it is never compiled into firmware and never shipped. The device bakes
# Helvetica Neue via tools/generate_fonts.py. Fetched on first run because we
# do not vendor a font just to draw mockups.
def ensure_fonts():
    import urllib.request
    os.makedirs(FONTS, exist_ok=True)
    for w in (200, 300, 400):
        dst = os.path.join(FONTS, "inter-%d.ttf" % w)
        if os.path.exists(dst) and os.path.getsize(dst) > 10000:
            continue
        url = ("https://cdn.jsdelivr.net/fontsource/fonts/inter@latest/"
               "latin-%d-normal.ttf" % w)
        urllib.request.urlretrieve(url, dst)
S = 6                                  # render scale
W, H = 320 * S, 170 * S

# ----------------------------------------------------------------- palette
INK   = (26, 26, 28)
def mix(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))

PAPERS = {                              # per-world paper tint
    "OPEN SEA":    (236, 239, 240),
    "DESERT":      (243, 238, 229),
    "MOSS FIELDS": (233, 238, 233),
    "ALPS":        (238, 240, 243),
}

# ------------------------------------------------------------------- type
def font(weight, px):
    return ImageFont.truetype(os.path.join(FONTS, "inter-%d.ttf" % weight), px * S)

# --------------------------------------------------------------- geometry
M_L, M_R = 22, 20                       # side margins (device px)
HEAD_Y   = 17                           # header baseline
RULE_X   = 104                          # vertical hairline
LIST_X   = 126                          # parameter name left
ROW_0    = 62                           # first row baseline
ROW_H    = 23                           # row pitch — uniform, always
SZ_ROW   = 13                           # parameter / value size
SZ_META  = 9                            # rail + header size
SZ_SUP   = 7                            # superscript count
TRACK    = 0.18                         # letter-spacing for small caps (em)

# ------------------------------------------------------------------- data
CATS = [
    ("FIELD",   [("World", None), ("FX", "Dream"),
                 ("Atmosphere", "62%"), ("Cell", "Harmony")]),
    ("HARMONY", [("Key", "D"), ("Tuning", "Just"),
                 ("Bass", "Fifth"), ("Color", "Warm")]),
    ("TONE",    [("Voice", "Bowed"), ("Synth", "Ambient"),
                 ("Resonance", "55%")]),
    ("SHAPE",   [("Attack", "40%"), ("Release", "65%"), ("Motion", "35%"),
                 ("Sweep", "50%"), ("EnvMod", "30%")]),
    ("AIR",     [("Space", "72%"), ("Shimmer", "45%"), ("Echo", "28%"),
                 ("Blur", "15%"), ("Age", "33%")]),
]

# ------------------------------------------------------------------- draw
def tracked(d, x_dev, baseline_dev, text, f, fill, track=TRACK):
    """Letter-spaced text. PIL has no tracking, so advance per glyph."""
    x = x_dev * S
    y = baseline_dev * S
    extra = track * f.size
    for ch in text:
        d.text((x, y), ch, font=f, fill=fill, anchor="ls")
        x += d.textlength(ch, font=f) + extra
    return x / S

def tracked_w(d, text, f, track=TRACK):
    extra = track * f.size
    return (sum(d.textlength(c, font=f) for c in text) + extra * (len(text) - 1)) / S

def hairline(d, x0, y0, x1, y1, fill, w=1):
    d.line([(x0 * S, y0 * S), (x1 * S, y1 * S)], fill=fill, width=max(1, round(w * S * 0.5)))

def battery(d, right_dev, baseline_dev, fill, pct=0.62):
    h, w_ = 9, 19
    x1, y0 = right_dev, baseline_dev - h + 1
    x0 = x1 - w_
    r = 2.5 * S
    d.rounded_rectangle([x0 * S, y0 * S, x1 * S, (y0 + h) * S], radius=r,
                        outline=fill, width=max(1, round(S * 0.5)))
    d.rounded_rectangle([(x1 + 0.4) * S, (y0 + 3.2) * S, (x1 + 1.9) * S, (y0 + h - 3.2) * S],
                        radius=0.6 * S, fill=fill)
    pad = 2
    d.rounded_rectangle([(x0 + pad) * S, (y0 + pad) * S,
                         (x0 + pad + (w_ - 2 * pad) * pct) * S, (y0 + h - pad) * S],
                        radius=1.2 * S, fill=fill)

def frame(world, ci, pi):
    paper = PAPERS[world]
    idle  = mix(paper, INK, 0.26)       # unselected — present but receding
    meta  = mix(paper, INK, 0.42)       # header / rail idle
    rule  = mix(paper, INK, 0.16)

    im = Image.new("RGB", (W, H), paper)
    d  = ImageDraw.Draw(im)

    f_row   = font(300, SZ_ROW)         # Light — parameters
    f_meta  = font(400, SZ_META)        # Regular at tiny size reads as caps
    rows    = CATS[ci][1]

    # header: the loaded world, and the battery
    tracked(d, M_L, HEAD_Y, world, f_meta, meta)
    battery(d, 320 - M_R, HEAD_Y, meta)

    # rail: the five categories, on the same baseline grid as the list
    f_sup = font(400, SZ_SUP)
    for i, (name, items) in enumerate(CATS):
        y = ROW_0 + i * ROW_H
        on = (i == ci)
        end = tracked(d, M_L, y, name, f_meta, INK if on else meta)
        # how many parameters live in there — informative and it sets the
        # category apart from a plain word
        d.text(((end + 3) * S, (y - 4) * S), str(len(items)),
               font=f_sup, fill=meta if on else mix(paper, INK, 0.28), anchor="ls")
        if on:                                   # hairline marker, not a bar
            hairline(d, M_L, y + 5, end, y + 5, INK)

    # the column rule
    hairline(d, RULE_X, ROW_0 - 15, RULE_X, ROW_0 + 4 * ROW_H + 7, rule)

    # list: uniform rows — the selected one is INK, the rest recede
    for i, (name, val) in enumerate(rows):
        y  = ROW_0 + i * ROW_H
        on = (i == pi)
        col = INK if on else idle
        d.text((LIST_X * S, y * S), name, font=f_row, fill=col, anchor="ls")
        v = val if val is not None else world.title()
        d.text(((320 - M_R) * S, y * S), v, font=f_row, fill=col, anchor="rs")

    return im


def main():
    ensure_fonts()
    out = os.path.join(HERE, "out", "design")
    os.makedirs(out, exist_ok=True)
    shots = [
        ("01_field",   "OPEN SEA",    0, 0),
        ("02_tone",    "OPEN SEA",    2, 2),
        ("03_shape",   "OPEN SEA",    3, 3),
        ("04_air",     "OPEN SEA",    4, 0),
        ("05_desert",  "DESERT",      0, 1),
        ("06_moss",    "MOSS FIELDS", 1, 2),
    ]
    for tag, world, ci, pi in shots:
        frame(world, ci, pi).save(os.path.join(out, "design_%s.png" % tag))

    # Reality check: the same design at TRUE panel resolution, quantised to the
    # 16 grey levels the 4-bit framebuffer has. If it does not survive this it
    # is not a design, it is a picture.
    globals().update(S=1, W=320, H=170)
    for tag, world, ci, pi in shots[:4]:
        im = frame(world, ci, pi)
        q = im.convert("L").point(lambda v: round(v / 255 * 15) * 17).convert("RGB")
        q.resize((320 * 6, 170 * 6), Image.NEAREST).save(
            os.path.join(out, "device_%s.png" % tag))
    print("wrote %d design + 4 device frames to %s" % (len(shots), out))


if __name__ == "__main__":
    main()
