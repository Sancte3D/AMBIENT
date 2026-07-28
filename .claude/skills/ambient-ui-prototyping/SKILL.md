---
name: ambient-ui-prototyping
description: Create pixel-accurate AMBIENT LCD mockups, host-simulator prototypes, state fixtures, fonts, icons, sprites, textures, and embedded-ready display assets. Use when turning a UI direction into testable 320×170 screens or preparing visual assets for the C renderer.
---

# AMBIENT UI Prototyping

Prototype against the real pixel and color constraints before changing the target renderer.

## Inspect

1. Read `../ambient-ui-lcd-smt/references/project-baseline.md`.
2. Inspect `field-ambience-current/firmware-c-next/tools/display_sim.html`, the draw primitives, font generator, baked font, color conversion, and representative UI tests.
3. Confirm the active panel configuration and framebuffer format.

## Build the prototype

- Use an exact 320 × 170 logical canvas for the default panel.
- Display enlarged previews only with integer nearest-neighbor scaling.
- Keep layout coordinates, clipping, type metrics, and RGB565 quantization identical to the intended renderer.
- Create deterministic fixtures for every important state rather than one idealized hero screen.
- Exercise longest labels, minimum/maximum values, empty state, active state, rapid changes, and overlays.
- Keep hardware controls visible beside or annotated outside the canvas when presenting interaction, but never draw those annotations into the production screen.
- Provide 1× output for pixel inspection and a larger integer-scaled output for review.

## Assets

- Prefer renderer primitives for simple geometry.
- Rasterize complex static artwork at build time, not on the MCU.
- Quantize colors to the actual RGB565 or grayscale/accent pipeline before approval.
- Measure raw and encoded bytes for every sprite, font, or texture.
- Use transparency formats and compression only when the decode cost, temporary RAM, and visual result are measured.
- Keep world assets stylistically related and technically interchangeable.
- Avoid anti-aliasing assumptions the target renderer cannot reproduce.
- Preserve source artwork separately from generated C arrays.

## Handoff

For each approved prototype provide:

- exact panel and orientation;
- named state fixture;
- 1× reference image or deterministic render;
- bounding boxes and z-order;
- color tokens after target quantization;
- font/glyph requirements and byte cost;
- assets with dimensions, format, encoded size, decode path, and license;
- motion hooks and dirty regions;
- differences between prototype and current renderer;
- acceptance checks for the host simulator and real panel.

Route visual judgment to `ambient-ui-art-direction`, behavior to `ambient-lcd-ux`, animation to `ambient-lcd-motion`, and target implementation to `ambient-display-pipeline`.
