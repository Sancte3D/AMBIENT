#!/usr/bin/env python3
"""
The scene sheet: nine little visual worlds, side by side.

The question this answers is not "does each drawing look nice" but "do nine
drawings read as nine different PLACES". A widget system fails that test —
every page looks like the last one with different words. Put them next to each
other and it is obvious within a second which is which.

  /tmp/render_wheel <dir> && python3 tools/make_scene_sheet.py <dir> <out.png>
"""
import pathlib
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("needs pillow:  pip install pillow")

SCENES = [
    ("03_scene_world",   "WORLD",   "a horizon — city, coast, highway, room"),
    ("04_scene_sound",   "SOUND",   "the waveform, its envelope, its triggers"),
    ("05_scene_pitch",   "PITCH",   "12 pitches on a ring; just intonation bends the spacing"),
    ("06_scene_harmony", "HARMONY", "the chord as a stack on a stave"),
    ("07_scene_room",    "ROOM",    "how far the sound carries, and what rings above it"),
    ("08_scene_time",    "TIME",    "repeats, and how hard they smear"),
    ("09_scene_texture", "TEXTURE", "grain density, and how worn it is"),
    ("10_scene_motion",  "MOTION",  "the drift, as an orbit"),
    ("11_scene_fx",      "FX",      "the chain, and which link is lit"),
]
PAIR = [("12_room_small", "Room · small"), ("13_room_large", "Room · large")]
WHEEL = [("01_wheel_world", "the wheel — navigation"),
         ("02_wheel_rotating", "mid-snap")]

SCALE, GAP, PAD = 2, 16, 34
BG, FG, DIM, ACC = (14, 14, 16), (240, 240, 242), (150, 150, 156), (124, 240, 132)


def font(size, bold=False):
    p = "/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf" % ("-Bold" if bold else "")
    return ImageFont.truetype(p, size) if pathlib.Path(p).exists() \
        else ImageFont.load_default()


def main() -> int:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    d, out = pathlib.Path(sys.argv[1]), sys.argv[2]

    def load(tag):
        p = d / f"{tag}.ppm"
        if not p.exists():
            sys.exit(f"missing {p} — run render_wheel first")
        return Image.open(p).convert("RGB")

    tw, th = 320 * SCALE, 170 * SCALE
    cols, rows = 3, 3
    head, cap = 108, 46
    W = PAD * 2 + cols * tw + (cols - 1) * GAP
    H = head + rows * (th + cap) + 150 + cap + th + 60

    sheet = Image.new("RGB", (W, H), BG)
    dr = ImageDraw.Draw(sheet)

    dr.text((PAD, 24), "AMBIENT · one scene per group · 320×170",
            font=font(30, True), fill=FG)
    dr.text((PAD, 62),
            "monoline at 1.5 px, one dim grey for structure, one blue for "
            "information, one green for what the hand is moving — the discipline "
            "measured off the OP-1, not its palette.",
            font=font(16), fill=DIM)

    y = head
    for i, (tag, name, sub) in enumerate(SCENES):
        r, c = divmod(i, cols)
        x = PAD + c * (tw + GAP)
        yy = head + r * (th + cap)
        sheet.paste(load(tag).resize((tw, th), Image.NEAREST), (x, yy))
        dr.rectangle([x - 1, yy - 1, x + tw, yy + th], outline=(60, 60, 66))
        dr.text((x, yy + th + 6), name, font=font(18, True), fill=ACC)
        dr.text((x + 90, yy + th + 8), sub, font=font(14), fill=DIM)
    y = head + rows * (th + cap) + 24

    dr.text((PAD, y), "the same scene at two settings — the geometry is the value",
            font=font(20, True), fill=FG)
    y += 34
    for i, (tag, lab) in enumerate(PAIR + WHEEL):
        x = PAD + i * (320 + GAP * 2)
        sheet.paste(load(tag), (x, y))
        dr.rectangle([x - 1, y - 1, x + 320, y + 170], outline=(60, 60, 66))
        dr.text((x, y + 176), lab, font=font(14), fill=DIM)

    sheet.save(out)
    print(f"{out}  {sheet.width}x{sheet.height}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
