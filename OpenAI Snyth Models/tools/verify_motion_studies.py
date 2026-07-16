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
    visible_pixels = 0
    colour_body_pixels = 0
    neon_pixels = 0
    first: Image.Image | None = None
    last: Image.Image | None = None
    midpoint: Image.Image | None = None
    with Image.open(path) as image:
        if image.n_frames != 96 or image.size != (320, 170):
            raise RuntimeError(f"{path.name}: invalid frame count or geometry")
        for frame_index in (0, 24, 48, 72, 95):
            image.seek(frame_index)
            rgb = image.convert("RGB")
            hashes.add(hashlib.sha256(rgb.tobytes()).hexdigest())
            if frame_index == 0:
                first = rgb.copy()
            if frame_index == 48:
                midpoint = rgb.copy()
            if frame_index == 95:
                last = rgb.copy()
            for count, colour in rgb.getcolors(maxcolors=256 * 256) or []:
                maximum = max(colour)
                minimum = min(colour)
                if count and maximum >= 20:
                    visible_colours.add(colour)
                    visible_pixels += count
                    if maximum < 80:
                        continue
                    if minimum >= 150 and maximum - minimum < 100:
                        continue
                    colour_body_pixels += count
                    if (maximum - minimum) * 100 >= maximum * 65:
                        neon_pixels += count
    if len(hashes) < 4:
        raise RuntimeError(f"{path.name}: sampled frames do not animate enough")
    if len(visible_colours) < 12:
        raise RuntimeError(f"{path.name}: insufficient chromatic range")
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
    print(f"motion GIF verification: {EXPECTED}/{EXPECTED} animated, distinct, and >=65% neon")


if __name__ == "__main__":
    main()
