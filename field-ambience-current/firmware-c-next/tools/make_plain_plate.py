#!/usr/bin/env python3
"""
Strip the UI out of the approved artwork and keep only the plate.

WHY THIS EXISTS

`assets/background_1920x1020.png` is the approved reference, and it has the
6-row parameter list baked into it — tracks, mint fills, labels, title, the
100 % pill. That was fine while the panel showed exactly that list. It is
useless as a background for any other layout, because the old rows show
through underneath the new ones.

What has to survive is the *plate*: the rose/coral gradient, the glass card,
its 1 px white edge and the outer glow. What has to go is everything that is
information. Then every layout draws its own content live at native 320x170,
which is what the pipeline in build_display_asset.py asks for anyway (step 5:
type and thin lines after the downsample, never scaled).

METHOD — diffusion inpaint, not mirror-copy

Two earlier attempts failed and it is worth recording why: mirroring the left
half onto the right pulled mirrored glyphs into the bars, and grouping rows
into bands merged rows 2-5 into one smear. Both tried to *replace* the masked
area with other image content.

This does the opposite. The plate is a smooth field, so the masked pixels are
recoverable from their own neighbourhood: blur the whole card interior, write
the known (unmasked) pixels back over it, repeat. Each pass pushes real
gradient one step further into the hole and never invents structure. A few
hundred passes at decreasing radius converge to the surface the gradient would
have had.

The card border and the glow sit OUTSIDE the inpaint region by construction
(the region is the card interior inset by MARGIN), so they are copied through
untouched — no reconstruction, no drift.

  python3 tools/make_plain_plate.py assets/background_1920x1020.png \
                                    assets/plate_plain_1920x1020.png
"""
import sys

try:
    import numpy as np
    from PIL import Image, ImageFilter
except ImportError:
    sys.exit("needs pillow + numpy:  pip install pillow numpy")

# Card rectangle, measured off the approved master by scanning for the bright
# border ridge (tools/make_plain_plate.py --probe reprints these).
CARD = (181, 13, 1737, 1008)
MARGIN = 26          # keep the border + its inner falloff out of the inpaint
DEV_THRESHOLD = 3.5  # luminance delta that counts as "this is UI, not plate"
DILATE = 15          # grow the mask so glyph antialiasing is covered too
REF_BLUR = 90        # reference surface radius — must be wider than any bar,
                     # otherwise the blur follows the bar and it reads as plate


def probe(a):
    """Re-derive the card rectangle from the image, for verification."""
    lum = a.mean(2)
    h, w = lum.shape
    xs = [i for i, v in enumerate(lum[h // 2]) if v > 200]
    ys = [i for i, v in enumerate(lum[:, w // 2]) if v > 200]
    return (xs[0], ys[0], xs[-1], ys[-1])


def ui_mask(a):
    """UI = what deviates from the locally smooth plate, inside the card.

    Three criteria, OR-ed. Luminance deviation alone is not enough: a mint fill
    is a large flat region, so a blur that is narrower than the bar simply
    follows it and the fill reads as plate — which is exactly how the first
    run left two green dots behind. Hence REF_BLUR wider than any bar, plus an
    explicit hue test, plus a brightness test for the pale empty tracks."""
    lum = Image.fromarray(a.mean(2).astype(np.uint8), "L")
    smooth = np.asarray(lum.filter(ImageFilter.MedianFilter(size=9))
                            .filter(ImageFilter.GaussianBlur(REF_BLUR))
                        ).astype(np.float32)
    dev = np.abs(a.mean(2) - smooth)

    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    mint  = (g > r - 40)                 # plate is always strongly red-dominant
    pale  = (a.mean(2) - smooth) > 2.0   # tracks and type sit above the plate

    m = (dev > DEV_THRESHOLD) | mint | pale
    # Dilate: a glyph's soft edge deviates less than its core but must still go.
    mi = Image.fromarray((m * 255).astype(np.uint8), "L")
    mi = mi.filter(ImageFilter.MaxFilter(size=DILATE))
    m = np.asarray(mi).astype(np.float32) / 255.0 > 0.5

    # Confine to the card interior — the border and glow must not be touched.
    keep = np.zeros_like(m)
    x0, y0, x1, y1 = CARD
    keep[y0 + MARGIN:y1 - MARGIN, x0 + MARGIN:x1 - MARGIN] = True
    return m & keep


def inpaint(a, mask, passes=420):
    """Diffuse the plate into the mask. Known pixels are restored every pass,
    so information only ever flows outward from real image data."""
    out = a.copy()
    hole = mask[..., None]
    # Seed the hole with the row mean of the known pixels so the first passes
    # start from something plausible instead of the old UI colour.
    for c in range(3):
        ch = out[..., c]
        for y in range(a.shape[0]):
            known = ~mask[y]
            if known.any() and mask[y].any():
                ch[y][mask[y]] = ch[y][known].mean()

    for i in range(passes):
        r = 24.0 * (1.0 - i / passes) + 2.0
        blur = np.stack([
            np.asarray(Image.fromarray(out[..., c].astype(np.uint8), "L")
                       .filter(ImageFilter.GaussianBlur(r))).astype(np.float32)
            for c in range(3)], axis=-1)
        out = np.where(hole, blur, a)
    return out


def main() -> int:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src, dst = sys.argv[1], sys.argv[2]
    a = np.asarray(Image.open(src).convert("RGB")).astype(np.float32)

    print(f"card measured {probe(a)}   using {CARD}")
    m = ui_mask(a)
    print(f"masked {m.sum()} px ({100.0 * m.mean():.1f} % of the master)")

    out = inpaint(a, m)
    Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGB").save(dst)
    print(f"wrote {dst}")

    Image.fromarray((m * 255).astype(np.uint8), "L").save(
        dst.replace(".png", ".mask.png"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
