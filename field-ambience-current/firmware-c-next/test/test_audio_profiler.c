/*
 * test_audio_profiler.c — host tests for the real-time render profiler.
 * Feeds synthetic cycle snapshots so the deadline/worst-case/miss logic is
 * exercised without any hardware.
 */
#include "audio_profiler.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>

#define CPU_HZ  480000000u
#define SR      44100u

static int checks = 0;
#define CHECK(c) do { assert(c); ++checks; } while (0)

/* budget for `frames` at 480 MHz / 44.1 kHz */
static uint32_t budget(int frames) {
    return (uint32_t)(((uint64_t)(uint32_t)frames * CPU_HZ) / SR);
}

int main(void) {
    audio_profiler_t p;
    audio_profiler_reset(&p, CPU_HZ, SR);

    /* budget @ 512 frames ~= 5.57 M cycles (11.6 ms) */
    uint32_t b512 = budget(512);
    printf("  budget(512) = %u cycles (~%.2f ms)\n",
           b512, 1000.0 * b512 / CPU_HZ);
    CHECK(b512 > 5500000u && b512 < 5600000u);

    /* 1) A render well under budget = no miss, load < 1 */
    audio_profiler_begin(&p, 1000u, 512);
    audio_profiler_end(&p, 1000u + 1000000u);          /* 1.0 M cycles */
    CHECK(p.render_count == 1);
    CHECK(p.last_cycles == 1000000u);
    CHECK(p.max_cycles == 1000000u);
    CHECK(p.deadline_miss_count == 0);
    CHECK(p.last_load > 0.17f && p.last_load < 0.19f);  /* ~0.18 */
    printf("  under-budget load = %.3f, misses = %u\n",
           p.last_load, p.deadline_miss_count);

    /* 2) A worse (but still safe) render raises max/peak but not misses */
    audio_profiler_begin(&p, 5000u, 512);
    audio_profiler_end(&p, 5000u + 3000000u);          /* 3.0 M cycles */
    CHECK(p.max_cycles == 3000000u);
    CHECK(p.peak_load > 0.53f && p.peak_load < 0.55f);
    CHECK(p.deadline_miss_count == 0);

    /* 3) An overrun (> budget) counts a deadline miss */
    audio_profiler_begin(&p, 9000u, 512);
    audio_profiler_end(&p, 9000u + b512 + 50000u);     /* over by 50k */
    CHECK(p.deadline_miss_count == 1);
    CHECK(p.last_load > 1.0f);
    printf("  overrun load = %.3f, misses = %u\n",
           p.last_load, p.deadline_miss_count);

    /* 4) Uint32 CYCCNT wrap-around must still measure the true elapsed */
    audio_profiler_reset(&p, CPU_HZ, SR);
    uint32_t near_top = 0xFFFFFFFFu - 500u;            /* begins near wrap */
    audio_profiler_begin(&p, near_top, 128);
    audio_profiler_end(&p, near_top + 800000u);        /* wraps past 0 */
    CHECK(p.last_cycles == 800000u);                   /* wrap handled */
    printf("  wrap-around elapsed = %u (expected 800000)\n", p.last_cycles);

    /* 5) Re-entrancy guard: a second begin without end is flagged, window kept */
    audio_profiler_reset(&p, CPU_HZ, SR);
    audio_profiler_begin(&p, 100u, 256);
    audio_profiler_begin(&p, 200u, 256);               /* re-entry */
    CHECK(p.reentry_count == 1);
    audio_profiler_end(&p, 100u + 400000u);            /* measures from 1st */
    CHECK(p.last_cycles == 400000u);

    /* 6) end without begin is a no-op (no underflow, no phantom render) */
    audio_profiler_reset(&p, CPU_HZ, SR);
    audio_profiler_end(&p, 12345u);
    CHECK(p.render_count == 0);
    CHECK(p.max_cycles == 0);

    /* 7) clip meter counts full-scale int16 samples (both rails) */
    audio_profiler_reset(&p, CPU_HZ, SR);
    int16_t blk[8] = { 0, 0,  32767, -32768,  100, -100,  -32767, 5 };
    audio_profiler_scan_clips(&p, blk, 4);             /* 4 stereo frames */
    CHECK(p.clip_count == 3);                           /* 32767, -32768, -32767 */
    printf("  clip meter = %u (expected 3)\n", p.clip_count);

    printf("audio_profiler: %d checks, 0 failures\n", checks);
    return 0;
}
