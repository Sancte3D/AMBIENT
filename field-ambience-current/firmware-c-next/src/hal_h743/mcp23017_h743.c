/*
 * mcp23017_h743.c — I²C1 transport + MCP23017 + 2× PCA9685 (r18.86,
 * Step 13.3 — the skeleton TODOs are now real HAL code).
 *
 * Bus: I²C1 @ ~396 kHz fast-mode on PB6/PB7 (AF4, open-drain; R4/R5 4.7 k
 * external pull-ups on the PCB). Kernel clock = rcc_pclk1 = 120 MHz.
 * TIMINGR derivation (RM0433 I2C_TIMINGR, t_I2CCLK = 8.33 ns):
 *   PRESC=2  → t_PRESC 25 ns
 *   SCLL=55  → t_LOW  1400 ns (≥ 1300 ns fast-mode spec)
 *   SCLH=28  → t_HIGH  725 ns (≥  600 ns)
 *   SDADEL=2 →  50 ns, SCLDEL=4 → 125 ns (≥ t_SU;DAT 100 ns)
 *   + sync/filter/rise ≈ 400 ns → f_SCL ≈ 396 kHz.
 *   TIMINGR = (2<<28)|(4<<20)|(2<<16)|(28<<8)|55 = 0x20421C37.
 *
 * Devices:
 *   MCP23017 @ 0x20 — 5 cells (GPA0-4) + XSMT out (GPA5) + jack (GPA6) +
 *     VBUS sense (GPA7) + 5 modifiers (GPB0-4) + EN4 push (GPB5).
 *     Register init sequence ported 1:1 from the bench-proven
 *     src/hal_pico/mcp23017_pico.c (byte-for-byte, plus GPB5 in the
 *     pull-up/INT masks — the Pico bench had the volume push on a Pico
 *     GPIO, the H7 board routes it via the expander).
 *   PCA9685 @ 0x40 (U6, 15 status LEDs + backlight) and @ 0x41 (U10,
 *     8 VU LEDs) — one shared init/write path, two addresses. /OE is
 *     hardwired LOW (r18.84); channels boot dark per DS LEDn_FULL_OFF.
 *
 * INTA → PC13 (EXTI15_10, falling). ISR only sets a flag; all I²C happens
 * in the main loop (same discipline as the Pico driver).
 */

#include "mcp23017.h"
#include "h743_hal.h"

/* --- MCP23017 register map, IOCON.BANK = 0 (power-on default) --- */
#define REG_IODIRA   0x00
#define REG_IODIRB   0x01
#define REG_IPOLA    0x02
#define REG_GPINTENA 0x04
#define REG_GPINTENB 0x05
#define REG_INTCONA  0x08
#define REG_INTCONB  0x09
#define REG_IOCON    0x0A
#define REG_GPPUA    0x0C
#define REG_GPPUB    0x0D
#define REG_GPIOA    0x12
#define REG_GPIOB    0x13
#define REG_OLATA    0x14

/* --- PCA9685 registers --- */
#define PCA_MODE1        0x00
#define PCA_MODE2        0x01
#define PCA_LED0_ON_L    0x06
#define PCA_PRESCALE     0xFE
#define PCA_MODE1_SLEEP  0x10
#define PCA_MODE1_AI     0x20
#define PCA_MODE2_OUTDRV 0x04
/* 25 MHz internal osc / (4096 × ~1 kHz) − 1 → 5 (≈1017 Hz, above audible) */
#define PCA_PRESCALE_1KHZ 5

I2C_HandleTypeDef h743_hi2c1;

static volatile bool     s_irq_pending = false;
static volatile uint16_t s_state = 0xFFFF;
static uint8_t           s_olata = 0x00;

/* --- shared I²C helpers (h743_hal.h) --- */

bool h743_i2c_wr(uint8_t addr7, const uint8_t *buf, uint16_t len) {
    return HAL_I2C_Master_Transmit(&h743_hi2c1, (uint16_t)(addr7 << 1),
                                   (uint8_t *)buf, len, 10) == HAL_OK;
}

bool h743_i2c_wr_reg(uint8_t addr7, uint8_t reg, uint8_t val) {
    uint8_t b[2] = { reg, val };
    return h743_i2c_wr(addr7, b, 2);
}

bool h743_i2c_rd_reg(uint8_t addr7, uint8_t reg, uint8_t *out) {
    if (HAL_I2C_Master_Transmit(&h743_hi2c1, (uint16_t)(addr7 << 1),
                                &reg, 1, 10) != HAL_OK) return false;
    return HAL_I2C_Master_Receive(&h743_hi2c1, (uint16_t)(addr7 << 1),
                                  out, 1, 10) == HAL_OK;
}

/* --- MspInit: pins + clocks (called by HAL_I2C_Init) --- */

void HAL_I2C_MspInit(I2C_HandleTypeDef *h) {
    if (h->Instance != I2C1) return;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_6 | GPIO_PIN_7;      /* PB6 SCL, PB7 SDA */
    g.Mode      = GPIO_MODE_AF_OD;
    g.Pull      = GPIO_PULLUP;                  /* belt-and-braces; R4/R5 on PCB */
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &g);
}

/* --- INTA (PC13) EXTI --- */

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_13) s_irq_pending = true;
}

/* --- public API (include/mcp23017.h) --- */

