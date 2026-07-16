#!/usr/bin/env python3
"""Assert that colour studies are animated, chromatic, and correctly sized."""

from __future__ import annotations

import hashlib
import pathlib

from PIL import Image


ROOT = pathlib.Path(__file__).resolve().parents[1]
DIRECTORY = ROOT / "ui" / "previews" / "color"


def verify(path: pathlib.Path) -> None:
    hashes: set[str] = set()
    colours: set[tuple[int, int, int]] = set()
    chromatic_pixels = 0
    with Image.open(path) as image:
        if image.n_frames != 96 or image.size != (320, 170):
            raise RuntimeError(f"{path.name}: invalid frame count or geometry")
        for frame_index in (0, 24, 48, 72):
            image.seek(frame_index)
            rgb = image.convert("RGB")
            raw = rgb.tobytes()
            hashes.add(hashlib.sha256(raw).hexdigest())
            for count, colour in rgb.getcolors(maxcolors=256 * 256) or []:
                if max(colour) > 16:
                    colours.add(colour)
                    if max(colour) - min(colour) >= 12:
                        chromatic_pixels += count
    if len(hashes) < 4:
        raise RuntimeError(f"{path.name}: sampled frames are not all animated")
    if len(colours) < 12:
        raise RuntimeError(f"{path.name}: only {len(colours)} visible colours")
    if chromatic_pixels < 150:
        raise RuntimeError(f"{path.name}: insufficient chromatic content")


def main() -> None:
    paths = sorted(DIRECTORY.glob("*.gif"))
    if len(paths) != 15:
        raise SystemExit(f"expected 15 colour GIFs, found {len(paths)}")
    for path in paths:
        verify(path)
    print("colour GIF verification: 15/15 animated and chromatic")


if __name__ == "__main__":
    main()
