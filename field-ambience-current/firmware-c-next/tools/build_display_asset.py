#!/usr/bin/env python3
"""
Display asset pipeline for the 320x170 ST7789.

THE RULE: 320x170 is the output medium, not the design canvas. Artwork is
composed at 6x (1920x1020, the exact 32:17 aspect) and reduced at the very
end. Drawing atmospheric imagery directly at panel size produces coarse shapes
and primitive gradients — downsampling instead *merges* fine detail into each
final pixel, which is why real photographs read convincingly on panels this
small.

    1. compose / load the background at 1920x1020
    2. all photographic work — fog, lighting, texture, grain — at full res
    3. downsample to 320x170 with Lanczos (area for pure photos)
    4. subtle sharpening only
    5. render text, values, icons and thin lines separately AT 320x170
    6. convert to RGB565
    7. ordered dithering where gradients would band

Steps 1-4 and 5 are deliberately split: shrinking a 6x-rendered glyph turns it
to mush, so type is drawn at native resolution after the downsample. Bitcount
additionally has to land on its own grid — see generate_fonts_bitcount.py.

Blending happens in LINEAR light, not sRGB. Averaging gamma-encoded values
darkens and muddies every transition; it is the single biggest reason
procedural gradients look cheap.

  python3 tools/build_display_asset.py --out assets/background
  python3 tools/build_display_asset.py --source figma_1920x1020.png \
                                       --out assets/background

Writes the four-stage preview the design review needs: the high-resolution
composition, the downsampled result, the simulated RGB565, and a
nearest-neighbour enlargement of the actual LCD pixels.
"""
import argparse
import pathlib
import sys

try:
    import numpy as np
    from PIL import Image, ImageFilter
except ImportError:
    sys.exit("needs pillow + numpy:  pip install pillow numpy")

PANEL = (320, 170)
SCALE = 6                                   # 1920 x 1020
HIRES = (PANEL[0] * SCALE, PANEL[1] * SCALE)

assert HIRES[0] / HIRES[1] == PANEL[0] / PANEL[1], "aspect must stay 32:17"


# ---------------------------------------------------------------- colour ---
def srgb_to_linear(a):
    a = a / 255.0
    return np.where(a <= 0.04045, a / 12.92, ((a + 0.055) / 1.055) ** 2.4)


def linear_to_srgb(a):
    a = np.clip(a, 0.0, 1.0)
    return np.where(a <= 0.0031308, a * 12.92,
                    1.055 * a ** (1 / 2.4) - 0.055) * 255.0


# ------------------------------------------------------------ background ---
def light(shape, cx, cy, rx, ry, rgb, power, gain=1.0):
    """One soft light field, evaluated in linear light."""
    h, w = shape
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    d = np.sqrt(((xx - cx * w) / (rx * w)) ** 2 + ((yy - cy * h) / (ry * h)) ** 2)
    a = np.clip(1.0 - d, 0.0, 1.0) ** power
    return (a * gain)[..., None] * srgb_to_linear(np.array(rgb, np.float32))


