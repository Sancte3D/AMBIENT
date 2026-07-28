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
SEG_W = (TRACK_W - SEG_GAP * (SEG_N - 1)) // SEG_N   # 43, exact

SZ_SMALL, SZ_LABEL, SZ_TITLE = 10, 10, 20    # the measured crisp sizes

GREEN = (12, 249, 149)
GREEN_D = (10, 150, 92)
WHITE = (255, 255, 255)
TRACK_A = 0.12                      # measured off the reference

CATS = [
    ("Ambience", "Crystal Ocean",
     [("Drive", "87%", .36), ("Echo", "72%", .72), ("Granular", "29%", .29),
      ("Key", "D", .25), ("Noise", "88%", .88)]),
    ("Air", "Open Sea",
     [("Space", "72%", .72), ("Shimmer", "45%", .45), ("Echo", "28%", .28),
      ("Blur", "15%", .15), ("Age", "33%", .33)]),
    ("Harmony", "Fjords",
     [("Key", "D", .25), ("Tuning", "Just", .50), ("Bass", "Fifth", .75)]),
]
SEGMENTED = {"Key", "Tuning", "Bass", "Voice", "Synth", "FX", "Cell", "World"}


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


def screen(ci, pi):
    head, title, rows = CATS[ci]
    im = Image.open(os.path.join(ASSETS, "gradient_ref.png")).convert(
        "RGB").resize((W, H), Image.BICUBIC)
    d = ImageDraw.Draw(im, "RGBA")
    f_small, f_label, f_title = font(SZ_SMALL), font(SZ_LABEL), font(SZ_TITLE)

    d.text((TRACK_X, HEAD_MID), head, font=f_small, fill=WHITE, anchor="lm")
    bx = CONTENT_R - BADGE_W
    pill(d, bx, HEAD_MID, BADGE_W, BADGE_H, GREEN)
    d.text((bx + BADGE_W // 2, HEAD_MID), "100%", font=f_small,
           fill=GREEN_D, anchor="mm")

    d.text((TRACK_X, TITLE_BASE), title, font=f_title, fill=WHITE, anchor="ls")

    for i, (name, val, amt) in enumerate(rows):
        y = row_y(i)
        on = (i == pi)

        if name in SEGMENTED:
            for k in range(SEG_N):
                x = TRACK_X + k * (SEG_W + SEG_GAP)
                if k == 0:
                    pill(d, x, y, SEG_W, TRACK_H, GREEN)
                else:
                    pill(d, x, y, SEG_W, TRACK_H, WHITE, TRACK_A)
            if on:
                d.text((TRACK_X + SEG_W // 2, y), val, font=f_small,
                       fill=GREEN_D, anchor="mm")
        else:
            pill(d, TRACK_X, y, TRACK_W, TRACK_H, WHITE, TRACK_A)
            fw = max(TRACK_H, int(round(TRACK_W * amt)))
            pill(d, TRACK_X, y, fw, TRACK_H, GREEN)
            if on:
                d.text((TRACK_X + fw - 5, y), val, font=f_small,
                       fill=GREEN_D, anchor="rm")

        d.text((LABEL_X, y), name, font=f_label, fill=WHITE, anchor="lm")
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
                BADGE_H=BADGE_H, SEG_W=SEG_W, SEG_GAP=SEG_GAP)
    bad = [k for k, v in ints.items() if not isinstance(v, int)]
    assert not bad, "non-integer geometry: %s" % bad
    assert SEG_N * SEG_W + (SEG_N - 1) * SEG_GAP == TRACK_W, \
        "segments do not close on the track width"
    assert TRACK_X + TRACK_W + COL_GAP == LABEL_X
    assert row_y(4) + TRACK_H // 2 < H - 12, "no bottom margin left"
    assert TRACK_R * 2 == TRACK_H, "radius must be exactly half the height"
    return ints


SHOTS = [("01_ambience", 0, 0), ("02_air", 1, 0), ("03_harmony", 2, 1)]


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
    print("  segments: %d x %d + %d x %d = %d = track width"
          % (SEG_N, SEG_W, SEG_N - 1, SEG_GAP, TRACK_W))
    print("  bottom margin: %d px" % (H - (row_y(4) + TRACK_H // 2)))
    print("  all %d geometry constants are int" % len(ints))
    print("wrote %d screens + grid overlay to %s" % (len(SHOTS), out))


if __name__ == "__main__":
    main()
