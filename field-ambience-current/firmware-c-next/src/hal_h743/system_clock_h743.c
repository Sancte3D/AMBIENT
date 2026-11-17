/*
 * system_clock_h743.c — clock tree + fatal trap (r18.86, Step 13.3).
 * Numbers documented in h743_hal.h; sources: RM0433 + DS12110 + SPEC §5.1.
 */

#include "h743_hal.h"

/* Silicon revision Y (DBGMCU_IDCODE.REV_ID 0x1003) is limited to 400 MHz and
 * has no VOS0. Revision V (0x2003) and later run 480 MHz at VOS0. Which
 * revision the assembler delivers is not known yet, so the firmware decides
 * at boot instead of overclocking an older chip. */
bool h743_is_rev_y(void) { return (DBGMCU->IDCODE >> 16) == 0x1003u; }

void SystemClock_Config(void) {
    const bool rev_y = h743_is_rev_y();
    /* LDO supply (SPEC: no SMPS pin on LQFP100 wiring). */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) { }
    if (!rev_y) {
        /* VOS0 sets the overdrive bit SYSCFG->PWRCR.ODEN. SYSCFG is unclocked
         * after reset (RCC_APB4ENR.SYSCFGEN = 0), so without this enable the
         * write is silently lost and the core would run 480 MHz at VOS1,
         * whose limit is 400 MHz (DS12110). Same order as ST's reference. */
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
        while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) { }
    }

    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 1;                   /* ref 8 MHz               */
    osc.PLL.PLLN       = rev_y ? 100 : 120;   /* VCO 800 / 960 MHz (wide) */
    osc.PLL.PLLP       = 2;                   /* SYSCLK 400 / 480 MHz    */
    osc.PLL.PLLQ       = 8;                   /* 100 / 120 MHz → SPI kern */
    osc.PLL.PLLR       = 2;
    osc.PLL.PLLRGE     = RCC_PLL1VCIRANGE_3;  /* ref 8..16 MHz           */
    osc.PLL.PLLVCOSEL  = RCC_PLL1VCOWIDE;     /* 192..960 MHz            */
    osc.PLL.PLLFRACN   = 0;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.SYSCLKDivider  = RCC_SYSCLK_DIV1;     /* CPU 480 MHz  */
    clk.AHBCLKDivider  = RCC_HCLK_DIV2;       /* AHB 240 MHz  */
    clk.APB3CLKDivider = RCC_APB3_DIV2;       /* 120 MHz      */
    clk.APB1CLKDivider = RCC_APB1_DIV2;       /* 120 MHz      */
    clk.APB2CLKDivider = RCC_APB2_DIV2;       /* 120 MHz      */
    clk.APB4CLKDivider = RCC_APB4_DIV2;       /* 120 MHz      */
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) Error_Handler();

    /* PLL3 → SAI1 kernel 11.289609 MHz (fractional; math in h743_hal.h).
     * ADC kernel ← CLKP(HSE 8 MHz): BAT_SENSE at 1 Hz needs no speed. */
    RCC_PeriphCLKInitTypeDef pclk = {0};
    pclk.PeriphClockSelection = RCC_PERIPHCLK_SAI1 | RCC_PERIPHCLK_ADC |
                                RCC_PERIPHCLK_CKPER;
    pclk.PLL3.PLL3M = 2;                      /* ref 4 MHz                */
    pclk.PLL3.PLL3N = 70;
    pclk.PLL3.PLL3FRACN = 4588;               /* +0.560059 → ×4/25        */
    pclk.PLL3.PLL3P = 25;                     /* 11.289609 MHz            */
    pclk.PLL3.PLL3Q = 25;
    pclk.PLL3.PLL3R = 25;
    pclk.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;   /* ref 4..8 MHz             */
    pclk.PLL3.PLL3VCOSEL = RCC_PLL3VCOMEDIUM; /* VCO 282.24 MHz (150-420) */
    pclk.Sai1ClockSelection   = RCC_SAI1CLKSOURCE_PLL3;
    pclk.CkperClockSelection  = RCC_CLKPSOURCE_HSE;
    pclk.AdcClockSelection    = RCC_ADCCLKSOURCE_CLKP;
    if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) Error_Handler();
}

/* Fatal path. The audio DMA is circular: if the CPU just parked, the DMA
 * would replay the last ~23 ms buffer forever through the amplifier, a loud
 * loop nobody can stop without pulling power. So: pull AMP_SHDN_N (PB14) and
 * AMP_MUTE_N (PB15) low and stop DMA1 Stream 0 with plain register writes
 * (no HAL; safe in any fault context; a write to an unclocked peripheral is
 * ignored). With a debugger attached, park for inspection (SWD, J4);
 * otherwise reset so the instrument comes back with its normal fade-in. */
