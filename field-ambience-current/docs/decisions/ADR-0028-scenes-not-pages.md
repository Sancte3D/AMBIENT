# ADR-0028 — Scenes, not pages: each group gets its own small visual world

**Status:** PROPOSED — nine scenes implemented on the four-encoder colour
mapping, rendered and running on the Pico bench.
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

That measurement gives the **drawing style**: one dim structural grey carrying
most of the picture, saturated accents used sparingly, one stroke weight
everywhere, open paths, and dots as the only filled form. `#353238` is
(53, 50, 56), almost exactly the (42, 42, 42) the wheel already used for inert
structure — a convergence, not a copy.

It does **not** give the mechanism, and the first version of this ADR said it
did ("the coherence is not the palette, it is the discipline"). That was wrong,
and it is worth saying plainly because the error cost a whole implementation
pass. `iter.png` is the purest statement of what those screens actually are:
four little needles in the four **encoder colours**, and nothing else. No
labels, no names, no legend — you never read which parameter is which, you learn
"the red one moves that". `iter-lab.svg` proves the other half: it is the *same
four needles* dressed as a chemistry set. The elaborate drawing is a costume
over four coloured value objects.

So the drawing is the optional half and the colour mapping is the load-bearing
one. A bespoke illustration with a single accent colour — which is exactly what
the first pass built — keeps the expensive half and drops the half that does the
work.

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

### The colour mapping is the interface

**AMBIENT's four encoders are RED, BLUE, GREEN, YELLOW**, and every movable
thing on screen is painted in the colour of the encoder that moves it.

| encoder | colour | role |
|---|---|---|
| EN3 Display | **green** (60, 230, 130) | navigation, **permanently, at every level** — the wheel, the selected node, where you are |
| EN1 Drive | **red** (244, 86, 96) | scene property 1 |
| EN2 Brightness | **blue** (92, 146, 255) | scene property 2 |
| EN4 Volume | **yellow** (242, 200, 70) | scene property 3 |

Green is never a scene property. "Where am I" and "what am I changing" must not
be confusable, and one reserved colour buys that for free. That leaves three
scene knobs — which is not a compromise, it is what the hardware has, and every
group in the wheel already carries one to three parameters, so the fit is
exact. At the top level those same three encoders keep their global roles
(Drive, Brightness, Volume), so the globals stay one press away.

Consequences that follow directly, and are the whole design:

- **There are exactly two kinds of ink in a scene**: the dim structural grey
  (42, 42, 42) for anything no encoder can move, and the encoder colours for
  everything that can. No decorative colour, no third tone. If it is coloured, a
  knob moves it; if it is grey, no knob does. `test_ui_scene.c` renders all nine
  scenes at four settings and fails on any pixel that is neither a legal ink nor
  a two-ink crossing.
- **Property names are gone from the screen.** The colour is the label. Three
  names plus three values do not fit in 250 px at 12 px per character anyway,
  but the real reason is that a name you have to read is a name you are reading
  instead of playing.
- **The value ring is three fixed slots**, left to right in the same order as
  the encoders under the hand. A knob's slot never moves between scenes — a slot
  that resized itself per group would teach nothing. A group with one property
  leaves the other two slots dim, which is the honest statement: those knobs do
  nothing here.
- **The battery is no longer green.** It was, and the moment green meant
  navigation it became a green object in the corner that the navigation encoder
  does not move. It is neutral (214, 214, 218) while healthy and earns colour
  only to warn — which also makes the warning states read as warnings.

Rules kept from the measurement:

- **One stroke weight, 1.5 px**, in every scene, exactly as measured.
- **One line of type per scene**: the three values right-aligned as a group
  against the battery (which owns x ≥ 262), scene name in whatever is left and
  **dropped, not truncated**, when it does not fit. "Soun" reads as a fault;
  absence reads as a quiet line.
