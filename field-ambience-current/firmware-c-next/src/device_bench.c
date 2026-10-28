#include "device_bench.h"
#if !defined(FAM_DEVICE_BENCH_CANDIDATE)
#error "Device observations belong only to the explicit bench candidate"
#endif
volatile device_bench_t g_device_bench;
static uint32_t increment(uint32_t n) {return n==UINT32_MAX ? n : n+1u;}
bool device_bench_init(uint32_t *low,uint32_t *high,uintptr_t current_sp) {
    g_device_bench=(device_bench_t){0};
    uintptr_t bottom=(uintptr_t)low,top=(uintptr_t)high;
    if((bottom&3u) || (top&3u) || top<bottom || top-bottom!=16384u ||
       current_sp>top || current_sp<bottom+2u*DEVICE_BENCH_GUARD_BYTES)return false;
    uintptr_t end=(current_sp-DEVICE_BENCH_GUARD_BYTES)&~(uintptr_t)3;
    /* No calls while painting. Compiler frame is audited below the guard.
     * Current Main/callee frames and the whole unpainted top remain intact. */
    for(uint32_t *p=low;(uintptr_t)p<end;++p)*p=DEVICE_BENCH_PATTERN;
    g_device_bench.schema=1;g_device_bench.stack_low=(uint32_t)bottom;
    g_device_bench.stack_high=(uint32_t)top;g_device_bench.painted_end=(uint32_t)end;
    g_device_bench.pattern=DEVICE_BENCH_PATTERN;g_device_bench.guard_flags=1;
    g_device_bench.magic=DEVICE_BENCH_MAGIC;return true;
}
void device_bench_audio_start(uint32_t now,uint32_t cpu_hz) {
    if(g_device_bench.magic!=DEVICE_BENCH_MAGIC)return;
    g_device_bench.audio_start_ms=g_device_bench.last_ms=now;
    g_device_bench.clock_hz=cpu_hz;g_device_bench.guard_flags|=2u;
}
void device_bench_audio_error(uint32_t sai_error,uint32_t dma_error) {
    if(g_device_bench.magic!=DEVICE_BENCH_MAGIC)return;
    g_device_bench.sai_errors=increment(g_device_bench.sai_errors);
    g_device_bench.last_sai_error=sai_error;g_device_bench.last_dma_error=dma_error;
}
void device_bench_save_begin(void) {
    /* Observe accepted attempts even when two retries fail within their
     * very first Main step and have the same terminal status/timestamp. */
    if(g_device_bench.magic==DEVICE_BENCH_MAGIC)g_device_bench.save_status=1;
}
void device_bench_poll(uint32_t now,uint32_t save_status) {
    if(g_device_bench.magic!=DEVICE_BENCH_MAGIC)return;
    /* Scene enum values are checked against the real API in the fixture. */
    if(save_status!=g_device_bench.save_status) {
        if(save_status==2)g_device_bench.saves_done=increment(g_device_bench.saves_done);
        if(save_status==3)g_device_bench.saves_failed=increment(g_device_bench.saves_failed);
    }
    g_device_bench.save_status=save_status;g_device_bench.last_ms=now;
}
