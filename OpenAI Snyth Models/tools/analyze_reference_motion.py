#!/usr/bin/env python3
"""Measure motion references without importing their pixels into the project.

The tool accepts arbitrary local image paths, prints a compact JSON report, and
can optionally create a temporary representative-frame sheet for human review.
It deliberately records no source image or frame in the repository.
"""

from __future__ import annotations

import argparse
import json
import pathlib
from statistics import mean

from PIL import Image, ImageChops, ImageDraw, ImageFont, ImageStat


ANALYSIS_SIZE = (160, 160)


def normalized_frame(source: Image.Image) -> Image.Image:
    frame = source.convert("RGB")
    frame.thumbnail(ANALYSIS_SIZE, Image.Resampling.BILINEAR)
    canvas = Image.new("RGB", ANALYSIS_SIZE, (0, 0, 0))
    canvas.paste(frame, ((ANALYSIS_SIZE[0] - frame.width) // 2,
                         (ANALYSIS_SIZE[1] - frame.height) // 2))
    return canvas


def frame_duration_ms(image: Image.Image) -> int:
    value = image.info.get("duration", 0)
    return int(value) if isinstance(value, (int, float)) else 0


def analyse(path: pathlib.Path) -> tuple[dict[str, object], list[Image.Image]]:
    with Image.open(path) as image:
        frame_count = getattr(image, "n_frames", 1)
        indices = sorted({0, frame_count // 4, frame_count // 2,
                          (frame_count * 3) // 4, frame_count - 1})
        representative: list[Image.Image] = []
        lumas: list[float] = []
        delays: list[int] = []
        active_fractions: list[float] = []
        chromatic_fractions: list[float] = []
        motion_means: list[float] = []
        motion_x = 0.0
        motion_y = 0.0
        motion_weight = 0.0
        motion_bounds: tuple[int, int, int, int] | None = None
        first: Image.Image | None = None
        previous: Image.Image | None = None
        last: Image.Image | None = None

        for index in range(frame_count):
            image.seek(index)
            frame = normalized_frame(image)
            grey = frame.convert("L")
            if index in indices:
                representative.append(frame.copy())
            lumas.append(float(ImageStat.Stat(grey).mean[0]))
            delays.append(frame_duration_ms(image))

            if index in indices:
                rgb_data = list(frame.get_flattened_data())
                active = sum(1 for r, g, b in rgb_data if max(r, g, b) >= 24)
                chromatic = sum(
                    1 for r, g, b in rgb_data
                    if max(r, g, b) >= 24 and max(r, g, b) - min(r, g, b) >= 18
                )
                active_fractions.append(active / len(rgb_data))
                chromatic_fractions.append(chromatic / len(rgb_data))

            if first is None:
                first = grey.copy()
            if previous is not None:
                diff = ImageChops.difference(previous, grey)
                values = list(diff.get_flattened_data())
                motion_means.append(sum(values) / (255.0 * len(values)))
                thresholded = diff.point(lambda value: value if value >= 12 else 0)
                bbox = thresholded.getbbox()
                if bbox is not None:
                    if motion_bounds is None:
                        motion_bounds = bbox
                    else:
                        motion_bounds = (
                            min(motion_bounds[0], bbox[0]),
                            min(motion_bounds[1], bbox[1]),
                            max(motion_bounds[2], bbox[2]),
                            max(motion_bounds[3], bbox[3]),
                        )
                for offset, value in enumerate(values):
                    if value < 12:
                        continue
                    x = offset % ANALYSIS_SIZE[0]
                    y = offset // ANALYSIS_SIZE[0]
                    motion_x += x * value
                    motion_y += y * value
                    motion_weight += value
            previous = grey
            last = grey.copy()

        assert first is not None and last is not None
        loop_diff = ImageChops.difference(first, last)
        loop_seam = float(ImageStat.Stat(loop_diff).mean[0]) / 255.0
        total_ms = sum(delay for delay in delays if delay > 0)
        if total_ms <= 0:
            total_ms = 0
        positive_delays = [delay for delay in delays if delay > 0]
        fps = frame_count * 1000.0 / total_ms if total_ms else 0.0

        if motion_weight:
            centroid = [round(motion_x / motion_weight / (ANALYSIS_SIZE[0] - 1), 3),
                        round(motion_y / motion_weight / (ANALYSIS_SIZE[1] - 1), 3)]
        else:
            centroid = None
        if motion_bounds is not None:
            bounds = [
                round(motion_bounds[0] / ANALYSIS_SIZE[0], 3),
                round(motion_bounds[1] / ANALYSIS_SIZE[1], 3),
                round(motion_bounds[2] / ANALYSIS_SIZE[0], 3),
                round(motion_bounds[3] / ANALYSIS_SIZE[1], 3),
            ]
        else:
            bounds = None

        report: dict[str, object] = {
            "id": path.stem,
            "format": image.format,
            "geometry": [image.width, image.height],
            "frames": frame_count,
            "duration_ms": total_ms,
            "estimated_fps": round(fps, 2),
            "frame_delay_ms": [min(positive_delays), max(positive_delays)]
            if positive_delays else [0, 0],
            "mean_luma_0_255": round(mean(lumas), 2),
            "luma_span_0_255": round(max(lumas) - min(lumas), 2),
            "active_pixel_fraction": round(mean(active_fractions), 4),
            "chromatic_pixel_fraction": round(mean(chromatic_fractions), 4),
            "mean_interframe_change": round(mean(motion_means), 5)
            if motion_means else 0.0,
            "loop_seam_change": round(loop_seam, 5),
            "motion_centroid_normalized": centroid,
            "motion_bounds_normalized": bounds,
            "loop": image.info.get("loop"),
        }
        return report, representative


def make_sheet(entries: list[tuple[dict[str, object], list[Image.Image]]],
               output: pathlib.Path) -> None:
    tile_w, tile_h, gap, label_h = 160, 160, 8, 38
    columns = 5
    rows = len(entries)
    canvas = Image.new(
        "RGB",
        (columns * tile_w + (columns + 1) * gap,
         rows * (tile_h + label_h) + (rows + 1) * gap),
        (4, 6, 11),
    )
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default(size=12)
    for row, (report, frames) in enumerate(entries):
        y = gap + row * (tile_h + label_h + gap)
        padded = frames + [frames[-1]] * (columns - len(frames))
        for column, frame in enumerate(padded[:columns]):
            x = gap + column * (tile_w + gap)
            canvas.paste(frame, (x, y))
        label = (
            f"{report['id'][:30]}  {report['frames']}f / "
            f"{report['duration_ms']}ms  motion={report['mean_interframe_change']}"
        )
        draw.text((gap + 4, y + tile_h + 6), label, fill=(218, 225, 239), font=font)
    output.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output, optimize=True)


def make_full_frame_sheet(path: pathlib.Path, output: pathlib.Path,
                          columns: int) -> None:
    """Write every composited frame to a numbered, review-only contact sheet."""
    tile_w, tile_h, label_h, gap = 112, 96, 18, 6
    with Image.open(path) as image:
        frame_count = getattr(image, "n_frames", 1)
        rows = (frame_count + columns - 1) // columns
        canvas = Image.new(
            "RGB",
            (columns * tile_w + (columns + 1) * gap,
             rows * (tile_h + label_h) + (rows + 1) * gap),
            (3, 4, 8),
        )
        draw = ImageDraw.Draw(canvas)
        font = ImageFont.load_default(size=11)
        previous: Image.Image | None = None
        for index in range(frame_count):
            image.seek(index)
            frame = image.convert("RGB")
            frame.thumbnail((tile_w, tile_h), Image.Resampling.LANCZOS)
            tile = Image.new("RGB", (tile_w, tile_h), (0, 0, 0))
            tile.paste(frame, ((tile_w - frame.width) // 2,
                               (tile_h - frame.height) // 2))
            column = index % columns
            row = index // columns
            x = gap + column * (tile_w + gap)
            y = gap + row * (tile_h + label_h + gap)
            canvas.paste(tile, (x, y))

            grey = tile.convert("L")
            if previous is None:
                delta = 0.0
            else:
                delta = float(ImageStat.Stat(
                    ImageChops.difference(previous, grey)
                ).mean[0]) / 255.0
            delay = frame_duration_ms(image)
            draw.text((x + 2, y + tile_h + 3),
                      f"{index:03d}  {delay:03d}ms  d{delta:.3f}",
                      fill=(205, 212, 225), font=font)
            previous = grey

    output.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output, optimize=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("images", nargs="+", type=pathlib.Path)
    parser.add_argument("--json", type=pathlib.Path)
    parser.add_argument("--contact-sheet", type=pathlib.Path)
    parser.add_argument("--full-sheet-dir", type=pathlib.Path)
    parser.add_argument("--sheet-columns", type=int, default=12)
    args = parser.parse_args()

    entries = [analyse(path) for path in args.images]
    reports = [report for report, _ in entries]
    payload = json.dumps(reports, indent=2, ensure_ascii=False)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(payload + "\n", encoding="utf-8")
    else:
        print(payload)
    if args.contact_sheet:
        make_sheet(entries, args.contact_sheet)
    if args.full_sheet_dir:
        if args.sheet_columns < 1 or args.sheet_columns > 24:
            raise SystemExit("--sheet-columns must be between 1 and 24")
        for path in args.images:
            make_full_frame_sheet(
                path,
                args.full_sheet_dir / f"{path.stem}-all-frames.png",
                args.sheet_columns,
            )


if __name__ == "__main__":
    main()
