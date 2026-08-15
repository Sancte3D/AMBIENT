# ADR-0027 — Radial navigation: the circle is the interface

**Status:** PROPOSED — implemented, rendered, running on the Pico bench.
**Date:** 2026-08-14
**Supersedes:** ADR-0026 (three information densities). The three candidates
were answers to "how many rows fit"; this replaces the question.

## Context

ADR-0026 established the hard limit: at 0.125 mm per pixel, a comfortably
readable row costs the 30 ppem face and 36 px of line height, so the panel
holds 4.7 lines total. Every candidate there was a different compromise on how
to spend those lines on a list of 16 parameters.

The user's direction removes the list. Navigation becomes spatial: a wheel
whose centre sits *below* the visible panel, with branches radiating out to
nodes. The encoder does not move a cursor between items — it rotates the whole
structure under a fixed selection point at twelve o'clock. Turn moves the
structure, press descends, turn changes the value.

Two consequences make this better than a list rather than merely different:

- The interface reads as larger than the screen. Because the hub is off-panel
  and the outer branches are clipped by the edges, the form itself says there
  is more outside the visible area, and it says "turn me" without a label.
- Legibility stops competing with density. Only the selected node needs a
  name, so that name gets the full width at 19.3′ while everything else is
  carried by position and icon.

## Decision

One interface object — the circle — in three states of the same geometry:

| | Branches carry | Nodes | Ring |
|---|---|---|---|
| `WHEEL_MAIN` | 9 groups | r 22, icons | dim hub |
| `WHEEL_GROUP` | the group's parameters | r 15, plain | shows the selected amount |
| `WHEEL_VALUE` | — retracted | — | the parameter itself, with the orb |

Geometry is measured off the reference render rather than invented. The card
in that image spans 720 px for the panel's 320, so panel = image × 0.4444:

| | image | panel |
|---|---:|---:|
| hub centre | (455, 500) | **(158, 171)** — one px below the bottom edge |
| node orbit | 235 | 104 |
| node radius | 50 | 22 |
| value ring | 145 | 64 |
| branch width | 18 | 8 |

The reference's five nodes sit at −81, −40, 0, +40.5, +81.5 degrees: an even
40.4° step. 360 / 40 = 9, so the wheel carries **nine groups**, and the 16
parameters partition across them (sizes 1–3). Group count is one table edit,
but it also sets the step: at 72° for five groups both neighbours fall off the
sides and only the selected node stays visible.

Rules that follow from the model rather than from taste:

- **Icons never rotate.** Nodes are circles, so they are rotation-invariant,
  and icons are emitted in screen space at the node's position. The required
  `icon_rotation = -θ` is a property of the construction, not a correction.
- **Icons are built from the interface's own vocabulary** — disc, capsule, arc
  — at 1–5 primitives each, so they read as instrument marks rather than app
  icons.
- **Depth sheds detail**: icons on the main wheel, plain discs one level down,
  no discs at all in the value state. Hierarchy is carried by scale and
  position, never by a breadcrumb.
- **A group of one skips its sub-wheel.** A wheel with a single branch is a
  question with one answer; World, Motion and FX go straight to their value.
- **Only the selected node carries plain text.** Icon for recognition, text
  for confirmation.
- **The value sweep is ±78°, not ±90°.** The ring crosses the bottom edge at
  ±89.1°, so a full half would put the orb centre exactly on y = 171 and clip
  the readout at 0 and 100 — the two values that must never be ambiguous.
- **Green marks exactly one thing**: what is active or changeable.
- **Snap, do not free-run.** One detent = one step, eased, locking at twelve
  o'clock. Level changes rotate the *short* way: θ accumulates freely as the
  user turns, so the target is resolved to the 360-periodic representative
  nearest the current angle. Without that, stepping back out of a group
  unwinds the entire rotation that got you there.

## Consequences

- **The plate asset is gone from the bench.** The new ground is black and the
  wheel is drawn from signed distance fields, so the UI carries no artwork at
  all: `design_bench` dropped from 169,440 to **63,128 bytes** of text, and the
  108,800-byte background is no longer needed to render a screen.
