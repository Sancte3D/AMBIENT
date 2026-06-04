/*
 * Host-side tests for the Step-11 reverb + engine mix-bus.
 *
 * Pure C — no Pico SDK. We stub AUDIO_BUFFER_FRAMES via -D so engine.c
 * compiles without dragging in audio.c's Pico-SDK dependencies.
 *
 * Build via run_tests.sh, or directly:
 *   cc -std=c11 -I../include -DAUDIO_HEADER_STUB \
 *      test_reverb_engine.c ../src/dsp.c ../src/pad.c \
 *      ../src/reverb.c ../src/engine.c -lm -o /tmp/r_test
 *
 * Exit 0 = all pass.
 */

#include "dsp.h"
#include "reverb.h"
#include "engine.h"
#include "pad.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_checks = 0;
static int g_fails  = 0;

#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        ++g_checks;                                                        \
        if (!(cond)) {                                                     \
            ++g_fails;                                                     \
            fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__);           \
            fprintf(stderr, __VA_ARGS__);                                  \
            fprintf(stderr, "\n");                                         \
        }                                                                  \
    } while (0)

/* ----------------------------------------------------------------- reverb */

#define SR  DSP_SAMPLE_RATE_HZ

static void render_silence_reverb(int frames) {
    enum { N = 256 };
    float in[N] = {0}, out[N], outR[N];
    int left = frames;
    while (left > 0) {
        int n = left < N ? left : N;
        reverb_render(in, in, out, outR, n);
        left -= n;
    }
}

static void test_reverb_impulse_decays(void) {
    reverb_init();
    reverb_set(0.7f, 0.3f);
    reverb_set_drive(0.0f);                  /* drive off → linear */
    /* Let coefficients settle (per-block smoothing). */
    render_silence_reverb(SR);

    /* Big impulse, then 5 s of silence. Peak during the first 4 s must be
     * bounded; the residual after 5 s must be small (decay). */
    enum { N = 256 };
    float in[N], inR[N], out[N], outR[N];
    memset(in, 0, sizeof in); memset(inR, 0, sizeof inR);
    in[0] = 1.0f; inR[0] = 1.0f;
    reverb_render(in, inR, out, outR, N);

    float peak_early = 0.0f;
    for (int i = 0; i < N; ++i) {
        float a = fmaxf(fabsf(out[i]), fabsf(outR[i]));
        if (a > peak_early) peak_early = a;
    }

    /* Run 5 s of silent input, capture the tail level. */
    memset(in, 0, sizeof in); memset(inR, 0, sizeof inR);
    float tail_peak_last_second = 0.0f, anywhere_peak = peak_early;
    int total_frames = 5 * SR;
    int rendered = 0;
    while (rendered < total_frames) {
        int n = (total_frames - rendered) < N ? (total_frames - rendered) : N;
        reverb_render(in, inR, out, outR, n);
        for (int i = 0; i < n; ++i) {
            float a = fmaxf(fabsf(out[i]), fabsf(outR[i]));
            if (a > anywhere_peak) anywhere_peak = a;
            if (rendered + i >= 4 * SR && a > tail_peak_last_second)
                tail_peak_last_second = a;
        }
        rendered += n;
    }
    printf("  reverb impulse peak (any) = %.4f   tail @4-5s = %.6f\n",
           anywhere_peak, tail_peak_last_second);
    CHECK(anywhere_peak < 2.0f, "reverb diverged: peak=%g", anywhere_peak);
    CHECK(tail_peak_last_second < 0.05f,
          "reverb did not decay (4-5s peak too high): %g", tail_peak_last_second);
}

static void test_reverb_silent_in_silent_out(void) {
    reverb_init();
    enum { N = 256 };
    float in[N] = {0}, out[N], outR[N];
    /* run 2 s of zero input then check that output is exactly zero (no DC
     * drift, no self-oscillation). */
    int rendered = 0;
    float peak = 0.0f;
    while (rendered < 2 * SR) {
        int n = (2 * SR - rendered) < N ? (2 * SR - rendered) : N;
        reverb_render(in, in, out, outR, n);
        for (int i = 0; i < n; ++i) {
            float a = fmaxf(fabsf(out[i]), fabsf(outR[i]));
            if (a > peak) peak = a;
        }
        rendered += n;
    }
    CHECK(peak == 0.0f, "silent-in did not produce silent-out: peak=%g", peak);
}

static void test_reverb_bounded_under_load(void) {
    reverb_init();
    reverb_set(0.9f, 0.1f);                       /* long, bright tail */
    reverb_set_drive(0.5f);
    render_silence_reverb(SR / 4);                /* settle smoothing */

    enum { N = 256 };
    float in[N], inR[N], out[N], outR[N];
    /* 5 s of unit-amplitude white-ish noise (deterministic LCG). */
    uint32_t state = 0xdeadbeefu;
    float peak = 0.0f;
    int rendered = 0;
    while (rendered < 5 * SR) {
        for (int i = 0; i < N; ++i) {
            state = state * 1664525u + 1013904223u;
            in[i]  = ((int32_t)state) / 2147483648.0f;
            state = state * 1664525u + 1013904223u;
            inR[i] = ((int32_t)state) / 2147483648.0f;
        }
        reverb_render(in, inR, out, outR, N);
        for (int i = 0; i < N; ++i) {
            float a = fmaxf(fabsf(out[i]), fabsf(outR[i]));
            if (a > peak) peak = a;
        }
        rendered += N;
    }
    printf("  reverb peak under 5s noise input = %.3f\n", peak);
    CHECK(peak < 4.0f, "reverb saturated past sane bound: %g", peak);
}

