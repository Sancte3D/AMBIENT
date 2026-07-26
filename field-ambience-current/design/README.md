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
