#!/usr/bin/env python3
"""Quality gates for the cinematic 320 x 170 master loops."""

from __future__ import annotations

import hashlib
import pathlib
from statistics import mean

from PIL import Image, ImageChops, ImageStat


ROOT = pathlib.Path(__file__).resolve().parents[1]
DIRECTORY = ROOT / "ui" / "previews" / "high-quality"
EXPECTED_DELAYS = {
    "radiant-threshold": 40,
    "spectral-horizon": 50,
    "prism-rain": 40,
    "biolume-weave": 60,
    "electric-script": 80,
    "particle-apparition": 40,
    "voxel-bloom": 50,
    "noise-chrysalis": 40,
    "wire-rain": 40,
    "focus-rail-hq": 80,
}
CHROMATIC = {
    "radiant-threshold",
    "spectral-horizon",
    "prism-rain",
    "biolume-weave",
    "electric-script",
    "particle-apparition",
    "voxel-bloom",
}


def difference_mean(a: Image.Image, b: Image.Image) -> float:
    return sum(ImageStat.Stat(ImageChops.difference(a, b)).mean) / 3.0


def verify(slug: str, delay: int) -> tuple[float, float, int]:
    path = DIRECTORY / f"{slug}.gif"
    png_path = DIRECTORY / f"{slug}.png"
    sheet_path = DIRECTORY / "frame-sheets" / f"{slug}-all-frames.png"
    if not path.is_file() or not png_path.is_file() or not sheet_path.is_file():
        raise RuntimeError(f"{slug}: GIF, still, or frame sheet missing")

    frames: list[Image.Image] = []
    durations: set[int] = set()
    with Image.open(path) as image:
        if image.size != (320, 170) or image.n_frames != 48:
            raise RuntimeError(f"{slug}: expected 48 frames at 320 x 170")
        if image.info.get("loop") != 0:
            raise RuntimeError(f"{slug}: GIF must loop forever")
        for index in range(image.n_frames):
            image.seek(index)
            durations.add(int(image.info.get("duration", 0)))
            frames.append(image.convert("RGB"))
    if durations != {delay}:
        raise RuntimeError(f"{slug}: unexpected frame delays {sorted(durations)}")

    unique = len({hashlib.sha256(frame.tobytes()).digest() for frame in frames})
    if unique < 24:
        raise RuntimeError(f"{slug}: only {unique}/48 distinct frames")

    adjacent = [difference_mean(frames[index - 1], frames[index])
                for index in range(1, len(frames))]
    movement = mean(adjacent)
    if movement < 0.22:
        raise RuntimeError(f"{slug}: motion is effectively static ({movement:.3f})")
    seam = difference_mean(frames[-1], frames[0])
    # Stepped/pixel systems intentionally hold several frames and then jump;
    # compare their loop closure with the largest authored in-loop transition,
    # not only with an average diluted by those holds.
    if seam > max(4.0, movement * 2.6, max(adjacent) * 1.15):
        raise RuntimeError(f"{slug}: loop seam {seam:.2f}, motion baseline {movement:.2f}")

    representative = frames[len(frames) // 2]
    pixels = list(representative.get_flattened_data())
    dark = sum(1 for colour in pixels if max(colour) <= 9)
    halo = sum(1 for colour in pixels if 12 <= max(colour) <= 190)
    required_dark = 0.025 if slug == "radiant-threshold" else 0.18
    if dark / len(pixels) < required_dark:
        raise RuntimeError(f"{slug}: insufficient negative space")
    if halo / len(pixels) < 0.015:
        raise RuntimeError(f"{slug}: missing mid-level light volume")
    if slug in CHROMATIC:
        visible = [colour for colour in pixels if max(colour) >= 28]
        chromatic = [colour for colour in visible
                     if max(colour) - min(colour) >= 28]
        if not visible or len(chromatic) / len(visible) < 0.12:
            raise RuntimeError(f"{slug}: colour body is too neutral")

    with Image.open(png_path) as still_image:
        still = still_image.convert("RGB")
    closest = min(difference_mean(frame, still) for frame in frames)
    if closest > 6.0:
        raise RuntimeError(f"{slug}: still-to-GIF drift is {closest:.2f}")
    return movement, seam, unique


def main() -> None:
    gifs = sorted(DIRECTORY.glob("*.gif"))
    actual = {path.stem for path in gifs}
    expected = set(EXPECTED_DELAYS)
    if actual != expected:
        raise SystemExit(
            f"high-quality GIF set mismatch: missing={sorted(expected - actual)}, "
            f"extra={sorted(actual - expected)}"
        )
    for slug, delay in EXPECTED_DELAYS.items():
        movement, seam, unique = verify(slug, delay)
        print(f"{slug:22} motion={movement:5.2f} seam={seam:5.2f} unique={unique}/48")
    print("high-quality visual verification: 10/10 passed")


if __name__ == "__main__":
    main()
