# Reference direction (`ui_ref.py`) — measured, not eyeballed

```
python3 design/ui_ref.py       # -> design/out/ref/*.png
```

The reference screen rebuilt from **measurements of the reference PNG**, not
from looking at it. The panel bounding box, every track run, the label column,
the badge, the row pitch, the colours and the type sizes were all read out of
the pixels; type sizes were solved by matching rendered string widths against
the measured ones. The constants in the file are those measurements as
fractions of the panel, so the same code renders at 1502x970 and at 320x170.

Two assets come straight out of the reference instead of being reinvented:

* **`assets/gradient_ref.png`** — the actual background field. Green fills, the
  glow, the tracks, the type, the badge and the white rim plus its bleed were
  masked out; the remaining background was median-sampled into a 32x20 grid and
  the holes filled by diffusion. **349 of 640 cells carry real measured
  pixels.** Smooth, so it upsamples to any size, and it costs **1.1 KB**.
* **`assets/BitcountGridSingle-Regular.ttf`** — the real face, SIL OFL. Unlike
  the Helvetica stand-ins elsewhere in `design/`, this one is *shippable*.
  Recorded in `THIRD_PARTY_NOTICES.md` §4.

## What measuring corrected

| | I had been drawing | measured |
|---|---|---|
| track | white at 22–34 % | **white at 12 %** — (248,126,140) over (249,107,126) |
| Key row | "138 px + 3 fractions" | **four equal segments**, 19 px gaps (runs 126-331, 350-554, 575-778, 801-1001) |
| value on active row | dark ink in its own capsule | **same green, one step darker**; the capsule is the fill's own rounded end |
| left axis | three separate margins | breadcrumb, title and every track all start at **exactly x=126** |

The written spec disagreed with the image on the Key row and on the track
opacity. The pixels win.

## The reduction

1502 px wide down to 320 is **4.69x**.

| | reference | at 320×170 |
|---|---:|---:|
| breadcrumb | 48 px | 8.4 px |
| title | 62 px | 10.9 px |
| label | 45 px | 7.9 px |
| value | 33 px | 5.8 px → clamped to 8 |

Bitcount Grid is a grid face and holds together far further down than Inter
did — this is why the font choice mattered. Only the value size is clamped;
every other size keeps the measured ratio exactly.

Still open: the gradient has not been put through the 16-entry palette yet
(see the glass section below for what that costs), and nothing is wired into
`menu.c`.

---

# Reference layout system (`ui_panel.py`)

```
python3 design/ui_panel.py     # -> design/out/panel/*.png
```

Built from the reference feedback, and driven by two corrections.

**The white rounded outline is the SCREEN, not a UI element.** Every earlier
pass here drew it as a card *inside* the display with a margin around it. It
is the visible display area itself. So the gradient is full-bleed, nothing is
inset from a card, and the reference's "internal panel padding" is the
screen's padding. That one misreading is what made the earlier layouts wrong
at the whole-surface level rather than in details.

**One coordinate system, not guessed positions.** Every number is a fraction
of the panel, named once at the top of the file and reused:

| | fraction | of |
|---|---:|---|
| left padding (shared axis) | 7.7 % | width |
| right padding | 8.8 % | width |
| track column | 57.2 % | width |
| track → label gap | 4.6 % | width |
| row pitch (constant) | 10.04 % | height |
| track height | 7.21 % | height |

The breadcrumb, the title and every track share **one** left axis. The labels
are a **fixed second column**, left-aligned, never nudged per row. Row pitch is
**constant and never derived from row count** — which is right for this
product and not only for the reference: a category holds 3 to 5 parameters,
and with a fixed pitch the rows do not jump when you switch category. The
visible consequence is bottom air on a 3-row category (see `03_harmony`), and
that is the intended trade.

The reference spec gives vertical padding both as "40 px" and as "7.8 %",
which disagree once a CSS percentage resolves against width. Vertical
fractions here resolve against **height**, which reproduces the measured 40 px.

## What does not survive the reduction

The reference panel is 1019 px wide; ours is 320. That is a **3.18× reduction**,
and it lands on the type:

| | reference | at 320×170 |
|---|---:|---:|
| breadcrumb | 27 px | 8.9 px |
| title | 40 px | 13.3 px |
| label | 31 px | 10.3 px |
| value / badge | 18 px | **6.0 px** |

