---
name: ambient-ui-art-direction
description: Define or review the visual language of AMBIENT's embedded LCD, including composition, typography, color, icons, world identities, screen mockups, and anti-generic art direction. Use when creating a screen, visual system, UI concept, display asset, or aesthetic critique for the instrument.
---

# AMBIENT UI Art Direction

Design the LCD as part of a physical musical instrument.

## Inspect first

1. Read `../ambient-ui-lcd-smt/references/project-baseline.md`.
2. Inspect the existing screen renderer, font assets, simulator, recent screenshots, and world definitions.
3. State the screen's single job, the viewing moment, and the hardware control that drives it.
4. Separate visual identity decisions from firmware implementation decisions.

## Establish a direction

Define:

- one visual premise grounded in AMBIENT's sound, materials, controls, and environments;
- one dominant composition rule;
- a compact type scale that works on the real pixel grid;
- black/white/gray neutrals plus one world accent at a time;
- an icon grammar based on common stroke, corner, fill, optical weight, and baseline rules;
- a motion role for each element that moves.

Keep the default 320 × 170 landscape canvas unless the current build targets a verified alternative.

## Visual rules

- Make hierarchy readable in a one-second glance.
- Keep persistent status information quieter than the current choice or playable state.
- Use negative space as structure, not as unused filler.
- Keep text short enough to avoid marquee behavior in normal operation.
- Snap repeated geometry to a small spacing system; derive exceptions optically.
- Render critical one-pixel strokes at integer coordinates.
- Use saturated accents in limited area. Do not make the whole UI pale to appear calm.
- Let each world vary through accent, texture, rhythm, or a restrained motif while preserving one common interface grammar.
- Design for the actual panel and bezel. Check the composition at physical size, not only enlarged on a desktop monitor.

## Avoid

- web cards, navigation bars, pills, glassmorphism, drop-shadow stacks, and dashboard chrome;
- decorative gradients that band badly in RGB565;
- tiny gray text on black;
- generic synth clichés such as arbitrary waveforms, equalizers, knobs, and neon grids;
- copying Teenage Engineering, Ableton, Elektron, or another product's composition or icon set;
- animation added solely to make an idle screen look busy;
- more colors than the current state can explain.

## Deliver

For a new or revised screen, provide:

1. intent and visual premise;
2. 320 × 170 layout with exact bounding boxes;
3. type sizes, weights, line heights, and truncation rules;
4. RGB565-ready color tokens and contrast notes;
5. icon construction rules;
6. state variants: normal, focused, active, disabled, loading, error where applicable;
7. motion handoff with element, trigger, duration, easing, start/end values, and interrupt behavior;
8. a list of assets or renderer primitives required.

Route animation feasibility to `ambient-lcd-motion` and implementation to `ambient-display-pipeline`.
