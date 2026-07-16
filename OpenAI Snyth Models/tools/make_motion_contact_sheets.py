#!/usr/bin/env python3
"""Build labelled overviews of the original motion-study renderers."""

from __future__ import annotations

import pathlib

from PIL import Image, ImageDraw, ImageFont


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "ui" / "previews" / "motion-studies"
OUTPUT_ALL = ROOT / "ui" / "previews" / "motion-contact-sheet.png"
OUTPUT_PRIMARY = ROOT / "ui" / "previews" / "motion-primary-contact-sheet.png"

VARIANTS = [
    ("focus-rail-ghost-orchid", "focus-rail-solar-ink"),
    ("prism-veins-ion-violet", "prism-veins-tidal-prism"),
    ("crystal-choir-arctic-bloom", "crystal-choir-nacre-dawn"),
    ("radiant-gate-deep-coral", "radiant-gate-ion-violet"),
    ("lumen-ribbon-biolume", "lumen-ribbon-acid-petal"),
    ("glyph-relay-tidal-prism", "glyph-relay-lunar-peach"),
    ("particle-current-ghost-orchid", "particle-current-arctic-bloom"),
    ("signal-chamber-solar-ink", "signal-chamber-biolume"),
    ("resonance-orb-nacre-dawn", "resonance-orb-ion-violet"),
    ("glitch-halo-acid-petal", "glitch-halo-tidal-prism"),
    ("twin-pulse-deep-coral", "twin-pulse-arctic-bloom"),
    ("chroma-fall-acid-petal", "chroma-fall-biolume"),
    ("softburst-lunar-peach", "softburst-tidal-prism"),
]


def build(slugs: list[str], columns: int, output: pathlib.Path) -> None:
    tile_w, image_h, label_h, gap = 320, 170, 28, 12
    rows = (len(slugs) + columns - 1) // columns
    canvas = Image.new(
        "RGB",
        (columns * tile_w + (columns + 1) * gap,
         rows * (image_h + label_h) + (rows + 1) * gap),
        (4, 6, 12),
    )
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default(size=15)
    for index, slug in enumerate(slugs):
        path = SOURCE / f"{slug}.png"
        if not path.exists():
            raise SystemExit(f"missing still {path}")
        column = index % columns
        row = index // columns
        x = gap + column * (tile_w + gap)
        y = gap + row * (image_h + label_h + gap)
        with Image.open(path) as source:
            canvas.paste(source.convert("RGB"), (x, y))
        draw.text((x + 5, y + image_h + 6), slug.replace("-", " ").upper(),
                  fill=(218, 225, 239), font=font)
    canvas.save(output, optimize=True)
    print(f"wrote {output}")


def main() -> None:
    expected = {slug for pair in VARIANTS for slug in pair}
    actual = {path.stem for path in SOURCE.glob("*.png")}
    if actual != expected:
        raise SystemExit(
            f"motion still set differs: missing={sorted(expected - actual)}, "
            f"extra={sorted(actual - expected)}"
        )
    build([slug for pair in VARIANTS for slug in pair], 4, OUTPUT_ALL)
    build([pair[0] for pair in VARIANTS], 3, OUTPUT_PRIMARY)


if __name__ == "__main__":
    main()
