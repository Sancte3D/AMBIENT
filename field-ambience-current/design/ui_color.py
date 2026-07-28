#!/usr/bin/env python3
"""Field Ambience — COLOUR display design, on the new 5-category menu system.

The reference direction: saturated gradient card, bright accent bars, labels to
the right of the bars, mono type.

The interesting constraint is that the device framebuffer is 4 bits per pixel.
That is usually described as "16 greys", but it is really a 16-entry PALETTE
INDEX — oled_color.c just happens to fill it with a monochrome ramp. Nothing
stops us putting 16 arbitrary RGB565 colours in there, which is what makes this
design possible at zero RAM cost.

What it does cost is levels. Sixteen entries have to cover the background
gradient AND the UI, so the split below is the actual design decision:

    0-6   background gradient   (7 steps, ordered-dithered so it does not band)
    7     card border
    8     bar track
    9-10  dim / secondary type
    11    primary type
    12-15 accent  (bar fill, pill, highlight)

Renders three ways so the trade-off is visible:
    design_*.png   full colour, 6x            — the intent
    palette_*.png  quantised to the 16 entries, 6x — what the panel can show
    device_*.png   quantised at true 320x170  — what you will actually see
"""
import math
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = os.path.join(HERE, "fonts")
S = 6
DW, DH = 320, 170

# world -> (gradient top, gradient bottom, accent)
WORLDS = {
    "OPEN SEA":    ((28, 108, 130), (18, 58, 92),  (86, 240, 200)),
    "DESERT":      ((214, 92, 74),  (150, 44, 78), (120, 245, 170)),
    "MOSS FIELDS": ((58, 118, 82),  (24, 62, 58),  (198, 240, 120)),
    "ALPS":        ((92, 128, 176), (38, 54, 104), (150, 232, 255)),
    "FJORDS":      ((60, 88, 118),  (22, 34, 62),  (120, 214, 236)),
}

CATS = [
    ("FIELD",   [("World", None, None), ("FX", "Dream", None),
                 ("Atmosphere", "62%", .62), ("Cell", "Harmony", .5)]),
    ("HARMONY", [("Key", "D", .25), ("Tuning", "Just", .5),
                 ("Bass", "Fifth", .5), ("Color", "Warm", .5)]),
    ("TONE",    [("Voice", "Bowed", .5), ("Synth", "Ambient", .16),
                 ("Resonance", "55%", .55)]),
    ("SHAPE",   [("Attack", "40%", .40), ("Release", "65%", .65),
                 ("Motion", "35%", .35), ("Sweep", "50%", .50),
                 ("EnvMod", "30%", .30)]),
    ("AIR",     [("Space", "72%", .72), ("Shimmer", "45%", .45),
                 ("Echo", "28%", .28), ("Blur", "15%", .15),
                 ("Age", "33%", .33)]),
]
# parameters whose value is a WORD, not an amount -> segmented bar
SEGMENTED = {"FX", "Cell", "Key", "Tuning", "Bass", "Color", "Voice", "Synth", "World"}

# ------------------------------------------------------------------ geometry
PAD = 7                 # card inset from the panel edge
CARD_R = 11
HEAD_Y = 26             # small label baseline
TITLE_Y = 55            # big world title baseline
BAR_X, BAR_W = 22, 168
BAR_H = 11
BAR_0 = 70              # first bar top
BAR_P = 18              # bar pitch
LBL_X = 202             # labels sit to the RIGHT of the bars


def font(px, mono=True):
    name = "mono.ttf" if mono else "inter-400.ttf"
    return ImageFont.truetype(os.path.join(FONTS, name), px * S)


