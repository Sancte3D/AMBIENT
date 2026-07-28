#!/usr/bin/env python3
"""Calculate raw SPI display-transfer budgets for rectangular regions."""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass


REGION_RE = re.compile(
    r"^(?:(?P<name>[A-Za-z0-9_.-]+)=)?(?P<width>[1-9][0-9]*)x(?P<height>[1-9][0-9]*)$"
)


@dataclass(frozen=True)
class Region:
    name: str
    width: int
    height: int


def parse_region(value: str) -> Region:
    match = REGION_RE.fullmatch(value)
    if not match:
        raise argparse.ArgumentTypeError(
            f"invalid region {value!r}; use NAME=WIDTHxHEIGHT or WIDTHxHEIGHT"
        )
    width = int(match.group("width"))
    height = int(match.group("height"))
    return Region(match.group("name") or f"{width}x{height}", width, height)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Calculate RGB/display-region SPI wire-time budgets."
    )
    parser.add_argument(
        "--region",
        action="append",
        type=parse_region,
        help="Region as NAME=WIDTHxHEIGHT; repeat for multiple regions.",
    )
    parser.add_argument("--spi-mhz", type=float, default=30.0)
    parser.add_argument("--bpp", type=int, default=16)
    parser.add_argument("--fps", type=float, default=60.0)
    parser.add_argument(
        "--overhead-percent",
        type=float,
        default=15.0,
        help="Planning margin added to raw wire time.",
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.spi_mhz <= 0:
        raise SystemExit("--spi-mhz must be positive")
    if args.bpp <= 0:
        raise SystemExit("--bpp must be positive")
    if args.fps <= 0:
        raise SystemExit("--fps must be positive")
    if args.overhead_percent < 0:
        raise SystemExit("--overhead-percent cannot be negative")

    regions = args.region or [
        Region("full", 320, 170),
        Region("strip", 320, 40),
        Region("tile", 80, 80),
    ]
    bits_per_second = args.spi_mhz * 1_000_000.0
    frame_budget_ms = 1000.0 / args.fps
    margin = 1.0 + args.overhead_percent / 100.0

    print(
        f"SPI: {args.spi_mhz:g} MHz · Pixel format: {args.bpp} bpp · "
        f"Target: {args.fps:g} fps ({frame_budget_ms:.3f} ms/frame) · "
        f"Planning margin: {args.overhead_percent:g}%"
    )
    print()
    print("| Region | Pixels | Bytes | Raw wire ms | Planned ms | Frame budget | Raw max fps |")
    print("|---|---:|---:|---:|---:|---:|---:|")

    for region in regions:
        pixels = region.width * region.height
        bits = pixels * args.bpp
        byte_count = (bits + 7) // 8
        wire_ms = bits / bits_per_second * 1000.0
        planned_ms = wire_ms * margin
        budget_share = planned_ms / frame_budget_ms * 100.0
        raw_max_fps = 1000.0 / wire_ms
        print(
            f"| {region.name} ({region.width}×{region.height}) "
            f"| {pixels:,} | {byte_count:,} | {wire_ms:.3f} "
            f"| {planned_ms:.3f} | {budget_share:.1f}% | {raw_max_fps:.1f} |"
        )

    print()
    print(
        "Wire time excludes window commands, DMA setup, cache maintenance, "
        "rasterization, interrupt contention, and scheduling gaps."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