/* ----------------------------------------------------------------- engine */

typedef struct { int pk_l, pk_r, diff_acc, nz_blocks; } eng_stats_t;

static eng_stats_t run_engine(int frames) {
    enum { N = 256 };
    int16_t buf[N * 2];
    eng_stats_t st = {0,0,0,0};
    int left = frames;
    while (left > 0) {
        int n = left < N ? left : N;
        engine_render(buf, n);
        int block_nonzero = 0;
        for (int i = 0; i < n; ++i) {
            int l = buf[i*2], r = buf[i*2+1];
            int al = abs(l), ar = abs(r);
            if (al > st.pk_l) st.pk_l = al;
            if (ar > st.pk_r) st.pk_r = ar;
            st.diff_acc += abs(l - r);
            if (al || ar) block_nonzero = 1;
        }
        if (block_nonzero) ++st.nz_blocks;
        left -= n;
    }
    return st;
}

static void test_engine_silent_then_note(void) {
    engine_init();
    engine_all_off();
    run_engine(SR * 6);                          /* drain any tail */

    /* Silent boot: output stays at zero. */
    eng_stats_t silent = run_engine(SR);
    CHECK(silent.pk_l == 0 && silent.pk_r == 0,
          "engine boot not silent: L=%d R=%d", silent.pk_l, silent.pk_r);

    /* Tap a cell; output rises through the pad attack + reverb buildup. */
    engine_note_on(0, 220.0f, 0.5f);
    eng_stats_t hit = run_engine((int)(2.5f * SR));
    CHECK(hit.pk_l > 1000 && hit.pk_r > 1000,
          "engine note did not produce audible output: L=%d R=%d",
          hit.pk_l, hit.pk_r);
    CHECK(hit.pk_l <= 32767 && hit.pk_r <= 32767, "engine clipped int16");
    CHECK(hit.diff_acc > 0, "engine output is mono (L==R)");
    printf("  engine peak after 2.5s note  L=%d  R=%d\n", hit.pk_l, hit.pk_r);
}

static void test_engine_reverb_tail_outlives_dry(void) {
    /* With a healthy wet-send, releasing the note should leave audible
     * reverb tail for several seconds — the whole point of Step 11. */
    engine_init();
    engine_set_send(0.8f); engine_set_wet_amp(0.7f);
    engine_set_reverb_size(0.85f);
    engine_all_off();
    run_engine(SR * 6);

    engine_note_on(1, 440.0f, 0.5f);
    run_engine((int)(2.5f * SR));                /* let it bloom */
    engine_note_off(1);

    /* Wait for the dry pad release to mostly finish (~3 s), then measure
     * the reverb tail energy. */
    run_engine((int)(3.5f * SR));
    eng_stats_t tail = run_engine(SR);            /* 1 s window for tail level */
    CHECK(tail.pk_l > 200 || tail.pk_r > 200,
          "reverb tail too quiet after dry release: L=%d R=%d",
          tail.pk_l, tail.pk_r);
    printf("  reverb tail 3.5-4.5s after release  L=%d  R=%d\n", tail.pk_l, tail.pk_r);

    /* And it does decay eventually: 15 s later it should be near silent. */
    run_engine((int)(15.0f * SR));
    eng_stats_t end = run_engine(SR / 2);
    CHECK(end.pk_l < 200 && end.pk_r < 200,
          "reverb tail never decays: L=%d R=%d", end.pk_l, end.pk_r);
}

static void test_engine_overload_holds_int16(void) {
    engine_init();
    engine_set_send(0.9f); engine_set_wet_amp(0.9f);
    engine_all_off();
    run_engine(SR * 6);

    for (int s = 0; s < PAD_MAX; ++s)
        engine_note_on((uint8_t)s, dsp_midi_to_hz(48.0f + s * 2), 1.0f);
    eng_stats_t st = run_engine(SR * 2);
    CHECK(st.pk_l <= 32767 && st.pk_r <= 32767,
          "engine wrapped int16 under overload: L=%d R=%d", st.pk_l, st.pk_r);
    CHECK(st.pk_l > 20000, "engine seems silent under overload: L=%d", st.pk_l);
    printf("  engine peak under full load  L=%d  R=%d\n", st.pk_l, st.pk_r);
}

int main(void) {
    dsp_init();

    printf("== reverb ==\n");
    test_reverb_impulse_decays();
    test_reverb_silent_in_silent_out();
    test_reverb_bounded_under_load();

    printf("== engine ==\n");
    test_engine_silent_then_note();
    test_engine_reverb_tail_outlives_dry();
    test_engine_overload_holds_int16();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
