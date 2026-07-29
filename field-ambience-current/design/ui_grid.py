#!/usr/bin/env python3
"""Field Ambience — the 320x170 pixel-perfect design system.

This inverts how design/ worked until now. Every other file here designs at a
large size and scales down; this one is authored AT 320x170 and previewed by
integer nearest-neighbour upscale only. Nothing is ever resampled, so what the
preview shows is what the panel puts out, pixel for pixel.

Rules, and they are enforced by assertions at the bottom, not just stated:

  * every coordinate, size and radius is an int;
  * no fractional positions, ever — the tracks share one x, the labels share
    one x, the rows sit on a constant integer pitch;
  * type sizes come from a measurement of the actual face, not from taste.

TYPE SIZE IS MEASURED, NOT CHOSEN.
    Bitcount Grid Single is a dot-grid face, so it is only crisp where its
    dots land on whole pixels. Rendering "Granular 87%" at 7..22 px and
    counting distinct ink values:

        px      7    8    9  *10*  11   12   13   14   16   18   20
        greys  44   38   13   *1*  37   25   84   55   73  100    6

    At 10 px there is exactly ONE ink value: every stem is a solid pixel, no
    partial coverage anywhere. 20 px is the next clean step and is where the
    dot pattern itself becomes visible, which is the face's whole character.
    So the scale is 10 and 20, and nothing in between — 11..14 px is measurably
    mushier than 10 px despite being larger.

PROPORTIONS come from ui_ref.py, which measured them out of the reference PNG.
They are rounded to integers here, and two roundings were chosen so the
arithmetic closes exactly rather than nearly:

    track 187 px wide, 4 segments of 43 with 5 px gaps  ->  4*43 + 3*5 = 187
    22 + 187 + 16 = 225, which is the measured label axis (0.7044 * 320)

WHERE THIS DEVIATES from the measured reference, and why:

    track height   measured ratio gives 10.7 px; drawn at 14.
                   At 1:1 on a 1.9-inch panel a 10 px capsule with a value
                   inside it is thin. 14 is a deliberate step up. It is NOT
                   16: five rows of 16 need a 22 px pitch, which leaves 12 px
                   of bottom margin against the reference's 24, and the
                   generous bottom margin is a real part of how the reference
                   reads. 14/20 keeps 15 px.
"""
import os

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ASSETS = os.path.join(HERE, "assets")

W, H = 320, 170

# The background is an ASSET, never generated in the renderer. Swapping the
# look is this one filename plus a new file in assets/ — no geometry moves.
GRADIENT = "gradient_ref.png"

# ------------------------------------------------------------- the grid (px)
PAD_L, PAD_R = 22, 24
TRACK_X, TRACK_W = 22, 187          # shared left axis
COL_GAP = 16
LABEL_X = TRACK_X + TRACK_W + COL_GAP        # 225 — the measured label axis
CONTENT_R = W - PAD_R                        # 296

HEAD_MID = 16                       # breadcrumb + badge share this centre line
TITLE_BASE = 48
ROW_0, ROW_PITCH = 68, 20           # centres 68 88 108 128 148
TRACK_H, TRACK_R = 14, 7            # radius = h/2, a true pill
BADGE_W, BADGE_H = 38, 12   # "100%" is 24 px; 7 px each side, as measured
SEG_N, SEG_GAP = 4, 5

# The value bubble on the row being edited. Measured off the reference: the
# plain fill is 61 px tall and the bubble peaks at 71 — 16 % taller — centred
# on the fill's right end. 14 * 1.16 = 16, and 16 is even so the radius is a
# clean 8. It grows into the 6 px gap between rows by 1 px per side, which the
# 20 px pitch absorbs.
BUBBLE_H, BUBBLE_R, BUBBLE_PAD = 16, 8, 10
SEG_W = (TRACK_W - SEG_GAP * (SEG_N - 1)) // SEG_N   # 43, exact

SZ_SMALL, SZ_LABEL, SZ_TITLE = 10, 10, 20    # the measured crisp sizes

GREEN = (12, 249, 149)
GREEN_D = (10, 150, 92)
WHITE = (255, 255, 255)
TRACK_A = 0.12                      # measured off the reference

# --------------------------------------------------------------- the system
# THE REAL ONE. Everything below is read out of the firmware, not invented:
# option tables from src/menu.c, labels from its LABELS[MP_COUNT], world names
# from src/worlds.c in their enum order.
#
# The earlier version of this file carried "Drive / Granular / Noise" over a
# world called "Crystal Ocean" — all four lifted straight from the reference
# picture, none of which exist in the device. Drive is a dedicated encoder
# (EN1), not a menu slot; Granular and Noise are not parameters at all.
WORLD_NAMES = ["Alps", "Open Sea", "Fjords", "Moss Fields", "Desert"]
KEY_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
VOICE_NAMES = ["Pad", "String", "Ember", "Bowed", "Horn", "Choir", "Guembri"]
SYNTH_NAMES = ["Ambient", "Acid", "FM Glass", "Mist", "Storm", "Orbit", "Bamboo"]
TUNING_NAMES = ["Equal", "Just"]
CELL_NAMES = ["Note", "Harmony", "Land"]
BASS_NAMES = ["Off", "Root", "Fifth", "Drift"]
COLOR_NAMES = ["Pure", "Open", "Warm", "Deep"]
FX_NAMES = ["Bypass", "Reverb", "Delay", "Chorus", "Tape", "Swell", "Shimmer",
            "Blur", "Dream"]

