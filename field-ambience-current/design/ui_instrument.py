#!/usr/bin/env python3
"""Field Ambience — LCD screens under the AMBIENT skill suite (.claude/skills).

This is the redo of the display design done through `ambient-ui-art-direction`,
`ambient-ui-prototyping` and `ambient-lcd-ux` instead of from a reference
picture. Following those skills changed the design, because reading the
current implementation first turned up two hard facts the earlier pass had
designed straight past.

FACT 1 — three type sizes exist, and only three.
    src/baked_font_data.c ships exactly:
        font_hn_value        advance 40, height 55   (Helvetica Light 56)
        font_hn_value_small  advance 26, height 36
        font_hn_label        advance 15, height 20   (Helvetica Thin 20)
    The earlier pass used 8/9/16 px, none of which exist, all of which need
    the Helvetica OTF that is not in the repo for licensing reasons. On a
    170 px-tall panel a 55 px value plus 20 px labels IS the composition;
    everything below follows from that and needs no new font baked.

FACT 2 — one encoder drives the menu, and it has two modes.
    include/encoders.h: EN1 drive, EN2 bright, EN3 *display* = the menu
    encoder, EN4 volume. include/menu.h: rotate browses, push enters edit,
    rotate edits, push leaves. So the screen has two jobs, not one, and the
    earlier design had no edit state at all — a functional gap, not a taste
    argument.

The colour finding, which is the reason for the LUT proposal below:

    oled_color.c builds lut[n] = grey(n*17) * accent / 255. Every level is
    the SAME hue at a different brightness. With a saturated accent the whole
    screen is that hue; there is no way to hold neutral white type next to a
    green marker. The baseline asks for "primarily black, white and gray with
    one controlled world accent" that stays "visibly saturated while occupying
    limited area" — which the current LUT cannot express at all.

    The minimal fix is a SPLIT ramp: levels 0-11 stay neutral grey, 12-15
    carry the accent. Same 16 entries, same 4bpp framebuffer, no extra RAM,
    about a dozen lines in rebuild(). Both LUTs are implemented here so the
    difference is visible rather than asserted.

Renders, per `ambient-ui-prototyping`:
    <fixture>_1x.png    exact 320x170, through the real LUT and RGB565
    <fixture>_6x.png    the same pixels, integer nearest-neighbour only
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = os.path.join(HERE, ".fonts")
DW, DH = 320, 170

# Accent per world. Kept saturated on purpose: the baseline explicitly says
# not to wash the interface out to make it feel calm.
WORLDS = {
    "OPEN SEA":    (0x35, 0xC8, 0xE8),
    "DESERT":      (0xFF, 0x8A, 0x2E),
    "MOSS FIELDS": (0x9B, 0xE0, 0x3C),
    "ALPS":        (0x8E, 0xB8, 0xFF),
    "FJORDS":      (0x46, 0xD6, 0xB0),
}

# ---------------------------------------------------------------- grey levels
# Drawing happens in 4-bit indices, exactly like the framebuffer. Naming them
# stops the design from inventing levels the panel does not have.
BG, RULE, DIM, TYPE_2, TYPE_1, ACC_D, ACC = 0, 3, 6, 9, 11, 13, 15


# -------------------------------------------------------------------- the LUT
def lut_current(accent):
    """oled_color.c as it stands: grey * accent, every level the same hue."""
    ar, ag, ab = accent
    out = []
    for n in range(16):
        g8 = n * 17
        out.append(((g8 * ar + 127) // 255,
                    (g8 * ag + 127) // 255,
                    (g8 * ab + 127) // 255))
    return out


def lut_split(accent):
    """Proposed: 0-11 neutral grey, 12-15 the accent. Same 16 entries.

    Levels 1-11 being a true grey ramp is also what keeps the existing
    max-blended antialiased font path working unchanged — the assumption
    "higher level = brighter" still holds inside the type range.
    """
    out = [(n * 17, n * 17, n * 17) for n in range(12)]
    ar, ag, ab = accent
    for k, f in enumerate((0.45, 0.65, 0.85, 1.0)):
        out.append((round(ar * f), round(ag * f), round(ab * f)))
    return out


def to_rgb565(c):
    """The driver packs r5g6b5. Show what survives that, not the float."""
    r, g, b = c
    r5, g6, b5 = r >> 3, g >> 2, b >> 3
    return (r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2)


# ------------------------------------------------------------------- surface
class FB:
    """A 4-bit index framebuffer, drawn the way the C renderer draws."""

    def __init__(self):
        self.a = np.zeros((DH, DW), np.uint8)

    def rect(self, x, y, w, h, level):
        x0, y0 = max(0, int(x)), max(0, int(y))
        x1, y1 = min(DW, int(x + w)), min(DH, int(y + h))
        if x1 > x0 and y1 > y0:
            self.a[y0:y1, x0:x1] = level

    def text(self, x, y_top, s, f, level):
        """Max-blend, which is what bfont_draw does with its maxgs argument."""
        m = Image.new("L", (DW, DH), 0)
        ImageDraw.Draw(m).text((x, y_top), s, font=f, fill=255)
        g = (np.asarray(m).astype(np.uint16) * level + 127) // 255
        np.maximum(self.a, g.astype(np.uint8), out=self.a)

    def render(self, accent, split=True):
        table = (lut_split if split else lut_current)(accent)
        table = [to_rgb565(c) for c in table]
        out = np.zeros((DH, DW, 3), np.uint8)
        for n, c in enumerate(table):
            out[self.a == n] = c
        return Image.fromarray(out)


def font(px, weight="light"):
    return ImageFont.truetype(os.path.join(FONTS, "hn-%s.ttf" % weight), px)


def width(f, s):
    return ImageDraw.Draw(Image.new("L", (1, 1))).textlength(s, font=f)


# ------------------------------------------------------------------- geometry
# One left margin, one baseline grid, one full-bleed rule. Every number below
# is derived from the three baked sizes; nothing is chosen to look nice at 6x.
M_L = 16                      # the single left margin
LABEL_TOP = 16                # font_hn_label, 20 px
VALUE_TOP = 52                # font_hn_value, 55 px
FOOT_TOP = 136                # font_hn_label, 20 px
RULE_Y, RULE_H = 160, 6       # full-bleed, edge to edge
SZ_VALUE, SZ_SMALL, SZ_LABEL = 55, 36, 20

# Five marks, top right: the five categories, which are the five cell keys
# under the thumb. This is the one thing occupying the right half, and it is
# there because the LCD-UX skill asks to show state near the control's
# conceptual location — not to balance the composition. It also stops the
# right 40% reading as unused filler rather than structure.
CELL_N = 5
CELL_W, CELL_H, CELL_GAP = 14, 4, 5
CELL_Y = LABEL_TOP + 8


def cells(fb, active):
    total = CELL_N * CELL_W + (CELL_N - 1) * CELL_GAP
    x = DW - M_L - total
    for k in range(CELL_N):
        fb.rect(x, CELL_Y, CELL_W, CELL_H, ACC if k == active else RULE)
        x += CELL_W + CELL_GAP


def value_font(s):
    """font_hn_value, or the 36 px fallback when the word will not fit.
    The fallback is not a nicety — it is the truncation rule the art-direction
    skill asks to define BEFORE shrinking type, and font_hn_value_small exists
    in the baked data precisely for this."""
    f = font(SZ_VALUE)
    if width(f, s) <= DW - 2 * M_L:
        return f
    return font(SZ_SMALL)


def screen_edit(world, cat, ci, name, value, amount, locked=False):
    """EDIT — the value is the screen's whole job. One glance, one number."""
    fb = FB()
    fb.text(M_L, LABEL_TOP, name.upper(), font(SZ_LABEL, "thin"), TYPE_2)
    cells(fb, ci)

    f = value_font(value)
    fb.text(M_L, VALUE_TOP, value, f, TYPE_1)

    # the rule carries the amount; discrete parameters get no partial fill
    fb.rect(0, RULE_Y, DW, RULE_H, RULE)
    if amount is not None:
        fb.rect(0, RULE_Y, DW * amount, RULE_H, ACC)

    foot = "%s · %s" % (cat, world.title())
    fb.text(M_L, FOOT_TOP, foot, font(SZ_LABEL, "thin"), DIM)
    if locked:
        fl = font(SZ_LABEL, "thin")
        fb.text(DW - M_L - width(fl, "LOCK"), FOOT_TOP, "LOCK", fl, ACC_D)
    return fb


