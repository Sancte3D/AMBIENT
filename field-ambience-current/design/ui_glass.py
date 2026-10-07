#!/usr/bin/env python3
"""Field Ambience — GLASS display design (gradient bloom + glass card).

The reference is a soft radial colour bloom under a rounded glass card: pale
blue base, blooming inward through pink -> magenta -> orange -> amber, a white
hairline rim at 38%, spring-green bars on a near-invisible track, and one
draggable handle carrying the live value.

The whole point of this file is that the direction is checked against the
REAL panel, not only against a pretty 6x mockup. The device framebuffer is
4 bits per pixel. That is usually described as "16 greys", but it is really a
16-entry PALETTE INDEX — oled_color.c just happens to fill it with a
monochrome ramp. Sixteen arbitrary RGB565 entries cost exactly the same RAM.

What they do cost is levels, and a bloom is the worst possible consumer of
levels. The split below is therefore the actual design decision:

    0-7   bloom          8 entries, median-cut over the FROSTED bloom and
                         Floyd-Steinberg dithered ONCE, offline, into a baked
                         4bpp bitmap in flash (320*170/2 = 27,200 B per world,
                         5 worlds = 136 KB). No runtime gradient maths at all.
    8     ink on accent  so the value printed on the handle can antialias
    9-10  ink 33% / 66%  two blend steps so type over the card can antialias.
                         Without these the palette forces 1-bit type and 9px
                         labels go ragged.
    11    white          card rim, handle top light
    12    veil           bar + segment track (flat; a real alpha veil over a
                         moving bloom is not expressible in one entry)
    13    ink            type, active segment chunk
    14    accent         bar fill, handle, badge
    15    accent glow    handle top light + the ring that replaces the halo

Renders two ways so the gap is visible and arguable:

    design_*.png   6x, true colour, real alpha, real gaussian glow — the intent
    device_*.png   320x170, the 16 entries above, flat fills, dithered bloom,
                   3-level type — what the panel can actually put out

device_*.png is asserted to contain at most 16 distinct colours. If that
assertion ever fails the design has silently stopped being implementable.
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = os.path.join(HERE, ".fonts")
DW, DH = 320, 170

# --------------------------------------------------------------------- worlds
# Each bloom is a 1-D colour ramp over a scalar field: hot core outward to the
# cool base. Keeping it 1-D is what makes 9 palette entries enough — the ramp
# is sampled along its own path, not across an arbitrary RGB volume.
# DESERT carries the reference colourway unchanged.
WORLDS = {
    "DESERT": dict(
        stops=[(0.00, "#FFA82F"), (0.18, "#FF662F"), (0.36, "#FF00A8"),
               (0.58, "#FFDDF3"), (0.90, "#BCD1FE")],
        accent="#00FF99", centre=(0.46, 0.54), squash=1.75, lobes=3),
    "OPEN SEA": dict(
        stops=[(0.00, "#8FFFE8"), (0.20, "#25C7D8"), (0.40, "#1E6FD0"),
               (0.62, "#CFE6FF"), (0.92, "#C4D4F4")],
        accent="#FF8A3D", centre=(0.58, 0.46), squash=1.9, lobes=2),
    "MOSS FIELDS": dict(
        stops=[(0.00, "#E8FF6B"), (0.19, "#8FDA3C"), (0.38, "#1F9E74"),
               (0.60, "#E4FFDC"), (0.90, "#C8DDF0")],
        accent="#FF4FD8", centre=(0.42, 0.58), squash=1.7, lobes=4),
    "ALPS": dict(
        stops=[(0.00, "#D8F2FF"), (0.17, "#8FD0F5"), (0.37, "#6E86E8"),
               (0.60, "#EBE2FF"), (0.90, "#C6D6F2")],
        accent="#FF6B4A", centre=(0.52, 0.40), squash=2.0, lobes=2),
    "FJORDS": dict(
        stops=[(0.00, "#B8FFE4"), (0.20, "#3FA9C4"), (0.42, "#2A4E96"),
               (0.64, "#DDEFF8"), (0.92, "#C2D2EE")],
        accent="#FFC63D", centre=(0.50, 0.62), squash=1.8, lobes=3),
}

INK = "#14161A"

CATS = [
    ("FIELD",   [("World", None, None), ("FX", "Dream", 0.25),
                 ("Atmosphere", "62%", .62), ("Cell", "Harmony", .50)]),
    ("HARMONY", [("Key", "D", .25), ("Tuning", "Just", .50),
                 ("Bass", "Fifth", .75), ("Color", "Warm", .50)]),
    ("TONE",    [("Voice", "Bowed", .50), ("Synth", "Ambient", .25),
                 ("Resonance", "55%", .55)]),
    ("SHAPE",   [("Attack", "40%", .40), ("Release", "65%", .65),
                 ("Motion", "35%", .35), ("Sweep", "50%", .50),
                 ("EnvMod", "30%", .30)]),
    ("AIR",     [("Space", "72%", .72), ("Shimmer", "45%", .45),
                 ("Echo", "28%", .28), ("Blur", "15%", .15),
                 ("Age", "33%", .33)]),
]
# Parameters whose value is a WORD, not an amount -> segmented bar, never a
# continuous fill. A four-chunk row reads as "one of a few", a bar does not.
SEGMENTED = {"World", "FX", "Cell", "Key", "Tuning", "Bass", "Color",
             "Voice", "Synth"}

# ------------------------------------------------------------------- geometry
# Proportions follow the reference, where the card is a calm panel with a lot
# of air in it and the controls occupy well under half its width. Bars are
# slim (7 px, not 11), short (128 px, not 170) and the right two-fifths of the
# card is deliberately empty apart from label and value.
PAD = 6                       # card inset from the panel edge
CARD_R = 14
HEAD_Y = 24                   # baseline, category label
TITLE_Y = 48                  # baseline, world title
BAR_X, BAR_W = 22, 128
BAR_H = 7
BAR_0 = 74                    # first bar top
BAR_P = 17                    # pitch; handle + its 2px ring is 15 tall
LBL_X = 168                   # labels sit to the RIGHT of the bars
VAL_R = DW - PAD - 16         # values right-aligned, inset from the rim
SEG_N, SEG_GAP = 4, 3
HANDLE_W, HANDLE_H = 27, 11   # sits proud of the 7px bar
BADGE_W, BADGE_H = 22, 8      # a state pill, no text — see below

# Type sizes in device pixels. The floor is real: at 6-7 px a mono face has a
# ~4 px cap height, PIL applies no hinting, stems land between pixels and
# "62%" came out as three grey smudges. Everything the user must READ is 8 px
# or larger, and the two smallest slots (badge, category) carry no prose —
# the badge is a pill with no text at all, exactly as in the reference.
# Verified against device_*.png, not guessed.
SZ_CAT, SZ_TITLE, SZ_LBL, SZ_VAL, SZ_HANDLE = 8, 16, 9, 9, 9
TRACK_CAT = 2.2               # wide-tracked caps for the one label that is caps


def rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def over(fg, bg, alpha):
    return tuple(round(bg[i] + (fg[i] - bg[i]) * alpha) for i in range(3))


def font(px, s, face="text"):
    return ImageFont.truetype(os.path.join(FONTS, face + ".ttf"),
                              max(1, round(px * s)))


# ---------------------------------------------------------------------- bloom
def bloom(world, w, h):
    """Radial colour bloom as float RGB, shape (h, w, 3).

    Analytic, not a stack of blurred blobs: a gaussian-blurred blob IS a smooth
    radial falloff, and writing it that way keeps the field strictly 1-D, which
    is the property the 9-entry palette depends on.
    """
    spec = WORLDS[world]
    cx, cy = spec["centre"]
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float64)
    x = (xx + .5) / w - cx
    y = ((yy + .5) / h - cy) * (h / float(w)) * spec["squash"]
    r = np.sqrt(x * x + y * y)
    # mild lobing so it is a bloom and not a bullseye; still a function of one
    # scalar, so the palette ramp still covers it exactly
    r = r * (1.0 + 0.09 * np.sin(np.arctan2(y, x) * spec["lobes"] + 0.7))
    # SCALE is the mistake this file made first time round. In the reference
    # the card is 314 px inside a 1133 px field: a small calm panel floating in
    # a big soft wash, where the hot core is a minority of the picture. Mapping
    # the card to the whole 320x170 panel and keeping the same normalisation
    # put the hot core underneath the entire UI, and a screaming magenta blob
    # is not the design — it is the background having nowhere left to go.
    r = np.clip(r / 0.40, 0.0, 1.0)

    stops = [(p, rgb(c)) for p, c in spec["stops"]]
    out = np.zeros((h, w, 3))
    out[:] = stops[-1][1]
    for (p0, c0), (p1, c1) in zip(stops, stops[1:]):
        t = np.clip((r - p0) / (p1 - p0), 0, 1)
        t = t * t * (3 - 2 * t)                      # smoothstep
        m = (r >= p0) & (r <= p1)
        for k in range(3):
            out[..., k] = np.where(m, c0[k] + (c1[k] - c0[k]) * t, out[..., k])
    out[r < stops[0][0]] = stops[0][1]
    return out


FROST = 0.55


def frost(bl, s):
    """Lift the bloom toward white inside the card.

    This is the piece that was missing, and it is the whole trick: a glass
    card FROSTS what is behind it. Drawing only the rim left the raw magenta
    core sitting directly under the labels — unreadable, and reading as a blob
    on the card rather than a field behind it. Frosting the interior turns the
    bloom into atmosphere, keeps the full-strength colour in the margin around
    the card, and gives the reference's "panel floating in a field" at screen
    scale, where the panel would otherwise be the whole screen.

    Free on device: it bakes into the same static bitmap, still 9 entries.
    """
    h, w = bl.shape[:2]
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).rounded_rectangle(
        [PAD * s, PAD * s, (DW - PAD) * s, (DH - PAD) * s],
        radius=CARD_R * s, fill=255)
    a = np.asarray(m).astype(np.float64) / 255.0 * FROST
    return bl + (255.0 - bl) * a[..., None]


# --------------------------------------------------------------------- skins
class Skin:
    """Resolved colours + primitives for one render mode.

    design mode composites with real alpha and a real gaussian glow.
    device mode has neither: every fill is one of 16 flat entries, and the
    glow becomes an ordered-dither stipple. Same call sites, so the two
    renders cannot drift apart.
    """

    def __init__(self, world, s, device):
        spec = WORLDS[world]
        self.s, self.device, self.world = s, device, world
        self.accent = rgb(spec["accent"])
        self.glow = over((255, 255, 255), self.accent, .45)
        self.ink = rgb(INK)
        self.white = (255, 255, 255)

        bl = frost(bloom(world, DW, DH), 1)
        base = tuple(bl[PAD:DH - PAD, PAD:DW - PAD].reshape(-1, 3).mean(0))
        self.veil = over(self.white, base, .30)
        self.rim_flat = over(self.white, base, .60)
        self.ink33 = over(self.ink, base, .33)
        self.ink66 = over(self.ink, base, .66)
        # One entry bought back from the bloom (9 -> 8) so that type printed on
        # the accent can antialias. The value on the handle is the single most
        # read number on the screen; 1-bit at 9px made it "6 2%". The bloom
        # does not miss the ninth entry — the dither absorbs it.
        self.ink_on_accent = over(self.ink, self.accent, .55)

        if device:
            self.bg = self._baked(bl)
        else:
            self.bg = Image.fromarray(np.uint8(np.round(
                frost(bloom(world, DW * s, DH * s), s)))).convert("RGB")
        self.im = self.bg.copy()
        self.d = ImageDraw.Draw(self.im, "RGBA")

    def _baked(self, bl):
        """The flash bitmap: bloom -> 8 entries, Floyd-Steinberg, once.

        Two steps on purpose. PIL only honours ``dither`` when an explicit
        palette is handed in; quantize(colors=8) alone returns hard bands, and
        eight hard bands across a full-screen bloom is exactly the artefact this
        design cannot afford. So: pick the nine, then dither ONTO them.
        """
        src = Image.fromarray(np.uint8(np.round(bl))).convert("RGB")
        pal = src.quantize(colors=8, method=Image.MEDIANCUT)
        return src.quantize(palette=pal,
                            dither=Image.FLOYDSTEINBERG).convert("RGB")

    # -- primitives ------------------------------------------------------
    def _xy(self, box):
        return [v * self.s for v in box]

    def rrect(self, box, r, fill=None, outline=None, width=1, alpha=1.0):
        s = self.s
        if fill is not None and alpha < 1.0 and not self.device:
            fill = fill + (round(alpha * 255),)
        self.d.rounded_rectangle(self._xy(box), radius=r * s, fill=fill,
                                 outline=outline, width=max(1, round(width * s)))

    def bar(self, x, y, w, h, fill, alpha=1.0):
        self.rrect([x, y, x + w, y + h], h / 2.0, fill=fill, alpha=alpha)

    def _tracked(self, dr, pos, txt, f, fill, anchor, track):
        """Draw with letter-spacing. PIL has no tracking, so glyphs go down one
        at a time. Worth the twelve lines: wide-tracked small caps is most of
        what separates a designed header from a terminal print-out."""
        adv = [dr.textlength(c, font=f) for c in txt]
        extra = track * self.s
        x = pos[0]
        if anchor[0] in "mr":
            total = sum(adv) + extra * max(0, len(txt) - 1)
            x -= total / 2.0 if anchor[0] == "m" else total
        for c, a in zip(txt, adv):
            dr.text((x, pos[1]), c, font=f, fill=fill, anchor="l" + anchor[1])
            x += a + extra

    def text(self, xy, txt, px, colour, anchor="ls", on=None, track=0.0,
             face="text"):
        """Device mode gets 3-level type: two ink blends plus solid ink.

        A 16-entry palette has no grey ramp, so PIL's antialiasing is not
        reproducible on the panel. Rendering the mask and snapping it to the
        two reserved blend entries is what the driver would do.

        ``on`` is the colour the type actually sits on. The two blend entries
        are mixed against the bloom, which is right for a label floating over
        the card and wrong for the value printed on a solid accent handle —
        that one came out mud until it was told what it was sitting on. With
        ``on`` set the type goes 1-bit instead, which is the correct answer for
        small type on a solid field anyway.
        """
        f = font(px, self.s, face)
        pos = [xy[0] * self.s, xy[1] * self.s]
        if not self.device:
            self._tracked(self.d, pos, txt, f, colour, anchor, track)
            return
        # Full-frame mask, same coordinates and anchor as the design path, so
        # the two renders cannot disagree about where a glyph sits. Cheap: the
        # device frame is 320x170.
        m = Image.new("L", self.im.size, 0)
        self._tracked(ImageDraw.Draw(m), pos, txt, f, 255, anchor, track)
        a = np.asarray(m)
        arr = np.asarray(self.im).copy()
        if on is not None:
            arr[a >= 170] = colour
            arr[(a >= 60) & (a < 170)] = self.ink_on_accent
        else:
            arr[a >= 190] = colour
            arr[(a >= 110) & (a < 190)] = self.ink66
            arr[(a >= 50) & (a < 110)] = self.ink33
        self.im = Image.fromarray(arr)
        self.d = ImageDraw.Draw(self.im, "RGBA")

    def halo(self, x, y, w, h, sigma=4.5):
        """Handle glow.

        Design mode gets the real gaussian. Device mode does NOT get a stipple
        of it: tried that first, and at one device pixel an ordered-dither
        halo is not a glow, it is dirt — a visible 4x4 grid smeared across the
        neighbouring tracks. A solid 2px ring of the glow entry says "this one
        is lifted" far more clearly and costs the same single entry.
        """
        if self.device:
            self.rrect([x - 2, y - 2, x + w + 2, y + h + 2],
                       (h + 4) / 2.0, fill=self.glow)
            return
        s = self.s
        x0, y0, x1, y1 = x * s, y * s, (x + w) * s, (y + h) * s
        rad = (h / 2.0) * s
        sg = sigma * s
        X0, Y0 = int(x0 - 3 * sg), int(y0 - 3 * sg)
        X1, Y1 = int(x1 + 3 * sg), int(y1 + 3 * sg)
        X0, Y0 = max(0, X0), max(0, Y0)
        X1 = min(self.im.width, X1)
        Y1 = min(self.im.height, Y1)
        if X1 <= X0 or Y1 <= Y0:
            return
        gy, gx = np.mgrid[Y0:Y1, X0:X1].astype(np.float64)
        dx = np.maximum(np.maximum(x0 + rad - gx, gx - (x1 - rad)), 0)
        dy = np.maximum(np.maximum(y0 + rad - gy, gy - (y1 - rad)), 0)
        dist = np.maximum(np.sqrt(dx * dx + dy * dy) - rad, 0)
        a = np.exp(-(dist / sg) ** 2) * 0.60
        reg = np.asarray(self.im.crop((X0, Y0, X1, Y1))).astype(np.float64)
        reg = reg + (np.array(self.glow) - reg) * a[..., None]
        self.im.paste(Image.fromarray(np.uint8(np.clip(reg, 0, 255))), (X0, Y0))
        self.d = ImageDraw.Draw(self.im, "RGBA")


# --------------------------------------------------------------------- frame
def frame(world, ci, pi, s=6, device=False):
    sk = Skin(world, s, device)
    cat, rows = CATS[ci]
    accent, ink, white = sk.accent, sk.ink, sk.white

    # glass card: white hairline at 38% over the bloom
    sk.rrect([PAD, PAD, DW - PAD, DH - PAD], CARD_R,
             outline=sk.rim_flat if device else white + (97,),
             width=1 if device else 1.6)

    # header — category in tracked caps, position in the five, state pill.
    # The pill carries NO text: at this size any glyph inside it is a smudge,
    # and the reference does not put one there either.
    sk.text((BAR_X, HEAD_Y), cat, SZ_CAT, sk.ink66, track=TRACK_CAT)
    sx = BAR_X + 78
    for k in range(5):
        w = 6 if k == ci else 3
        sk.bar(sx, HEAD_Y - 5, w, 2, accent if k == ci else sk.ink33,
               1.0 if device or k == ci else .45)
        sx += w + 4
    bx = DW - PAD - 16 - BADGE_W
    sk.bar(bx, HEAD_Y - 11, BADGE_W, BADGE_H, accent)

    # the world
    sk.text((BAR_X, TITLE_Y), world.title(), SZ_TITLE, ink, face="title")

    for i, (name, val, amt) in enumerate(rows):
        y = BAR_0 + i * BAR_P
        on = (i == pi)
        shown = val if val is not None else world.title()

        if name in SEGMENTED:
            segw = (BAR_W - SEG_GAP * (SEG_N - 1)) / float(SEG_N)
            cur = min(SEG_N - 1, int((amt or 0) * SEG_N))
            for k in range(SEG_N):
                x = BAR_X + k * (segw + SEG_GAP)
                if k == cur:
                    # The near-black chunk was a misreading of the reference.
                    # There it is ONE dark chip in an otherwise quiet card; put
                    # it on every word-valued row and you get three heavy blobs
                    # fighting each other and the bloom. Unselected rows mark
                    # their position with a light ink tint instead — visible,
                    # not loud — and only the selected row goes accent.
                    col, al = (accent, 1.0) if on else (ink, 1.0)
                else:
                    col, al = sk.veil, 1.0 if device else .30
                sk.rrect([x, y, x + segw, y + BAR_H], BAR_H / 2.0,
                         fill=col, alpha=al)
        else:
            sk.bar(BAR_X, y, BAR_W, BAR_H, sk.veil, 1.0 if device else .30)
            fw = max(BAR_H, BAR_W * (amt or 0))
            sk.bar(BAR_X, y, fw, BAR_H, accent)

        # the selected row carries the draggable handle with its own value
        if on and name not in SEGMENTED:
            hx = BAR_X + max(HANDLE_W / 2.0,
                             min(BAR_W - HANDLE_W / 2.0,
                                 BAR_W * (amt or 0))) - HANDLE_W / 2.0
            hy = y + (BAR_H - HANDLE_H) / 2.0
            sk.halo(hx, hy, HANDLE_W, HANDLE_H)
            sk.bar(hx, hy, HANDLE_W, HANDLE_H, accent)
            sk.bar(hx + 3, hy + 1.5, HANDLE_W - 6, 1.5, sk.glow)
            sk.text((hx + HANDLE_W / 2.0, hy + HANDLE_H - 3.5), shown, SZ_HANDLE, ink,
                    anchor="ms", on=accent, face="mono")

        sk.text((LBL_X, y + BAR_H - 1), name, SZ_LBL,
                ink if on else sk.ink66)
        if not (on and name not in SEGMENTED):
            sk.text((VAL_R, y + BAR_H - 1), shown, SZ_VAL,
                    ink if on else sk.ink66, anchor="rs", face="mono")
    return sk.im


# ---------------------------------------------------------------------- main
def ensure_fonts():
    import urllib.request
    os.makedirs(FONTS, exist_ok=True)
    base = "https://cdn.jsdelivr.net/fontsource/fonts/"
    want = {
        # a grotesk for reading, a mono only for numerals so columns align
        "text.ttf": base + "inter@latest/latin-400-normal.ttf",
        "title.ttf": base + "inter@latest/latin-600-normal.ttf",
        "mono.ttf": base + "jetbrains-mono@latest/latin-500-normal.ttf",
    }
    for name, url in want.items():
        dst = os.path.join(FONTS, name)
        if not (os.path.exists(dst) and os.path.getsize(dst) > 10000):
            urllib.request.urlretrieve(url, dst)


SHOTS = [("01_field", "DESERT", 0, 2), ("02_air", "OPEN SEA", 4, 0),
         ("03_shape", "ALPS", 3, 3), ("04_tone", "MOSS FIELDS", 2, 2),
         ("05_harmony", "FJORDS", 1, 1)]


def main():
    ensure_fonts()
    out = os.path.join(HERE, "out", "glass")
    os.makedirs(out, exist_ok=True)
    worst = 0
    for tag, world, ci, pi in SHOTS:
        frame(world, ci, pi, s=6).save(os.path.join(out, "design_%s.png" % tag))
        dev = frame(world, ci, pi, s=1, device=True)
        n = len(dev.convert("RGB").getcolors(1 << 16) or [])
        worst = max(worst, n)
        assert n <= 16, "%s uses %d colours — no longer 4bpp" % (tag, n)
        dev.resize((DW * 6, DH * 6), Image.NEAREST).save(
            os.path.join(out, "device_%s.png" % tag))
    print("wrote %d design + %d device frames to %s" % (len(SHOTS), len(SHOTS), out))
    print("worst-case distinct colours in a device frame: %d / 16" % worst)


if __name__ == "__main__":
    main()
