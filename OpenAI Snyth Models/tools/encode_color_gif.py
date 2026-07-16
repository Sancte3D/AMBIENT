#!/usr/bin/env python3
"""Encode C-rendered RGB frames with one stable, high-quality GIF palette."""

from __future__ import annotations

import argparse
import glob
import pathlib

from PIL import Image


def build_global_palette(paths: list[pathlib.Path]) -> Image.Image:
    sample_count = min(12, len(paths))
    indices = [round(i * (len(paths) - 1) / max(1, sample_count - 1)) for i in range(sample_count)]
    atlas = Image.new("RGB", (80 * 4, 43 * 3), (0, 0, 0))
    for slot, frame_index in enumerate(indices):
        with Image.open(paths[frame_index]) as image:
            sample = image.convert("RGB").resize((80, 43), Image.Resampling.BILINEAR)
        atlas.paste(sample, ((slot % 4) * 80, (slot // 4) * 43))
    return atlas.quantize(colors=192, method=Image.Quantize.MEDIANCUT)


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
            frame = rgb.quantize(palette=palette, dither=Image.Dither.FLOYDSTEINBERG)
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
        optimize=True,
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
