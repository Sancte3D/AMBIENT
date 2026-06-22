/*
 * ST7789 320×170 IPS-LCD driver — r16 panel replacement for the SSD1322 OLED.
 *
 * WHY LCD: chosen for outdoor readability, higher pixel density, smoother UI
 * transitions and no burn-in. The VISUAL design stays minimal monochrome
 * (black/white/grey, OP-1 language) — colour is deliberately NOT used. The
 * whole draw/font/menu layer keeps drawing into the SAME 4-bit GREY
 * framebuffer (src/oled_draw.c); this driver converts grey → RGB565 on the
 * fly while streaming, so only the panel driver, the dimensions and the
 * layout constants changed.
 *
 * Panel: 1.9" 170×320 IPS, ST7789(V2) controller. In landscape (320 wide ×
 * 170 tall) the 170-px side sits offset inside the controller's 240×320 GRAM,
 * so the visible window is columns 0..319, rows 35..204 (Y offset = 35).
 *
 * Pins reuse the SPI0 group the OLED used (SPEC v0.6 §5) — no pin-map change:
 *   GP5 CS, GP6 SCK, GP7 MOSI(SDA), GP8 DC, GP9 RES.
 * Backlight: the panel's BLK line is NOT a Pico GPIO (all 24 are allocated);
 * it is driven by a spare PCA9685 PWM channel through a small N-FET, so
 * brightness is set over I²C in the board layer — not here. This file owns
 * SPI + reset + init + show only, mirroring the old oled.c split.
 *
 * Device-only: pulls in the Pico SDK and is NOT compiled by the host tests.
 */

#include "oled.h"
#include "oled_color.h"          /* shared accent-tinted grey→RGB565 LUT */
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

/* --- pin map (reuses the SSD1322 SPI0 group, SPEC v0.6 §5) ---
 * Every pin is #ifndef-guarded so bench targets (tools/display_hw_test.c)
 * can re-map to their breadboard wiring via CMake compile definitions
 * without touching this file. Device builds use the defaults below. */
#ifndef LCD_SPI
#define LCD_SPI      spi0
#endif
#ifndef LCD_PIN_SCK
#define LCD_PIN_SCK  6
#endif
#ifndef LCD_PIN_MOSI
#define LCD_PIN_MOSI 7
#endif
#ifndef LCD_PIN_CS
#define LCD_PIN_CS   5
#endif
#ifndef LCD_PIN_DC
#define LCD_PIN_DC   8
#endif
#ifndef LCD_PIN_RES
#define LCD_PIN_RES  9
#endif
/* ST7789 tolerates fast SPI; 32 MHz gives ~30 fps full-frame and a clean edge
 * on the PCB header. Lower to 16 MHz if testing over long dupont leads —
 * the breadboard target (tools/display_hw_test.c) does exactly that via a
 * compile definition, hence the #ifndef guard. */
#ifndef LCD_SPI_HZ
#define LCD_SPI_HZ   (32 * 1000 * 1000)
#endif

/* Landscape orientation. MADCTL 0x60 = MX | MV (row/col exchange). If the
 * image comes up mirrored or upside-down on the bench, flip to 0xA0/0xC0/0x00
 * — purely cosmetic.
 *
 * GRAM X/Y offsets come from include/oled.h (panel-selectable via
 * FAM_LCD_PANEL_2_0). 1.9" needs Y=35 (170-px window inside the controller's
 * 240×320 GRAM); 2.0" is the full 240×320 die, no offset. */
#define LCD_MADCTL   0x60

/* ST7789 command set (subset used here). */
#define ST_SWRESET 0x01
#define ST_SLPOUT  0x11
#define ST_NORON   0x13
#define ST_INVON   0x21
#define ST_DISPON  0x29
#define ST_CASET   0x2A
#define ST_RASET   0x2B
#define ST_RAMWR   0x2C
#define ST_MADCTL  0x36
#define ST_COLMOD  0x3A

/* grey 0..15 → RGB565 now lives in oled_color.c (shared with the H743 driver
 * + host preview, and carries the per-world accent tint). The default accent
 * is white, so with no world selected this is the legacy neutral grey ramp. */

/* --- low-level CS/DC-managed command + bulk data --- */

