#!/usr/bin/env python3
"""Field Ambience — a critique of ui_grid, rendered rather than argued.

Variant A is the current design (ui_grid.screen). Variant B is what the
critique proposes. Same state, same data, same font, same background, so the
only difference is the argument.

The four things B changes, in the order they cost the user the most:

1. READING ORDER. A reads bar -> value -> NAME. You learn what you are
   looking at last, at the far right, after passing a bar and a number that
   mean nothing until you know the name. The reference could do this because
   it is a poster with six fixed labels you learn positionally; we have 21
   parameters whose names change with every category, so the name is the
   entry point. B puts it first.

2. VISUAL WEIGHT vs IMPORTANCE. In A the loudest thing on screen is the world
   title at 20 px, and the world is ambient state you are not editing. Second
   loudest are five equally saturated green bars, so the eye has nothing to
   land on. The focus marker — the chip — is smaller than the things it is
   supposed to win against. B quiets the bars that are not selected, and
   demotes the title.

3. BIPOLAR PARAMETERS ARE DRAWN WRONG. menu.h: MP_ATTACK and MP_RELEASE are
   "0..100 %, 50 = neutral", and menu.c initialises both to 50. A fills them
   from the left, which says 0 is the resting state. It is not. B fills them
   from the centre, so 50 % is an empty bar with a centre tick and turning
   either way grows a fill in that direction.

4. THE BADGE SAYS NOTHING. In A the strongest colour on the screen sits in the
   most prominent corner and reads "100%" with no unit and no label. It is
   inherited decoration. B spends that corner on the thing the hardware
   actually needs: which of the five cells you are on, and where this
   parameter sits in the whole set of 21 (menu.c already exposes
   menu_render_bar_only(total, active) for exactly this).

What B does NOT change: the pixel grid, the type scale, the colour, the
gradient, the chip-grows-in-edit idea. Those hold up.
"""
import os

from PIL import Image, ImageDraw

import ui_grid as G

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------- B geometry
NAME_X, NAME_W = 22, 60          # "Atmosphere" is 60 px — the entry point
BAR_X, BAR_W = 88, 124
VAL_R = 296                      # values right-aligned against the content edge
CELL_Y = 16
CELL_W, CELL_H, CELL_GAP = 9, 3, 4

BIPOLAR = {"Attack", "Release"}  # menu.h: 50 = neutral

# I repeated my own mistake here on the first pass: dimming the unselected
# fills with ALPHA turned them into the same muddy olive I had already
# rejected in A. Green at 34 % over pink is not a quiet green, it is a dirty
# brown. A quiet green has to be a SOLID second colour.
GREEN_Q = (70, 195, 140)


