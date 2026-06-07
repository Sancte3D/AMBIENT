#ifndef FAM_OLED_H
#define FAM_OLED_H

/*
 * Display abstraction (historical "oled_" namespace).
 *
 * r16 panel change: switched from the SSD1322 256×64 mono OLED to a 1.9"
 * ST7789 320×170 IPS LCD — chosen for readability, higher pixel density,
 * smooth UI transitions and outdoor brightness (no burn-in). The VISUAL
 * design stays minimal monochrome (black/white/grey, OP-1 language); colour
 * is not used. The whole draw/font/menu layer keeps working on a 4-bit GREY
 * framebuffer; the device driver (src/lcd_st7789.c) converts grey → RGB565
 * when streaming to the panel, so only the driver + dimensions changed.
 *
 * Framebuffer: WIDTH×HEIGHT, 4-bit grey, 2 px/byte (high nibble = left pixel).
 * All draws are buffered; nothing reaches the panel until oled_show().
 */

#include <stdint.h>
#include <stddef.h>

#define OLED_WIDTH   320
#define OLED_HEIGHT  170
#define OLED_FB_SIZE ((OLED_WIDTH * OLED_HEIGHT + 1) / 2)  /* 4 bpp grey */

/* Bring up SPI0, the GPIOs, reset the panel, run the init sequence, and
 * push a zeroed framebuffer so the display is dark before turning on. */
void oled_init(void);

/* Fill the framebuffer with a uniform grey level (0..15). */
void oled_fill(uint8_t gs);

/* Draw an 8x8 ASCII glyph at pixel (x,y) in grey level gs (0..15).
 * Step 2 only ships glyphs for the chars used in the static banner
 * ("FIELD AMBIENCE V0.9 STEP 2"); unknown chars render as a blank cell.
 * Later steps will extend the font. */
void oled_text(int x, int y, const char *s, uint8_t gs);

/* Stream the framebuffer to the panel (column + row window then RAM write). */
void oled_show(void);

/* --- Step 12b #7: lower-level draw primitives (used by the menu) ----------
 * All write to the in-RAM framebuffer; they don't push to the panel. Bounds
 * are clipped automatically — out-of-screen calls are safe no-ops. */

/* Set a single pixel to grey level gs (0..15). */
void oled_pixel(int x, int y, uint8_t gs);

/* Filled rectangle. */
void oled_rect_fill(int x, int y, int w, int h, uint8_t gs);

/* Filled rounded-end pill (horizontal): a w×h rect with semicircular ends —
 * the pill indicator in the menu. h is the diameter at the ends. */
void oled_pill(int x, int y, int w, int h, uint8_t gs);

/* Anti-aliased rounded rectangle, corner radius r. SDF-based: edges are
 * smoothed across the 4-bit grey, so pills/battery look clean, not jaggy.
 * Pixels are max-blended so overlapping AA shapes don't darken each other. */
void oled_rrect_fill(int x, int y, int w, int h, int r, uint8_t gs);

/* Anti-aliased rounded-rectangle outline of stroke thickness `t` (px). */
void oled_rrect_stroke(int x, int y, int w, int h, int r, int t, uint8_t gs);

/* Plot a pixel taking the brighter of existing/gs (for AA compositing). */
void oled_pixel_max(int x, int y, uint8_t gs);

/* Scaled bitmap font: draws each 8x8 glyph as scale×scale pixel blocks.
 * scale=1 is identical to oled_text. Used for the big value display. */
void oled_text_scaled(int x, int y, const char *s, uint8_t gs, int scale);

/* Anti-aliased scaled text: the 8x8 glyph is bilinearly upscaled and mapped to
 * grey levels, so edges are smooth instead of blocky — uses the panel's 4-bit
 * greyscale. Looks far cleaner than oled_text_scaled at scale ≥ 2. */
void oled_text_smooth(int x, int y, const char *s, uint8_t gs, int scale);

/* Pixel width of a string when rendered with the given scale. */
int oled_text_width(const char *s, int scale);

/* Expose the framebuffer for host-side rendering (preview PNG). NOT used on
 * device — there oled_show() reads it directly. */
const uint8_t *oled_framebuffer(void);

#endif