static inline void lcd_cs(bool low) { gpio_put(LCD_PIN_CS, low ? 0 : 1); }
static inline void lcd_dc(bool data) { gpio_put(LCD_PIN_DC, data ? 1 : 0); }

static void cmd(uint8_t c) {
    lcd_dc(false);
    lcd_cs(true);
    spi_write_blocking(LCD_SPI, &c, 1);
    lcd_cs(false);
}

static void data(const uint8_t *buf, size_t n) {
    lcd_dc(true);
    lcd_cs(true);
    spi_write_blocking(LCD_SPI, buf, n);
    lcd_cs(false);
}

static void data_one(uint8_t b) { data(&b, 1); }

static void reset_panel(void) {
    gpio_put(LCD_PIN_RES, 1);
    sleep_ms(10);
    gpio_put(LCD_PIN_RES, 0);
    sleep_ms(20);
    gpio_put(LCD_PIN_RES, 1);
    sleep_ms(120);
}

static void run_init_sequence(void) {
    cmd(ST_SWRESET);            sleep_ms(150);
    cmd(ST_SLPOUT);             sleep_ms(120);
    cmd(ST_COLMOD);  data_one(0x55);   /* 16-bit/pixel RGB565 */
    cmd(ST_MADCTL);  data_one(LCD_MADCTL);
    cmd(ST_INVON);                     /* IPS panels are inverted */
    cmd(ST_NORON);   sleep_ms(10);
    oled_fill(0);                      /* dark framebuffer */
    oled_show();                       /* clear GRAM before turning on */
    cmd(ST_DISPON);  sleep_ms(20);
}

void oled_init(void) {
    spi_init(LCD_SPI, LCD_SPI_HZ);
    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(LCD_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(LCD_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(LCD_PIN_CS);  gpio_set_dir(LCD_PIN_CS,  GPIO_OUT); gpio_put(LCD_PIN_CS,  1);
    gpio_init(LCD_PIN_DC);  gpio_set_dir(LCD_PIN_DC,  GPIO_OUT); gpio_put(LCD_PIN_DC,  0);
    gpio_init(LCD_PIN_RES); gpio_set_dir(LCD_PIN_RES, GPIO_OUT); gpio_put(LCD_PIN_RES, 1);

    reset_panel();
    run_init_sequence();
}

/* Address-window the full visible area, then stream the framebuffer converting
 * each 4-bit grey pixel to RGB565 a row at a time (640 B per row → 170 rows). */
void oled_show(void) {
    const uint8_t *fb = oled_framebuffer();

    uint16_t xs = OLED_LCD_X_OFFSET, xe = OLED_LCD_X_OFFSET + OLED_WIDTH  - 1;
    uint16_t ys = OLED_LCD_Y_OFFSET, ye = OLED_LCD_Y_OFFSET + OLED_HEIGHT - 1;
    cmd(ST_CASET); { const uint8_t d[4] = { (uint8_t)(xs >> 8), (uint8_t)xs,
                                            (uint8_t)(xe >> 8), (uint8_t)xe }; data(d, 4); }
    cmd(ST_RASET); { const uint8_t d[4] = { (uint8_t)(ys >> 8), (uint8_t)ys,
                                            (uint8_t)(ye >> 8), (uint8_t)ye }; data(d, 4); }
    cmd(ST_RAMWR);

    const uint16_t *grey565 = oled_grey565_lut();   /* accent-tinted, shared */
    static uint8_t line[OLED_WIDTH * 2];   /* one row of RGB565, byte-swapped */
    lcd_dc(true);
    lcd_cs(true);
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        const uint8_t *row = fb + (size_t)y * (OLED_WIDTH / 2);
        uint8_t *o = line;
        for (int x = 0; x < OLED_WIDTH; x += 2) {
            uint8_t b = *row++;
            uint16_t hp = grey565[b >> 4];      /* even x = high nibble (left) */
            uint16_t lp = grey565[b & 0x0F];    /* odd  x = low  nibble        */
            *o++ = (uint8_t)(hp >> 8); *o++ = (uint8_t)hp;   /* MS byte first */
            *o++ = (uint8_t)(lp >> 8); *o++ = (uint8_t)lp;
        }
        spi_write_blocking(LCD_SPI, line, sizeof line);
    }
    lcd_cs(false);
}