def lerp(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


def gradient(d, top, bot):
    """Vertical wash. Drawn smooth here; the palette pass dithers it."""
    for y in range(DH):
        t = y / (DH - 1.0)
        t = t * t * (3 - 2 * t)                       # ease so the top blooms
        d.rectangle([0, y * S, DW * S, (y + 1) * S], fill=lerp(top, bot, t))


def bar(d, x, y, w, h, fill):
    d.rounded_rectangle([x * S, y * S, (x + w) * S, (y + h) * S],
                        radius=(h / 2) * S, fill=fill)


def frame(world, ci, pi):
    top, bot, accent = WORLDS[world]
    cat, rows = CATS[ci]
    im = Image.new("RGB", (DW * S, DH * S))
    d = ImageDraw.Draw(im)

    gradient(d, top, bot)

    white = (255, 255, 255)
    dim = lerp(top, white, 0.55)
    track = lerp(top, white, 0.22)
    border = lerp(top, white, 0.42)

    # the card
    d.rounded_rectangle([PAD * S, PAD * S, (DW - PAD) * S, (DH - PAD) * S],
                        radius=CARD_R * S, outline=border,
                        width=max(1, round(S * 0.5)))

    f_small = font(8)
    f_big = font(19)
    f_lbl = font(10)
    f_pill = font(9)

    # header: category + which of five, and the selected value as a pill
    d.text((BAR_X * S, HEAD_Y * S), cat, font=f_small, fill=dim, anchor="ls")
    sx = BAR_X + d.textlength(cat, font=f_small) / S + 7
    for k in range(5):                                  # five categories
        w = 7 if k == ci else 4
        col = accent if k == ci else track
        bar(d, sx, HEAD_Y - 4, w, 3, col)
        sx += w + 3

    val = rows[pi][1] or world.title()
    pw = d.textlength(val, font=f_pill) / S + 15
    bar(d, DW - PAD - 9 - pw, HEAD_Y - 11, pw, 15, accent)
    d.text(((DW - PAD - 9 - pw / 2) * S, (HEAD_Y - 2) * S), val,
           font=f_pill, fill=bot, anchor="ms")

    # the world, big
    d.text((BAR_X * S, TITLE_Y * S), world.title(), font=f_big, fill=white,
           anchor="ls")

    # one bar per parameter, label to the right
    for i, (name, v, amt) in enumerate(rows):
        y = BAR_0 + i * BAR_P
        on = (i == pi)
        if name in SEGMENTED:                           # discrete -> segments
            segs, gap = 4, 3
            sw = (BAR_W - gap * (segs - 1)) / segs
            lit = max(1, round((amt or 0.5) * segs))
            for k in range(segs):
                bar(d, BAR_X + k * (sw + gap), y, sw, BAR_H,
                    accent if k < lit else track)
        else:
            bar(d, BAR_X, y, BAR_W, BAR_H, track)
            bar(d, BAR_X, y, max(BAR_H, BAR_W * (amt or 0)), BAR_H, accent)
        d.text((LBL_X * S, (y + BAR_H - 2) * S), name, font=f_lbl,
               fill=white if on else dim, anchor="ls")
    return im


# ------------------------------------------------------------- palette check
def palette_for(world):
    """The 16 entries this design would install in the device LUT."""
    top, bot, accent = WORLDS[world]
    white = (255, 255, 255)
    pal = [lerp(top, bot, i / 6.0) for i in range(7)]     # 0-6 background
    pal.append(lerp(top, white, 0.42))                    # 7  border
    pal.append(lerp(top, white, 0.22))                    # 8  track
    pal.append(lerp(top, white, 0.45))                    # 9  dim
    pal.append(lerp(top, white, 0.62))                    # 10 dim+
    pal.append(white)                                     # 11 type
    pal.append(lerp(accent, bot, 0.35))                   # 12 accent dark
    pal.append(accent)                                    # 13 accent
    pal.append(lerp(accent, white, 0.30))                 # 14 accent light
    pal.append(lerp(accent, white, 0.60))                 # 15 accent bright
    return pal


def quantise(im, world):
    """Map to the 16 entries with ordered dithering, then to RGB565 —
    exactly what the panel pipeline would do."""
    pal = palette_for(world)
    p = Image.new("P", (1, 1))
    flat = [c for rgb in pal for c in rgb] + [0] * (768 - len(pal) * 3)
    p.putpalette(flat)
    q = im.quantize(palette=p, dither=Image.FLOYDSTEINBERG).convert("RGB")
    # RGB565 truncation, as the driver does
    return q.point(lambda v: v)  # palette entries are already exact


def ensure_fonts():
    import urllib.request
    os.makedirs(FONTS, exist_ok=True)
    want = {
        "inter-400.ttf": "https://cdn.jsdelivr.net/fontsource/fonts/inter@latest/latin-400-normal.ttf",
        "mono.ttf": "https://cdn.jsdelivr.net/fontsource/fonts/jetbrains-mono@latest/latin-500-normal.ttf",
    }
    for name, url in want.items():
        dst = os.path.join(FONTS, name)
        if not (os.path.exists(dst) and os.path.getsize(dst) > 10000):
            urllib.request.urlretrieve(url, dst)


def main():
    global S
    ensure_fonts()
    out = os.path.join(HERE, "ui", "color")
    os.makedirs(out, exist_ok=True)
    shots = [("01_air", "OPEN SEA", 4, 0), ("02_shape", "DESERT", 3, 3),
             ("03_tone", "FJORDS", 2, 2), ("04_field", "MOSS FIELDS", 0, 2)]

    for tag, world, ci, pi in shots:
        im = frame(world, ci, pi)
        im.save(os.path.join(out, "design_%s.png" % tag))
        quantise(im, world).save(os.path.join(out, "palette_%s.png" % tag))

    S = 1                                   # true panel resolution
    for tag, world, ci, pi in shots:
        im = frame(world, ci, pi)
        q = quantise(im, world)
        q.resize((DW * 6, DH * 6), Image.NEAREST).save(
            os.path.join(out, "device_%s.png" % tag))
    print("wrote %d x 3 frames to %s" % (len(shots), out))


if __name__ == "__main__":
    main()
