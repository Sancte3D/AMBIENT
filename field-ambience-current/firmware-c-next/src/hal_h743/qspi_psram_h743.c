/*
 * qspi_psram_h743.c — APS6404L 8 MB QSPI PSRAM on QUADSPI Bank 2 (ADR-0022).
 *
 * ⚠⚠ BENCH-PENDING — NEVER RUN ON HARDWARE. The board doesn't exist yet. This
 * is a register-level, datasheet-grounded starting point + a self-test hook,
 * NOT a proven driver. Written against the AP Memory APS6404L datasheet
 * Rev. 2.1 command set and the STM32H743 reference manual (RM0433) QUADSPI. On
 * first bring-up EXPECT to tune: the read dummy-cycle count (DCYC), the clock
 * prescaler, and the CS-high time — with a scope + the psram_selftest() below.
 *
 * Pins (ADR-0022 / PINMAP): PB2 = QUADSPI_CLK (AF9), PC11 = BK2_NCS (AF9),
 * PE7-10 = BK2_IO0-3 (AF10). Bank 2 selected via CR.FSEL.
 *
 * APS6404L command set (Rev 2.1): 0x66 reset-enable, 0x99 reset, 0x35 enter
 * QPI, 0xEB fast-read-quad (24-bit addr, 6 dummy), 0x38 quad-write.
 *
 * Memory model: STM32 QUADSPI memory-mapped mode is READ-ONLY, so reads go
 * through the 0x90000000 window (cacheable, MPU-normal) and writes use the
 * indirect path (psram_write). Read-mostly cold data only (ADR-0022).
 */

#include "psram.h"
#include "h743_hal.h"

/* CCR phase-mode encodings */
#define MODE_NONE   0u
#define MODE_1LINE  1u
#define MODE_4LINE  3u
/* CCR functional modes (FMODE) */
#define FMODE_WRITE 0u
#define FMODE_READ  1u
#define FMODE_MMAP  3u

/* APS6404L opcodes */
#define CMD_RESET_EN  0x66u
#define CMD_RESET     0x99u
#define CMD_ENTER_QPI 0x35u
#define CMD_READ_QUAD 0xEBu
#define CMD_WRITE_QUAD 0x38u
#define READ_DUMMY_CYC 6u        /* ⚠ tune on bench for the actual clock */

static bool s_ready = false;

static void gpio_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af) {
    GPIO_InitTypeDef g = {0};
    g.Pin       = pin;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = af;
    HAL_GPIO_Init(port, &g);
}

static bool wait_not_busy(uint32_t spins) {
    while (QUADSPI->SR & QUADSPI_SR_BUSY_Msk) {
        if (spins-- == 0) return false;      /* never hang forever */
    }
    return true;
}

/* Fire a QUADSPI command in indirect mode. Blocks until complete (bounded). */
static bool qspi_cmd(uint8_t instr, uint32_t imode, uint32_t admode,
                     uint32_t address, uint32_t dmode, uint32_t dummy,
                     uint32_t fmode, uint8_t *data, uint32_t len) {
    if (!wait_not_busy(1000000u)) return false;
    QUADSPI->FCR = QUADSPI_FCR_CTCF_Msk;                 /* clear TC flag */
    if (dmode != MODE_NONE && len) QUADSPI->DLR = len - 1;

    uint32_t ccr = ((uint32_t)instr << QUADSPI_CCR_INSTRUCTION_Pos)
                 | (imode  << QUADSPI_CCR_IMODE_Pos)
                 | (admode << QUADSPI_CCR_ADMODE_Pos)
                 | (2u     << QUADSPI_CCR_ADSIZE_Pos)    /* 24-bit address */
                 | (dummy  << QUADSPI_CCR_DCYC_Pos)
                 | (dmode  << QUADSPI_CCR_DMODE_Pos)
                 | (fmode  << QUADSPI_CCR_FMODE_Pos);
    QUADSPI->CCR = ccr;
    if (admode != MODE_NONE) QUADSPI->AR = address;

    if (fmode == FMODE_MMAP) return true;                /* window now live */

    /* indirect data phase */
    if (dmode != MODE_NONE && len) {
        for (uint32_t i = 0; i < len; ++i) {
            if (fmode == FMODE_WRITE) {
                while (!(QUADSPI->SR & QUADSPI_SR_FTF_Msk))
                    if (!(QUADSPI->SR & QUADSPI_SR_BUSY_Msk)) break;
                *(volatile uint8_t *)&QUADSPI->DR = data[i];
            } else {
                uint32_t s = 0;
                while (!(QUADSPI->SR & (QUADSPI_SR_FTF_Msk | QUADSPI_SR_TCF_Msk)))
                    if (s++ > 1000000u) return false;
                data[i] = *(volatile uint8_t *)&QUADSPI->DR;
            }
        }
    }
    if (!wait_not_busy(1000000u)) return false;
    QUADSPI->FCR = QUADSPI_FCR_CTCF_Msk;
    return true;
}

/* MPU region for the mapped window: normal, cacheable write-back write-alloc,
 * non-shareable — the CPU may cache PSRAM reads (no DMA into this region). */