def screen_browse(world, cat, ci, name, value, index, total):
    """BROWSE — which parameter am I on, and where is it in the whole list.

    The bottom rule is menu_render_bar_only(total, active): the segment for
    the current parameter is lit. That primitive already exists; the design
    does not get to invent a new one for the same job.
    """
    fb = FB()
    fb.text(M_L, LABEL_TOP, cat.upper(), font(SZ_LABEL, "thin"), TYPE_2)
    cells(fb, ci)

    fb.text(M_L, VALUE_TOP, name, value_font(name), TYPE_1)

    fl = font(SZ_LABEL, "thin")
    fb.text(M_L, FOOT_TOP, value, fl, DIM)
    right = world.title()
    fb.text(DW - M_L - width(fl, right), FOOT_TOP, right, fl, DIM)

    seg = DW / float(total)
    fb.rect(0, RULE_Y, DW, RULE_H, RULE)
    fb.rect(index * seg, RULE_Y, seg, RULE_H, ACC)
    return fb


# ---------------------------------------------------------------------- main
def ensure_fonts():
    """Helvetica Neue Light/Thin are not in the repo (licensing). Inter Light
    and Thin (SIL OFL) stand in for design review at the same pixel sizes —
    a drawing stand-in only, never compiled in. See THIRD_PARTY_NOTICES.md."""
    import urllib.request
    os.makedirs(FONTS, exist_ok=True)
    base = "https://cdn.jsdelivr.net/fontsource/fonts/inter@latest/"
    for name, url in (("hn-light.ttf", base + "latin-300-normal.ttf"),
                      ("hn-thin.ttf", base + "latin-200-normal.ttf")):
        dst = os.path.join(FONTS, name)
        if not (os.path.exists(dst) and os.path.getsize(dst) > 10000):
            urllib.request.urlretrieve(url, dst)


