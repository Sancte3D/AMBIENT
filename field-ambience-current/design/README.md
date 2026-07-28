# Display design

`ui_design.py` — the light two-column ("duo") menu, drawn in device
proportions (320×170) at 6× with real vector type.

```
python3 design/ui_design.py          # -> design/ui/design/*.png
```

It writes two sets:

* `design_*.png` — the design intent at high resolution.
* `device_*.png` — **the same layout at true 320×170, quantised to the 16 grey
  levels the 4-bit framebuffer actually has.** Always look at these too. A
  layout that only works at 6× is a picture, not a design.

## What it proposes

**Layout.** 21 parameters, none merged — they do different things to the sound
and one control changing three of them would be worse, not better. Only the
presentation changes: five categories in a left rail, the parameters of the
open one in a list, values right-aligned. Row height is uniform, so nothing can
ever overlap. Contrast carries the hierarchy — the selected item is ink, the
rest recede — which is what lets five rows sit on the screen without noise.

No category holds more than five parameters, so the screen never fills up.

**Light theme.** This needs no change to the drawing code. The device LUT
(`oled_color.c`) maps grey multiplicatively onto the accent, so level 0 is
always black — a dark theme by construction. Keep drawing "ink = high level"
exactly as today and invert the LUT instead: 0 = paper, 15 = ink. The per-world
accent survives and now tints the *paper* (Desert warm, Moss cool).

## Before this can be built

`tools/generate_fonts.py` bakes three sizes — 56, 36 and **20** px. This design
uses ~13 px rows and ~9 px rail caps, so **two new sizes must be baked**, and
that needs the Helvetica Neue OTF, which is not in the repo (licensing).

## Font in this renderer

Inter (SIL OFL) stands in for Helvetica Neue, fetched to `.fonts/` on first
run. It is a *drawing* stand-in for design review only — never compiled into
firmware, never shipped. See `THIRD_PARTY_NOTICES.md` for what actually ships.

---

# Colour direction (`ui_color.py`)

```
python3 design/ui_color.py     # -> design/ui/color/*.png
```

Saturated gradient card, bright accent bars, labels right of the bars, mono
type — on the same five-category menu system. Writes three sets:
`design_*` (intent), `palette_*` (through the 16 entries), `device_*` (both, at
true 320×170).

## Why this is possible at all

The framebuffer is 4 bits per pixel. That gets described as "16 greys", but it
is really a **16-entry palette index** — `oled_color.c` merely happens to fill
it with a monochrome ramp built from the accent. Nothing stops us writing 16
arbitrary RGB565 colours into that table, so full colour costs **no extra RAM**
and no change to the framebuffer format.

## What it does cost

Sixteen entries have to cover the background gradient *and* the whole UI. The
split is the real design decision:

| entries | use |
|---|---|
| 0–6 | background gradient (7 steps) |
| 7 | card border |
| 8 | bar track |
| 9–10 | dim / secondary type |
| 11 | primary type |
| 12–15 | accent — bar fill, pill, highlight |

**Seven steps is not enough for a smooth wash.** Compare `device_*.png`
(dithered) against `device_NODITHER.png` (nearest entry): undithered it bands
into seven visible stripes. So the gradient fill has to dither as it draws —
an ordered/Bayer threshold in the fill routine, which is a few lines and no
extra memory. Either that, or commit to the banding as stepped strata.

## What has to change in firmware

* `oled_color.c` — `rebuild()` stops being a `grey × accent` ramp and becomes a
  per-world palette table. The accent crossfade still works; it just
  interpolates palette entries instead of one ramp.
* **The anti-aliased font path assumes "higher level = brighter"** and
  max-blends on that basis. With a palette that assumption is gone. The
  reference look uses a mono face with hard pixel edges anyway, so drawing type
  **without** AA is both on-style and the simple way out.
* `oled_draw.c` fills need the dither described above.
