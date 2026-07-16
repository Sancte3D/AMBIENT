#!/usr/bin/env python3
"""Assert that every motion study is animated, chromatic, and distinct."""

from __future__ import annotations

import hashlib
import pathlib

from PIL import Image, ImageChops, ImageStat


ROOT = pathlib.Path(__file__).resolve().parents[1]
DIRECTORY = ROOT / "ui" / "previews" / "motion-studies"
EXPECTED = 26


def verify(path: pathlib.Path) -> str:
    hashes: set[str] = set()
    visible_colours: set[tuple[int, int, int]] = set()
    first: Image.Image | None = None
    last: Image.Image | None = None
    with Image.open(path) as image:
        if image.n_frames != 96 or image.size != (320, 170):
            raise RuntimeError(f"{path.name}: invalid frame count or geometry")
        for frame_index in (0, 24, 48, 72, 95):
            image.seek(frame_index)
            rgb = image.convert("RGB")
            hashes.add(hashlib.sha256(rgb.tobytes()).hexdigest())
            if frame_index == 0:
                first = rgb.copy()
            if frame_index == 95:
                last = rgb.copy()
            for count, colour in rgb.getcolors(maxcolors=256 * 256) or []:
                if count and max(colour) >= 20 and max(colour) - min(colour) >= 10:
                    visible_colours.add(colour)
    if len(hashes) < 4:
        raise RuntimeError(f"{path.name}: sampled frames do not animate enough")
    if len(visible_colours) < 12:
        raise RuntimeError(f"{path.name}: insufficient chromatic range")
    assert first is not None and last is not None
    seam = ImageStat.Stat(ImageChops.difference(first, last).convert("L")).mean[0]
    if seam > 70.0:
        raise RuntimeError(f"{path.name}: excessive loop seam ({seam:.1f})")
    return hashlib.sha256(first.tobytes()).hexdigest()


def main() -> None:
    paths = sorted(DIRECTORY.glob("*.gif"))
    if len(paths) != EXPECTED:
        raise SystemExit(f"expected {EXPECTED} motion GIFs, found {len(paths)}")
    first_frame_hashes = [verify(path) for path in paths]
    if len(set(first_frame_hashes)) != EXPECTED:
        raise RuntimeError("two motion-study first frames are identical")
    print(f"motion GIF verification: {EXPECTED}/{EXPECTED} animated, chromatic, and distinct")


if __name__ == "__main__":
    main()