bool mcp_init(void) {
    h743_hi2c1.Instance              = I2C1;
    h743_hi2c1.Init.Timing           = 0x20421C37;   /* derivation above */
    h743_hi2c1.Init.OwnAddress1      = 0;
    h743_hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    h743_hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    h743_hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    h743_hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&h743_hi2c1) != HAL_OK) return false;

    /* Probe: any register read must ACK. */
    uint8_t dummy;
    if (!h743_i2c_rd_reg(MCP_I2C_ADDR, REG_IODIRA, &dummy)) return false;

    /* Ported Pico sequence (see file header). IODIR: 1 = input.
     * Port A: GPA0-4 in, GPA5 out (XSMT), GPA6 in (jack), GPA7 in (VBUS)
     *   → 0b1101_1111. Port B: all inputs. */
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_IODIRA, 0xDF);
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_IODIRB, 0xFF);
    /* Pull-ups: GPA0-4 + GPA6 (jack; GPA7 VBUS is driven by its divider),
     * GPB0-4 modifiers + GPB5 EN4-push (H7 board; Pico bench had none). */
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_GPPUA, 0x5F);
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_GPPUB, 0x3F);
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_IPOLA, 0x00);
    /* INT-on-change vs previous value (INTCON=0), incl. GPB5. */
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_GPINTENA, 0x5F);
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_GPINTENB, 0x3F);
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_INTCONA, 0x00);
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_INTCONB, 0x00);
    /* IOCON: MIRROR=1 → INTA reflects both ports; active-low push-pull. */
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_IOCON, 0x40);
    /* XSMT (GPA5) boots LOW = DAC muted (R_XSMT_PD agrees in hardware). */
    s_olata = 0x00;
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_OLATA, s_olata);

    /* PC13 = INTA input, falling-edge EXTI (R20 10k pull-up on PCB). */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_13;
    g.Mode = GPIO_MODE_IT_FALLING;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &g);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* Prime cached state + drain any pending change. */
    (void)mcp_service();

    /* The LED driver shares the bus — bring it up here so main() has a
     * single "expander + LEDs ready" point, same as the Pico path.
     * (r18.87: U10/pca2 removed — U6 is the only PCA9685.) */
    pca_init();
    return true;
}

bool mcp_irq_pending(void) { return s_irq_pending; }

uint16_t mcp_service(void) {
    s_irq_pending = false;
    uint8_t a = 0xFF, b = 0xFF;
    h743_i2c_rd_reg(MCP_I2C_ADDR, REG_GPIOA, &a);
    h743_i2c_rd_reg(MCP_I2C_ADDR, REG_GPIOB, &b);
    s_state = (uint16_t)(((uint16_t)b << 8) | a);
    return s_state;
}

uint16_t mcp_state(void) { return s_state; }

bool mcp_read_gpio(uint16_t *out) {
    uint8_t a, b;
    if (!h743_i2c_rd_reg(MCP_I2C_ADDR, REG_GPIOA, &a)) return false;
    if (!h743_i2c_rd_reg(MCP_I2C_ADDR, REG_GPIOB, &b)) return false;
    s_state = (uint16_t)(((uint16_t)b << 8) | a);
    *out = s_state;
    return true;
}

void mcp_set_xsmt(bool on) {
    if (on) s_olata |= (uint8_t)(1u << 5);
    else    s_olata &= (uint8_t)~(1u << 5);
    h743_i2c_wr_reg(MCP_I2C_ADDR, REG_OLATA, s_olata);
}

/* --- PCA9685 ×2: shared protocol, two addresses --- */

static void pca_dev_init(uint8_t addr7) {
    h743_i2c_wr_reg(addr7, PCA_MODE1, PCA_MODE1_SLEEP);        /* sleep    */
    h743_i2c_wr_reg(addr7, PCA_PRESCALE, PCA_PRESCALE_1KHZ);   /* ~1 kHz   */
    h743_i2c_wr_reg(addr7, PCA_MODE1, PCA_MODE1_AI);           /* wake+AI  */
    HAL_Delay(1);                                              /* ≥500 µs osc */
    h743_i2c_wr_reg(addr7, PCA_MODE2, PCA_MODE2_OUTDRV);       /* totem    */
    /* /OE is hardwired LOW (r18.84); channels stay dark via the DS
     * LEDn_FULL_OFF power-up default until the first pca*_set_pwm. */
}

static void pca_dev_set_pwm(uint8_t addr7, uint8_t ch,
                            uint16_t on_count, uint16_t off_count) {
    if (ch > 15) return;
    /* Duty 0 → LEDn_FULL_OFF flag (bit 4 of OFF_H): exact dark, no sliver. */
    if (off_count == 0) {
        uint8_t b[5] = { (uint8_t)(PCA_LED0_ON_L + 4u * ch), 0, 0, 0, 0x10 };
        h743_i2c_wr(addr7, b, 5);
        return;
    }
    if (off_count > 4095) off_count = 4095;
    uint8_t b[5] = { (uint8_t)(PCA_LED0_ON_L + 4u * ch),
                     (uint8_t)(on_count & 0xFF),
                     (uint8_t)((on_count >> 8) & 0x0F),
                     (uint8_t)(off_count & 0xFF),
                     (uint8_t)((off_count >> 8) & 0x0F) };
    h743_i2c_wr(addr7, b, 5);
}

void pca_init(void)  { pca_dev_init(0x40); }

void pca_set_pwm(uint8_t channel, uint16_t on_count, uint16_t off_count) {
    pca_dev_set_pwm(0x40, channel, on_count, off_count);
}

