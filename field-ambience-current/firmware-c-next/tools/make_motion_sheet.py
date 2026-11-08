#!/usr/bin/env python3
"""
Assemble the motion sheets from render_wheel's anim*.ppm frames.

Two outputs, because they answer different questions:

  MOTION_STRIPS.png  every 2nd frame laid out left to right at real 16 ms
                     spacing. The frames bunch up toward the end of each row —
                     that IS the deceleration, visible as geometry rather than
                     asserted in prose.
  wheel_*.gif        the same frames as animations, 16 ms per frame, so the
                     motion can just be watched.

  python3 tools/make_motion_sheet.py <dir> <outdir>
"""
import pathlib
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("needs pillow:  pip install pillow")

SEQS = [
    (0, "one detent — the wheel snaps one step",
        "EASE_OUT, 200 ms · leaves at 3x speed, settles into the lock"),
    (1, "press — descending into a group",
        "EASE_IN_OUT, 280 ms · rest to rest, the two levels cross-fade"),
    (2, "turning a value",
        "EASE_OUT, 90 ms per detent · the orb follows the hand, never leads it"),
]
BG, FG, DIM, ACC = (14, 14, 16), (240, 240, 242), (150, 150, 156), (124, 240, 132)


def font(size, bold=False):
    p = "/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf" % ("-Bold" if bold else "")
    return ImageFont.truetype(p, size) if pathlib.Path(p).exists() \
        else ImageFont.load_default()


def main() -> int:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    d, outdir = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
    outdir.mkdir(parents=True, exist_ok=True)

    frames = {}
    for seq, _, _ in SEQS:
        fs = sorted(d.glob(f"anim{seq}_*.ppm"))
        if not fs:
            sys.exit(f"no anim{seq}_*.ppm in {d} — run render_wheel first")
        frames[seq] = [Image.open(f).convert("RGB") for f in fs]

    # --- animated GIFs, the thing you actually watch ---
    for seq, title, _ in SEQS:
        gif = outdir / f"wheel_motion_{seq}.gif"
        ims = [im.resize((640, 340), Image.NEAREST) for im in frames[seq]]
        ims[0].save(gif, save_all=True, append_images=ims[1:],
                    duration=16, loop=0)
        print(f"{gif}  {len(ims)} frames @ 16 ms")

    # --- filmstrip ---
    step, scale, gap, pad = 2, 1, 6, 30
    tw, th = 320 * scale, 170 * scale
    cols = len(range(0, len(frames[0]), step))
    W = pad * 2 + cols * (tw + gap)
    H = 96 + len(SEQS) * (th + 74)

    sheet = Image.new("RGB", (W, H), BG)
    dr = ImageDraw.Draw(sheet)
    dr.text((pad, 22), "AMBIENT · motion · every 2nd frame at 16 ms",
            font=font(28, True), fill=FG)
    dr.text((pad, 58),
            "the model moves on the encoder edge; only these frames lag. "
            "Frames bunching toward the right of a row is the deceleration.",
            font=font(16), fill=DIM)

    y = 96
    for seq, title, sub in SEQS:
        dr.text((pad, y), title, font=font(19, True), fill=ACC)
        dr.text((pad + 470, y + 2), sub, font=font(15), fill=DIM)
        y += 30
        for i, f in enumerate(range(0, len(frames[seq]), step)):
            x = pad + i * (tw + gap)
            sheet.paste(frames[seq][f].resize((tw, th), Image.NEAREST), (x, y))
            dr.rectangle([x - 1, y - 1, x + tw, y + th], outline=(52, 52, 58))
            dr.text((x, y + th + 4), f"{f * 16} ms", font=font(12), fill=DIM)
        y += th + 44

    out = outdir / "MOTION_STRIPS.png"
    sheet.save(out)
    print(f"{out}  {sheet.width}x{sheet.height}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
