#!/usr/bin/env python3
"""Assert that colour studies are animated, chromatic, and correctly sized."""

from __future__ import annotations

import hashlib
import pathlib

from PIL import Image, ImageChops, ImageStat


ROOT = pathlib.Path(__file__).resolve().parents[1]
DIRECTORY = ROOT / "ui" / "previews" / "color"


def verify(path: pathlib.Path) -> None:
    hashes: set[str] = set()
    colours: set[tuple[int, int, int]] = set()
    visible_pixels = 0
    colour_body_pixels = 0
    neon_pixels = 0
    midpoint: Image.Image | None = None
    with Image.open(path) as image:
        if image.n_frames != 96 or image.size != (320, 170):
            raise RuntimeError(f"{path.name}: invalid frame count or geometry")
        for frame_index in (0, 24, 48, 72):
            image.seek(frame_index)
            rgb = image.convert("RGB")
            if frame_index == 48:
                midpoint = rgb.copy()
            raw = rgb.tobytes()
            hashes.add(hashlib.sha256(raw).hexdigest())
            for count, colour in rgb.getcolors(maxcolors=256 * 256) or []:
                maximum = max(colour)
                minimum = min(colour)
                if maximum >= 20:
                    colours.add(colour)
                    visible_pixels += count
                    if maximum < 80:
                        continue
                    if minimum >= 150 and maximum - minimum < 100:
                        continue
                    colour_body_pixels += count
                    if (maximum - minimum) * 100 >= maximum * 65:
                        neon_pixels += count
    if len(hashes) < 4:
        raise RuntimeError(f"{path.name}: sampled frames are not all animated")
    if len(colours) < 12:
        raise RuntimeError(f"{path.name}: only {len(colours)} visible colours")
    if not visible_pixels or colour_body_pixels * 100 < visible_pixels * 3:
        raise RuntimeError(f"{path.name}: no substantial neon accent body")
    if not colour_body_pixels or neon_pixels * 100 < colour_body_pixels * 65:
        ratio = neon_pixels / colour_body_pixels if colour_body_pixels else 0.0
        raise RuntimeError(f"{path.name}: neon saturation ratio only {ratio:.1%}")
    if midpoint is None:
        raise RuntimeError(f"{path.name}: midpoint frame missing")
    with Image.open(path.with_suffix(".png")) as still:
        difference = ImageChops.difference(midpoint, still.convert("RGB"))
    mean_error = sum(ImageStat.Stat(difference).mean) / 3.0
    if mean_error > 5.0:
        raise RuntimeError(f"{path.name}: GIF colour drift is {mean_error:.2f}")


def main() -> None:
    paths = sorted(DIRECTORY.glob("*.gif"))
    if len(paths) != 15:
        raise SystemExit(f"expected 15 colour GIFs, found {len(paths)}")
    for path in paths:
        verify(path)
    print("colour GIF verification: 15/15 animated with >=65% neon pixels")


if __name__ == "__main__":
    main()
