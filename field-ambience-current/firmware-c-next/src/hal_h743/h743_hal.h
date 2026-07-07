#ifndef FAM_H743_HAL_H
#define FAM_H743_HAL_H

/*
 * h743_hal.h — shared internals of the STM32H743 HAL layer (r18.86,
 * Step 13.3). Private to src/hal_h743/; the portable code talks to the
 * public headers in include/ only.
 */

#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/* --- clock tree (system_clock_h743.c) ---------------------------------
 * HSE 8 MHz (Y1) →
 *   PLL1: M=1 (ref 8 MHz), N=120 → VCO 960 MHz, P=2 → SYSCLK 480 MHz,
 *         Q=8 → 120 MHz kernel for SPI1/2/3.
 *         480 MHz requires VOS0 (LDO supply, H743 rev V — the only rev
 *         JLC stocks in 2026; on a rev-Y part drop PLL1N to 100 = 400 MHz).
 *   PLL3: M=2 (ref 4 MHz), N=70 + FRACN=4588/8192, P=25
 *         → 11.289609 MHz SAI1 kernel (+0.8 ppm vs 256×44.1 kHz — far
 *         inside the ±30 ppm crystal tolerance). MCKDIV=1 → FS exactly
 *         44.1 kHz, BCK 2.8224 MHz (64-bit frames).
 *   Buses: AHB 240 MHz (HPRE/2), APB1/2/3/4 120 MHz (PPRE/2).
 *   Flash: LATENCY_4 / WRHIGHFREQ 2 (VOS0, 240 MHz AXI per RM0433 Tab. 17).
 */
void SystemClock_Config(void);

/* Fatal-error trap (blinks nothing yet — parks the CPU so a debugger via
 * J4/SWD lands on a readable call stack). */
void Error_Handler(void);

/* --- shared I²C1 transport (mcp23017_h743.c owns the bus) -------------- */
extern I2C_HandleTypeDef h743_hi2c1;
bool h743_i2c_wr(uint8_t addr7, const uint8_t *buf, uint16_t len);
bool h743_i2c_wr_reg(uint8_t addr7, uint8_t reg, uint8_t val);
bool h743_i2c_rd_reg(uint8_t addr7, uint8_t reg, uint8_t *out);

/* --- handles referenced from stm32h7xx_it.c ---------------------------- */
extern SAI_HandleTypeDef h743_hsai1a;        /* audio_h743.c        */
extern DMA_HandleTypeDef h743_hdma_sai;      /* audio_h743.c        */
extern DMA_HandleTypeDef h743_hdma_spi1_tx;  /* lcd_st7789_h743.c   */

/* --- BAT_SENSE ADC (adc_h743.c): PA3 = ADC12_INP15, 100k/100k divider.
 * bat_adc_read_volts returns VBAT in volts (blocking ~250 us, UI loop
 * only — never the audio path). ------------------------------------------ */
bool  bat_adc_init(void);
float bat_adc_read_volts(void);

/* --- 1 kHz encoder/push service, called from SysTick_Handler ----------- */
void enc_tick_1khz(void);

/* --- EN4 (volume) push arrives via MCP23017 GPB5, not a TIM GPIO; the
 * main loop feeds the debounced level here so enc_pushed(4) stays true
 * to the shared encoders.h contract. ------------------------------------ */
void enc_set_ext_push(uint8_t id, bool pushed);

#endif
