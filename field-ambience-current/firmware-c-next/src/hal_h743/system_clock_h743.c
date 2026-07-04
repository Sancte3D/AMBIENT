/*
 * system_clock_h743.c — clock tree + fatal trap (r18.86, Step 13.3).
 * Numbers documented in h743_hal.h; sources: RM0433 + DS12110 + SPEC §5.1.
 */

#include "h743_hal.h"

void SystemClock_Config(void) {
    /* LDO supply (SPEC: no SMPS pin on LQFP100 wiring) + VOS0 for 480 MHz.
     * HAL_PWREx_ControlVoltageScaling busy-waits on VOSRDY internally. */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) { }

    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 1;                   /* ref 8 MHz               */
    osc.PLL.PLLN       = 120;                 /* VCO 960 MHz (wide)      */
    osc.PLL.PLLP       = 2;                   /* SYSCLK 480 MHz          */
    osc.PLL.PLLQ       = 8;                   /* 120 MHz → SPI1/2/3 kern */
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

void Error_Handler(void) {
    __disable_irq();
    for (;;) { __NOP(); }     /* park for the SWD debugger (J4) */
}

/* newlib-nano glue: no OS. --specs=nosys.specs provides the rest. */
