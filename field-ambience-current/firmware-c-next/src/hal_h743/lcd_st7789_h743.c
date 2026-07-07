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

/* SPI1-TX DMA (async framebuffer flush). Global so the DMA2_Stream3 IRQ in
 * stm32h7xx_it.c can dispatch into it. */
DMA_HandleTypeDef h743_hdma_spi1_tx;

/* Async flush state (row-pipelined): two line buffers ping-pong — DMA streams
 * one row while the CPU converts the next. 32-byte aligned in D1 (.bss) so the
 * D-cache clean before each DMA hits exact lines. `s_flush_active` is the
 * busy flag; the ISR walks s_flush_row 0..OLED_HEIGHT. */
__attribute__((aligned(32))) static uint8_t s_line[2][OLED_WIDTH * 2];
static volatile int s_flush_active = 0;
static volatile int s_flush_row    = 0;
static int          s_flush_buf    = 0;
static const uint16_t *s_flush_lut = 0;

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

    /* SPI1-TX DMA for the async framebuffer flush: DMA2 Stream 3, request
     * SPI1_TX, memory→peripheral, 8-bit, normal (one row per kick). Priority
     * below audio's DMA (medium vs high) so an audio refill always wins. */
    __HAL_RCC_DMA2_CLK_ENABLE();
    h743_hdma_spi1_tx.Instance                 = DMA2_Stream3;
    h743_hdma_spi1_tx.Init.Request             = DMA_REQUEST_SPI1_TX;
    h743_hdma_spi1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    h743_hdma_spi1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    h743_hdma_spi1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    h743_hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    h743_hdma_spi1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    h743_hdma_spi1_tx.Init.Mode                = DMA_NORMAL;
    h743_hdma_spi1_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    h743_hdma_spi1_tx.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    h743_hdma_spi1_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
    h743_hdma_spi1_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
    h743_hdma_spi1_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&h743_hdma_spi1_tx) != HAL_OK) Error_Handler();
    __HAL_LINKDMA(h, hdmatx, h743_hdma_spi1_tx);

    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
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

/* Address-window the full visible area and issue RAMWR (leaves CS high). */
static void set_full_window(void) {
    uint16_t xs = OLED_LCD_X_OFFSET, xe = OLED_LCD_X_OFFSET + OLED_WIDTH  - 1;
    uint16_t ys = OLED_LCD_Y_OFFSET, ye = OLED_LCD_Y_OFFSET + OLED_HEIGHT - 1;
    cmd(ST_CASET); { const uint8_t d[4] = { (uint8_t)(xs >> 8), (uint8_t)xs,
                                            (uint8_t)(xe >> 8), (uint8_t)xe }; data(d, 4); }
    cmd(ST_RASET); { const uint8_t d[4] = { (uint8_t)(ys >> 8), (uint8_t)ys,
                                            (uint8_t)(ye >> 8), (uint8_t)ye }; data(d, 4); }
    cmd(ST_RAMWR);
}

/* Blocking full-frame flush (the proven path — used for init/clear-GRAM and as
 * the async fallback). Converts + streams a row at a time via the shared
 * oled_convert_row(). */
void oled_show(void) {
    /* If an async flush is mid-flight, let it finish so we don't interleave
     * two RAMWR streams on the bus. */
    while (s_flush_active) { /* spin — completes in the DMA ISR */ }

    set_full_window();
    const uint16_t *lut = oled_grey565_lut();   /* accent-tinted, shared */
    lcd_dc(true);
    lcd_cs(true);
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        oled_convert_row(y, lut, s_line[0]);
        spi_tx(s_line[0], (uint16_t)sizeof s_line[0]);
    }
    lcd_cs(false);
}

/* --- Async SPI-DMA flush: free the main loop during the ~29 ms panel write. --
 * set_full_window() (small blocking cmds), then hold CS low / DC high and DMA
 * the pixel rows back-to-back. Each TxCplt fires the NEXT row's conversion +
 * DMA from the ISR (DMA2_Stream3, priority 6 — below audio's 5). The caller
 * must not touch the framebuffer while oled_flush_busy() is true. */
int oled_flush_busy(void) { return s_flush_active; }

static void kick_row_dma(int buf) {
    /* CPU wrote the line through the D-cache — clean it so the DMA reads the
     * fresh bytes from SRAM (same discipline as the audio TX buffer). */
    SCB_CleanDCache_by_Addr((uint32_t *)s_line[buf], (int32_t)sizeof s_line[buf]);
    if (HAL_SPI_Transmit_DMA(&s_hspi1, s_line[buf],
                             (uint16_t)sizeof s_line[buf]) != HAL_OK)
        Error_Handler();
}

void oled_show_async(void) {
    if (s_flush_active) return;                 /* one flush in flight — skip */

    s_flush_lut = oled_grey565_lut();
    set_full_window();
    lcd_dc(true);
    lcd_cs(true);

    s_flush_active = 1;
    s_flush_buf    = 0;
    oled_convert_row(0, s_flush_lut, s_line[0]);
    s_flush_row    = 1;                          /* next row to convert */
    kick_row_dma(0);
}

/* SPI1 TX-DMA complete: emitted from HAL via the DMA2_Stream3 IRQ. Advance the
 * row pipeline; on the last row raise CS and clear the busy flag. */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *h) {
    if (h->Instance != SPI1 || !s_flush_active) return;

    if (s_flush_row >= OLED_HEIGHT) {           /* last row shifted out */
        lcd_cs(false);
        s_flush_active = 0;
        return;
    }
    int nb = s_flush_buf ^ 1;                    /* convert into the idle buffer */
    oled_convert_row(s_flush_row, s_flush_lut, s_line[nb]);
    s_flush_buf = nb;
    s_flush_row++;
    kick_row_dma(nb);
}