# The 21 MP_* slots grouped into five categories — one per cell key. Every
# parameter appears EXACTLY ONCE; the counts are 4+4+3+5+5 = 21, which is
# MP_COUNT. (name, options or None for a 0-100 % value, default)
CATS = [
    ("Field", [
        ("World",      WORLD_NAMES, 1),
        ("FX",         FX_NAMES,    8),      # Dream is the boot default
        ("Atmosphere", None,        0.62),
        ("Cell",       CELL_NAMES,  1),
    ]),
    ("Harmony", [
        ("Key",        KEY_NAMES,   2),
        ("Tuning",     TUNING_NAMES, 1),
        ("Bass",       BASS_NAMES,  2),
        ("Color",      COLOR_NAMES, 2),
    ]),
    ("Tone", [
        ("Voice",      VOICE_NAMES, 3),
        ("Synth",      SYNTH_NAMES, 0),
        ("Resonance",  None,        0.55),
    ]),
    ("Shape", [
        ("Attack",     None,        0.40),
        ("Release",    None,        0.65),
        ("Motion",     None,        0.35),
        ("Sweep",      None,        0.50),
        ("EnvMod",     None,        0.30),
    ]),
    ("Air", [
        ("Space",      None,        0.72),
        ("Shimmer",    None,        0.45),
        ("Echo",       None,        0.28),
        ("Blur",       None,        0.15),
        ("Age",        None,        0.33),
    ]),
]

assert sum(len(r) for _, r in CATS) == 21, "the menu has 21 MP_ slots"


def seg_gap(n):
    """A 12-option Key cannot be four fat chunks.

    SEG_N used to be hard-coded to 4 while the real option counts are
    2, 3, 4, 5, 7, 9 and 12 — so the row said "one of four" no matter what the
    parameter actually offered. The slot count now comes from the parameter and
    the gap shrinks as the count grows, so twelve slots still tile the same
    187 px track and stay individually visible.
    """
    return 5 if n <= 5 else (3 if n <= 8 else 2)


def seg_slot(n, k):
    """Exact tiling: slot k of n across TRACK_W, no leftover pixels."""
    g = seg_gap(n)
    x0 = TRACK_X + int(round(k * TRACK_W / float(n)))
    x1 = TRACK_X + int(round((k + 1) * TRACK_W / float(n)))
    return x0, max(4, x1 - x0 - g)


def font(px):
    return ImageFont.truetype(
        os.path.join(ASSETS, "BitcountGridSingle-Regular.ttf"), px)


def row_y(i):
    return ROW_0 + i * ROW_PITCH


