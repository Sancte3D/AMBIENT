# Motion-reference analysis and clean translation

This record documents what was measured in the ten user-supplied files and the
three public Pinterest previews on 16 July 2026. The references are review
inputs only: no source pixel, frame, icon, figure, character, code, or media
file is stored in this package or present in a generated preview.

The GIF metrics come from `tools/analyze_reference_motion.py`. Frames are
normalized to a 160 × 160 analysis canvas; inter-frame and loop-seam change are
mean absolute luminance differences on a 0–1 scale. The numbers compare motion
behavior, not artistic similarity.

The analyser can now emit a numbered contact sheet containing every composited
frame with its delay and inter-frame delta. Those sheets were generated in a
temporary review directory for this pass and inspected in full. They are not
stored in the repository because reference pixels are analysis inputs, not
product assets.

## Full frame-sequence audit

| Sequence | Frame-by-frame finding | Timing decision in the cinematic redesign |
|---|---|---|
| 17-frame blue trace | alternating hold/jump rhythm; the large visual change occurs on packet replacement, with near-identical frames between changes | **ELECTRIC SCRIPT** quantizes motion to twelve states and intentionally repeats poses |
| 24-frame square halo | global brightness stays almost fixed while many local rectangles change position/size every frame | **VOXEL BLOOM** keeps a stable cavity and moves independently seeded cells rather than pulsing the whole ring |
| 44-frame particle form | curved gathered state opens into a broad cloud, crosses a bright central state, then curls back before closure | **PARTICLE APPARITION** uses a periodic gather field and a separately moving parametric current |
| 26-frame wire chamber | floor perspective remains locked; rain and canopy detail rewrite at 50 fps across almost the full image | **WIRE RAIN** separates a static depth scaffold from integer-rate wrapping streaks |
| 24-frame crystal field | the center seam remains legible while needle lengths, tips, and colour neighbourhoods change on every frame | **PRISM RAIN** gives each of 116 needles its own 2–5× loop rate around one authored seam |
| 19-frame radial flash | central white mass is stable; ray length, direction, density, and exposure change aggressively every 40 ms | **RADIANT THRESHOLD** keeps its abstract aperture fixed while 54 rays flare and steer independently |
| 86-frame green ribbon | slow travelling compression; silhouette and overall luminance barely change, but underside fibres continually propagate | **BIOLUME WEAVE** uses two coupled waves and procedural fibre motion at a slower 60 ms frame delay |
| 75-frame monochrome volume | boundary remains approximately spherical while the dense internal line field is replaced continuously; lateral caustics persist | **NOISE CHRYSALIS** fills the body with 92 moving cross-sections and retains original side-light logic |
| 177-frame selector | long readable holds are separated by short focus transitions; motion is state-driven, not decorative | **FOCUS RAIL HQ** travels through five original rows and back with a seamless cosine schedule |
| static chromatic horizon | sparse asymmetric peaks, black negative space, narrow white cores, and coloured vertical bloom carry the image without global motion | **SPECTRAL HORIZON** limits motion amplitude and preserves the dark field |

This audit corrected a major error in the first translation: merely drawing the
same nouns—ray, particle, line, square—does not reproduce the visual behavior.
The cinematic set therefore models separate light bodies, cores, bloom, and
temporal roles. Its full specification and device-capacity options are in
`HIGH_QUALITY_VISUALS.md`.

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
