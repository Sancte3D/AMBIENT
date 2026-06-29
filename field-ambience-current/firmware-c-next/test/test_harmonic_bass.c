/*
 * test_harmonic_bass.c — host test for the +28 harmonic-bass voice.
 * Bounded output, no NaN, the +28 partial is actually present, idle = silent.
 */
#include "dsp.h"
#include "harmonic_bass.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int checks = 0, fails = 0;
#define CHECK(c, ...) do { ++checks; if(!(c)){ ++fails; \
    fprintf(stderr,"FAIL %s:%d  ",__FILE__,__LINE__); \
    fprintf(stderr,__VA_ARGS__); fprintf(stderr,"\n"); } } while(0)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define SR        44100
enum { N = 256 };
#define SR_BLOCKS (SR / N)          /* ~172 blocks ≈ 1 s */
static float dL[N], dR[N], sL[N], sR[N];

static void clr(void){ memset(dL,0,sizeof dL); memset(dR,0,sizeof dR);
                       memset(sL,0,sizeof sL); memset(sR,0,sizeof sR); }

static void test_bounded_and_clean(void){
    harmonic_bass_init();
    harmonic_bass_set_drive(1.0f);       /* max drive = worst case for clipping */
    harmonic_bass_note(dsp_midi_to_hz(28.0f));   /* E1 */
    float peak = 0.0f; int bad = 0;
    for (int s = 0; s < 20; ++s)
        for (int b = 0; b < SR_BLOCKS; ++b){
            clr();
            harmonic_bass_render_mix(dL,dR,sL,sR,N);
            for (int i=0;i<N;++i){
                if (isnan(dL[i]) || isinf(dL[i])) ++bad;
                float a = fabsf(dL[i]); if (a>peak) peak=a;
                CHECK(dL[i]==dR[i], "L/R must match (mono)");
            }
        }
    CHECK(bad == 0, "%d NaN/Inf samples", bad);
    CHECK(peak < 1.2f, "diverged: peak %.3f", peak);
    CHECK(peak > 0.05f, "silent while held: peak %.3f", peak);
    printf("  bounded: 20 s held, peak %.3f, %d NaN\n", peak, bad);
}

static void test_idle_silent(void){
    harmonic_bass_init();                /* never noted → must be silent */
    float peak = 0.0f;
    for (int b = 0; b < SR_BLOCKS; ++b){
        clr(); harmonic_bass_render_mix(dL,dR,sL,sR,N);
        for (int i=0;i<N;++i){ float a=fabsf(dL[i]); if(a>peak)peak=a; }
    }
    CHECK(peak == 0.0f, "idle not silent: peak %.6f", peak);
    printf("  idle silent: peak %.6f\n", peak);
}

static void test_release_decays(void){
    harmonic_bass_init();
    harmonic_bass_note(dsp_midi_to_hz(33.0f));
    for (int b=0;b<SR_BLOCKS;++b){ clr(); harmonic_bass_render_mix(dL,dR,sL,sR,N); }
    harmonic_bass_release();
    /* Measure the TAIL: the env is still ~1.0 the instant after release, so a
     * peak over the whole window would always read loud. Assert the voice is
     * silent in the final second — i.e. it actually decayed to nothing. */
    float peak = 0.0f;
    for (int s=0;s<4;++s) for (int b=0;b<SR_BLOCKS;++b){
        clr(); harmonic_bass_render_mix(dL,dR,sL,sR,N);
        if (s < 3) continue;                       /* only the last second */
        for (int i=0;i<N;++i){ float a=fabsf(dL[i]); if(a>peak)peak=a; }
    }
    CHECK(peak < 0.02f, "did not decay after release: tail peak %.4f", peak);
    printf("  release tail (after 3 s) = %.4f\n", peak);
}

/* The +28 partial must actually be in the spectrum: a 1-bin Goertzel at the
 * root vs at root*2^(28/12) should both show real energy. */
static void test_harmonic_present(void){
    harmonic_bass_init();
    harmonic_bass_set_bite(1.0f);
    float root = dsp_midi_to_hz(40.0f);          /* E2 */
    float fh   = root * powf(2.0f, 28.0f/12.0f);
    harmonic_bass_note(root);
    for (int b=0;b<SR_BLOCKS*2;++b){ clr(); harmonic_bass_render_mix(dL,dR,sL,sR,N); } /* settle */
    /* Goertzel magnitude at fh over ~0.5 s */
    double wr = 2.0*M_PI*fh/SR, cr = cos(wr);
    double q1r=0, q2r=0;
    double wb = 2.0*M_PI*root/SR, cb = cos(wb);
    double q1b=0, q2b=0;
    long n=0;
    for (int b=0;b<SR_BLOCKS/2;++b){
        clr(); harmonic_bass_render_mix(dL,dR,sL,sR,N);
        for (int i=0;i<N;++i){ double x=dL[i];
            double q0r=2*cr*q1r-q2r+x; q2r=q1r; q1r=q0r;
            double q0b=2*cb*q1b-q2b+x; q2b=q1b; q1b=q0b; ++n; }
    }
    double magH = sqrt(q1r*q1r+q2r*q2r-2*cr*q1r*q2r)/n;
    double magB = sqrt(q1b*q1b+q2b*q2b-2*cb*q1b*q2b)/n;
    CHECK(magH > 1e-3, "+28 partial absent: magH %.5f", magH);
    printf("  +28 partial present: |root|=%.4f  |+28|=%.4f\n", magB, magH);
}

int main(void){
    dsp_init();
    printf("== harmonic_bass ==\n");
    test_bounded_and_clean();
    test_idle_silent();
    test_release_decays();
    test_harmonic_present();
    printf("\n%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
