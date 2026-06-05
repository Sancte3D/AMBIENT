/*
 * Host-side tests for the Step-10 famTexture bed + the dsp_svf_bp output.
 *
 * Build via run_tests.sh, or directly:
 *   cc -std=c11 -I../include test_texture.c ../src/dsp.c ../src/texture.c \
 *      -lm -o /tmp/tex_test
 *
 * Exit 0 = all pass.
 */

#include "dsp.h"
#include "texture.h"

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

#define SR DSP_SAMPLE_RATE_HZ

/* --------------------------------------------------------------- svf bp */

static float svf_bp_rms(float freq, float fc, float q) {
    dsp_svf_t s; dsp_svf_reset(&s); dsp_svf_set(&s, fc, q);
    float ph = 0.0f, dt = freq / (float)SR;
    for (int n = 0; n < 4096; ++n) { dsp_svf_bp(&s, sinf(6.2831853f * ph)); ph += dt; if (ph>=1) ph-=1; }
    double acc = 0.0; int N = 8192;
    for (int n = 0; n < N; ++n) {
        float y = dsp_svf_bp(&s, sinf(6.2831853f * ph));
        acc += (double)y * y; ph += dt; if (ph>=1) ph-=1;
    }
    return (float)sqrt(acc / N);
}

static void test_svf_bandpass_shape(void) {
    /* Bandpass at 600 Hz: passes the centre, rejects an octave below and a
     * couple octaves above. Input unit sine RMS = 0.707. */
    float centre = svf_bp_rms(600.0f, 600.0f, 1.6f);
    float below  = svf_bp_rms(150.0f, 600.0f, 1.6f);
    float above  = svf_bp_rms(4800.0f, 600.0f, 1.6f);
    printf("  BP 600Hz: centre=%.3f  150Hz=%.3f  4800Hz=%.3f  (in=0.707)\n",
           centre, below, above);
    CHECK(centre > 0.45f, "BP centre too weak: %g", centre);
    CHECK(below  < centre * 0.5f, "BP did not reject below band: %g vs %g", below, centre);
    CHECK(above  < centre * 0.5f, "BP did not reject above band: %g vs %g", above, centre);
}

/* ------------------------------------------------------------- texture */

typedef struct { float pk_l, pk_r; double e_l, e_r, diff; } tex_stats_t;

static tex_stats_t render_texture(int frames, float send_amount) {
    enum { N = 256 };
    float dl[N], dr[N], sl[N], sr[N];
    tex_stats_t st = {0,0,0,0,0};
    int left = frames;
    while (left > 0) {
        int n = left < N ? left : N;
        memset(dl,0,sizeof(float)*n); memset(dr,0,sizeof(float)*n);
        memset(sl,0,sizeof(float)*n); memset(sr,0,sizeof(float)*n);
        texture_render_mix(dl, dr, sl, sr, n, send_amount);
        for (int i = 0; i < n; ++i) {
            float al = fabsf(dl[i]), ar = fabsf(dr[i]);
            if (al > st.pk_l) st.pk_l = al;
            if (ar > st.pk_r) st.pk_r = ar;
            st.e_l += (double)dl[i]*dl[i];
            st.e_r += (double)dr[i]*dr[i];
            st.diff += fabs((double)dl[i] - dr[i]);
            /* the send buffer must be the dry scaled by send_amount */
            CHECK(fabsf(sl[i] - dl[i]*send_amount) < 1e-6f, "send L != dry*send");
        }
        left -= n;
    }
    return st;
}

static void test_texture_silent_at_zero(void) {
    texture_init();                              /* boots at amount 0 */
    tex_stats_t st = render_texture(SR, 0.5f);
    CHECK(st.pk_l == 0.0f && st.pk_r == 0.0f,
          "texture not silent at amount 0: L=%g R=%g", st.pk_l, st.pk_r);
}

static void test_texture_blooms_and_is_stereo(void) {
    texture_init();
    texture_set_amount(0.8f);
    /* Let the ~2 s amp glide settle, then measure. */
    render_texture(4 * SR, 0.5f);
    tex_stats_t st = render_texture(2 * SR, 0.5f);

    CHECK(st.pk_l > 0.0f && st.pk_r > 0.0f, "texture produced no output");
    CHECK(st.pk_l < 1.0f && st.pk_r < 1.0f, "texture too hot: L=%g R=%g", st.pk_l, st.pk_r);

    /* Independent L/R noise → channels must differ substantially. */
    double avg_diff = st.diff / (2.0 * SR);
    CHECK(avg_diff > 1e-3, "texture not stereo (L≈R): avg|L-R|=%g", avg_diff);

    /* Both channels carry comparable energy (neither dead). */
    double rms_l = sqrt(st.e_l / (2.0 * SR));
    double rms_r = sqrt(st.e_r / (2.0 * SR));
    CHECK(rms_l > 1e-4 && rms_r > 1e-4, "a texture channel is dead: L=%g R=%g", rms_l, rms_r);
    printf("  texture rms L=%.4f R=%.4f  peak L=%.3f R=%.3f  avg|L-R|=%.4f\n",
           rms_l, rms_r, st.pk_l, st.pk_r, avg_diff);
}

static void test_texture_bounded_long_run(void) {
    /* 30 s at full amount: the leaky brown integrator + sweeping BP must not
     * wander off (no DC runaway, no divergence). */
    texture_init();
    texture_set_amount(1.0f);
    render_texture(3 * SR, 0.5f);                 /* settle */
    tex_stats_t st = render_texture(30 * SR, 0.5f);
    CHECK(st.pk_l < 1.5f && st.pk_r < 1.5f,
          "texture diverged over 30 s: L=%g R=%g", st.pk_l, st.pk_r);
    printf("  texture 30s peak L=%.3f R=%.3f\n", st.pk_l, st.pk_r);
}

int main(void) {
    dsp_init();

    printf("== svf bandpass ==\n");
    test_svf_bandpass_shape();

    printf("== famTexture ==\n");
    test_texture_silent_at_zero();
    test_texture_blooms_and_is_stereo();
    test_texture_bounded_long_run();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
