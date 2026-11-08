#!/usr/bin/env python3
"""
Convert a design PNG into an uncompressed RGB565 asset for the ST7789.

The PNG is the design source; the device never sees it. This bakes it once,
offline, into the exact bytes the panel consumes — no decoder, no runtime
conversion, no CPU cost, no surprise colour shifts.

  python3 tools/png_to_rgb565.py background.png assets/background
    -> assets/background.rgb565   raw, big-endian (ST7789 wants high byte first)
    -> assets/background.c/.h     const array in .rodata (flash-resident)
    -> assets/background.preview.png   what the panel will actually show

Size is fixed by the panel: 320x170 x 2 byte = 108,800 byte. That is 5.2 % of
the H743's 2 MB flash and must NOT be copied to RAM — see the memory note in
docs/hardware/RESOURCE_BUDGET.md (D1 87 %, D2 96 %). The row compositor reads
it straight out of flash.

BANDING: RGB565 has 5/6/5 bits, so a slow wide gradient will step visibly,
especially through dark blues and greys. --dither trades a little noise for
smooth ramps and is on by default; --dither none reproduces a plain truncation
if you want to compare. Dithering happens ONCE here, offline, so it costs the
device nothing.
"""
import argparse
import pathlib
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("needs Pillow:  pip install pillow")

PANEL_W, PANEL_H = 320, 170


def to565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def expand565(v: int) -> tuple:
    """RGB565 back to 8-bit — what the panel actually emits."""
    r5, g6, b5 = (v >> 11) & 0x1F, (v >> 5) & 0x3F, v & 0x1F
    return ((r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2))


def quantise(img: Image.Image, mode: str):
    """Return (list[int] rgb565, preview Image)."""
    w, h = img.size
    src = [list(px) for px in
           [list(img.getdata())[y * w:(y + 1) * w] for y in range(h)]]
    src = [[list(p) for p in row] for row in src]
    out = []
    for y in range(h):
        for x in range(w):
            r, g, b = (max(0, min(255, int(round(c)))) for c in src[y][x])
            v = to565(r, g, b)
            out.append(v)
            if mode == "floyd":
                # error diffusion against what the panel will really show
                er, eg, eb = (a - b_ for a, b_ in zip((r, g, b), expand565(v)))
                for dx, dy, f in ((1, 0, 7 / 16), (-1, 1, 3 / 16),
                                  (0, 1, 5 / 16), (1, 1, 1 / 16)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < w and 0 <= ny < h:
                        p = src[ny][nx]
                        p[0] += er * f
                        p[1] += eg * f
                        p[2] += eb * f
    prev = Image.new("RGB", (w, h))
    prev.putdata([expand565(v) for v in out])
    return out, prev


BAYER8 = [[(x * 5 + y * 3) % 8 for x in range(8)] for y in range(8)]


def quantise_bayer(img: Image.Image):
    w, h = img.size
    px = img.load()
    out = []
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            # +/- half a quantisation step, ordered — cheaper artefacts than
            # Floyd on smooth gradients, and tiles predictably.
            t = (BAYER8[y & 7][x & 7] - 3.5) / 8.0
            r = max(0, min(255, int(r + t * 8)))
            g = max(0, min(255, int(g + t * 4)))
            b = max(0, min(255, int(b + t * 8)))
            out.append(to565(r, g, b))
    prev = Image.new("RGB", (w, h))
    prev.putdata([expand565(v) for v in out])
    return out, prev


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("png")
    ap.add_argument("out_stem", help="output path without extension")
    ap.add_argument("--dither", choices=("bayer", "floyd", "none"), default="bayer",
                    help="banding countermeasure, applied offline (default: bayer)")
    ap.add_argument("--name", default=None, help="C symbol name (default: from stem)")
    ap.add_argument("--allow-resize", action="store_true",
                    help="resize instead of rejecting a wrong-sized PNG")
    a = ap.parse_args()

    img = Image.open(a.png).convert("RGB")
    if img.size != (PANEL_W, PANEL_H):
        if not a.allow_resize:
            return (f"{a.png} is {img.width}x{img.height}, panel is "
                    f"{PANEL_W}x{PANEL_H}. Export the Figma frame at exactly that "
                    f"size and 1x scale, or pass --allow-resize.")
        img = img.resize((PANEL_W, PANEL_H), Image.LANCZOS)

    if a.dither == "bayer":
        vals, prev = quantise_bayer(img)
    elif a.dither == "floyd":
        vals, prev = quantise(img, "floyd")
    else:
        vals, prev = quantise(img, "none")

    stem = pathlib.Path(a.out_stem)
    stem.parent.mkdir(parents=True, exist_ok=True)
    name = a.name or stem.name.replace("-", "_").replace(".", "_")

    raw = bytearray()
    for v in vals:                      # big-endian: ST7789 takes high byte first
        raw.append((v >> 8) & 0xFF)
        raw.append(v & 0xFF)
    stem.with_suffix(".rgb565").write_bytes(raw)

    with stem.with_suffix(".h").open("w") as f:
        f.write(f"/* generated by tools/png_to_rgb565.py from {pathlib.Path(a.png).name}"
                f" (dither: {a.dither}) — do not edit */\n"
                f"#ifndef FAM_{name.upper()}_H\n#define FAM_{name.upper()}_H\n"
                f"#include <stdint.h>\n"
                f"#define {name.upper()}_W {PANEL_W}\n"
                f"#define {name.upper()}_H {PANEL_H}\n"
                f"extern const uint16_t {name}[{PANEL_W} * {PANEL_H}];\n#endif\n")
    with stem.with_suffix(".c").open("w") as f:
        f.write(f'/* generated by tools/png_to_rgb565.py from '
                f'{pathlib.Path(a.png).name} (dither: {a.dither}) — do not edit.\n'
                f' * {len(raw)} byte, .rodata → flash. Never memcpy this to RAM. */\n'
                f'#include "{stem.name}.h"\n\n'
                f"const uint16_t {name}[{PANEL_W} * {PANEL_H}] = {{\n")
        for i in range(0, len(vals), 12):
            f.write("    " + " ".join(f"0x{v:04X}," for v in vals[i:i + 12]) + "\n")
        f.write("};\n")

    prev.resize((PANEL_W * 3, PANEL_H * 3), Image.NEAREST) \
        .save(stem.with_suffix(".preview.png"))

    print(f"{stem.with_suffix('.rgb565')}  {len(raw)} byte "
          f"({len(raw) / 1024:.2f} KiB, {len(raw) / (2 * 1024 * 1024) * 100:.2f} % of flash)")
    print(f"{stem.with_suffix('.c')}  C array '{name}'")
    print(f"{stem.with_suffix('.preview.png')}  panel-accurate preview (3x nearest)")
    print(f"unique colours after quantisation: {len(set(vals))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