6 px is measured on this panel as three grey smudges — no hinting, stems
between pixels. The small size is therefore clamped to a **9 px floor** and is
the one deliberate deviation from the ratios; everything else keeps the
reference proportion exactly. The dot-matrix face the reference spec asks for
is a real option *because* of this — a hand-hinted bitmap font is what
survives at 6–10 px — but it is the opposite of the earlier "no pixel font"
instruction, so it is left as a decision rather than assumed.

Renders `design_*.png` at 1019×513 (the system at its designed size) and
`device_*_1x.png` / `_6x.png` at the true panel size, integer-scaled only.

---

# Instrument direction (`ui_instrument.py`) — done under `.claude/skills`

```
python3 design/ui_instrument.py    # -> design/out/instrument/*.png
```

The redo of the display design routed through `ambient-ui-art-direction`,
`ambient-ui-prototyping` and `ambient-lcd-ux` rather than from a reference
picture. Those skills say to read the current implementation first, and doing
that turned up three things that change the design — not opinions, facts from
the source.

**1. Only three type sizes exist.** `src/baked_font_data.c` ships
`font_hn_value` (advance 40, height 55), `font_hn_value_small` (26/36) and
`font_hn_label` (15/20). Every earlier mockup here used 8–16 px, none of which
exist, all of which need the Helvetica OTF that is not in the repo. On a
170 px-tall panel a 55 px value plus 20 px labels **is** the composition.
This direction needs no new font baked.

**2. One encoder drives the menu, and it has two modes.** `include/encoders.h`:
EN1 drive, EN2 bright, EN3 *display* = the menu encoder, EN4 volume.
`include/menu.h`: rotate browses, push enters edit, rotate edits, push leaves.
So the screen has two jobs. The earlier card design had **no edit state at
all** — a functional gap, not a matter of taste. Both are rendered here.

**3. The current LUT cannot hold a neutral next to an accent.**
`oled_color.c` builds `lut[n] = grey(n*17) * accent / 255`, so every level is
the same hue at a different brightness. With a saturated accent the whole
screen is that hue. Compare `compare_lut_current_6x.png` against
`compare_lut_split_6x.png` — same pixels, and in the current LUT the type,
the labels and the rule are all orange.

The baseline asks for "primarily black, white and gray with one controlled
world accent" that stays "visibly saturated while occupying limited area".
That is **not expressible** with the multiplicative ramp. The minimal fix is a
**split ramp**: levels 0–11 neutral grey, 12–15 the accent. Same 16 entries,
same 4 bpp framebuffer, no extra RAM, about a dozen lines in `rebuild()`.
Levels 1–11 staying a true grey ramp also keeps the existing max-blended
antialiased font path working unchanged.

Compared with the glass direction below, this costs **no baked bloom bitmaps
(−136 KB flash), no new fonts, and no new draw primitives.**

## Fixtures

Deterministic, one per state that matters — not a single hero screen:
`edit_continuous`, `edit_min`, `edit_max`, `edit_discrete`, `edit_longest`
(longest word + LOCK), `browse_field`, `browse_harmony`. Each is written at
1× (320×170, through the real LUT and RGB565) and at 6× nearest-neighbour.

## Conflict to decide

`ambient-ui-art-direction` lists under **Avoid**: "web cards, navigation bars,
pills, glassmorphism, drop-shadow stacks", "decorative gradients that band
badly in RGB565", and "Do not make the whole UI pale to appear calm". The
glass direction below is all four. Both are in the repo; this is a call to
make, not something to resolve silently.

---

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

---

# Glass direction (`ui_glass.py`) — the current proposal

```
python3 design/ui_glass.py      # -> design/out/glass/*.png
```

Built from the supplied reference: a soft radial colour **bloom** under a
rounded **glass card**, a white hairline rim, spring-green bars on a nearly
invisible track, and **one draggable handle carrying the live value**. Same
five-category menu system, same 21 parameters, nothing merged.

Two renders per screen, and the second one is the point:

* `design_*.png` — 6×, true colour, real alpha, real gaussian glow. The intent.
* `device_*.png` — **true 320×170 through the real 16-entry palette**, flat
  fills, dithered bloom, three-level type. What the panel can actually put out.

`main()` asserts every `device_*.png` contains **at most 16 distinct colours**.
If that assertion ever fails, the design has silently stopped being buildable.

