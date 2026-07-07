#ifndef FAM_AUDIO_PROFILER_H
#define FAM_AUDIO_PROFILER_H

#include <stdint.h>

/*
 * audio_profiler — real-time render deadline profiler (P0.1).
 *
 * "Never reason about real-time audio performance when you can measure the
 * deadline." engine_render() runs in the SAI-DMA IRQ; if one render burst
 * overruns the block window the DMA replays the last buffer → audible click.
 * Average CPU is meaningless here — a single outlier is an audio bug.
 *
 * This module is PURE ACCOUNTING — no hardware. The h743 audio ISR feeds it
 * DWT->CYCCNT snapshots (audio_h743.c); host tests feed synthetic cycle
 * values. So the worst-case / deadline-miss logic is fully host-testable,
 * while the actual cycle read stays in the (h743-only) ISR.
 *
 * Deadline budget per render = frames * cpu_hz / sample_rate_hz. Our product
 * gate (REALTIME_AUDIO_RULES.md) wants peak_load well under 1.0 with margin;
 * a miss (load >= 1.0) is a hard defect to chase on real silicon.
 */

typedef struct audio_profiler {
    uint32_t cpu_hz;              /* core clock (e.g. 480 MHz) for the budget */
    uint32_t sample_rate_hz;      /* audio rate (44100) */
    uint32_t budget_cycles;       /* budget for the in-flight render */
    uint32_t begin_cycles;        /* CYCCNT snapshot at begin */
    int      in_flight;           /* re-entrancy guard flag */

    uint32_t last_cycles;         /* elapsed cycles of the most recent render */
    uint32_t max_cycles;          /* worst-case elapsed since reset */
    uint32_t last_frames;
    uint32_t render_count;
    uint32_t deadline_miss_count; /* renders where elapsed >= budget */
    uint32_t reentry_count;       /* begin called while already in flight */
    uint32_t clip_count;          /* int16 samples at +-32767 (from scan) */

    float    last_load;           /* last_cycles / budget (0..1+) */
    float    peak_load;           /* worst load since reset */
} audio_profiler_t;

/* Initialise / clear all counters. Safe to call anytime. */
void audio_profiler_reset(audio_profiler_t *p, uint32_t cpu_hz,
                          uint32_t sample_rate_hz);

/* Mark the start of a render of `frames` frames. `now` = current cycle count.
 * Computes this render's deadline budget. */
void audio_profiler_begin(audio_profiler_t *p, uint32_t now, int frames);

/* Mark the end of the render. `now` = current cycle count. Updates last/max
 * cycles, load, and the deadline-miss counter. Uint32 wrap is handled. */
void audio_profiler_end(audio_profiler_t *p, uint32_t now);

/* Count int16 output samples that hit full scale (+-32767) in a rendered
 * block — a cheap post-limiter clip meter. Adds to clip_count. */
void audio_profiler_scan_clips(audio_profiler_t *p,
                               const int16_t *interleaved, int frames);

#endif /* FAM_AUDIO_PROFILER_H */
