#ifndef FAM_OLED_H
#define FAM_OLED_H

/*
 * Display abstraction (historical "oled_" namespace).
 *
 * Panel: 320×WIDTH or 240×WIDTH ST7789 IPS LCD — selectable via the
 * FAM_LCD_PANEL define so we can prep for ADR-0015's 1.9"→2.0" pivot
 * without changing default behaviour. The whole draw/font/menu layer
 * keeps working on a 4-bit GREY framebuffer; the device driver
 * (src/hal_pico/lcd_st7789_pico.c, src/hal_h743/lcd_st7789_h743.c)
 * converts grey → RGB565 via src/oled_color.c (accent-tinted).
 *
 * Framebuffer: WIDTH×HEIGHT, 4-bit grey, 2 px/byte (high nibble = left pixel).
 * All draws are buffered; nothing reaches the panel until oled_show().
 *
 * --- Panel selection (ADR-0015) ----------------------------------------
 * Default: 1.9" 170×320 (current hardware). Pass -DFAM_LCD_PANEL_2_0 to
 * build for the proposed 2.0" 240×320 module. UI layout constants in
 * menu.c / display_hw_test.c (BAR_Y etc.) are tuned for 170 px height;
 * the 2.0" build will compile and display correctly but leaves visible
 * dead space at the bottom until the layout is rebalanced — that's a
 * separate task after the physical module is verified.
 *
 * Anti-guess: do NOT enable FAM_LCD_PANEL_2_0 in any default
 * configuration until the real Waveshare 2" module SKU + pin-order +
 * mechanical dimensions are confirmed against the shipped part (see
 * BOM_MASTER §5 UNVERIFIED entry).
 */

#include <stdint.h>
#include <stddef.h>

#if defined(FAM_LCD_PANEL_2_0)
#  define OLED_PANEL_NAME       "2.0in 240x320 ST7789 (PROPOSED, UNVERIFIED)"
#  define OLED_WIDTH            320
#  define OLED_HEIGHT           240
#  define OLED_LCD_X_OFFSET     0
#  define OLED_LCD_Y_OFFSET     0      /* 2.0" die is the full 240×320 */
#else  /* default: FAM_LCD_PANEL_1_9 */
#  define OLED_PANEL_NAME       "1.9in 170x320 ST7789V2"
#  define OLED_WIDTH            320
#  define OLED_HEIGHT           170
#  define OLED_LCD_X_OFFSET     0
#  define OLED_LCD_Y_OFFSET     35     /* 170-px window inside 240×320 GRAM */
#endif

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

/* Stream the framebuffer to the panel (column + row window then RAM write).
 * Blocking — the proven path; used for init/clear-GRAM and as the async
 * fallback. */
void oled_show(void);

/* Convert one framebuffer row to RGB565 (MS byte first) via the accent LUT.
 * `out` holds OLED_WIDTH*2 bytes. Pure — shared by the blocking + async
 * drivers; host-tested. */
void oled_convert_row(int y, const uint16_t *lut565, uint8_t *out);

/* Non-blocking flush (h743 SPI-DMA): kicks a background row-pipelined transfer
 * of the whole framebuffer and returns immediately, freeing the main loop
 * during the ~29 ms panel write (REALTIME_AUDIO_RULES §8). Skips silently if a
 * flush is already in flight. On targets without DMA this falls back to the
 * blocking oled_show(). */
void oled_show_async(void);

/* True while an async flush is streaming — the caller must NOT redraw the
 * framebuffer until this clears (the DMA/ISR is reading it). */
int  oled_flush_busy(void);

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
