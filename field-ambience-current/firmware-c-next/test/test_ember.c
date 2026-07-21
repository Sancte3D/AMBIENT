/*
 * test_ember.c — r19.28 warm subtractive voice (ember.c).
 *
 * Statistical/behavioural checks (no golden audio):
 *   - silence before any note;
 *   - a struck note produces signal that stays bounded (no NaN, no blow-up
 *     from the resonant filter);
 *   - a soft attack: the first millisecond is quieter than the body (env ramp);
 *   - the voice self-retires after its decay (active_count returns to 0);
 *   - the pool steals rather than overflowing.
 */
#include <stdio.h>
#include <math.h>
#include "ember.h"
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

#define N 256
static float L[N], R[N], SL[N], SR_[N];
static void clearbufs(void){ for (int i=0;i<N;++i){L[i]=R[i]=SL[i]=SR_[i]=0.0f;} }
static float block_peak(void){ float p=0; for(int i=0;i<N;++i){
    float a=fabsf(L[i]); if(a>p)p=a; a=fabsf(R[i]); if(a>p)p=a; } return p; }
static int block_finite(void){ for(int i=0;i<N;++i){
    if(!isfinite(L[i])||!isfinite(R[i])||!isfinite(SL[i])||!isfinite(SR_[i])) return 0; }
    return 1; }

int main(void) {
    printf("== ember warm voice (r19.28) ==\n");
    dsp_init();
    ember_init();

    /* 1. silence before a note */
    clearbufs(); ember_render_mix(L,R,SL,SR_,N);
    CHECK(block_peak() == 0.0f, "silent before any note");
    CHECK(ember_active_count() == 0, "no active voices at rest");

    /* 2. strike → signal, bounded, finite */
    ember_note(dsp_midi_to_hz(69.0f), 0.9f);   /* A4 */
    CHECK(ember_active_count() == 1, "one voice active after strike");
    clearbufs(); ember_render_mix(L,R,SL,SR_,N);
    float first = block_peak();
    CHECK(first > 0.0f, "signal present after strike");
    CHECK(block_finite(), "no NaN/Inf in first block");

    /* 3. soft attack: a later block within the note is louder than the very
     * first (the env is still ramping up across the opening blocks). */
    float bodypk = 0.0f;
    for (int b = 0; b < 8; ++b) { clearbufs(); ember_render_mix(L,R,SL,SR_,N);
                                  float p = block_peak(); if (p > bodypk) bodypk = p;
                                  CHECK(block_finite(), "finite during body"); }
    CHECK(bodypk > first, "soft attack: body louder than the first block (%.4f>%.4f)", bodypk, first);
    CHECK(bodypk < 4.0f, "output stays bounded through the resonant filter (%.3f)", bodypk);

    /* 4. self-retire: run well past the ~2.6 s decay */
    int blocks = (int)(4.0f * 44100.0f / N);
    for (int b = 0; b < blocks; ++b) { clearbufs(); ember_render_mix(L,R,SL,SR_,N); }
    CHECK(ember_active_count() == 0, "voice retired after its decay");

    /* 5. pool steals, never overflows */
    for (int i = 0; i < EMBER_VOICES + 4; ++i) ember_note(dsp_midi_to_hz(60.0f + i), 0.7f);
    CHECK(ember_active_count() <= EMBER_VOICES, "pool bounded by EMBER_VOICES");
    clearbufs(); ember_render_mix(L,R,SL,SR_,N);
    CHECK(block_finite(), "finite with a full stolen pool");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
