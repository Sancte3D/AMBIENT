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
    ("rotate — the structure turns under a fixed selection point, and the "
     "selected node is EN3's own green", [
        ("01_wheel_world",    "the wheel · World"),
        ("02_wheel_rotating", "mid-snap · icons stay upright"),
        ("02b_wheel_room",    "settled on Room"),
    ]),
    ("press — the group becomes a place, and the ring becomes three "
     "encoder-coloured slots", [
        ("07_scene_room",     "Room · red = EN1 Space, blue = EN2 Shimmer"),
        ("12_room_small",     "EN1 at 0 — its slot is a bare head dot"),
        ("13_room_large",     "EN1 at 100 — EN2 untouched, as its slot shows"),
    ]),
    ("one drawing, three live properties — no property names anywhere", [
        ("04_scene_sound",    "Sound · the only group that uses all three"),
        ("05_scene_pitch",    "Pitch · just intonation bends the spacing"),
        ("03_scene_world",    "World · one property, two dim slots"),
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
    # 46 for the 1:1 heading, 170 for the tiles themselves, then padding — the
    # old constant 150 was short by the tile height and cropped the row that is
    # supposed to be the honest one.
    H = head + len(ROWS) * (rowhead + th + cap + GAP) + 46 + 170 + PAD

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
    for i, tag in enumerate(["01_wheel_world", "07_scene_room", "04_scene_sound"]):
        x = PAD + i * (320 + GAP * 2)
        sheet.paste(tiles[tag], (x, y))
        dr.rectangle([x - 1, y - 1, x + 320, y + 170], outline=(60, 60, 66))

    sheet.save(out)
    print(f"{out}  {sheet.width}x{sheet.height}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
