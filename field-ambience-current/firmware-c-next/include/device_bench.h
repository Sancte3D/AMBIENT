#ifndef FAM_DEVICE_BENCH_H
#define FAM_DEVICE_BENCH_H
#include <stdbool.h>
#include <stdint.h>
#define DEVICE_BENCH_MAGIC 0x31424441u /* ADB1 */
#define DEVICE_BENCH_PATTERN 0xa5c39e17u
#define DEVICE_BENCH_GUARD_BYTES 256u
/* Dedicated opt-in bench. Single MSP, no RTOS/PSP: paint before HAL/IRQs.
 * Main owns lifecycle/save fields, SAI IRQ owns error fields. Halted Thread
 * snapshots only. This records evidence, never grants device acceptance. */
typedef struct {
    uint32_t magic,schema,stack_low,stack_high,painted_end,pattern;
    uint32_t audio_start_ms,last_ms,sai_errors,last_sai_error,last_dma_error;
    uint32_t saves_done,saves_failed,save_status,guard_flags,clock_hz;
} device_bench_t;
_Static_assert(sizeof(device_bench_t)==64,"ADB1 ABI");
extern volatile device_bench_t g_device_bench;
bool device_bench_init(uint32_t *low,uint32_t *high,uintptr_t current_sp);
void device_bench_audio_start(uint32_t now,uint32_t cpu_hz);
void device_bench_audio_error(uint32_t sai_error,uint32_t dma_error);
void device_bench_save_begin(void);
void device_bench_poll(uint32_t now,uint32_t save_status);
#endif