def bar_row(d, i, name, opts, val, sel, edit, f):
    y = G.row_y(i)
    ink = G.WHITE if sel else tuple(G.WHITE) + (int(G.DIM_TEXT * 255),)
    fill = G.GREEN if sel else GREEN_Q

    d.text((NAME_X, y), name, font=f, fill=ink, anchor="lm")

    if opts is not None:
        n = len(opts)
        k = max(0, min(n - 1, int(round(val))))
        if n > 6:
            # More than six options is not a segmented control — twelve 6 px
            # capsules read as a barcode you cannot count. A continuous track
            # with a marker at the position says "3 of 12" far better.
            G.pill(d, BAR_X, y, BAR_W, G.TRACK_H, G.WHITE, G.TRACK_A)
            mx = BAR_X + int(round(k * (BAR_W - G.TRACK_H) / float(n - 1)))
            G.pill(d, mx, y, G.TRACK_H, G.TRACK_H, fill)
        else:
            for j in range(n):
                x0 = BAR_X + int(round(j * BAR_W / float(n)))
                x1 = BAR_X + int(round((j + 1) * BAR_W / float(n)))
                G.pill(d, x0, y, max(4, x1 - x0 - 4), G.TRACK_H,
                       fill if j == k else G.WHITE,
                       1.0 if j == k else G.TRACK_A)
        text = opts[k]
    else:
        G.pill(d, BAR_X, y, BAR_W, G.TRACK_H, G.WHITE, G.TRACK_A)
        if name in BIPOLAR:
            # from the centre, because 50 % IS the neutral state
            mid = BAR_X + BAR_W // 2
            w = int(round(abs(val - 0.5) * BAR_W))
            if w >= 2:
                G.pill(d, mid if val > 0.5 else mid - w, y, w, G.TRACK_H, fill)
            d.line([(mid, y - G.TRACK_H // 2), (mid, y + G.TRACK_H // 2 - 1)],
                   fill=tuple(G.WHITE) + (120,))
        else:
            G.pill(d, BAR_X, y, max(G.TRACK_H, int(round(BAR_W * val))),
                   G.TRACK_H, fill)
        text = "%d%%" % round(val * 100)

    if sel:
        tw = int(d.textlength(text, font=f))
        w = tw + 2 * G.BUBBLE_PAD
        h = G.BUBBLE_H if edit else G.TRACK_H
        x = VAL_R - w
        if edit:
            for k2, a in ((3, 0.10), (1, 0.16)):
                G.pill(d, x - k2, y, w + 2 * k2, h + 2 * k2, G.GREEN, a)
        G.pill(d, x, y, w, h, G.GREEN)
        d.text((x + w // 2, y), text, font=f, fill=G.WHITE, anchor="mm")
    else:
        d.text((VAL_R, y), text, font=f, fill=ink, anchor="rm")


def screen_b(ci, pi, edit=False):
    head, rows = G.CATS[ci]
    im = Image.open(os.path.join(G.ASSETS, G.GRADIENT)).convert(
        "RGB").resize((G.W, G.H), Image.BICUBIC)
    d = ImageDraw.Draw(im, "RGBA")
    f = G.font(G.SZ_SMALL)

    # header: category left, five cell marks right. The marks replace the
    # meaningless badge and answer "which key am I on, of how many".
    d.text((NAME_X, CELL_Y), head, font=f, fill=G.WHITE, anchor="lm")
    total = 5 * CELL_W + 4 * CELL_GAP
    x = G.CONTENT_R - total
    for k in range(5):
        G.pill(d, x, CELL_Y, CELL_W, CELL_H,
               G.GREEN if k == ci else G.WHITE, 1.0 if k == ci else G.TRACK_A)
        x += CELL_W + CELL_GAP

    # the world is state, not the thing being edited — 10 px, not 20
    world = G.WORLD_NAMES[G.CATS[0][1][0][2]]
    d.text((NAME_X, 34), world, font=f, fill=tuple(G.WHITE) + (170,),
           anchor="lm")

    for i, (name, opts, val) in enumerate(rows):
        bar_row(d, i, name, opts, val, i == pi, edit, f)

    # position in all 21 slots — menu.c already has render_bar(total, active)
    done = sum(len(r) for _, r in G.CATS[:ci]) + pi
    seg = G.W / 21.0
    d.rectangle([0, G.H - 4, G.W, G.H - 1], fill=tuple(G.WHITE) + (70,))
    d.rectangle([int(done * seg), G.H - 4, int((done + 1) * seg), G.H - 1],
                fill=G.GREEN)
    return im


# ---------------------------------------------------------------- C geometry
# B fixed the reading order but created a new problem: with the name at x=22
# and the value at x=296, the two things you must connect are 274 px apart,
# the full width of the screen. A at least kept value and label adjacent.
# So neither is right, and the order that matches what you actually need is
#   1. which parameter   2. what is it now   3. roughly where in range
# which is name -> value -> bar. C is that.
C_NAME_X = 22
C_VAL_X = 90                     # chip lane, 76 px, left-aligned
C_BAR_X, C_BAR_W = 174, 122


def row_c(d, i, name, opts, val, sel, edit, f):
    y = G.row_y(i)
    ink = G.WHITE if sel else tuple(G.WHITE) + (int(G.DIM_TEXT * 255),)
    fill = G.GREEN if sel else GREEN_Q
    d.text((C_NAME_X, y), name, font=f, fill=ink, anchor="lm")

    if opts is not None:
        n = len(opts)
        k = max(0, min(n - 1, int(round(val))))
        if n > 6:
            G.pill(d, C_BAR_X, y, C_BAR_W, G.TRACK_H, G.WHITE, G.TRACK_A)
            mx = C_BAR_X + int(round(k * (C_BAR_W - G.TRACK_H) / float(n - 1)))
            G.pill(d, mx, y, G.TRACK_H, G.TRACK_H, fill)
        else:
            for j in range(n):
                x0 = C_BAR_X + int(round(j * C_BAR_W / float(n)))
                x1 = C_BAR_X + int(round((j + 1) * C_BAR_W / float(n)))
                G.pill(d, x0, y, max(4, x1 - x0 - 4), G.TRACK_H,
                       fill if j == k else G.WHITE,
                       1.0 if j == k else G.TRACK_A)
        text = opts[k]
    else:
        G.pill(d, C_BAR_X, y, C_BAR_W, G.TRACK_H, G.WHITE, G.TRACK_A)
        if name in BIPOLAR:
            mid = C_BAR_X + C_BAR_W // 2
            w = int(round(abs(val - 0.5) * C_BAR_W))
            if w >= 2:
                G.pill(d, mid if val > 0.5 else mid - w, y, w, G.TRACK_H, fill)
            d.line([(mid, y - G.TRACK_H // 2), (mid, y + G.TRACK_H // 2 - 1)],
                   fill=tuple(G.WHITE) + (120,))
        else:
            G.pill(d, C_BAR_X, y, max(G.TRACK_H, int(round(C_BAR_W * val))),
                   G.TRACK_H, fill)
        text = "%d%%" % round(val * 100)

    if sel:
        w = int(d.textlength(text, font=f)) + 2 * G.BUBBLE_PAD
        h = G.BUBBLE_H if edit else G.TRACK_H
        x = C_VAL_X - G.BUBBLE_PAD
        if edit:
            for k2, a in ((3, 0.10), (1, 0.16)):
                G.pill(d, x - k2, y, w + 2 * k2, h + 2 * k2, G.GREEN, a)
        G.pill(d, x, y, w, h, G.GREEN)
        d.text((x + w // 2, y), text, font=f, fill=G.WHITE, anchor="mm")
    else:
        d.text((C_VAL_X, y), text, font=f, fill=ink, anchor="lm")


def screen_c(ci, pi, edit=False):
    head, rows = G.CATS[ci]
    im = Image.open(os.path.join(G.ASSETS, G.GRADIENT)).convert(
        "RGB").resize((G.W, G.H), Image.BICUBIC)
    d = ImageDraw.Draw(im, "RGBA")
    f, ft = G.font(G.SZ_SMALL), G.font(G.SZ_TITLE)

    d.text((C_NAME_X, CELL_Y), head, font=f, fill=G.WHITE, anchor="lm")
    total = 5 * CELL_W + 4 * CELL_GAP
    x = G.CONTENT_R - total
    for k in range(5):
        G.pill(d, x, CELL_Y, CELL_W, CELL_H,
               G.GREEN if k == ci else G.WHITE, 1.0 if k == ci else G.TRACK_A)
        x += CELL_W + CELL_GAP
    # the world keeps presence, but at 20 px it out-shouted the parameter the
    # encoder is actually on. Half a step down, not a whisper.
    world = G.WORLD_NAMES[G.CATS[0][1][0][2]]
    d.text((C_NAME_X, 44), world, font=ft, fill=tuple(G.WHITE) + (200,),
           anchor="ls")

    for i, (name, opts, val) in enumerate(rows):
        row_c(d, i, name, opts, val, i == pi, edit, f)

    done = sum(len(r) for _, r in G.CATS[:ci]) + pi
    seg = G.W / 21.0
    d.rectangle([0, G.H - 4, G.W, G.H - 1], fill=tuple(G.WHITE) + (70,))
    d.rectangle([int(done * seg), G.H - 4, int((done + 1) * seg), G.H - 1],
                fill=G.GREEN)
    return im


SHOTS = [("field", 0, 0, True), ("harmony", 1, 0, True),
         ("shape", 3, 0, True), ("air", 4, 2, False)]


def main():
    out = os.path.join(HERE, "out", "critique")
    os.makedirs(out, exist_ok=True)
    for tag, ci, pi, ed in SHOTS:
        a = G.screen(ci, pi, ed)
        b = screen_b(ci, pi, ed)
        c = screen_c(ci, pi, ed)
        pair = Image.new("RGB", (G.W, G.H * 3 + 8), (20, 20, 24))
        for k, im in enumerate((a, b, c)):
            pair.paste(im, (0, k * (G.H + 4)))
        pair.resize((G.W * 4, (G.H * 3 + 8) * 4), Image.NEAREST).save(
            os.path.join(out, "%s_ABC_4x.png" % tag))
        c.resize((G.W * 4, G.H * 4), Image.NEAREST).save(
            os.path.join(out, "%s_C_4x.png" % tag))
    print("A = current (ui_grid) · B = name-first · C = name, value, bar")
    print("wrote %d A/B pairs to %s" % (len(SHOTS), out))


if __name__ == "__main__":
    main()