- Shapes are antialiased, text is not. Both rules are now enforced in one
  place (`tools/ui_draw.c`), shared with the ADR-0026 layouts.
- **Shapes accumulate coverage before they are blended.** Compositing two
  overlapping same-coloured shapes in sequence never reaches full opacity —
  where each covers half a pixel the result lands at 0.75 of the colour — so
  every junction drew itself a darker hairline: branch into ring, stem into
  node, the four strokes crossing in the FX icon. Primitives now write into a
  per-row coverage mask with `max()` and a whole same-coloured group is blended
  once. Cost went from 0.22 to 0.27 ms/frame, against 29 ms of SPI.
- **Draw order is branches, then ring, then nodes**, so an arm never notches
  the ring it crosses. Branches start on the ring's centreline (`R_BRANCH0 =
  R_RING`): their rounded cap then spans 60..68 px, entirely inside the ring
  band, leaving no gap outside and no stub protruding into the black inside.
- **The hub arc spans ±92°, wider than the ±78° value sweep.** It has to reach
  past the outermost branch or the ±80° arms emerge from nothing.
- **One inactive grey** for branch, inactive node and ring alike. They are one
  object drawn in three parts, so three near-but-not-equal greys read as a
  rendering fault rather than as hierarchy.
- **The battery is a rounded rectangle with a nub and a gradient fill.** Two
  earlier attempts were pills — a dim track with a proportional fill on top
  puts two rounded caps in the middle of a 24 × 12 px shape, and at any partial
  charge that reads as a blob rather than as a battery. A rectangle gives the
  charge boundary a straight edge, so the silhouette stays a battery at every
  level; the gradient runs vertically inside the fill, which is what keeps a
  20-px block from looking like a printed swatch.
- **One typeface at every size.** The big value was baked from Bitcount
  SemiBold while everything else came from Regular, and at 30 ppem SemiBold's
  dots stop merging — the lattice breaks open and the headline reads as a
  different face from its own label. All three faces now come from Regular,
  which stays solid to 30 ppem (it breaks up at 40). The cost is honest: the
  30 ppem digit is 18 px rather than 20, so the headline subtends **19.3′**
  instead of 21.4′ at 40 cm. One voice at 19.3′ beats two voices at 21.4′.
- **Motion is a system, not a per-site decision** — see
  `docs/ui/MOTION_RULES.md` and `tools/ui_motion.h`. The model moves on the
  encoder edge and only the presentation eases, transitions retarget rather
  than queue, timing is dt-based, and the curve follows the cause (ease-out for
  the hand, ease-in-out for system transitions). Encoder and button are on GPIO
  interrupts on the bench, because a polled loop loses every detent that
  arrives during the 29 ms blit.
- **A long press (350 ms) climbs back out of the current level**, so there is
  always an exit without a modifier. It acts on the way DOWN at the threshold;
  waiting for the release would make the exit feel later than the gesture.
- Compositing measures 0.22 ms/frame on the host against 29 ms of SPI transfer
  per full frame at 32 MHz, so the panel, not the drawing, sets the frame rate.
  Partial-region updates are still available if the H743 needs them later.
- `test/test_ui_wheel.c` checks the parts a still preview cannot show: the
  nine groups partition all 16 parameters exactly once, navigation closes,
  press/back preserves the group, level changes take the short way, the ease
  terminates within ~60 frames, and the orb stays fully on the panel at 0 and
  100.

## Open

- **Colour values are estimated from the reference image**, not sampled from a
  source file. Green, node grey and selected-node white should be replaced
  from the Figma export.
- The group split (9 groups, sizes 1–3) is a proposal. Names were chosen so no
  group shares a name with one of its own members.
- Not yet judged at the lowest backlight step or off-axis — that is what
  `design_bench.uf2` exists for.
- ADR-0026's three layouts and `render_layouts.c` are still in the tree. They
  should be deleted once the wheel is confirmed on hardware.
