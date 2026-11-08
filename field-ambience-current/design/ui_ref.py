#!/usr/bin/env python3
"""Field Ambience — the reference screen, measured rather than eyeballed.

Nothing here is estimated from looking at the mockup. The reference PNG was
read pixel by pixel: the panel bounding box, every track run, the label
column, the badge, the row pitch, the colours, and the type sizes (solved by
matching rendered string widths against the measured ones). The constants
below are those measurements expressed as fractions of the panel, so the same
code renders at the reference size and at 320x170.

Two assets come straight out of the reference instead of being reinvented:

  assets/gradient_ref.png
      The actual background field, extracted from the mockup. Every UI
      element — green fills, the glow around the first fill, the tracks, the
      dot-matrix type, the badge, and the white rim plus its bleed — was
      masked out, the remaining background median-sampled into a 32x20 grid,
      and the holes filled by diffusion. 349 of 640 cells carry real measured
      pixels; the rest are interpolated between them. It is a smooth field, so
      it upsamples to any size and costs 1.1 KB.

  assets/BitcountGridSingle-Regular.ttf
      The real face (SIL OFL, see assets/BitcountGridSingle-OFL.txt). Unlike
      the Helvetica stand-ins used elsewhere in design/, this one is actually
      shippable, which matters because the panel needs a hinted small face and
      Bitcount is a grid font built for exactly that.

What the measurement corrected in my earlier reading:

  - The tracks are white at about 12 %, not the 22-34 % I had been drawing.
    Measured: track (248,126,140) over background (249,107,126).
  - The Key row is FOUR EQUAL segments with a 19 px gap, not "138 px plus
    three fractions" as the written spec said. Measured runs: 126-331,
    350-554, 575-778, 801-1001.
  - The value on the active row is drawn in the SAME green as the fill, one
    step darker — it is not dark ink in a separate capsule. The capsule is
    the fill's own rounded end.
  - "Ambience", "Crystal Ocean" and every track start at exactly x=126. The
    single shared left axis is not an interpretation; it is in the pixels.
"""
import os

from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
ASSETS = os.path.join(HERE, "assets")

# ------------------------------------------- measured from the reference PNG
# source panel: x 18..1519, y 22..991  ->  1502 x 970
PAD_L = 0.0719          # shared left axis: breadcrumb, title, every track
TRACK_W = 0.5839
COL_GAP = 0.0486
LABEL_X = 0.7044
BADGE_W, BADGE_R = 0.0819, 0.9228        # badge width, right edge
SEG_GAP = 0.01265                        # 19 px between the four Key segments

HEAD_Y = 0.1088         # optical centre of the breadcrumb
TITLE_Y = 0.2515
ROW_0 = 0.3588          # centre of the first track
ROW_PITCH = 0.0933      # constant
TRACK_H = 0.0629
BADGE_H = 0.0598

# type sizes solved by matching rendered width to the measured width
SZ_HEAD = 0.0495        # "Ambience"      48 px at H=970
SZ_TITLE = 0.0639       # "Crystal Ocean" 62 px
SZ_LABEL = 0.0464       # "Drive"         45 px
SZ_SMALL = 0.0340       # "87%" / "100%"

GREEN = (12, 249, 149)
GREEN_D = (10, 150, 92)          # the value printed on the fill / on the badge
TRACK_A = 0.12                   # white at 12 %, measured
WHITE = (255, 255, 255)

CATS = [
    ("Ambience", "Crystal Ocean",
     [("Drive", "87%", .36), ("Echo", "72%", .72), ("Granular", "29%", .29),
      ("Key", "D", .25), ("Noise", "88%", .88), ("Delay", "88%", .88)]),
    ("Air", "Open Sea",
     [("Space", "72%", .72), ("Shimmer", "45%", .45), ("Echo", "28%", .28),
      ("Blur", "15%", .15), ("Age", "33%", .33)]),
    ("Harmony", "Fjords",
     [("Key", "D", .25), ("Tuning", "Just", .50), ("Bass", "Fifth", .75)]),
]
SEGMENTED = {"Key", "Tuning", "Bass", "Voice", "Synth", "FX", "Cell", "World"}


