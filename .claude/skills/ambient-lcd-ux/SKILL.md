---
name: ambient-lcd-ux
description: Design and review AMBIENT's embedded LCD interaction model, information hierarchy, encoder and button mappings, feedback states, menus, shortcuts, accessibility, and screen flows. Use for any behavior seen on the display or any control-to-screen interaction.
---

# AMBIENT LCD UX

Make the physical controls primary and the screen explanatory.

## Model the interaction

1. Read `../ambient-ui-lcd-smt/references/project-baseline.md`.
2. Read the current control map and state machine before proposing changes.
3. Name the user goal in physical terms: play, alter, hold, switch world, save, recall, clear, generate, or inspect.
4. Map each action to a control, gesture, state transition, display response, LED response, and audio response.
5. Check that the action remains understandable without reading a manual.

Use a state table:

| Current state | Input | Immediate feedback | Result | Undo/recovery |
|---|---|---|---|---|

## Rules

- Show the consequence near the control's conceptual location when possible.
- Acknowledge button presses immediately, even if the operation completes later.
- Keep encoder feedback continuous, stable, and monotonic.
- Let acceleration change value rate, not visual continuity.
- Make focus visible without relying only on color.
- Distinguish temporary overlays from persistent modes.
- Preserve the user's mental location during transitions.
- Allow animations to be interrupted and retargeted by new input.
- Prefer one shallow menu over nested navigation.
- Do not hide core performance functions behind long press unless the gesture is already established and visible.
- Use destructive-action confirmation only when recovery is genuinely difficult; otherwise provide immediate undo.
- Define behavior for simultaneous keys, held cells, modifier order, rapid encoder movement, and input during animation.

## Legibility

- Evaluate at the real display size and expected viewing distance.
- Test the lowest backlight setting and off-axis viewing.
- Avoid essential information in one-pixel low-contrast text.
- Define truncation before shrinking type.
- Use icons only when their meaning is learned, standard, or paired with a label.
- Do not use touch conventions; there is no touch target.

## Deliver

Provide:

- screen/state inventory;
- hardware-control map;
- transition table;
- feedback timing;
- exceptional and recovery states;
- copy strings with maximum lengths;
- accessibility/legibility risks;
- acceptance scenarios that can become deterministic tests.

Route visual styling to `ambient-ui-art-direction`, motion to `ambient-lcd-motion`, and test implementation to `ambient-embedded-verification`.
