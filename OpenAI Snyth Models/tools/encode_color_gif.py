#!/usr/bin/env python3
"""Encode C-rendered RGB frames with one stable, high-quality GIF palette."""

from __future__ import annotations

import argparse
import glob
import math
import pathlib

from PIL import Image


def build_global_palette(paths: list[pathlib.Path]) -> Image.Image:
    # The C renderer emits at most 16 exact colours per frame. Build the GIF
    # palette from those colours directly: resizing thin lines would mix them
    # with black and produce the washed-out dark palette this tool must avoid.
    colours: set[tuple[int, int, int]] = {(0, 0, 0)}
    for path in paths:
        with Image.open(path) as image:
            counts = image.convert("RGB").getcolors(maxcolors=256 * 256)
        if counts is None:
            raise RuntimeError(f"too many source colours in {path}")
        colours.update(colour for _, colour in counts)

    swatches: list[tuple[int, int, int]] = []
    for colour in sorted(colours):
        maximum = max(colour)
        minimum = min(colour)
        repeats = 2 if maximum >= 64 and maximum - minimum >= 32 else 1
        swatches.extend([colour] * repeats)
    swatches.extend([(0, 0, 0)] * max(64, len(swatches) // 10))
    width = 64
    height = math.ceil(len(swatches) / width)
    swatches.extend([(0, 0, 0)] * (width * height - len(swatches)))
    atlas = Image.new("RGB", (width, height), (0, 0, 0))
    atlas.putdata(swatches)
    return atlas.quantize(colors=240, method=Image.Quantize.MEDIANCUT)


def encode(paths: list[pathlib.Path], gif_path: pathlib.Path, still_path: pathlib.Path) -> None:
    palette = build_global_palette(paths)
    frames: list[Image.Image] = []
    still_index = len(paths) // 2
    still_rgb: Image.Image | None = None
    for index, path in enumerate(paths):
        with Image.open(path) as source:
            rgb = source.convert("RGB")
            if index == still_index:
                still_rgb = rgb.copy()
            frame = rgb.quantize(palette=palette, dither=Image.Dither.NONE)
            frames.append(frame)
    if still_rgb is None:
        raise RuntimeError("no still frame selected")

    gif_path.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        gif_path,
        save_all=True,
        append_images=frames[1:],
        duration=42,
        loop=0,
        disposal=2,
        optimize=False,
        comment=b"Sancte3D original 4-bit framebuffer colour study",
    )
    still_rgb.save(still_path, optimize=True)

    with Image.open(gif_path) as result:
        if result.n_frames != len(paths) or result.size != (320, 170):
            raise RuntimeError(f"invalid GIF result: {result.n_frames} frames at {result.size}")
    print(f"encoded {gif_path.name}: {len(paths)} frames")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("frame_glob")
    parser.add_argument("output_gif", type=pathlib.Path)
    parser.add_argument("output_png", type=pathlib.Path)
    args = parser.parse_args()
    paths = [pathlib.Path(path) for path in sorted(glob.glob(args.frame_glob))]
    if not paths:
        raise SystemExit(f"no frames matched {args.frame_glob}")
    encode(paths, args.output_gif, args.output_png)


if __name__ == "__main__":
    main()