def compose_background(w, h, seed=7):
    """Rose/coral atmosphere matching the reference palette, built at full
    resolution: layered light, volumetric haze, fine grain."""
    rng = np.random.default_rng(seed)
    base = srgb_to_linear(np.array([206, 118, 158], np.float32))
    img = np.ones((h, w, 3), np.float32) * base

    for cx, cy, rx, ry, rgb, p, g in [
        (0.50, 0.42, 0.85, 1.15, (250,  96, 122), 1.05, 0.95),   # broad body
        (0.62, 0.30, 0.45, 0.62, (255, 128, 132), 1.30, 0.70),   # upper warmth
        (0.30, 0.72, 0.50, 0.70, (232,  98, 156), 1.25, 0.55),   # lower rose
        (0.86, 0.62, 0.34, 0.55, (255, 150, 170), 1.45, 0.40),   # right lift
        (0.10, 0.20, 0.32, 0.44, (198, 122, 190), 1.40, 0.35),   # violet corner
    ]:
        img = img * (1.0 - 0.55) + (img + light((h, w), cx, cy, rx, ry, rgb, p, g)) * 0.55

    # volumetric haze: low-frequency noise, blurred hard, added as light
    n = rng.random((h // 24, w // 24)).astype(np.float32)
    haze = np.asarray(Image.fromarray((n * 255).astype(np.uint8))
                      .resize((w, h), Image.BICUBIC)).astype(np.float32) / 255.0
    haze = np.asarray(Image.fromarray((haze * 255).astype(np.uint8))
                      .filter(ImageFilter.GaussianBlur(w * 0.02))).astype(np.float32) / 255.0
    img += (haze - 0.5)[..., None] * 0.05

    # gentle vignette so the corners fall away instead of ending abruptly
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    v = 1.0 - 0.16 * np.clip(np.sqrt(((xx/w - .5)*2)**2 + ((yy/h - .5)*2)**2) - 0.35, 0, 1) ** 1.5
    img *= v[..., None]

    out = linear_to_srgb(img)
    # film grain, added in display space at full resolution so the downsample
    # averages it into something fine rather than blotchy
    out += rng.normal(0.0, 1.6, out.shape).astype(np.float32)
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGB")


# ------------------------------------------------------------- pipeline ---
def downsample(img, method="lanczos"):
    f = {"lanczos": Image.LANCZOS, "area": Image.BOX, "bicubic": Image.BICUBIC}[method]
    return img.resize(PANEL, f)


def sharpen(img, percent=110):
    if percent <= 0:
        return img
    return img.filter(ImageFilter.UnsharpMask(radius=0.7, percent=percent, threshold=2))


def to565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def expand565(v):
    r5, g6, b5 = (v >> 11) & 0x1F, (v >> 5) & 0x3F, v & 0x1F
    return ((r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2))


BAYER8 = np.array([[(x * 5 + y * 3) % 8 for x in range(8)] for y in range(8)],
                  np.float32)


def quantise565(img, dither=True):
    a = np.asarray(img).astype(np.float32)
    h, w, _ = a.shape
    if dither:
        t = (np.tile(BAYER8, (h // 8 + 1, w // 8 + 1))[:h, :w] - 3.5) / 8.0
        a = a + t[..., None] * np.array([8.0, 4.0, 8.0], np.float32)
    a = np.clip(a, 0, 255).astype(np.uint8)
    vals = [to565(int(r), int(g), int(b)) for r, g, b in a.reshape(-1, 3)]
    prev = Image.new("RGB", PANEL)
    prev.putdata([expand565(v) for v in vals])
    return vals, prev


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--source", help=f"background at {HIRES[0]}x{HIRES[1]} "
                                     f"(or any 32:17 image); omit to generate one")
    ap.add_argument("--out", required=True, help="output stem")
    ap.add_argument("--resample", choices=("lanczos", "area", "bicubic"), default="lanczos")
    ap.add_argument("--sharpen", type=int, default=110, help="UnsharpMask percent, 0 = off")
    ap.add_argument("--no-dither", action="store_true")
    ap.add_argument("--ui", help="optional 320x170 RGBA overlay drawn AFTER the "
                                 "downsample (text, values, thin lines)")
    a = ap.parse_args()

    # 1-2 — full-resolution composition
    if a.source:
        hi = Image.open(a.source).convert("RGB")
        if abs(hi.width / hi.height - PANEL[0] / PANEL[1]) > 0.01:
            return (f"{a.source} is {hi.width}x{hi.height}; aspect must be 32:17 "
                    f"({PANEL[0]}:{PANEL[1]}). Re-export the frame.")
        if hi.size != HIRES:
            print(f"note: source is {hi.width}x{hi.height}, working size is "
                  f"{HIRES[0]}x{HIRES[1]}")
    else:
        hi = compose_background(*HIRES)

    stem = pathlib.Path(a.out)
    stem.parent.mkdir(parents=True, exist_ok=True)
    hi.save(stem.with_suffix(".hires.png"))

    # 3-4 — reduce, then sharpen gently
    small = sharpen(downsample(hi, a.resample), a.sharpen)
    small.save(stem.with_suffix(".320.png"))

    # 5 — native-resolution UI on top, never scaled
    if a.ui:
        ui = Image.open(a.ui).convert("RGBA")
        if ui.size != PANEL:
            return f"--ui must be exactly {PANEL[0]}x{PANEL[1]}, got {ui.size}"
        small = Image.alpha_composite(small.convert("RGBA"), ui).convert("RGB")

    # 6-7 — RGB565 with ordered dithering
    vals, prev = quantise565(small, dither=not a.no_dither)
    raw = bytearray()
    for v in vals:
        raw.append((v >> 8) & 0xFF)
        raw.append(v & 0xFF)
    stem.with_suffix(".rgb565").write_bytes(raw)
    prev.save(stem.with_suffix(".565.png"))
    prev.resize((PANEL[0] * 4, PANEL[1] * 4), Image.NEAREST) \
        .save(stem.with_suffix(".lcdzoom.png"))

    print(f"1-2  composition   {hi.width}x{hi.height}   {stem.with_suffix('.hires.png')}")
    print(f"3-4  downsample    {a.resample} + unsharp {a.sharpen}%   "
          f"{stem.with_suffix('.320.png')}")
    print(f"5    UI overlay    {'native 320x170' if a.ui else 'none'}")
    print(f"6-7  RGB565        dither {'off' if a.no_dither else 'bayer 8x8'}   "
          f"{len(raw)} byte")
    print(f"     colours       {len(set(vals))} distinct")
    print(f"     LCD zoom      {stem.with_suffix('.lcdzoom.png')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
