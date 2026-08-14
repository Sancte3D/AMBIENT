#!/usr/bin/env python3
"""
Assemble the radial-navigation flow sheet from render_wheel's PPM output.

Tiles are integer-scaled with NEAREST, so the sheet shows real device pixels.
The 1:1 row at the bottom is the honest one — that is the size the panel is.

  /tmp/render_wheel <dir> && python3 tools/make_wheel_sheet.py <dir> <out.png>
"""
import pathlib
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("needs pillow:  pip install pillow")

ROWS = [
    ("rotate — the structure turns under a fixed selection point", [
        ("01_main_world",     "main wheel · World"),
        ("02_main_rotating",  "mid-snap · icons stay upright"),
        ("03_main_space",     "settled on Room"),
    ]),
    ("press — the same geometry goes one level in", [
        ("04_group_space",    "group wheel · Room"),
        ("05_group_shimmer",  "rotated to Shimmer"),
        ("09_group_synth",    "a three-member group"),
    ]),
    ("press again — the circle becomes the parameter", [
        ("06_value_shimmer",  "value · branches retracted"),
        ("07_value_turned",   "turned up"),
        ("08_value_world",    "discrete value on the same ring"),
    ]),
]

SCALE, GAP, PAD = 2, 18, 34
BG, FG, DIM, ACC = (14, 14, 16), (240, 240, 242), (150, 150, 156), (124, 240, 132)


def font(size, bold=False):
    p = "/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf" % ("-Bold" if bold else "")
    return ImageFont.truetype(p, size) if pathlib.Path(p).exists() \
        else ImageFont.load_default()


def main() -> int:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    d, out = pathlib.Path(sys.argv[1]), sys.argv[2]

    tiles = {}
    for _, shots in ROWS:
        for tag, _ in shots:
            p = d / f"{tag}.ppm"
            if not p.exists():
                sys.exit(f"missing {p} — run render_wheel first")
            tiles[tag] = Image.open(p).convert("RGB")

    tw, th = 320 * SCALE, 170 * SCALE
    head, rowhead, cap = 96, 46, 26
    W = PAD * 2 + 3 * tw + 2 * GAP
    H = head + len(ROWS) * (rowhead + th + cap + GAP) + PAD + 150

    sheet = Image.new("RGB", (W, H), BG)
    dr = ImageDraw.Draw(sheet)

    dr.text((PAD, 24), "AMBIENT · radial navigation · 320×170",
            font=font(30, True), fill=FG)
    dr.text((PAD, 62),
            "hub centre (158, 171) — one pixel below the bottom edge · node orbit "
            "104 px · ring 64 px · branches 40° apart, 9 groups",
            font=font(17), fill=DIM)

    y = head
    for title, shots in ROWS:
        dr.text((PAD, y + 4), title, font=font(21, True), fill=ACC)
        y += rowhead
        for i, (tag, scap) in enumerate(shots):
            x = PAD + i * (tw + GAP)
            sheet.paste(tiles[tag].resize((tw, th), Image.NEAREST), (x, y))
            dr.rectangle([x - 1, y - 1, x + tw, y + th], outline=(60, 60, 66))
            dr.text((x, y + th + 6), scap, font=font(16), fill=DIM)
        y += th + cap + GAP

    dr.text((PAD, y + 6), "1:1 — actual size on the panel",
            font=font(20, True), fill=FG)
    y += 40
    for i, tag in enumerate(["01_main_world", "04_group_space", "06_value_shimmer"]):
        x = PAD + i * (320 + GAP * 2)
        sheet.paste(tiles[tag], (x, y))
        dr.rectangle([x - 1, y - 1, x + 320, y + 170], outline=(60, 60, 66))

    sheet.save(out)
    print(f"{out}  {sheet.width}x{sheet.height}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
