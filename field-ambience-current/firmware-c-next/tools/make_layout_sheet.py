#!/usr/bin/env python3
"""
Assemble the layout comparison sheet from render_layouts' PPM output.

Every tile is the real thing scaled by an integer factor with NEAREST, so the
sheet shows actual device pixels — no resampling, no flattery. The 1:1 strip at
the bottom is the honest one: that is the size the panel really is.

  ./render_layouts <dir> && python3 tools/make_layout_sheet.py <dir> <out.png>
"""
import pathlib
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("needs pillow:  pip install pillow")

LAYOUTS = [
    ("a_focus",   "A — FOCUS",
     "one parameter, value at 19.3' · position via 16 pills"),
    ("b_context", "B — CONTEXT",
     "selected at 19.3', neighbours at 12.9' · 3 of 16 visible"),
    ("c_pages",   "C — PAGES",
     "4 pages of 4, all rows at 12.9' · closest to a list"),
]
STATES = [
    ("browse_space", "Space · continuous"),
    ("edit_world",   "World · longest value"),
    ("browse_fx",    "FX · 9 options"),
]

SCALE, GAP, PAD = 2, 18, 34
BG, FG, DIM, ACC = (14, 14, 16), (240, 240, 242), (150, 150, 156), (12, 250, 149)


def font(size, bold=False):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf"
              % ("-Bold" if bold else ""),):
        if pathlib.Path(p).exists():
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()


def main() -> int:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    d, out = pathlib.Path(sys.argv[1]), sys.argv[2]

    tiles = {}
    for lk, _, _ in LAYOUTS:
        for sk, _ in STATES:
            p = d / f"{lk}_{sk}.ppm"
            if not p.exists():
                sys.exit(f"missing {p} — run render_layouts first")
            tiles[(lk, sk)] = Image.open(p).convert("RGB")

    tw, th = 320 * SCALE, 170 * SCALE
    head, rowhead, cap = 92, 54, 26
    W = PAD * 2 + 3 * tw + 2 * GAP
    H = head + len(LAYOUTS) * (rowhead + th + cap + GAP) + PAD + 150

    sheet = Image.new("RGB", (W, H), BG)
    dr = ImageDraw.Draw(sheet)

    dr.text((PAD, 24), "AMBIENT · 320×170 · three information densities",
            font=font(30, True), fill=FG)
    dr.text((PAD, 62),
            "active area 39.1 × 21.2 mm · 0.125 mm/px · digit heights 6/12/18 px "
            "= 0.75/1.50/2.24 mm = 6.4′/12.9′/19.3′ at 40 cm",
            font=font(17), fill=DIM)

    y = head
    for lk, title, sub in LAYOUTS:
        dr.text((PAD, y + 6), title, font=font(23, True), fill=ACC)
        dr.text((PAD + 210, y + 10), sub, font=font(17), fill=DIM)
        y += rowhead
        for i, (sk, scap) in enumerate(STATES):
            x = PAD + i * (tw + GAP)
            sheet.paste(tiles[(lk, sk)].resize((tw, th), Image.NEAREST), (x, y))
            dr.rectangle([x - 1, y - 1, x + tw, y + th], outline=(60, 60, 66))
            dr.text((x, y + th + 6), scap, font=font(16), fill=DIM)
        y += th + cap + GAP

    dr.text((PAD, y + 6), "1:1 — actual size on the panel",
            font=font(20, True), fill=FG)
    y += 40
    for i, (lk, title, _) in enumerate(LAYOUTS):
        x = PAD + i * (320 + GAP * 2)
        sheet.paste(tiles[(lk, "browse_space")], (x, y))
        dr.rectangle([x - 1, y - 1, x + 320, y + 170], outline=(60, 60, 66))
        dr.text((x, y + 176), title, font=font(15), fill=DIM)

    sheet.save(out)
    print(f"{out}  {sheet.width}x{sheet.height}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
