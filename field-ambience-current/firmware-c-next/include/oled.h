#ifndef FAM_OLED_H
#define FAM_OLED_H

/*
 * SSD1322 256x64 OLED driver — C port of firmware/ssd1322.py.
 *
 * Targets the ER-OLEDM032-1W module per SPEC v0.6 §6. 4-wire SPI, 4-bit
 * greyscale, GDDRAM layout = 2 px per byte with the high nibble = left
 * pixel (matches MicroPython framebuf.GS4_HMSB so behaviour is identical).
 *
 * Pin map (per SPEC v0.6 §5; firmware/config.py PIN_OLED_*):
 *   SPI0 SCK   GP6  -> OLED pin 12 SCLK
 *   SPI0 MOSI  GP7  -> OLED pin 13 SDIN
 *   CS         GP5  -> OLED pin 8  CS#
 *   DC         GP8  -> OLED pin 9  D/C#
 *   RES        GP9  -> OLED pin 7  RES#
 *   (MISO/GP4 is unused by SSD1322; later steps will reassign it to I2S DIN.)
 *
 * All draw operations write to an in-RAM framebuffer; nothing reaches the
 * panel until oled_show() is called. That matches the .py driver's contract.
 */

#include <stdint.h>
#include <stddef.h>

#define OLED_WIDTH   256
#define OLED_HEIGHT  64
#define OLED_FB_SIZE ((OLED_WIDTH * OLED_HEIGHT) / 2)  /* 4 bpp -> 8192 bytes */

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

#endif
