#!/usr/bin/env python3
"""Build a labelled overview of every generated colour study."""

from __future__ import annotations

import pathlib

from PIL import Image, ImageDraw, ImageFont


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "ui" / "previews" / "color"
OUTPUT = ROOT / "ui" / "previews" / "color-contact-sheet.png"


def main() -> None:
    paths = sorted(SOURCE.glob("*.png"))
    if len(paths) != 15:
        raise SystemExit(f"expected 15 colour stills, found {len(paths)}")
    columns = 3
    tile_w, image_h, label_h, gap = 320, 170, 28, 12
    rows = (len(paths) + columns - 1) // columns
    canvas = Image.new(
        "RGB",
        (columns * tile_w + (columns + 1) * gap,
         rows * (image_h + label_h) + (rows + 1) * gap),
        (5, 7, 13),
    )
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default(size=15)
    for index, path in enumerate(paths):
        column = index % columns
        row = index // columns
        x = gap + column * (tile_w + gap)
        y = gap + row * (image_h + label_h + gap)
        with Image.open(path) as source:
            canvas.paste(source.convert("RGB"), (x, y))
        label = path.stem.replace("-", " ").upper()
        draw.text((x + 5, y + image_h + 6), label, fill=(218, 225, 239), font=font)
    canvas.save(OUTPUT, optimize=True)
    print(f"wrote {OUTPUT}")


if __name__ == "__main__":
    main()