class Screen:
    def __init__(self, w, h):
        self.W, self.H = w, h
        self.im = Image.open(os.path.join(ASSETS, "gradient_ref.png")).convert(
            "RGB").resize((w, h), Image.BICUBIC)
        self.d = ImageDraw.Draw(self.im, "RGBA")
        self.x_left = PAD_L * w
        self.x_label = LABEL_X * w
        self.track_w = TRACK_W * w
        self.track_h = TRACK_H * h

    def font(self, frac):
        """Reference proportion, with a floor.

        The reference panel is 1502 px wide and ours is 320: a 4.7x reduction.
        The reference's 45 px labels become 7.9 px. Bitcount Grid is a grid
        face and holds up further down than a text face would, but the value
        type still bottoms out, so the small size is clamped. That clamp is
        the only deviation from the measured ratios.
        """
        px = max(8, round(frac * self.H))
        return ImageFont.truetype(
            os.path.join(ASSETS, "BitcountGridSingle-Regular.ttf"), px)

    def capsule(self, x, y_mid, w, h, fill, alpha=1.0):
        if alpha < 1.0:
            fill = tuple(fill) + (round(alpha * 255),)
        self.d.rounded_rectangle([x, y_mid - h / 2.0, x + w, y_mid + h / 2.0],
                                 radius=h / 2.0, fill=fill)

    def glow(self, x, y_mid, w, h):
        """The soft halo the reference puts around the ACTIVE fill only.

        Drawn as a blurred copy underneath, so it reads as light coming off
        the capsule rather than as a second outlined shape.
        """
        pad = int(max(4, h))
        lay = Image.new("RGBA", self.im.size, (0, 0, 0, 0))
        ImageDraw.Draw(lay).rounded_rectangle(
            [x, y_mid - h / 2.0, x + w, y_mid + h / 2.0],
            radius=h / 2.0, fill=tuple(GREEN) + (150,))
        lay = lay.filter(ImageFilter.GaussianBlur(pad * 0.45))
        self.im = Image.alpha_composite(self.im.convert("RGBA"), lay).convert("RGB")
        self.d = ImageDraw.Draw(self.im, "RGBA")


def screen(w, h, ci, pi):
    head, title, rows = CATS[ci]
    s = Screen(w, h)
    f_head, f_title = s.font(SZ_HEAD), s.font(SZ_TITLE)
    f_label, f_small = s.font(SZ_LABEL), s.font(SZ_SMALL)

    # header: breadcrumb on the shared axis, badge on the right content edge
    s.d.text((s.x_left, HEAD_Y * h), head, font=f_head, fill=WHITE, anchor="lm")
    bw, bh = BADGE_W * w, BADGE_H * h
    bx = BADGE_R * w - bw
    s.capsule(bx, HEAD_Y * h, bw, bh, GREEN)
    s.d.text((bx + bw / 2.0, HEAD_Y * h), "100%", font=f_small,
             fill=GREEN_D, anchor="mm")

    s.d.text((s.x_left, TITLE_Y * h), title, font=f_title, fill=WHITE,
             anchor="lm")

    for i, (name, val, amt) in enumerate(rows):
        y = (ROW_0 + i * ROW_PITCH) * h
        on = (i == pi)

        if name in SEGMENTED:
            gap = SEG_GAP * w
            segw = (s.track_w - 3 * gap) / 4.0
            for k in range(4):
                x = s.x_left + k * (segw + gap)
                if k == 0:
                    s.capsule(x, y, segw, s.track_h, GREEN)
                else:
                    s.capsule(x, y, segw, s.track_h, WHITE, TRACK_A)
            if on:
                s.d.text((s.x_left + segw / 2.0, y), val, font=f_small,
                         fill=GREEN_D, anchor="mm")
        else:
            s.capsule(s.x_left, y, s.track_w, s.track_h, WHITE, TRACK_A)
            fw = max(s.track_h, s.track_w * amt)
            if on:
                s.glow(s.x_left, y, fw, s.track_h)
            s.capsule(s.x_left, y, fw, s.track_h, GREEN)
            if on:
                # the value sits inside the fill, near its right end — the
                # capsule IS the fill's rounded end, not a separate chip
                s.d.text((s.x_left + fw - s.track_h * 0.9, y), val,
                         font=f_small, fill=GREEN_D, anchor="rm")

        s.d.text((s.x_label, y), name, font=f_label, fill=WHITE, anchor="lm")
    return s.im


SHOTS = [("01_ambience", 0, 0), ("02_air", 1, 0), ("03_harmony", 2, 1)]


def main():
    out = os.path.join(HERE, "out", "ref")
    os.makedirs(out, exist_ok=True)
    for tag, ci, pi in SHOTS:
        screen(1502, 970, ci, pi).save(os.path.join(out, "design_%s.png" % tag))
        dev = screen(320, 170, ci, pi)
        dev.save(os.path.join(out, "device_%s_1x.png" % tag))
        dev.resize((320 * 6, 170 * 6), Image.NEAREST).save(
            os.path.join(out, "device_%s_6x.png" % tag))
    print("reference 1502 wide -> 320 is a %.2fx reduction" % (1502 / 320.0))
    for nm, frac in (("breadcrumb", SZ_HEAD), ("title", SZ_TITLE),
                     ("label", SZ_LABEL), ("value", SZ_SMALL)):
        print("  %-11s reference %2.0f px  ->  device %4.1f px"
              % (nm, frac * 970, frac * 170))
    print("wrote %d design + %d device frames to %s"
          % (len(SHOTS), len(SHOTS), out))


if __name__ == "__main__":
    main()
