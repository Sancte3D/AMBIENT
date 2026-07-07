/*
 * audio_profiler.c — see audio_profiler.h.
 *
 * Pure C, no hardware, no floating-point in the hot begin/end path except a
 * single division for the load ratio (kept off the per-sample loop — this is
 * once per block). Uint32 subtraction wraps naturally, so the DWT cycle
 * counter rolling over (every ~8.9 s at 480 MHz) is handled without a branch.
 */

#include "audio_profiler.h"

void audio_profiler_reset(audio_profiler_t *p, uint32_t cpu_hz,
                          uint32_t sample_rate_hz) {
    p->cpu_hz              = cpu_hz;
    p->sample_rate_hz      = sample_rate_hz ? sample_rate_hz : 1u;
    p->budget_cycles       = 0;
    p->begin_cycles        = 0;
    p->in_flight           = 0;
    p->last_cycles         = 0;
    p->max_cycles          = 0;
    p->last_frames         = 0;
    p->render_count        = 0;
    p->deadline_miss_count = 0;
    p->reentry_count       = 0;
    p->clip_count          = 0;
    p->last_load           = 0.0f;
    p->peak_load           = 0.0f;
}

void audio_profiler_begin(audio_profiler_t *p, uint32_t now, int frames) {
    /* If a begin arrives while one is already in flight, the render path was
     * re-entered (e.g. an IRQ nesting bug) — flag it, keep the first window. */
    if (p->in_flight) { p->reentry_count++; return; }

    if (frames < 0) frames = 0;
    p->last_frames   = (uint32_t)frames;
    /* budget = frames * cpu_hz / sample_rate. Do the multiply in 64-bit to
     * avoid overflow (frames * cpu_hz can exceed 32 bits). */
    p->budget_cycles = (uint32_t)(((uint64_t)(uint32_t)frames * p->cpu_hz)
                                  / p->sample_rate_hz);
    p->begin_cycles  = now;
    p->in_flight     = 1;
}

void audio_profiler_end(audio_profiler_t *p, uint32_t now) {
    if (!p->in_flight) return;              /* end without a matching begin */
    p->in_flight = 0;

    uint32_t elapsed = now - p->begin_cycles;   /* wrapping subtract */
    p->last_cycles = elapsed;
    if (elapsed > p->max_cycles) p->max_cycles = elapsed;
    p->render_count++;

    if (p->budget_cycles != 0) {
        p->last_load = (float)elapsed / (float)p->budget_cycles;
        if (p->last_load > p->peak_load) p->peak_load = p->last_load;
        if (elapsed >= p->budget_cycles) p->deadline_miss_count++;
    } else {
        p->last_load = 0.0f;
    }
}

void audio_profiler_scan_clips(audio_profiler_t *p,
                               const int16_t *interleaved, int frames) {
    const int n = frames * 2;               /* interleaved stereo */
    for (int i = 0; i < n; ++i) {
        int16_t s = interleaved[i];
        if (s == 32767 || s == -32767 || s == -32768) p->clip_count++;
    }
}
