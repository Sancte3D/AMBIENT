# ADR-0028 — Scenes, not pages: each group gets its own small visual world

**Status:** PROPOSED — nine scenes implemented, rendered and running on the
Pico bench.
**Date:** 2026-08-15
**Builds on:** ADR-0027 (radial navigation). The wheel is unchanged; what it
opens is different.

## Context

ADR-0027 gave the instrument a navigator: a wheel of nine groups that rotates
under a fixed selection point. Below it sat a sub-wheel of abstract parameter
names, and below that a value screen with a number and an arc. That middle
layer was a menu wearing the wheel's clothes — it named things instead of
showing them.

The OP-1 does something else, and the repacker archive lets us measure exactly
what. `op1repacker/assets/display/iter-lab.svg` is a real OP-1 display asset at
**320 × 160**, the native canvas:

| measured | |
|---|---|
| 304 drawable elements | 273 `path`, 27 `ellipse`, 4 `line`, **no `rect` at all** |
| 294 of 304 carry `fill="none"` | 96.7 % outline |
| 282 of 304 at `stroke-width: 1.5` | 93 % one single weight |
| path vocabulary | `L` 297, `M` 273, `S` 113, `C` 45, `Z` 11 — 11 closed paths |
| 19 of 27 ellipses are `rx = ry = 1` | dots, not discs |
| 157 of ~300 strokes are `#353238` | over half is one dim grey |
| then, sparsely | `#698eff` 24, `#00ed95` 22, `#383572` 20, `#ff3a5d` 15, … |

The coherence of those screens is not the palette — it is the **discipline**:
one dim structural grey carrying most of the drawing, a handful of saturated
accents used sparingly, one stroke weight everywhere, open paths, and dots as
the only filled form. `#353238` is (53, 50, 56), almost exactly the (42, 42, 42)
the wheel already used for inert structure — a convergence, not a copy.

The archive contains **no runtime**: `op1repacker` only moves static SVG
elements (`move_all` / `move_element`, rewriting `x`, `cx`, path `d`, polyline
`points`). How a value reaches a shape lives in the compiled firmware and is
not recoverable. What *is* recoverable is the vocabulary — and the element
names in the tape patch (`centerline`, `grid`, `loopin`, `loopout`,
`track_active`, `track_semiactive`, `track_inactive`) say plainly that a screen
is a named **scene**, not a stack of widgets.

## Decision

**Each mode gets its own illustration; the controls change properties OF that
illustration.** The wheel still navigates. Pressing into a group now opens a
scene rather than a sub-menu, and there is nothing below it — so the model
drops from three levels to two.

| scene | properties | what the geometry says |
|---|---|---|
| World | World | a horizon: city cuts square, coast rolls, highway converges, room is furniture |
| Sound | Synth, Voice, Cell | the waveform, its envelope tilt, its trigger points |
| Pitch | Key, Tuning | 12 pitches on a ring — **just intonation visibly bends the spacing** |
| Harmony | Bass, Color | the chord as a stack on a stave |
| Room | Space, Shimmer | how far the sound carries, and what rings above it |
| Time | Echo, Blur | repeats decaying away, and how hard they smear |
| Texture | Atmosphere, Age | grain density, and how worn it is |
| Motion | Motion | the drift, as an orbit that opens |
| FX | FX | the chain, and which link is lit |

Pitch is the clearest case for the whole idea: "Tuning: Just" is a word, but
uneven spacing on a ring *is* the parameter. No number shows that as fast.

Rules, kept from the measurement:

- **One stroke weight, 1.5 px**, in every scene, exactly as measured.
- **Four semantic tones**, not nine hues — the project baseline asks for black,
  white, grey and one controlled accent: `DIM` inert scaffolding, `INFO`
  structure that carries information, `LIVE` what the hand is moving, `TEXT`
  naming.
- **One line of type per scene**, laid out right to left so nothing collides:
  battery owns x ≥ 262, value right-aligned against it, property name left of
  that, scene name in whatever is left — and **dropped, not truncated**, when
  it does not fit. "Soun" reads as a fault; absence reads as a quiet line.
- **The ring stays** under every scene as the one constant, but **thins from
  9 px to 3 px** as a scene takes over. At the hub's weight it is six times the
  monoline and shouts over the drawing.
- Scenes are drawn from the same signed-distance primitives as the wheel
  (`ui_draw.c`), so they cost no framebuffer and no artwork.

## Consequences

- `WHEEL_VALUE` is gone and `WHEEL_GROUP` became `WHEEL_SCENE`. A press inside
  a scene no longer descends — it hands the encoder to the next property, which
  on the product is what a second encoder would hold simultaneously. Long press
  still exits.
- Every property gets its own tween, so all features of a drawing move, not
  just the one being turned.
- The drawing carries character, the arc carries the amount. Both stay: they
  answer different questions, and a scene alone cannot be read precisely.
- `test/test_ui_scene.c` checks the three things that break silently: every
  scene draws something at every setting, stays inside its box, and **visibly
  responds to each of its own properties** — a scene rendering identical pixels
  at 0 and 100 is decoration, not an instrument. It caught the Pitch reference
  ring running past the box into the label above and the ring below.

## Open

- **Four encoders, not one.** The product has EN1 Drive, EN2 Brightness, EN3
  Display, EN4 Volume. The scene model wants one encoder per property, held
  simultaneously — that is the OP-1's actual trick and the reason its screens
  need no labels. The bench has one encoder, so it cycles focus instead. Which
  physical encoder owns which scene property is an open interaction decision.
- Scene sizes are 1–3 properties because the groups were sized for the wheel's
  measured 40° step. Four properties per scene would map one-to-one onto four
  encoders, but that means six groups at 60°, which changes the wheel's look.
- Colours remain estimated rather than sampled from a source file.
- Not yet judged on glass at low backlight — `design_bench.uf2`.
