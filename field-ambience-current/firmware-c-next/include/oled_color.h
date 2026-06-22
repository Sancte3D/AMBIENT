#ifndef FAM_OLED_COLOR_H
#define FAM_OLED_COLOR_H

/*
 * Accent-tinted grey → RGB565 conversion (ADR-0015 step 1).
 *
 * The whole UI draws into a 4-bit GREY framebuffer (oled.h / oled_draw.c).
 * On the way to the panel each grey level 0..15 is mapped to an RGB565 colour.
 * Historically that mapping lived inline in the ST7789 driver as a neutral
 * grey ramp (r=g=b). This module centralises it so the Pico driver, the H743
 * driver (Step 13.3) and the host preview tool all share ONE conversion and
 * can never drift, and adds a per-world ACCENT tint.
 *
 * Accent model — a cast, not a recolour:
 *   out_channel = grey4/15 · accent_channel/255, packed to RGB565.
 * With the default WHITE accent (255,255,255) this reproduces the classic
 * neutral grey ramp bit-for-bit — the OP-1 monochrome look is unchanged. A
 * coloured accent tints the ramp: mids carry the hue, the brightest pixels
 * become "<accent> white". Text stays legible; the screen takes on the world's
 * colour. Keep at least one accent channel near 255 so whites stay bright.
 *
 * The conversion is a 16-entry LUT, rebuilt lazily when the accent changes —
 * cheap, and the per-pixel path in the driver stays a single array index.
 */

#include <stdint.h>

/* Set the accent IMMEDIATELY (live = target). Default is white (mono). Use at
 * boot / init so the first frame is already the right colour, no fade-in. */
void            oled_set_accent(uint8_t r, uint8_t g, uint8_t b);

/* Read the live (currently-displayed) accent. */
void            oled_get_accent(uint8_t *r, uint8_t *g, uint8_t *b);

/* Set the accent TARGET — the live accent then crossfades toward it as
 * oled_accent_tick() is called from the render loop (~120 ms time constant).
 * This is what a world change uses, so the screen morphs blue→aqua→… instead
 * of snapping. */
void            oled_set_accent_target(uint8_t r, uint8_t g, uint8_t b);

/* Advance the live accent toward the target by the time since the last tick.
 * Call once per rendered frame from the UI loop; returns non-zero while the
 * accent is still moving (so the caller knows to keep redrawing). No-op once
 * live == target. */
int             oled_accent_tick(uint32_t now_ms);

/* Snap live ← target instantly (for static renderers / the preview tool that
 * have no frame loop to tick). */
void            oled_accent_settle(void);

/* Convert one 4-bit grey level (0..15) to accent-tinted RGB565. */
uint16_t        oled_grey565(uint8_t grey4);

/* The full 16-entry LUT for the driver's hot row loop (index by grey nibble). */
const uint16_t *oled_grey565_lut(void);

#endif
