/*
 * Host-side tests for the Step-12b #2 drone.
 *
 * Build via run_tests.sh, or:
 *   cc -std=c11 -I../include test_drone.c ../src/dsp.c ../src/drone.c \
 *      -lm -o /tmp/drone_test
 */

#include "dsp.h"
#include "drone.h"

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

typedef struct { float pk_l, pk_r; double e, diff; } dstats_t;

static dstats_t render(int frames) {
    enum { N = 256 };
    float dl[N], dr[N], sl[N], sr[N];
    dstats_t st = {0,0,0,0};
    int left = frames;
    while (left > 0) {
        int n = left < N ? left : N;
        memset(dl,0,sizeof(float)*n); memset(dr,0,sizeof(float)*n);
        memset(sl,0,sizeof(float)*n); memset(sr,0,sizeof(float)*n);
        drone_render_mix(dl, dr, sl, sr, n);
        for (int i = 0; i < n; ++i) {
            float al = fabsf(dl[i]), ar = fabsf(dr[i]);
            if (al > st.pk_l) st.pk_l = al;
            if (ar > st.pk_r) st.pk_r = ar;
            st.e += (double)dl[i]*dl[i];
            st.diff += fabs((double)dl[i] - dr[i]);
            /* send must be the dry scaled by 0.45 */
            CHECK(fabsf(sl[i] - dl[i]*0.45f) < 1e-6f, "send != dry*0.45");
        }
        left -= n;
    }
    return st;
}

static void test_idle_silent(void) {
    drone_init();
    CHECK(!drone_active(), "drone active at init");
    dstats_t st = render(SR);
    CHECK(st.pk_l == 0.0f && st.pk_r == 0.0f, "idle drone not silent: %g/%g", st.pk_l, st.pk_r);
}

static void test_bloom_and_stereo(void) {
    drone_init();
    drone_set_root_midi(48);              /* C3 */
    drone_enable(true);
    CHECK(drone_active(), "drone not active after enable");

    /* Slow 6 s bloom: the first 64 ms must be near-silent. */
    dstats_t onset = render(2048);
    CHECK(onset.pk_l < 0.01f && onset.pk_r < 0.01f, "drone onset not a slow bloom: %g/%g",
          onset.pk_l, onset.pk_r);

    /* After ~6 s it should be clearly present but quiet (amp 0.05). */
    dstats_t sus = render(7 * SR);
    CHECK(sus.pk_l > 0.01f && sus.pk_r > 0.01f, "drone never bloomed: %g/%g", sus.pk_l, sus.pk_r);
    CHECK(sus.pk_l < 0.3f && sus.pk_r < 0.3f, "drone too loud: %g/%g", sus.pk_l, sus.pk_r);
    /* Haas + opposite pan → channels differ. */
    double avg_diff = sus.diff / (7.0 * SR);
    CHECK(avg_diff > 1e-4, "drone not stereo: avg|L-R|=%g", avg_diff);
    printf("  drone sustain peak L=%.3f R=%.3f  avg|L-R|=%.4f\n", sus.pk_l, sus.pk_r, avg_diff);
}

static void test_glide_no_retrigger(void) {
    /* Change the root while sounding: must stay active and not collapse to a
     * fresh bloom from silence. */
    drone_init();
    drone_set_root_midi(48);
    drone_enable(true);
    render(7 * SR);                       /* reach sustain */
    dstats_t before = render(512);

    drone_set_root_midi(43);              /* glide down to G2 */
    CHECK(drone_active(), "drone dropped out on root change");
    dstats_t after = render(512);
    CHECK(after.pk_l > before.pk_l * 0.5f,
          "root change re-triggered bloom (level collapsed): %g -> %g", before.pk_l, after.pk_l);
}

static void test_disable_drains(void) {
    drone_init();
    drone_set_root_midi(50);
    drone_enable(true);
    render(7 * SR);
    drone_enable(false);
    render(10 * SR);                      /* 4 s tail (tau 1.33) + margin */
    CHECK(!drone_active(), "drone did not drain after disable");
    dstats_t after = render(SR);
    CHECK(after.pk_l == 0.0f && after.pk_r == 0.0f, "drone not silent after disable: %g/%g",
          after.pk_l, after.pk_r);
}

static void test_bounded_long(void) {
    drone_init();
    drone_set_root_midi(55);
    drone_enable(true);
    render(7 * SR);
    dstats_t st = render(30 * SR);
    CHECK(st.pk_l < 0.5f && st.pk_r < 0.5f, "drone diverged over 30 s: %g/%g", st.pk_l, st.pk_r);
    printf("  drone 30 s peak L=%.3f R=%.3f\n", st.pk_l, st.pk_r);
}

int main(void) {
    dsp_init();
    printf("== drone (step12b #2) ==\n");
    test_idle_silent();
    test_bloom_and_stereo();
    test_glide_no_retrigger();
    test_disable_drains();
    test_bounded_long();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
