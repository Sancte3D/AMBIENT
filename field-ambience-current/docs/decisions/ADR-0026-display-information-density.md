# ADR-0026 — Information density on the 320×170 panel

**Status:** PROPOSED — three candidates built and rendered, awaiting the
user's choice.
**Date:** 2026-08-12
**Supersedes in practice:** the 6-row parameter list baked into
`assets/background_1920x1020.png`.

## Context

The first implementation of the approved artwork put six labelled parameter
rows on the panel at once. On real glass it could not be read. The reason is
arithmetic, not styling.

The Waveshare 1.9″ module has an active area of **39.1 × 21.2 mm** over
**320 × 170 px** (`docs/hardware/MECHANICAL_REQUIREMENTS.md` §"Active Area"),
so one pixel is **0.125 mm**. Digit heights of the three baked Bitcount faces,
measured out of `src/baked_font_data.c`:

| Face | ppem | digit height | physical | subtended at 40 cm |
|---|---:|---:|---:|---:|
| `font_hn_label` | 10 | 6 px | 0.75 mm | **6.4′** |
| `font_hn_value_small` | 20 | 12 px | 1.50 mm | 12.9′ |
| `font_hn_value` | 30 | 20 px | 2.24 mm | **19.3′** |

A healthy eye resolves a letter at roughly 5′ under ideal conditions;
sustained comfortable reading wants about 20′. The six-row list drew its
labels at **6.4′** — at the acuity floor, and below it once the backlight is
dimmed or the panel is viewed off-axis.

The consequence bounds the layout: a comfortable row costs the 30 ppem face
and its 36 px line box, and 170 / 36 = **4.7 lines** for the entire panel
before a title, margins or a bar exist. **Six labelled rows do not fit at any
legible size.** The parameter count is also 16, not 6 (`MP_COUNT`), so a flat
list was never going to show the whole model anyway.

Two further constraints already in the project point the same way:

- `.claude/skills/ambient-ui-lcd-smt/references/project-baseline.md`: *"The
  display supports the instrument. It must not turn the product into a
  menu-heavy miniature computer."*
- `src/menu.c` already implements a one-parameter focus view with a 16-pill
  position strip, and has since r18. The 6-row list replaced a working
  interaction model with a denser, less readable one.

## Decision

Build all three honest points on the density curve, in the approved visual
language, sharing one row compositor (`tools/ui_layouts.c`), and judge them on
real glass before committing:

| | Shows | Type sizes | Trade |
|---|---|---|---|
| **A — FOCUS** | 1 of 16 + position | value 19.3′, label 12.9′ | most legible; no comparison between parameters |
| **B — CONTEXT** | 3 of 16 + position | focus 19.3′, neighbours 12.9′ | keeps a sense of place; costs the neighbours' legibility |
| **C — PAGES** | 4 of 16, grouped in 4 pages | all rows 12.9′ | closest to a list; riskiest at low backlight |

A is `src/menu.c`'s existing model re-skinned, so choosing it is the smallest
firmware change and keeps a proven interaction.

## Consequences

- The background asset had to be rebuilt. `tools/make_plain_plate.py` removes
  the UI from the approved master by diffusion inpainting and keeps the
  gradient, glass card, white edge and glow, producing `assets/plate_plain.*`.
  The card border sits outside the inpaint region by construction, so it is
  copied through untouched rather than reconstructed.
- All UI is now drawn live at native 320 × 170 — which is what
  `tools/build_display_asset.py` step 5 always required, and what keeps type
  off the downsampler.
- The row compositor stays: flash-resident plate + per-scanline foreground,
  two 640-byte line buffers, no colour framebuffer. Non-negotiable while
  RAM_D1 is at 87 % and RAM_D2 at 96 % against 11 % flash.
- `test/test_ui_layouts.c` locks the ppem grid (6/12/18 px) and checks that no
  layout draws outside the glass card across all 96 layout × parameter ×
  mode combinations.
- Whichever layout wins, the losing two and `render_layouts.c` should be
  deleted rather than left as dead choices.

## Open

- The choice itself.
- Layout C needs short labels ("Atmos" for "Atmosphere") and truncates long
  values; if C wins, the label set needs a proper pass rather than the
  mechanical shortening used here.
- None of the three has been evaluated at the lowest backlight step or
  off-axis, which is exactly what `design_bench.uf2` exists for.
