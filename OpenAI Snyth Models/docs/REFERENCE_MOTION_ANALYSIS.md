# Motion-reference analysis and clean translation

This record documents what was measured in the ten user-supplied files and the
three public Pinterest previews on 16 July 2026. The references are review
inputs only: no source pixel, frame, icon, figure, character, code, or media
file is stored in this package or present in a generated preview.

The GIF metrics come from `tools/analyze_reference_motion.py`. Frames are
normalized to a 160 × 160 analysis canvas; inter-frame and loop-seam change are
mean absolute luminance differences on a 0–1 scale. The numbers compare motion
behavior, not artistic similarity.

## Uploaded files

| Ref. | Source behavior measured | Reusable principle | New implementation |
|---|---|---|---|
| U1 | monochrome selector, 900 × 600, 177 frames / 7.08 s, motion 0.00489, seam 0.00206 | stable hierarchy, one high-contrast focus row, slow state changes | **FOCUS RAIL** uses original abstract controls, geometry, spacing, and a sound-reactive status pulse |
| U2 | static 500 × 500 chromatic spectrum on black | sparse vertical energy, asymmetry, large negative space | **PRISM VEINS** builds deterministic vertical streaks from band energy |
| U3 | crystalline spectrum, 500 × 500, 24 frames / 0.96 s, motion 0.04329, seam 0.04228 | mirrored needles, bright transient tips, dense center band | **CRYSTAL CHOIR** creates a new procedural needle field |
| U4 | figurative radial flash, 495 × 500, 19 frames / 0.76 s, motion 0.10990, seam 0.12441 | brief central burst and directional rays | **RADIANT GATE** deliberately replaces the person with a non-figurative breathing aperture |
| U5 | slow organic crawler, 600 × 233, 86 frames / 4.30 s, motion 0.00398, seam 0.00791 | segmented propagation and very restrained locomotion | **LUMEN RIBBON** is a non-anatomical bead-and-fibre signal ribbon, not the referenced organism |
| U6 | pixel trace, 498 × 281, 17 frames / 1.70 s, motion 0.01494, seam 0.02201 | path reveal, low frame rate, luminous head | **GLYPH RELAY** draws a newly authored stepped signal glyph without the hand or animal form |
| U7 | star-field particle figure, 500 × 500, 44 frames / 1.76 s, motion 0.01354, seam 0.00681 | particles gather into a legible current, then disperse | **PARTICLE CURRENT** stays abstract and uses an original spiral flow instead of an animal silhouette |
| U8 | static 540 × 540 monochrome network chamber | perspective grid, falling nodes, horizon depth | **SIGNAL CHAMBER** combines a new vanishing grid with bounded signal rain |
| U9 | static 600 × 600 oscillating-line orb study | side-emphasized shell and many close traces | **RESONANCE ORB** uses six independently modulated procedural contours |
| U10 | square-particle halo, 500 × 500, 24 frames / 1.20 s, motion 0.03271, seam 0.03359 | rectangular particles define a hollow ring | **GLITCH HALO** places newly seeded rectangles around an audio-deformed ellipse |

The fastest uploaded animation is U4 and the calmest animated references are
U5 and U1. The new set therefore avoids a single global motion speed: menus and
organic systems move slowly, while transient-driven light systems can flare.

## Public Pinterest previews

Only the public static preview and page metadata were inspected; no Pinterest
media was downloaded into the repository and no account was used.

| Ref. | Public link | Composition principle | New implementation |
|---|---|---|---|
| P1 | <https://in.pinterest.com/pin/wave-pattern-graphic--104779128829130368/> | bilateral waveform lobes around one bright axis | **TWIN PULSE** generates six new symmetric contours from bass and mid energy |
| P2 | <https://br.pinterest.com/pin/15692298698346810/> | a soft vertical light body that appears to fall or melt | **CHROMA FALL** uses nested procedural shells and five descending signal streaks |
| P3 | <https://ru.pinterest.com/pin/104779128829130366/> | blurred radial capsules with a quiet center | **SOFTBURST** uses discrete line capsules and original elliptical spacing on black |

## Deliberate separation rules

- Human, animal, clothing, and hand silhouettes were not reproduced.
- Reference typography, labels, icons, palettes, frame sequences, and exact
  proportions were not traced.
- Every new frame starts as an empty packed framebuffer and is drawn by the C
  primitives in `firmware/src/ambient_visuals.c`.
- The attached files remain outside `OpenAI Snyth Models`; SHA manifests cover
  only project-authored source and generated project assets.
- Two palette treatments per renderer test range without changing geometry or
  hiding weak motion design behind one color scheme.

This is a technical authorship record, not a legal opinion. Final product art,
names, and marketing still require the commercial review described in
`ORIGINALITY_AND_IP.md`.
