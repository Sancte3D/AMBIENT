/*
 * lcd_st7789_h743.c — STM32H743 SPI1 driver skeleton for the ST7789 LCD.
 *
 * Implements the same lcd_st7789.h API as src/hal_pico/lcd_st7789_pico.c.
 * Replaces RP2040 SPI0 with STM32 SPI1.
 *
 * Pin map (DS12110-verified):
 *   PA5 (pin 29) = SPI1_SCK   (AF5) → LCD SCK
 *   PA7 (pin 31) = SPI1_MOSI  (AF5) → LCD MOSI
 *   PA6 (pin 30) = GPIO       → LCD CS
 *   PC4 (pin 32) = GPIO       → LCD DC  (data/command)
 *   PC5 (pin 33) = GPIO       → LCD RES
 *   Backlight    = MCP23017-driven via PCA9685 ch12 (LCD_BLK_PWM gates Q2)
 *
 * Step 13.3 (TODO): STM32CubeH7 SPI1 init + DMA-driven frame flush. The
 * menu re-renders only the slot region (60-90% of the panel is static
 * background), so SPI throughput at 24 MHz is plenty without DMA — but a
 * DMA pump leaves the CPU free for the 1 kHz audio refill IRQ.
 */

#include "oled.h"   /* shared header: oled_init / oled_show */

void oled_init(void) {
    /* TODO(Step 13.3):
     *   1. Enable GPIOA/C clocks. Configure PA5/PA7 = AF5 (SPI1), high-speed.
     *      Configure PA6/PC4/PC5 = GPIO output, high-speed.
     *   2. Enable SPI1 clock. Configure SPI1 master, MSB-first, 8-bit, mode 0,
     *      baud = APB2/2 (≈ 60 MHz / 2 = 30 MHz, or div /4 = 15 MHz if EMI
     *      flags). The ST7789 datasheet says 62.5 ns SCL min → 16 MHz max
     *      for safety; 24 MHz is the bench-proven sweet spot.
     *   3. Run the standard ST7789 init sequence (same byte stream as the
     *      Pico driver — porting that block verbatim).
     *   4. Stream the 4-bit grey framebuffer converting via the SHARED
     *      oled_color.c LUT (oled_grey565_lut()) — same conversion + per-world
     *      accent tint as the Pico driver, so the two never drift.
     *
     * NB framebuffer placement (ADR-0015 D3): the RGB565 line/scratch buffer
     * goes in AXI-SRAM (D1, 0x24000000), NOT DTCM — DMA1/DMA2 on the H743
     * cannot reach DTCM (only MDMA can), so an SPI-DMA flush must read from
     * AXI-SRAM. (Earlier drafts of this comment said DTCM — that was wrong.)
     */
}

void oled_show(void) {
    /* TODO(Step 13.3): kick off a DMA write of the framebuffer to SPI1.
     * Block (or signal-completion) so the menu loop knows when it's safe
     * to mutate the framebuffer again. */
}