void h743_fatal_stop(void) {
    __disable_irq();
    GPIOB->BSRR = (uint32_t)(GPIO_PIN_14 | GPIO_PIN_15) << 16;
    DMA1_Stream0->CR &= ~DMA_SxCR_EN;
    SAI1_Block_A->CR1 &= ~SAI_xCR1_SAIEN;
    __DSB();
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        for (;;) { __NOP(); }
    }
    NVIC_SystemReset();
}

void Error_Handler(void) { h743_fatal_stop(); }

/* MPU background region (ST AN4838/AN4861): the Cortex-M7 may read
 * speculatively from any "Normal" region of the default memory map, including
 * the FMC and QUADSPI windows (0x60000000-0xDFFFFFFF) while those peripherals
 * are unconfigured, which ST documents as a cause of bus stalls. Region 0
 * covers 4 GB as no-access, execute-never and strongly ordered; subregions
 * 0, 1, 2 and 7 (code/flash/TCM, SRAM, peripherals, system) are disabled so
 * they keep the default map. The PSRAM self-test opens its window as region 1.
 * Call before the caches are enabled. */
void h743_mpu_init(void) {
    __DMB();
    MPU->CTRL = 0;
    MPU->RNR  = 0;
    MPU->RBAR = 0;
    MPU->RASR = (31u << MPU_RASR_SIZE_Pos)       /* 2^(31+1) = 4 GB */
              | (0x87u << MPU_RASR_SRD_Pos)      /* keep 0..1.5 GB and 3.5..4 GB default */
              | (0u << MPU_RASR_AP_Pos)          /* no access */
              | MPU_RASR_XN_Msk                  /* TEX=0 C=0 B=0: strongly ordered */
              | MPU_RASR_ENABLE_Msk;
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    __DSB(); __ISB();
}

/* Independent watchdog: a hang in the main loop (stuck bus, endless wait)
 * restarts the instrument after about 8 s instead of leaving it deaf to every
 * control until the power is pulled. LSI ~32 kHz / 256 = 125 Hz, reload 1000.
 * 8 s also covers the longest blocking step in the loop, a 128 KB Flash sector
 * erase for a Scene save (fed right before it). Frozen while a debugger halts
 * the core. Once started it cannot be stopped; start it only for normal play,
 * never in the bench/service modes, which block for longer. */
void h743_watchdog_start(void) {
    DBGMCU->APB4FZ1 |= DBGMCU_APB4FZ1_DBG_IWDG1;
    IWDG1->KR  = 0xCCCCu;                        /* start (also starts LSI) */
    IWDG1->KR  = 0x5555u;                        /* unlock PR/RLR */
    IWDG1->PR  = 6u;                             /* /256 */
    IWDG1->RLR = 1000u;                          /* 8.0 s at 32 kHz */
    for (uint32_t spins = 0; IWDG1->SR != 0u && spins < 2000000u; ++spins) { }
    IWDG1->KR  = 0xAAAAu;                        /* reload */
}
void h743_watchdog_feed(void) { IWDG1->KR = 0xAAAAu; }

/* Supply monitor. Without a battery every power-off is a slow sag of the
 * 3.3 V rail. Below 2.7 V the x64 Flash program/erase used for Scenes is out
 * of specification (RM0433) and the amplifier pops on the way down. The PVD
 * at 2.7 V therefore takes the fatal path at once: amplifier muted, audio
 * stopped, then reset (the restart waits for a good supply via the BOR/POR).
 * A supply that is already low at boot only blocks Scene writes. */
static volatile bool s_supply_low;
bool h743_supply_ok(void) { return !s_supply_low && !__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO); }

void h743_supply_monitor_start(void) {
    PWR_PVDTypeDef pvd = { .PVDLevel = PWR_PVDLEVEL_5, .Mode = PWR_PVD_MODE_IT_RISING_FALLING };
    HAL_PWR_ConfigPVD(&pvd);
    HAL_PWR_EnablePVD();
    for (volatile uint32_t i = 0; i < 10000u; ++i) { }   /* PVD settling */
    s_supply_low = __HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) != 0u;
    HAL_NVIC_SetPriority(PVD_AVD_IRQn, 0, 0);            /* above audio */
    HAL_NVIC_EnableIRQ(PVD_AVD_IRQn);
}

void HAL_PWR_PVDCallback(void) {
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO)) h743_fatal_stop();   /* VDD below 2.7 V */
}

/* newlib-nano glue: no OS. --specs=nosys.specs provides the rest. */