# Deterministic fixtures, not one hero screen: the prototyping skill asks for
# longest label, min and max, discrete, and the locked state.
FIXTURES = [
    ("edit_continuous", lambda: screen_edit("DESERT", "AIR", 4, "Space", "62%", .62)),
    ("edit_min",        lambda: screen_edit("FJORDS", "SHAPE", 3, "Attack", "0%", .0)),
    ("edit_max",        lambda: screen_edit("OPEN SEA", "AIR", 4, "Shimmer", "100%", 1.)),
    ("edit_discrete",   lambda: screen_edit("MOSS FIELDS", "FIELD", 0, "FX", "Dream", None)),
    ("edit_longest",    lambda: screen_edit("ALPS", "FIELD", 0, "World", "Moss Fields", None,
                                            locked=True)),
    ("browse_field",    lambda: screen_browse("DESERT", "FIELD", 0, "Atmosphere", "62%", 2, 21)),
    ("browse_harmony",  lambda: screen_browse("FJORDS", "HARMONY", 1, "Tuning", "Just", 6, 21)),
]
ACCENT_OF = {"edit_continuous": "DESERT", "edit_min": "FJORDS",
             "edit_max": "OPEN SEA", "edit_discrete": "MOSS FIELDS",
             "edit_longest": "ALPS", "browse_field": "DESERT",
             "browse_harmony": "FJORDS"}


def main():
    ensure_fonts()
    out = os.path.join(HERE, "out", "instrument")
    os.makedirs(out, exist_ok=True)

    for tag, build in FIXTURES:
        fb = build()
        accent = WORLDS[ACCENT_OF[tag]]
        im = fb.render(accent, split=True)
        im.save(os.path.join(out, "%s_1x.png" % tag))
        im.resize((DW * 6, DH * 6), Image.NEAREST).save(
            os.path.join(out, "%s_6x.png" % tag))

    # the comparison the LUT proposal rests on, same pixels both ways
    fb = FIXTURES[0][1]()
    accent = WORLDS["DESERT"]
    for name, split in (("lut_split", True), ("lut_current", False)):
        fb.render(accent, split=split).resize(
            (DW * 6, DH * 6), Image.NEAREST).save(
            os.path.join(out, "compare_%s_6x.png" % name))

    # every fixture must stay inside the 16 levels the framebuffer has
    for tag, build in FIXTURES:
        used = np.unique(build().a)
        assert used.max() < 16, tag
    print("wrote %d fixtures (1x + 6x) + 2 LUT comparisons to %s"
          % (len(FIXTURES), out))
    print("levels used across all fixtures: %s"
          % sorted({int(v) for _, b in FIXTURES for v in np.unique(b().a)}))


if __name__ == "__main__":
    main()
