/*
 * lcd_st7789_h743.c — STM32H743 SPI1 driver for the ST7789 LCD (r18.86,
 * Step 13.3 — real driver; same lcd_st7789.h contract as the Pico driver).
 *
 * Pin map (DS12110 + generator NETS, r18.83-verified):
 *   PA5 (pin 29) = SPI1_SCK   (AF5) → LCD SCK
 *   PA7 (pin 31) = SPI1_MOSI  (AF5) → LCD MOSI (SDA)
 *   PA6 (pin 30) = GPIO out   → LCD CS   (idle HIGH)
 *   PC4 (pin 32) = GPIO out   → LCD DC   (data/command)
 *   PC5 (pin 33) = GPIO out   → LCD RES  (idle HIGH)
 *   Backlight    = PCA9685 ch15 (LCD_BLK_PWM gates Q2 2N7002) — set over
 *                  I²C in the board layer, not here (mirrors the Pico split).
 *
 * SPI clock: SPI1 kernel ← pll1_q_ck = 120 MHz (D2CCIP1R SPI123SEL reset
 * default; PLL1Q=8 in system_clock_h743.c). Prescaler /4 → 30 MHz SCK.
 * The identical panel runs bench-proven at 32 MHz on the Pico driver, so
 * 30 MHz stays inside proven territory. Full frame = 320×170×2 B ≈ 29 ms.
 *
 * Transfers are blocking CPU writes (no DMA): audio refill runs in the
 * DMA1 IRQ at priority 5, so a blocked main loop cannot glitch audio, and
 * blocking keeps the oled_show() contract identical to the Pico driver
 * (safe to touch the framebuffer as soon as it returns). No cache
 * maintenance needed either — the CPU reads the framebuffer itself.
 */

#include "oled.h"
#include "oled_color.h"          /* shared accent-tinted grey→RGB565 LUT */
#include "h743_hal.h"

/* Landscape orientation — same value as the bench-proven Pico driver.
 * MADCTL 0x60 = MX | MV (row/col exchange). Flip to 0xA0/0xC0/0x00 if the
 * bench shows it mirrored — purely cosmetic. */
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

static SPI_HandleTypeDef s_hspi1;

/* --- low-level CS/DC-managed command + bulk data --- */

static inline void lcd_cs(bool low) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, low ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
static inline void lcd_dc(bool data) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, data ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void spi_tx(const uint8_t *buf, uint16_t n) {
    if (HAL_SPI_Transmit(&s_hspi1, (uint8_t *)buf, n, 100) != HAL_OK)
        Error_Handler();
}

static void cmd(uint8_t c) {
    lcd_dc(false);
    lcd_cs(true);
    spi_tx(&c, 1);
    lcd_cs(false);
}

static void data(const uint8_t *buf, uint16_t n) {
    lcd_dc(true);
    lcd_cs(true);
    spi_tx(buf, n);
    lcd_cs(false);
}

static void data_one(uint8_t b) { data(&b, 1); }

static void reset_panel(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(120);
}

/* Same byte stream + timing as the Pico driver — the panel doesn't know
 * which MCU is talking. */
static void run_init_sequence(void) {
    cmd(ST_SWRESET);            HAL_Delay(150);
    cmd(ST_SLPOUT);             HAL_Delay(120);
    cmd(ST_COLMOD);  data_one(0x55);   /* 16-bit/pixel RGB565 */
    cmd(ST_MADCTL);  data_one(LCD_MADCTL);
    cmd(ST_INVON);                     /* IPS panels are inverted */
    cmd(ST_NORON);   HAL_Delay(10);
    oled_fill(0);                      /* dark framebuffer */
    oled_show();                       /* clear GRAM before turning on */
    cmd(ST_DISPON);  HAL_Delay(20);
}

/* --- MspInit: pins + clocks (HAL_SPI_Init dispatches here) --- */
void HAL_SPI_MspInit(SPI_HandleTypeDef *h) {
    if (h->Instance != SPI1) return;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_5 | GPIO_PIN_7;      /* SCK, MOSI */
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;         /* 30 MHz edges */
    g.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &g);
}

void oled_init(void) {
    /* Control GPIOs first, in their idle states (CS high, DC low, RES high)
     * so no glitch reaches the panel while SPI1 comes up. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);      /* CS  */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);    /* DC  */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);      /* RES */

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Pin   = GPIO_PIN_6;                                    /* PA6 CS  */
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin   = GPIO_PIN_4 | GPIO_PIN_5;                       /* PC4/PC5 */
    HAL_GPIO_Init(GPIOC, &g);

    /* SPI1: master TX-only, mode 0, MSB first, 8 bit, 120 MHz/4 = 30 MHz. */
    s_hspi1.Instance                        = SPI1;
    s_hspi1.Init.Mode                       = SPI_MODE_MASTER;
    s_hspi1.Init.Direction                  = SPI_DIRECTION_2LINES_TXONLY;
    s_hspi1.Init.DataSize                   = SPI_DATASIZE_8BIT;
    s_hspi1.Init.CLKPolarity                = SPI_POLARITY_LOW;
    s_hspi1.Init.CLKPhase                   = SPI_PHASE_1EDGE;
    s_hspi1.Init.NSS                        = SPI_NSS_SOFT;
    s_hspi1.Init.BaudRatePrescaler          = SPI_BAUDRATEPRESCALER_4;
    s_hspi1.Init.FirstBit                   = SPI_FIRSTBIT_MSB;
    s_hspi1.Init.TIMode                     = SPI_TIMODE_DISABLE;
    s_hspi1.Init.CRCCalculation             = SPI_CRCCALCULATION_DISABLE;
    s_hspi1.Init.CRCPolynomial              = 7;
    s_hspi1.Init.NSSPMode                   = SPI_NSS_PULSE_DISABLE;
    s_hspi1.Init.FifoThreshold              = SPI_FIFO_THRESHOLD_01DATA;
    /* Keep SCK/MOSI driven at idle between transfers — floating lines
     * between HAL calls would show as sparkle on the panel. */
    s_hspi1.Init.MasterKeepIOState          = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    s_hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    s_hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    s_hspi1.Init.MasterSSIdleness           = SPI_MASTER_SS_IDLENESS_00CYCLE;
    s_hspi1.Init.MasterInterDataIdleness    = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    s_hspi1.Init.MasterReceiverAutoSusp     = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    s_hspi1.Init.IOSwap                     = SPI_IO_SWAP_DISABLE;
    if (HAL_SPI_Init(&s_hspi1) != HAL_OK) Error_Handler();

    reset_panel();
    run_init_sequence();
}

/* Address-window the full visible area, then stream the framebuffer converting
 * each 4-bit grey pixel to RGB565 a row at a time (640 B per row → 170 rows).
 * Same loop as the Pico driver; only the SPI primitive differs. */
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
        spi_tx(line, (uint16_t)sizeof line);
    }
    lcd_cs(false);
}
