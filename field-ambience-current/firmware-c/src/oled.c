/*
 * SSD1322 256x64 OLED driver — C port of firmware/ssd1322.py.
 *
 * The init sequence is a byte-for-byte mirror of the MicroPython version
 * so the panel comes up the same way on both firmware paths. The GS4_HMSB
 * pixel format (2 px per byte, MSN = left pixel) matches the SSD1322's
 * GDDRAM layout, so the framebuffer streams out as-is.
 */

#include "oled.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <string.h>

/* --- pin map, per SPEC v0.6 §5 --- */
#define OLED_SPI     spi0
#define OLED_PIN_SCK 6
#define OLED_PIN_MOSI 7
#define OLED_PIN_CS  5
#define OLED_PIN_DC  8
#define OLED_PIN_RES 9
#define OLED_SPI_HZ  (8 * 1000 * 1000)  /* 8 MHz per firmware/config.py */

/* Column-address window for the 256-px-wide visible region.
 * Each column address spans 4 pixels: 0x1C..0x5B = 64 cols = 256 px. */
#define OLED_COL_START 0x1C
#define OLED_COL_END   0x5B
#define OLED_ROW_START 0x00
#define OLED_ROW_END   0x3F

/* In-RAM framebuffer; nothing reaches the panel until oled_show(). */
static uint8_t fb[OLED_FB_SIZE];

/* The starter font lives in font_8x8.c. */
extern const uint8_t *font_glyph(char c);


/* --- low-level: CS-/DC-managed single-byte command and bulk data --- */

static inline void oled_cs(bool low) { gpio_put(OLED_PIN_CS, low ? 0 : 1); }
static inline void oled_dc(bool data) { gpio_put(OLED_PIN_DC, data ? 1 : 0); }

static void cmd(uint8_t c) {
    oled_dc(false);
    oled_cs(true);
    spi_write_blocking(OLED_SPI, &c, 1);
    oled_cs(false);
}

static void data(const uint8_t *buf, size_t n) {
    oled_dc(true);
    oled_cs(true);
    spi_write_blocking(OLED_SPI, buf, n);
    oled_cs(false);
}

static void data_one(uint8_t b) {
    data(&b, 1);
}

/* --- reset + init: byte-for-byte mirror of firmware/ssd1322.py ---  */

static void reset_panel(void) {
    gpio_put(OLED_PIN_RES, 1);
    sleep_ms(10);
    gpio_put(OLED_PIN_RES, 0);
    sleep_ms(50);
    gpio_put(OLED_PIN_RES, 1);
    sleep_ms(50);
}

static void run_init_sequence(void) {
    /* Each (cmd, data...) pair is what the MicroPython driver writes. */
    cmd(0xFD); data_one(0x12);                              /* unlock */
    cmd(0xAE);                                              /* display off */
    cmd(0xB3); data_one(0x91);                              /* clock div / osc */
    cmd(0xCA); data_one(0x3F);                              /* mux = 64 */
    cmd(0xA2); data_one(0x00);                              /* offset */
    cmd(0xA1); data_one(0x00);                              /* start line */
    cmd(0xA0); { const uint8_t d[2] = {0x14, 0x11}; data(d, 2); }  /* remap + dual COM */
    cmd(0xB5); data_one(0x00);                              /* GPIO disabled */
    cmd(0xAB); data_one(0x01);                              /* internal VDD */
    cmd(0xB4); { const uint8_t d[2] = {0xA0, 0xFD}; data(d, 2); }  /* display enh A */
    cmd(0xC1); data_one(0x9F);                              /* contrast */
    cmd(0xC7); data_one(0x0F);                              /* master contrast = max */
    cmd(0xB1); data_one(0xE2);                              /* phase length */
    cmd(0xD1); { const uint8_t d[2] = {0x82, 0x20}; data(d, 2); }  /* display enh B */
    cmd(0xBB); data_one(0x1F);                              /* pre-charge V */
    cmd(0xB6); data_one(0x08);                              /* pre-charge T */
    cmd(0xBE); data_one(0x07);                              /* VCOMH */
    cmd(0xA6);                                              /* normal display */
    cmd(0xA9);                                              /* exit partial */
    oled_show();                                            /* clear before turning on */
    cmd(0xAF);                                              /* display on */
}

void oled_init(void) {
    /* SPI0 on GP6/GP7 (MISO/GP4 unused — later steps repurpose it). */
    spi_init(OLED_SPI, OLED_SPI_HZ);
    spi_set_format(OLED_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(OLED_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(OLED_PIN_MOSI, GPIO_FUNC_SPI);

    /* Manual GPIOs for CS, DC, RES. */
    gpio_init(OLED_PIN_CS);  gpio_set_dir(OLED_PIN_CS,  GPIO_OUT); gpio_put(OLED_PIN_CS,  1);
    gpio_init(OLED_PIN_DC);  gpio_set_dir(OLED_PIN_DC,  GPIO_OUT); gpio_put(OLED_PIN_DC,  0);
    gpio_init(OLED_PIN_RES); gpio_set_dir(OLED_PIN_RES, GPIO_OUT); gpio_put(OLED_PIN_RES, 1);

    memset(fb, 0, sizeof fb);
    reset_panel();
    run_init_sequence();
}

void oled_fill(uint8_t gs) {
    /* GS4_HMSB: 2 px per byte, both nibbles = gs. */
    uint8_t v = (uint8_t)(((gs & 0x0F) << 4) | (gs & 0x0F));
    memset(fb, v, sizeof fb);
}

void oled_show(void) {
    cmd(0x15); { const uint8_t d[2] = {OLED_COL_START, OLED_COL_END}; data(d, 2); }
    cmd(0x75); { const uint8_t d[2] = {OLED_ROW_START, OLED_ROW_END}; data(d, 2); }
    cmd(0x5C);                                              /* write RAM */
    data(fb, sizeof fb);
}

/* --- text rendering --- */

static void plot(int x, int y, uint8_t gs) {
    if ((unsigned)x >= OLED_WIDTH || (unsigned)y >= OLED_HEIGHT) return;
    size_t idx = (size_t)y * (OLED_WIDTH / 2) + (size_t)(x >> 1);
    uint8_t byte = fb[idx];
    if (x & 1) {
        byte = (uint8_t)((byte & 0xF0) | (gs & 0x0F));     /* low nibble = right px */
    } else {
        byte = (uint8_t)((byte & 0x0F) | ((gs & 0x0F) << 4)); /* high nibble = left px */
    }
    fb[idx] = byte;
}

static void blit_glyph(int x, int y, const uint8_t *rows, uint8_t gs) {
    for (int dy = 0; dy < 8; ++dy) {
        uint8_t row = rows[dy];
        for (int dx = 0; dx < 8; ++dx) {
            if (row & (0x80 >> dx)) plot(x + dx, y + dy, gs);
        }
    }
}

void oled_text(int x, int y, const char *s, uint8_t gs) {
    int cx = x;
    while (*s) {
        const uint8_t *g = font_glyph(*s);
        if (g) blit_glyph(cx, y, g, gs);
        cx += 8;  /* fixed-pitch 8 px advance */
        ++s;
    }
}
