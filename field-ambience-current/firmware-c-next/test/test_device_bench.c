#include "device_bench.h"
#include "scenes_save_async.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t arena[4096+16],before[4096+16];
#ifdef DEVICE_BENCH_ROUTING_FIXTURE
typedef struct {uint32_t ErrorCode;} DMA_HandleTypeDef;
typedef struct {uint32_t ErrorCode;DMA_HandleTypeDef *hdmatx;} SAI_HandleTypeDef;
static SAI_HandleTypeDef h743_hsai1a;
#include "device_bench_callback.inc"
#endif
int main(void) {
    _Static_assert(SCENES_SAVE_DONE==2 && SCENES_SAVE_FAILED==3,"Bench enum contract");
    uint32_t *low=arena+8,*high=low+4096;uintptr_t sp=(uintptr_t)(high-16);
    memset(arena,0x37,sizeof arena);memcpy(before,arena,sizeof arena);
    assert(!device_bench_init(low,high-1,sp) && !memcmp(arena,before,sizeof arena));
    assert(!device_bench_init(low,high,(uintptr_t)low+64) && !memcmp(arena,before,sizeof arena));
    assert(!device_bench_init(low,high,(uintptr_t)high+4) && !memcmp(arena,before,sizeof arena));
    assert(device_bench_init(low,high,sp));
    unsigned painted=(unsigned)((sp-DEVICE_BENCH_GUARD_BYTES-(uintptr_t)low)/4);
    for(unsigned i=0;i<painted;++i)assert(low[i]==DEVICE_BENCH_PATTERN);
    assert(!memcmp(low+painted,before+8+painted,(4096-painted)*4));
    assert(!memcmp(arena,before,32) && !memcmp(high,before+4104,32));
    assert(g_device_bench.magic==DEVICE_BENCH_MAGIC && g_device_bench.guard_flags==1);
    device_bench_audio_start(UINT32_MAX-20,480000000);
    device_bench_poll(20,SCENES_SAVE_BUSY);device_bench_poll(21,SCENES_SAVE_DONE);
    device_bench_poll(22,SCENES_SAVE_DONE);assert(g_device_bench.saves_done==1);
    device_bench_poll(23,SCENES_SAVE_BUSY);device_bench_poll(24,SCENES_SAVE_FAILED);
    device_bench_audio_error(3,5);device_bench_audio_error(7,9);
    assert(g_device_bench.saves_failed==1 && g_device_bench.last_ms==24 &&
        g_device_bench.sai_errors==2 && g_device_bench.last_sai_error==7 && g_device_bench.last_dma_error==9);
    g_device_bench.sai_errors=UINT32_MAX;device_bench_audio_error(1,2);
    assert(g_device_bench.sai_errors==UINT32_MAX && g_device_bench.guard_flags==3);
    device_bench_save_begin();device_bench_poll(24,SCENES_SAVE_FAILED);
    device_bench_save_begin();device_bench_poll(24,SCENES_SAVE_FAILED);
    assert(g_device_bench.saves_failed==3);
#ifdef DEVICE_BENCH_ROUTING_FIXTURE
    g_device_bench.sai_errors=0;SAI_HandleTypeDef other={.ErrorCode=99};
    HAL_SAI_ErrorCallback(&other);assert(g_device_bench.sai_errors==0);
    DMA_HandleTypeDef dma={.ErrorCode=13};h743_hsai1a=(SAI_HandleTypeDef){.ErrorCode=11,.hdmatx=&dma};
    HAL_SAI_ErrorCallback(&h743_hsai1a);assert(g_device_bench.sai_errors==1 && g_device_bench.last_dma_error==13);
    h743_hsai1a.hdmatx=NULL;HAL_SAI_ErrorCallback(&h743_hsai1a);
    assert(g_device_bench.sai_errors==2 && g_device_bench.last_sai_error==11 && g_device_bench.last_dma_error==0);
#endif
    puts("Device bench: guarded startup stack paint, invalid bounds, real Scene status transitions, sticky SAI/DMA errors and saturated counters pass");
}
