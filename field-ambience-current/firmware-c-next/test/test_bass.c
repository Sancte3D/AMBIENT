/*
 * Host-side tests for the Step-8 two-layer bass + dsp_svf highpass / dsp_tri.
 *
 * Build via run_tests.sh, or directly:
 *   cc -std=c11 -I../include test_bass.c ../src/dsp.c ../src/bass.c \
 *      -lm -o /tmp/bass_test
 *
 * Exit 0 = all pass.
 */

#include "dsp.h"
#include "bass.h"

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

/* --------------------------------------------------------------- dsp_tri */

static void test_tri_shape(void) {
    CHECK(fabsf(dsp_tri(0.0f)  - 1.0f) < 1e-3f, "tri(0) != +1: %g", dsp_tri(0.0f));
    CHECK(fabsf(dsp_tri(0.25f) - 0.0f) < 1e-3f, "tri(0.25) != 0: %g", dsp_tri(0.25f));
    CHECK(fabsf(dsp_tri(0.5f)  + 1.0f) < 1e-3f, "tri(0.5) != -1: %g", dsp_tri(0.5f));
    CHECK(fabsf(dsp_tri(0.75f) - 0.0f) < 1e-3f, "tri(0.75) != 0: %g", dsp_tri(0.75f));
    /* periodic + bounded */
    float mx = 0.0f;
    for (int k = 0; k < 5000; ++k) {
        float v = dsp_tri((float)k * 0.000731f);
        if (fabsf(v) > mx) mx = fabsf(v);
    }
    CHECK(mx <= 1.001f, "tri out of range: %g", mx);
    CHECK(fabsf(dsp_tri(0.3f) - dsp_tri(1.3f)) < 1e-4f, "tri not periodic");
}

/* ------------------------------------------------------------ svf highpass */

static float svf_hp_rms(float freq, float fc) {
    dsp_svf_t s; dsp_svf_reset(&s); dsp_svf_set(&s, fc, 0.707f);
    float ph = 0.0f, dt = freq / (float)SR;
    for (int n = 0; n < 4096; ++n) { dsp_svf_hp(&s, sinf(6.2831853f*ph)); ph+=dt; if(ph>=1)ph-=1; }
    double acc = 0; int N = 8192;
    for (int n = 0; n < N; ++n) { float y = dsp_svf_hp(&s, sinf(6.2831853f*ph)); acc += (double)y*y; ph+=dt; if(ph>=1)ph-=1; }
    return (float)sqrt(acc/N);
}

static void test_svf_highpass(void) {
    /* HP 50 Hz: passes 500 Hz ≈ unity, strongly attenuates 12 Hz. */
    float pass = svf_hp_rms(500.0f, 50.0f);
    float stop = svf_hp_rms(12.0f, 50.0f);
    printf("  HP 50Hz: pass(500)=%.3f  stop(12)=%.3f  (in=0.707)\n", pass, stop);
    CHECK(pass > 0.6f, "HP passband weak: %g", pass);
    CHECK(stop < 0.2f, "HP did not attenuate subsonic: %g", stop);
}

/* ------------------------------------------------------------------ bass */

typedef struct { float pk; double e_dry, e_send; int mono_ok; } bass_stats_t;

static bass_stats_t render_bass(int frames) {
    enum { N = 256 };
    float dl[N], dr[N], sl[N], sr[N];
    bass_stats_t st = {0,0,0,1};
    int left = frames;
    while (left > 0) {
        int n = left < N ? left : N;
        memset(dl,0,sizeof(float)*n); memset(dr,0,sizeof(float)*n);
        memset(sl,0,sizeof(float)*n); memset(sr,0,sizeof(float)*n);
        bass_render_mix(dl, dr, sl, sr, n);
        for (int i = 0; i < n; ++i) {
            if (dl[i] != dr[i]) st.mono_ok = 0;          /* bass is mono */
            float a = fabsf(dl[i]);
            if (a > st.pk) st.pk = a;
            st.e_dry  += (double)dl[i]*dl[i];
            st.e_send += (double)sl[i]*sl[i];
        }
        left -= n;
    }
    return st;
}

static void test_bass_idle_silent(void) {
    bass_init();
    CHECK(!bass_active(), "bass active at init");
    bass_stats_t st = render_bass(SR);
    CHECK(st.pk == 0.0f, "idle bass not silent: %g", st.pk);
}

static void test_bass_blooms_and_is_mono(void) {
    bass_init();
    bass_set_depth(1.0f);
    bass_note(110.0f);                          /* A2 → sub 27.5 Hz, deep 55 Hz */
    CHECK(bass_active(), "bass not active after note");

    /* Onset near-silent (3 s / 2.5 s blooms). */
    bass_stats_t onset = render_bass(256);
    CHECK(onset.pk < 0.02f, "bass onset not a bloom: %g", onset.pk);

    /* After ~3 s it should be clearly present. */
    bass_stats_t sus = render_bass(3 * SR);
    CHECK(sus.pk > 0.05f, "bass never bloomed: %g", sus.pk);
    CHECK(sus.pk < 1.0f, "bass too hot: %g", sus.pk);
    CHECK(sus.mono_ok, "bass is not mono (L != R)");
    /* The reverb send is the two layers at 0.03/0.08 weight → far quieter
     * than the dry sum in aggregate. */
    CHECK(sus.e_send < sus.e_dry * 0.05f,
          "bass send energy too high vs dry (%.3g vs %.3g)", sus.e_send, sus.e_dry);
    printf("  bass sustain peak = %.3f\n", sus.pk);

    /* Release → silence (exp tail to the −60 dB idle cutoff ≈ 7 s; allow margin). */
    bass_release();
    render_bass(10 * SR);
    CHECK(!bass_active(), "bass did not drain after release");
    bass_stats_t after = render_bass(SR);
    CHECK(after.pk == 0.0f, "bass not silent after release: %g", after.pk);
}

static void test_bass_legato_no_retrigger(void) {
    /* Changing the held root while sounding must glide (stay active), not
     * drop back to idle/bloom. */
    bass_init();
    bass_set_depth(0.8f);
    bass_note(220.0f);
    render_bass(3 * SR);                         /* reach sustain */
    bass_stats_t before = render_bass(256);

    bass_note(165.0f);                           /* new lower root, still held */
    CHECK(bass_active(), "bass dropped out on root change");
    /* Should not collapse to near-silence (no re-bloom from 0). */
    bass_stats_t after = render_bass(256);
    CHECK(after.pk > before.pk * 0.4f,
          "root change re-triggered bloom (level collapsed): %g → %g",
          before.pk, after.pk);
}

static void test_bass_bounded_long(void) {
    bass_init();
    bass_set_depth(1.0f);
    bass_note(130.81f);
    render_bass(3 * SR);
    bass_stats_t st = render_bass(20 * SR);
    CHECK(st.pk < 1.5f, "bass diverged over 20 s: %g", st.pk);
    printf("  bass 20s peak = %.3f\n", st.pk);
}

int main(void) {
    dsp_init();

    printf("== dsp_tri / svf highpass ==\n");
    test_tri_shape();
    test_svf_highpass();

    printf("== famSubBass + famDeepBass ==\n");
    test_bass_idle_silent();
    test_bass_blooms_and_is_mono();
    test_bass_legato_no_retrigger();
    test_bass_bounded_long();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
