/*
 * Host-side unit tests for the Step-9 DSP primitives (polyBLEP oscillators,
 * TPT state-variable filter) and the famPadCore voice (pad.c).
 *
 * Pure C — compiles and runs without the Pico SDK. Build + run via
 * ./run_tests.sh, or directly:
 *   cc -std=c11 -I../include test_pad.c ../src/dsp.c ../src/pad.c -lm -o /tmp/pad_test
 *
 * Exit 0 = all pass.
 */

#include "dsp.h"
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

/* ----------------------------------------------------------- oscillators */

static void test_polyblep_bounded(void) {
    /* Across a spread of frequencies the band-limited saw/square must stay
     * bounded (a broken polyBLEP overshoots wildly). */
    float worst_saw = 0.0f, worst_sq = 0.0f;
    const float freqs[] = { 55.0f, 220.0f, 880.0f, 3000.0f, 9000.0f };
    for (unsigned fi = 0; fi < sizeof freqs / sizeof freqs[0]; ++fi) {
        float dt = freqs[fi] / (float)DSP_SAMPLE_RATE_HZ;
        float ph = 0.0f;
        for (int n = 0; n < 20000; ++n) {
            float sw = dsp_poly_saw(ph, dt);
            float sq = dsp_poly_square(ph, dt);
            if (fabsf(sw) > worst_saw) worst_saw = fabsf(sw);
            if (fabsf(sq) > worst_sq) worst_sq = fabsf(sq);
            ph += dt; if (ph >= 1.0f) ph -= 1.0f;
        }
    }
    CHECK(worst_saw < 1.25f, "saw out of bounds: %g", worst_saw);
    CHECK(worst_sq  < 1.25f, "square out of bounds: %g", worst_sq);
    printf("  polyBLEP peak  saw=%.3f  square=%.3f\n", worst_saw, worst_sq);
}

/* ------------------------------------------------------------------ SVF */

/* RMS of a sine of `freq` after a lowpass at `fc` (after settling). */
static float svf_sine_rms(float freq, float fc, float q) {
    dsp_svf_t s; dsp_svf_reset(&s); dsp_svf_set(&s, fc, q);
    float ph = 0.0f, dt = freq / (float)DSP_SAMPLE_RATE_HZ;
    /* settle */
    for (int n = 0; n < 4096; ++n) { dsp_svf_lp(&s, sinf(6.2831853f * ph)); ph += dt; if (ph>=1) ph-=1; }
    double acc = 0.0; int N = 8192;
    for (int n = 0; n < N; ++n) {
        float y = dsp_svf_lp(&s, sinf(6.2831853f * ph));
        acc += (double)y * y; ph += dt; if (ph>=1) ph-=1;
    }
    return (float)sqrt(acc / N);
}

static void test_svf_lowpass(void) {
    /* Passband (100 Hz, cutoff 1 kHz) ≈ unity; stopband (8 kHz) ≫ attenuated.
     * Reference: unit sine RMS = 0.707. */
    float pass = svf_sine_rms(100.0f, 1000.0f, 0.707f);
    float stop = svf_sine_rms(8000.0f, 1000.0f, 0.707f);
    CHECK(pass > 0.55f && pass < 0.85f, "passband not ~unity: rms=%g", pass);
    CHECK(stop < 0.10f, "stopband not attenuated: rms=%g", stop);
    printf("  SVF pass(100Hz)=%.3f  stop(8kHz)=%.4f  (in=0.707)\n", pass, stop);

    /* DC must pass at unity gain. */
    dsp_svf_t s; dsp_svf_reset(&s); dsp_svf_set(&s, 1000.0f, 0.707f);
    float y = 0.0f;
    for (int n = 0; n < 8192; ++n) y = dsp_svf_lp(&s, 1.0f);
    CHECK(fabsf(y - 1.0f) < 0.02f, "DC gain != 1: %g", y);
}

/* ------------------------------------------------------------------ pad */

typedef struct { int pk_l, pk_r, max_step, diff_acc; } pad_stats_t;

static pad_stats_t render_pad(int frames) {
    enum { CH = 64 };
    int16_t buf[CH * 2];
    pad_stats_t st = {0,0,0,0};
    int prev = 0, have_prev = 0, left = frames;
    while (left > 0) {
        int n = left < CH ? left : CH;
        pad_render(buf, n);
        for (int i = 0; i < n; ++i) {
            int l = buf[i*2], r = buf[i*2+1];
            int al = abs(l), ar = abs(r);
            if (al > st.pk_l) st.pk_l = al;
            if (ar > st.pk_r) st.pk_r = ar;
            st.diff_acc += abs(l - r);
            if (have_prev) { int step = abs(l - prev); if (step > st.max_step) st.max_step = step; }
            prev = l; have_prev = 1;
        }
        left -= n;
    }
    return st;
}

static void drain(void) { pad_all_off(); render_pad(DSP_SAMPLE_RATE_HZ * 5); }