static void mpu_region_psram(void) {
    __DMB();
    MPU->CTRL = 0;                                        /* disable while editing */
    MPU->RNR  = 0;                                        /* region 0 (spare) */
    MPU->RBAR = PSRAM_BASE_ADDR & MPU_RBAR_ADDR_Msk;
    /* SIZE=22 → 8 MB; TEX=001,C=1,B=1 = normal WB-WA; AP=011 full; ENABLE */
    MPU->RASR = (22u << MPU_RASR_SIZE_Pos)
              | (0b011u << MPU_RASR_AP_Pos)
              | (0b001u << MPU_RASR_TEX_Pos)
              | MPU_RASR_C_Msk | MPU_RASR_B_Msk
              | MPU_RASR_ENABLE_Msk;
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    __DSB(); __ISB();
}

bool psram_init(void) {
    s_ready = false;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    gpio_af(GPIOB, GPIO_PIN_2, GPIO_AF9_QUADSPI);        /* CLK */
    gpio_af(GPIOC, GPIO_PIN_11, GPIO_AF9_QUADSPI);       /* BK2_NCS */
    gpio_af(GPIOE, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10,
            GPIO_AF10_QUADSPI);                          /* BK2_IO0-3 */

    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();

    /* CR: prescaler /4 (⚠ tune), FIFO threshold 1, Bank-2 select, enable. */
    QUADSPI->CR = (3u << QUADSPI_CR_PRESCALER_Pos)
                | (0u << QUADSPI_CR_FTHRES_Pos)
                | QUADSPI_CR_FSEL_Msk
                | QUADSPI_CR_EN_Msk;
    /* DCR: FSIZE=22 → 2^23 = 8 MB; CS-high time 1 cycle. */
    QUADSPI->DCR = (22u << QUADSPI_DCR_FSIZE_Pos)
                 | (0u  << QUADSPI_DCR_CSHT_Pos);

    /* APS6404L reset (still in SPI 1-line mode after power-up). */
    if (!qspi_cmd(CMD_RESET_EN, MODE_1LINE, MODE_NONE, 0, MODE_NONE, 0,
                  FMODE_WRITE, 0, 0)) return false;
    if (!qspi_cmd(CMD_RESET, MODE_1LINE, MODE_NONE, 0, MODE_NONE, 0,
                  FMODE_WRITE, 0, 0)) return false;
    /* Enter QPI: all later commands are 4-line. */
    if (!qspi_cmd(CMD_ENTER_QPI, MODE_1LINE, MODE_NONE, 0, MODE_NONE, 0,
                  FMODE_WRITE, 0, 0)) return false;

    /* Arm memory-mapped read (0xEB, 4-4-4, 24-bit addr, dummy cycles). */
    if (!qspi_cmd(CMD_READ_QUAD, MODE_4LINE, MODE_4LINE, 0, MODE_4LINE,
                  READ_DUMMY_CYC, FMODE_MMAP, 0, 0)) return false;

    mpu_region_psram();
    s_ready = true;
    return true;
}

const uint8_t *psram_base(void) { return (const uint8_t *)PSRAM_BASE_ADDR; }
size_t         psram_size(void) { return PSRAM_SIZE_BYTES; }

bool psram_write(uint32_t addr, const void *src, size_t len) {
    if (!s_ready || addr + len > PSRAM_SIZE_BYTES) return false;
    /* Leaving memory-mapped mode requires an abort before an indirect cmd. */
    QUADSPI->CR |= QUADSPI_CR_ABORT_Msk;
    while (QUADSPI->CR & QUADSPI_CR_ABORT_Msk) { }

    bool ok = qspi_cmd(CMD_WRITE_QUAD, MODE_4LINE, MODE_4LINE, addr, MODE_4LINE,
                       0, FMODE_WRITE, (uint8_t *)src, (uint32_t)len);

    /* Re-arm the mapped read window + drop the just-written lines from cache
     * so the next read returns fresh PSRAM contents. */
    qspi_cmd(CMD_READ_QUAD, MODE_4LINE, MODE_4LINE, 0, MODE_4LINE,
             READ_DUMMY_CYC, FMODE_MMAP, 0, 0);
    SCB_InvalidateDCache_by_Addr((void *)(PSRAM_BASE_ADDR + (addr & ~31u)),
                                 (int32_t)(len + 64));
    return ok;
}

bool psram_selftest(void) {
    if (!s_ready) return false;
    static const uint32_t offs[] = { 0, 1024, 0x100000, PSRAM_SIZE_BYTES - 256 };
    uint8_t pat[256];
    for (unsigned t = 0; t < sizeof offs / sizeof offs[0]; ++t) {
        for (int i = 0; i < 256; ++i) pat[i] = (uint8_t)(i ^ (offs[t] >> 4) ^ 0xA5);
        if (!psram_write(offs[t], pat, sizeof pat)) return false;
        const volatile uint8_t *m = psram_base() + offs[t];
        for (int i = 0; i < 256; ++i) if (m[i] != pat[i]) return false;
    }
    return true;
}
