/*
 * stm32h7xx_it.c — interrupt handlers (r18.86, Step 13.3).
 *
 * Names must match vendor/.../startup_stm32h743xx.s vector-table entries.
 * Everything not listed falls through to the startup file's
 * Default_Handler (infinite loop → SWD-readable).
 */

#include "h743_hal.h"

/* ---- Cortex-M7 faults: park readable. ---- */
void NMI_Handler(void)        { for (;;) { } }
void HardFault_Handler(void)  { for (;;) { } }
void MemManage_Handler(void)  { for (;;) { } }
void BusFault_Handler(void)   { for (;;) { } }
void UsageFault_Handler(void) { for (;;) { } }
void SVC_Handler(void)        { }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { }

/* ---- 1 kHz system tick: HAL time base + encoder/push sampling. ---- */
void SysTick_Handler(void) {
    HAL_IncTick();
    enc_tick_1khz();
}

/* ---- SAI1-A TX DMA (DMA1 Stream 0): half/complete refill callbacks
 * land in audio_h743.c via the HAL SAI dispatch. ---- */
void DMA1_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&h743_hdma_sai);
}

/* ---- MCP23017 INTA on PC13 (falling edge, R20 pull-up). The GPIO EXTI
 * callback lives in mcp23017_h743.c. ---- */
void EXTI15_10_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}