def pill(d, x, y_mid, w, h, fill, alpha=1.0):
    """Integer geometry only. PIL draws hard-edged, so an integer box gives
    exactly the pixels the panel will light — no resampling anywhere."""
    x, w = int(x), int(w)
    top, bot = y_mid - h // 2, y_mid - h // 2 + h - 1
    if alpha < 1.0:
        fill = tuple(fill) + (int(alpha * 255),)
    d.rounded_rectangle([x, top, x + w - 1, bot], radius=h // 2, fill=fill)


def bubble(d, x_end, y, text, f, centre=False):
    """The value capsule at the fill's right end, 16 % taller than the track.

    The value inside is WHITE, not dark. Sampling the reference's fill instead
    of its glyphs is what made me call it dark green before; the brightest
    pixel in that capsule is (255,255,255).
    """
    tw = int(d.textlength(text, font=f))
    w = tw + 2 * BUBBLE_PAD
    x = x_end - w // 2 if centre else x_end - w
    x = min(TRACK_X + TRACK_W - w, max(TRACK_X, x))
    for k, a in ((3, 0.10), (1, 0.16)):          # a two-step halo, not a blur:
        pill(d, x - k, y, w + 2 * k, BUBBLE_H + 2 * k, GREEN, a)
    pill(d, x, y, w, BUBBLE_H, GREEN)
    d.text((x + w // 2, y), text, font=f, fill=WHITE, anchor="mm")


def screen(ci, pi):
    head, rows = CATS[ci]
    im = Image.open(os.path.join(ASSETS, GRADIENT)).convert(
        "RGB").resize((W, H), Image.BICUBIC)
    d = ImageDraw.Draw(im, "RGBA")
    f_small, f_title = font(SZ_SMALL), font(SZ_TITLE)

    # The title is the WORLD, which is state, and World is also a row in
    # Field, which is the control. Showing both is correct, not redundant.
    world = WORLD_NAMES[CATS[0][1][0][2]]
    d.text((TRACK_X, HEAD_MID), head, font=f_small, fill=WHITE, anchor="lm")
    bx = CONTENT_R - BADGE_W
    pill(d, bx, HEAD_MID, BADGE_W, BADGE_H, GREEN)
    d.text((bx + BADGE_W // 2, HEAD_MID), "100%", font=f_small,
           fill=GREEN_D, anchor="mm")
    d.text((TRACK_X, TITLE_BASE), world, font=f_title, fill=WHITE, anchor="ls")

    for i, (name, opts, val) in enumerate(rows):
        y = row_y(i)
        if opts is not None:
            n = len(opts)
            for k in range(n):
                x, w = seg_slot(n, k)
                pill(d, x, y, w, TRACK_H, GREEN if k == val else WHITE,
                     1.0 if k == val else TRACK_A)
            # centred on the LIT slot, not at the track end: a chip parked on
            # the right edge points at the last option no matter which one is
            # actually selected. It may overhang its slot — that is fine, it
            # is anchored to the right place.
            sx, sw = seg_slot(n, val)
            end, text, mid = sx + sw // 2, opts[val], True
        else:
            pill(d, TRACK_X, y, TRACK_W, TRACK_H, WHITE, TRACK_A)
            fw = max(TRACK_H, int(round(TRACK_W * val)))
            pill(d, TRACK_X, y, fw, TRACK_H, GREEN)
            end, text, mid = TRACK_X + fw, "%d%%" % round(val * 100), False
        if i == pi:
            bubble(d, end, y, text, f_small, mid)
        d.text((LABEL_X, y), name, font=f_small, fill=WHITE, anchor="lm")
    return im


def grid_overlay(im):
    """Proof, not decoration: the shared axes and the row centres drawn on
    top, so a misaligned element is visible instead of arguable."""
    im = im.copy()
    d = ImageDraw.Draw(im, "RGBA")
    for x in (TRACK_X, TRACK_X + TRACK_W, LABEL_X, CONTENT_R):
        d.line([(x, 0), (x, H - 1)], fill=(0, 200, 255, 130))
    for i in range(5):
        y = row_y(i)
        d.line([(0, y), (W - 1, y)], fill=(255, 0, 120, 110))
    d.line([(0, HEAD_MID), (W - 1, HEAD_MID)], fill=(255, 220, 0, 110))
    d.line([(0, TITLE_BASE), (W - 1, TITLE_BASE)], fill=(255, 220, 0, 110))
    return im


def check():
    """Every number that reaches a draw call must be an int."""
    ints = dict(PAD_L=PAD_L, PAD_R=PAD_R, TRACK_X=TRACK_X, TRACK_W=TRACK_W,
                COL_GAP=COL_GAP, LABEL_X=LABEL_X, CONTENT_R=CONTENT_R,
                HEAD_MID=HEAD_MID, TITLE_BASE=TITLE_BASE, ROW_0=ROW_0,
                ROW_PITCH=ROW_PITCH, TRACK_H=TRACK_H, BADGE_W=BADGE_W,
                BADGE_H=BADGE_H)
    bad = [k for k, v in ints.items() if not isinstance(v, int)]
    assert not bad, "non-integer geometry: %s" % bad
    for n in (2, 3, 4, 5, 7, 9, 12):     # every real option count
        x, w = seg_slot(n, n - 1)
        assert x + w + seg_gap(n) == TRACK_X + TRACK_W, \
            "%d slots do not close on the track width" % n
    assert TRACK_X + TRACK_W + COL_GAP == LABEL_X
    assert row_y(4) + TRACK_H // 2 < H - 12, "no bottom margin left"
    assert TRACK_R * 2 == TRACK_H, "radius must be exactly half the height"
    return ints


SHOTS = [("01_field", 0, 0), ("02_harmony", 1, 0), ("03_tone", 2, 1),
         ("04_shape", 3, 2), ("05_air", 4, 0)]


def main():
    ints = check()
    out = os.path.join(HERE, "out", "grid")
    os.makedirs(out, exist_ok=True)
    for tag, ci, pi in SHOTS:
        im = screen(ci, pi)
        im.save(os.path.join(out, "%s_1x.png" % tag))
        im.resize((W * 6, H * 6), Image.NEAREST).save(
            os.path.join(out, "%s_6x.png" % tag))
    grid_overlay(screen(0, 0)).resize((W * 6, H * 6), Image.NEAREST).save(
        os.path.join(out, "00_grid_6x.png"))
    print("authored at %dx%d, previewed by integer NEAREST only" % (W, H))
    print("  type scale (measured crisp sizes): %d / %d px"
          % (SZ_SMALL, SZ_TITLE))
    print("  rows at y: %s" % [row_y(i) for i in range(5)])
    print("  slot widths by option count: %s"
          % {n: seg_slot(n, 0)[1] for n in (2, 3, 4, 5, 7, 9, 12)})
    print("  bottom margin: %d px" % (H - (row_y(4) + TRACK_H // 2)))
    print("  all %d geometry constants are int" % len(ints))
    print("wrote %d screens + grid overlay to %s" % (len(SHOTS), out))


if __name__ == "__main__":
    main()