static void test_pad_bloom_and_silence(void) {
    drain();
    CHECK(pad_active_count() == 0, "pool not empty: %d", pad_active_count());

    pad_note_on(0, 130.81f, 0.5f);           /* C3 */
    CHECK(pad_active_count() == 1, "note_on did not arm a voice");

    /* Bloom: the first few ms must be near-silent (env starts at ~0); a click
     * would slam to full scale immediately. */
    pad_stats_t onset = render_pad(64);
    CHECK(onset.pk_l < 400 && onset.pk_r < 400, "pad onset not a bloom: L=%d R=%d",
          onset.pk_l, onset.pk_r);

    /* After the ~1.6 s attack it should be clearly audible. */
    pad_stats_t sus = render_pad((int)(2.0f * DSP_SAMPLE_RATE_HZ));
    CHECK(sus.pk_l > 1000 && sus.pk_r > 1000, "pad never reached sustain: L=%d R=%d",
          sus.pk_l, sus.pk_r);
    CHECK(sus.pk_l <= 32767 && sus.pk_r <= 32767, "pad clipped past int16");
    printf("  pad sustain peak  L=%d  R=%d\n", sus.pk_l, sus.pk_r);

    /* Genuine stereo: Haas + opposing pan must decorrelate the channels. */
    CHECK(sus.diff_acc > 0, "pad is mono (L==R everywhere)");
    int avg_diff = sus.diff_acc / (int)(2.0f * DSP_SAMPLE_RATE_HZ);
    CHECK(avg_diff > 20, "pad stereo image too narrow: avg|L-R|=%d", avg_diff);
    printf("  pad avg |L-R| = %d\n", avg_diff);

    /* Release → full silence (3 s release ≈ several time-constants + margin). */
    pad_note_off(0);
    render_pad(12 * DSP_SAMPLE_RATE_HZ);
    CHECK(pad_active_count() == 0, "pad did not drain: %d", pad_active_count());
    pad_stats_t after = render_pad(1024);
    CHECK(after.pk_l == 0 && after.pk_r == 0, "non-silent after release: L=%d R=%d",
          after.pk_l, after.pk_r);
}

static void test_pad_retrigger_no_stack(void) {
    drain();
    pad_note_on(2, 196.0f, 0.4f);
    render_pad(1024);
    CHECK(pad_active_count() == 1, "expected 1 voice");
    pad_note_on(2, 261.6f, 0.4f);            /* same source → re-bloom, not stack */
    CHECK(pad_active_count() == 1, "re-trigger stacked: %d", pad_active_count());
}

static void test_pad_pool_bounded(void) {
    drain();
    for (int s = 0; s < PAD_MAX; ++s)
        pad_note_on((uint8_t)s, dsp_midi_to_hz(48.0f + s), 0.3f);
    render_pad(512);
    CHECK(pad_active_count() == PAD_MAX, "pool not full: %d", pad_active_count());

    pad_note_on(99, 440.0f, 0.3f);           /* 9th distinct source must steal */
    CHECK(pad_active_count() == PAD_MAX, "pool overflowed: %d", pad_active_count());

    /* Hammer all voices loud: master soft-clip must hold int16. */
    pad_stats_t st = render_pad(DSP_SAMPLE_RATE_HZ);
    CHECK(st.pk_l <= 32767 && st.pk_r <= 32767, "overload wrapped int16");
    CHECK(st.pk_l > 0 && st.pk_r > 0, "full load produced silence");
    printf("  pad peak under full load  L=%d  R=%d\n", st.pk_l, st.pk_r);
}

/* Step 12b #3: the global voice-mix crossfade (warm/strings/brass). At warm
 * (0) the timbre is pure saw; raising it mixes in squares. The control is
 * global + smoothed, so it must keep the note sounding and stay click-free
 * while gliding into the new timbre. */
static void test_pad_voice_mix(void) {
    drain();
    pad_set_voice_mix(0.0f);                          /* warm */
    pad_note_on(0, 196.0f, 0.6f);
    render_pad(2 * DSP_SAMPLE_RATE_HZ);               /* reach sustain */
    pad_stats_t warm = render_pad(DSP_SAMPLE_RATE_HZ / 2);
    CHECK(warm.pk_l > 1000, "warm pad produced near-silence: L=%d", warm.pk_l);

    /* Slam to brass: the timbre changes but the note must not stop, stay
     * bounded, and glide click-free (no full-scale jump). */
    pad_set_voice_mix(1.2f);                          /* brass */
    pad_stats_t glide = render_pad(DSP_SAMPLE_RATE_HZ);
    CHECK(glide.pk_l > 1000, "brass pad collapsed to near-silence: L=%d", glide.pk_l);
    CHECK(glide.pk_l <= 32767 && glide.pk_r <= 32767, "brass pad clipped int16");
    CHECK(glide.max_step < 9000, "voice-mix change not click-free: step=%d", glide.max_step);
    printf("  voice-mix peak  warm L=%d  brass L=%d\n", warm.pk_l, glide.pk_l);

    pad_note_off(0);
    pad_set_voice_mix(0.0f);                          /* restore default */
    drain();
}

int main(void) {
    dsp_init();
    pad_init();

    printf("== Step 9 DSP primitives ==\n");
    test_polyblep_bounded();
    test_svf_lowpass();

    printf("== famPadCore ==\n");
    test_pad_bloom_and_silence();
    test_pad_retrigger_no_stack();
    test_pad_pool_bounded();
    test_pad_voice_mix();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