## Palette budget

| entries | use |
|---|---|
| 0–7 | the bloom — 8 entries, median-cut over the frosted bloom |
| 8 | ink on accent — lets the value on the handle antialias |
| 9–10 | ink 33 % / 66 % — the two blend steps that let type antialias |
| 11 | white — card rim, handle top light |
| 12 | veil — bar and segment track |
| 13 | ink — type, active segment chunk |
| 14 | accent — bar fill, handle, badge |
| 15 | accent glow — handle top light, the ring around the handle |

## The frost, and the proportion mistake it fixes

The first pass mapped the reference's card onto the whole 320×170 panel and
drew only its rim. That is wrong twice over. In the reference the card is
**314 px inside a 1133 px field** — a calm panel floating in a big soft wash,
where the hot core of the bloom is a minority of the picture. Blown up to fill
the screen, with the bloom normalisation unchanged, the core ended up directly
under the labels: an unreadable magenta blob that read as an object on the
card rather than a field behind it.

Two corrections, and the second is the one that matters:

* the bloom's radial normalisation was tightened so the hot core is small
  again relative to the frame;
* **the card interior is frosted** — the bloom lifted 55 % toward white inside
  the rounded rect, full strength in the 6 px margin around it. That is what a
  glass card actually does, it turns the bloom into atmosphere, and it gives
  the reference's "panel in a field" read on a screen where the panel would
  otherwise *be* the whole screen. It costs nothing: it bakes into the same
  static bitmap, still 8 entries.

Three smaller things also came from comparing against the reference rather
than against the previous render:

* **Proportion.** Bars are 7 px tall and 128 px wide, not 11 × 170. The right
  two-fifths of the card is deliberately empty apart from label and value.
* **The dark chunk.** In the reference it is ONE crisp near-black chip in an
  otherwise quiet card. Softening it to grey to avoid heaviness just made it
  muddy — the fix was to keep it full ink and make it *small*.
* **Two faces.** A grotesk for reading (labels, title), a mono for numerals
  only, so the value column aligns. The category label is the one thing set in
  caps, and it is wide-tracked; PIL has no tracking, so glyphs go down one at
  a time.

## The three things that were actually verified, not assumed

**The bloom does not have to be computed at runtime.** It is static per world,
so it bakes into a 4bpp bitmap in flash: `320×170/2 = 27,200 B` per world, five
worlds = **136 KB**. The H743VI has 2 MB. No gradient maths in the draw path at
all, and the Floyd–Steinberg dither runs once, offline, where its cost is free.

**Nine entries is enough — but only because the bloom is one-dimensional.** It
is written as an analytic radial falloff rather than a stack of blurred blobs,
so colour is a function of a single scalar and the nine entries are sampled
along that one path instead of scattered through RGB. Undithered it bands hard;
`quantize(colors=9)` alone gives nine flat stripes, because PIL only honours
`dither` when it is handed an explicit palette. Pick the nine, then dither
*onto* them.

**Two things from the reference do not survive 320×170, and were changed:**

* *The glow around the handle.* Ordered-dithering a gaussian into one palette
  entry was tried first. At one device pixel it is not a glow, it is dirt — a
  visible 4×4 grid smeared over the neighbouring tracks. Replaced on device by
  a solid 2 px ring of the glow entry, which reads as "lifted" and costs the
  same single entry. The 6× render keeps the real gaussian.
* *Type below ~9 px.* The reference sets the handle value and the state badge
  very small. At 6 and 7 px JetBrains Mono has a ~4 px cap height, PIL applies
  no hinting, stems land between pixels, and `62%` came out as three grey
  smudges. Everything the user has to read is now **9 px or larger**
  (`SZ_*` in the source), which is why the rows are pitched 20 px apart.

## Still open before this can ship

* The type sizes above are validated for **this** face at **this** size. The
  shipped font path (`tools/generate_fonts.py`) bakes 56/36/20 px only — the
  9/10/19 px sizes this design uses still have to be baked, and 8–10 px bitmap
  type wants hand-hinting, not autoscaling.
* JetBrains Mono (SIL OFL) is a *drawing* stand-in for design review here. It
  is not compiled into firmware and not shipped. See `THIRD_PARTY_NOTICES.md`.
* Nothing here is wired into `menu.c` yet. This is a design proposal with a
  verified implementation budget, not an implementation.
