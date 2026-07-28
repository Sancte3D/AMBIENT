---
name: ambient-display-pipeline
description: Implement or review AMBIENT's STM32H743 and ST7789 display pipeline, including SPI commands, panel offsets and orientation, RGB565 conversion, partial flush, DMA, D-cache coherency, DMA2D, memory placement, and host or hardware display tests.
---

# AMBIENT Display Pipeline

Work from the current driver, not from a generic ST7789 tutorial.

## Inspect

Read:

- `../ambient-ui-lcd-smt/references/project-baseline.md`;
- `field-ambience-current/firmware-c-next/include/oled.h`;
- `include/oled_color.h` and its source;
- `src/hal_h743/lcd_st7789_h743.c`;
- `src/hal_pico/lcd_st7789_pico.c`;
- `tools/display_hw_test.c`;
- the current linker script and map;
- ADR-0015 and later display-related decisions.

Confirm current panel, landscape dimensions, RAM location, SPI kernel clock/prescaler, DMA stream/request, IRQ priorities, and framebuffer ownership.

## Keep layers explicit

Separate:

1. UI state and layout;
2. draw primitives;
3. compact framebuffer or render surface;
4. color conversion;
5. dirty-region/window calculation;
6. ST7789 command transport;
7. asynchronous DMA lifecycle;
8. panel configuration and offsets.

Do not leak one panel's physical offset into UI layout constants.

## ST7789 transfer rules

- Set CASET and RASET from a validated landscape coordinate transform.
- Issue RAMWR once per contiguous region.
- Keep CS/DC transitions valid around command and pixel phases.
- Use big-endian byte order on the wire for RGB565.
- Verify MADCTL, inversion, color order, and row/column offsets on the actual module.
- Keep the 1.9-inch and any verified 2.0-inch configuration selectable from central constants.
- Reject out-of-bounds regions before programming the window.

## STM32H743 DMA/cache rules

- Do not place DMA1/DMA2 source buffers in DTCM.
- Place DMA-visible buffers in a verified DMA-accessible SRAM region.
- Align buffers and cache-maintenance ranges to 32-byte D-cache lines.
- Clean source ranges before DMA reads; invalidate destination ranges after DMA writes when applicable.
- Keep line-buffer state `volatile` only where ISR/main visibility requires it; use explicit ownership and barriers instead of broad volatile use.
- Handle HAL busy/error/abort paths without leaving CS asserted or the flush state permanently busy.
- Keep display IRQ priority numerically lower in urgency than audio refill.
- If adding DMA2D, document source/destination formats, strides, cache ownership, completion ordering, and the measured CPU saving.

## Change discipline

- Prefer partial-region flush over a new full-frame architecture.
- Recalculate memory from the current map before adding framebuffers, sprites, fonts, or decoded images.
- Keep the existing grayscale-to-accent RGB565 path when it satisfies the visual requirement.
- Introduce native per-pixel color only for a demonstrated requirement and with a measured RAM/CPU/transfer budget.
- Keep API behavior explicit: synchronous, asynchronous, busy-return, or callback. Do not pretend an async flush is synchronous.

## Validate

- Run host tests and both configured panel builds.
- Add unit tests for coordinate transforms, clipping, offsets, byte order, and dirty-region merging.
- Render reference frames in the host simulator.
- On hardware, verify solid color bars, one-pixel borders, corners, orientation, partial updates, repeated async flush, and recovery after intentional busy/error conditions.
- Run display stress under maximum audio load and record underruns, DMA errors, frame intervals, and visual artifacts.
