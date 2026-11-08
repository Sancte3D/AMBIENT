#include "bench.h"
#include "scenes_flash_h743_async.h"
#include "stm32h7xx_hal.h"

__attribute__((section(".bench_mailbox"), aligned(32)))
volatile scene_bench_mailbox_t g_scene_bench;
static scene_bench_t s_bench;
static scenes_flash_h743_async_t s_adapter;
extern uint32_t g_pfnVectors[];
extern void BusFault_Handler(void);
extern void FLASH_IRQHandler(void);

static void fail(uint32_t reason) {
    g_scene_bench.magic = BENCH_MAGIC; g_scene_bench.version = BENCH_VERSION;
    g_scene_bench.boot_error = reason; g_scene_bench.state = BENCH_FAILED;
    for (;;) { __NOP(); }
}
int main(void) {
    /* Independent image: retain reset HSI clock, no board PLL/peripherals/audio.
     * The SWD bench mailbox and all CPU-only state live in uncached DTCM. */
    SCB->VTOR = (uint32_t)(uintptr_t)g_pfnVectors;
    __DSB(); __ISB();
    if (HAL_Init() != HAL_OK) fail(1);
    SCB_EnableICache(); SCB_EnableDCache();
    if (g_pfnVectors[5] != ((uint32_t)(uintptr_t)BusFault_Handler | 1u) ||
        g_pfnVectors[16u + FLASH_IRQn] != ((uint32_t)(uintptr_t)FLASH_IRQHandler | 1u)) fail(2);
    uint32_t flash_kib = *(const volatile uint16_t *)(uintptr_t)FLASHSIZE_BASE;
    if (flash_kib != 2048u || (FLASH->OPTCR & FLASH_OPTCR_SWAP_BANK)) fail(3);
    if (!scenes_flash_h743_async_init(&s_adapter)) fail(4);
    scene_journal_io_t journal_io;
    scene_journal_async_io_t async_io;
    if (!scenes_flash_h743_async_make_io(&s_adapter, &journal_io, &async_io) ||
        !scene_bench_init(&s_bench, &g_scene_bench, &journal_io, &async_io)) fail(5);
    g_scene_bench.cpu_hz = SystemCoreClock;
    g_scene_bench.device_id = HAL_GetDEVID();
    g_scene_bench.flash_kib = flash_kib;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    uint32_t previous = HAL_GetTick() - 1u;
    for (;;) {
        uint32_t now = HAL_GetTick();
        if (now != previous) {
            previous = now;
            uint32_t start = DWT->CYCCNT;
            scene_bench_tick(&s_bench, now);
            uint32_t elapsed = DWT->CYCCNT - start;
            if (elapsed > g_scene_bench.max_step_cycles) g_scene_bench.max_step_cycles = elapsed;
        }
        __WFI();
    }
}