- **No value is ever truncated either**, and that promise is kept at the source
  rather than at the draw call: `test_ui_wheel.c` walks the widest reachable
  value of every property of every group and fails if a group's worst case
  overflows the line. It is why Cell's "Harmony" is now **Chord** (it also
  collided with the *group* named Harmony — one word, two meanings, one level
  apart) and why "FM Glass" is now **FM**.
- **The ring stays** under every scene as the one constant, but **thins from
  9 px to 3 px** as a scene takes over. At the hub's weight it is six times the
  monoline and shouts over the drawing.
- Scenes are drawn from the same signed-distance primitives as the wheel
  (`ui_draw.c`), so they cost no framebuffer and no artwork.

## Consequences

- `WHEEL_VALUE` is gone and `WHEEL_GROUP` became `WHEEL_SCENE`. There is nothing
  to descend into and nothing to select; long press still exits.
- **`ui_wheel_turn_knob(st, knob, dir, coarse)` is the product input path** —
  one call per physical encoder, and the encoder index *is* the colour index.
  `ui_wheel_turn()` is EN3 and navigates. A knob a scene does not use is a
  no-op, not an error: the hand finding an unused encoder should feel like
  nothing, not like a mistake.
- `ui_scene_t` has **no focus field**, deliberately. All properties are live
  because all three encoders are, and a focused property would reintroduce
  exactly the modal selection the scene exists to remove.
- Every property gets its own tween, so all features of a drawing move, not
  just the one being turned.
- The drawing carries character, the arcs carry the amounts. Both stay: they
  answer different questions, and a scene alone cannot be read precisely.
- `test/test_ui_scene.c` checks the four things that break silently: every scene
  draws something at every setting, stays inside its box, **visibly responds to
  each of its own properties** — a scene rendering identical pixels at 0 and 100
  is decoration, not an instrument — and **uses no ink outside grey and the
  encoder colours**. It caught the Pitch reference ring running past the box
  into the label above and the ring below.
- `test/test_ui_wheel.c` adds the two that break the *mechanism*: each encoder
  moves its own property and nothing else (a colour that lies is worse than no
  colour), and no readout can overflow its line.

### The bench is one encoder, and says so

`wheel_state_t.one_encoder` exists so the Pico bench can be honest rather than
pretend. With one physical encoder something has to indicate which value a turn
will hit, so the bench cycles `member` on press and draws an underline under
that value in its own colour. The product sets neither and draws no underline —
all three are live, which is the point.

### Corrected along the way

Holding three of `ui_wheel_param_value()`'s return pointers aliased its shared
static format buffer, so Room's 62 and 38 both printed as **"38 38"**. Three
identical numbers are worse than no numbers, because the readout still looks
like it works. The values are copied now, and `test_values_do_not_alias()`
holds the line.

## Open

- **The four hues are placeholders.** Red / blue / green / yellow is the
  assignment asked for, and the RGB values are estimates chosen for separation
  on an ST7789 at low backlight, not sampled from a source file or from the
  physical encoder caps. When the caps are specified, the four constants in
  `ui_scene.c` change and nothing else does — that is the point of having the
  mapping in one table.
- **Yellow is only exercised by one scene.** Sound is the only group with three
  properties, so EN4 is dark in eight of nine scenes. That is honest — those
  knobs genuinely do nothing there — but it means the yellow half of the
  mapping is barely taught. Either some groups grow a third property or the
  wheel's 40° step gets revisited.
- Scene sizes are 1–3 properties because the groups were sized for the wheel's
  measured 40° step. Four properties per scene would map one-to-one onto four
  encoders, but that means six groups at 60°, which changes the wheel's look —
  and would need EN3 to stop navigating, which is not on offer.
- **The four encoders are not yet wired.** `ui_wheel_turn_knob()` exists and is
  tested; the H743 input layer still has to route each physical encoder to its
  index (Step 13.3).
- Not yet judged on glass at low backlight — `design_bench.uf2`.
