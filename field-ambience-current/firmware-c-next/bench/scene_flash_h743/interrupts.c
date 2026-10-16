#include "bench.h"
#include "scenes_flash_h743_async.h"
#include "stm32h7xx_hal.h"
extern volatile scene_bench_mailbox_t g_scene_bench;
void SysTick_Handler(void) { HAL_IncTick(); }
void FLASH_IRQHandler(void) {
    ++g_scene_bench.flash_irqs;
    scenes_flash_h743_async_flash_irq();
}
/* Unrelated faults remain parked; fault registers stay available over SWD. */
void HardFault_Handler(void) { for (;;) { __NOP(); } }
void MemManage_Handler(void) { for (;;) { __NOP(); } }
void UsageFault_Handler(void) { for (;;) { __NOP(); } }
void NMI_Handler(void) { for (;;) { __NOP(); } }
