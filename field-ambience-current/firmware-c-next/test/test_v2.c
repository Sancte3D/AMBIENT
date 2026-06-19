/*
 * test_v2.c — host-side smoke + module tests for Engine V2.
 *
 * Compiled by run_tests.sh against the v2/ sources plus dsp + reverb from V1.
 * Goal: prove each module behaves; final engine renders non-silent, non-NaN,
 * non-clipped audio under default macros.
 */

#include "v2/engine_v2.h"
#include "v2/motion.h"
#include "v2/harmony_field.h"
#include "v2/material_texture.h"
#include "v2/diffuser.h"
#include "v2/mod_delay.h"
#include "v2/beauty_guard.h"
#include "v2/field_voice.h"
#include "v2/worlds.h"
#include "dsp.h"
#include "audio.h"
#include "reverb.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static int pass = 0, fail = 0;
#define EXPECT(cond, msg) do { \
    if (cond) { pass++; printf("  ✓ %s\n", msg); } \
    else      { fail++; printf("  ✗ %s\n", msg); } \
} while (0)

/* ---------------------------------------------------------------- helpers */

static int has_nan_or_inf(const float *x, int n) {
    for (int i = 0; i < n; ++i)
        if (!(x[i] == x[i]) || x[i] > 1e30f || x[i] < -1e30f) return 1;
    return 0;
}
static float peak_abs(const float *x, int n) {
    float p = 0.0f;
    for (int i = 0; i < n; ++i) {
        float a = x[i] >= 0.0f ? x[i] : -x[i];
        if (a > p) p = a;
    }
    return p;
}
static float rms(const float *x, int n) {
    double s = 0.0;
    for (int i = 0; i < n; ++i) s += x[i] * x[i];
    return (float)sqrt(s / (double)n);
}
static float peak_i16(const int16_t *x, int n) {
    int16_t p = 0;
    for (int i = 0; i < n; ++i) {
        int16_t a = x[i] >= 0 ? x[i] : (int16_t)(-x[i]);
        if (a > p) p = a;
    }
    return (float)p / 32768.0f;
}
static float rms_i16(const int16_t *x, int n) {
    double s = 0.0;
    for (int i = 0; i < n; ++i) {
        double v = (double)x[i] / 32768.0;
        s += v * v;
    }
    return (float)sqrt(s / (double)n);
}

/* ------------------------------------------------------ motion ---------- */

static void test_motion(void) {
    printf("\n=== motion ===\n");
    motion_t m;
    motion_init(&m, 12345);
    EXPECT(motion_slow1(&m) >= -1.001f && motion_slow1(&m) <= 1.001f, "slow1 in [-1,1] at init");
    EXPECT(motion_breath(&m) >= 0.0f && motion_breath(&m) <= 1.0f, "breath in [0,1]");

    /* Advance through 60 s in 256-frame blocks @ 44.1k. */
    float dt = 256.0f / 44100.0f;
    int steps = (int)(60.0f / dt);
    float min1 = 99, max1 = -99;
    float min_walk = 99, max_walk = -99;
    int distinct_entropy = 0;
    float last_e = -1.0f;
    for (int i = 0; i < steps; ++i) {
        motion_advance(&m, dt);
        float s = motion_slow1(&m);
        if (s < min1) min1 = s;
        if (s > max1) max1 = s;
        float w = motion_walk(&m);
        if (w < min_walk) min_walk = w;
        if (w > max_walk) max_walk = w;
        float e = motion_entropy(&m);
        if (fabsf(e - last_e) > 0.01f) distinct_entropy++;
        last_e = e;
    }
    EXPECT(max1 > 0.6f && min1 < -0.6f, "slow1 sweeps full range in 60s");
    EXPECT(max_walk > 0.1f && min_walk < -0.1f, "walk visits both signs");
    EXPECT(distinct_entropy > steps/2, "entropy refreshes per advance");

    /* Determinism: same seed, same sequence. */
    motion_t a, b;
    motion_init(&a, 999);
    motion_init(&b, 999);
    for (int i = 0; i < 100; ++i) {
        motion_advance(&a, 0.01f);
        motion_advance(&b, 0.01f);
    }
    EXPECT(fabsf(motion_slow1(&a) - motion_slow1(&b)) < 1e-6f, "deterministic per seed");
}

/* ------------------------------------------------ harmony_field --------- */

static void test_harmony_field(void) {
    printf("\n=== harmony_field ===\n");
    hf_init(7777, 57);  /* A3 */
    EXPECT(hf_active_count() >= 3, "at least bass+root+fifth active");

    const hf_voice_t *bass = hf_voice(HF_VOICE_BASS);
    const hf_voice_t *root = hf_voice(HF_VOICE_ROOT);
    EXPECT(bass != NULL && root != NULL, "voice pointers valid");
    EXPECT(bass->freq_hz > 50.0f && bass->freq_hz < 200.0f, "bass in low register");
    EXPECT(root->freq_hz > 100.0f && root->freq_hz < 500.0f, "root mid-low");

    /* Advance 5s; voices should glide UP in amp from 0. */
    float dt = 256.0f / 44100.0f;
    for (int i = 0; i < (int)(5.0f / dt); ++i) hf_advance(dt);
    EXPECT(bass->amp > 0.05f, "bass amp glided up after 5s");
    EXPECT(root->amp > 0.05f, "root amp glided up after 5s");

    /* Change density → fewer active voices when low. */
    hf_set_voice_target_count(3.0f);
    EXPECT(hf_active_count() == 3, "density=low → 3 active");
    hf_set_voice_target_count(7.0f);
    EXPECT(hf_active_count() == 7, "density=high → 7 active");

    /* Center change should slide pitches. */
    int old_midi = bass->midi_note;
    hf_set_center(60);
    EXPECT(bass->midi_note == old_midi + 3, "bass transposed by center delta");
}

/* ------------------------------------------------ field_voice ----------- */

static void test_field_voice(void) {
    printf("\n=== field_voice ===\n");
    fv_state_t g, t, p;
    fv_reset(&g, FV_GLASS, 1);
    fv_reset(&t, FV_TAPE, 2);
    fv_reset(&p, FV_PARTICLE, 3);
    dsp_init();

    float buf[2048];
    for (int i = 0; i < 2048; ++i) buf[i] = fv_render(&g, 220.0f, 0.5f, 0.6f, 0.0f, 0.0f);
    EXPECT(!has_nan_or_inf(buf, 2048), "glass: no NaN/inf");
    EXPECT(peak_abs(buf, 2048) > 0.05f, "glass: produces audible signal");
    EXPECT(rms(buf, 2048) > 0.01f, "glass: has RMS energy");

    for (int i = 0; i < 2048; ++i) buf[i] = fv_render(&t, 220.0f, 0.5f, 0.5f, 0.1f, 0.0f);
    EXPECT(!has_nan_or_inf(buf, 2048), "tape: no NaN/inf");
    EXPECT(peak_abs(buf, 2048) > 0.05f, "tape: produces signal");

    /* Particle voice triggers grains stochastically (3-15 Hz rate). Render
     * long enough to guarantee at least 2-3 grains regardless of phase
     * init — 44100 samples = 1 second. */
    float pbuf[44100];
    for (int i = 0; i < 44100; ++i) pbuf[i] = fv_render(&p, 440.0f, 0.5f, 0.5f, 0.0f, 0.5f);
    EXPECT(!has_nan_or_inf(pbuf, 44100), "particle: no NaN/inf");
    EXPECT(peak_abs(pbuf, 44100) > 0.005f, "particle: produces grains");
}

/* ----------------------------------------------- material_texture ------- */

static void test_texture(void) {
    printf("\n=== material_texture ===\n");
    mt_state_t mt;
    mt_init(&mt, 42);
    mt_set_layer_balance(&mt, 0.5f, 0.3f, 0.2f);

    float L[1024] = {0}, R[1024] = {0};
    /* Zero macro = no contribution. */
    mt_render_add(&mt, L, R, 1024, 0.0f, 0.0f);
    EXPECT(peak_abs(L, 1024) < 1e-6f, "texture: 0 macro = silent");

    /* 0.5 macro → audible noise. */
    mt_render_add(&mt, L, R, 1024, 0.5f, 0.0f);
    EXPECT(peak_abs(L, 1024) > 0.001f, "texture: 0.5 macro produces noise");
    EXPECT(!has_nan_or_inf(L, 1024), "texture: no NaN/inf");

    /* Stereo: L and R should differ (not bit-identical). */
    int diff = 0;
    for (int i = 0; i < 1024; ++i) if (fabsf(L[i] - R[i]) > 1e-5f) ++diff;
    EXPECT(diff > 100, "texture: stereo decorrelated");
}

/* ----------------------------------------------------- diffuser --------- */

static void test_diffuser(void) {
    printf("\n=== diffuser ===\n");
    diffuser_t d;
    diffuser_init(&d);
    diffuser_set_amount(&d, 0.5f);

    /* Impulse input → ringing all-pass output. */
    float L[2048] = {0}, R[2048] = {0};
    L[0] = 1.0f;
    R[0] = 1.0f;
    diffuser_process(&d, L, R, 2048);
    EXPECT(!has_nan_or_inf(L, 2048), "diffuser: no NaN/inf");

    /* Echo should appear at first delay tap (308 samples). */
    EXPECT(fabsf(L[308]) > 0.05f, "diffuser: tap1 echo at 7ms");

    /* Energy should not blow up: all-pass preserves power. */
    float in_energy = 1.0f;
    float out_energy = 0.0f;
    for (int i = 0; i < 2048; ++i) out_energy += L[i] * L[i];
    EXPECT(out_energy < 30.0f * in_energy, "diffuser: bounded energy");
}

/* --------------------------------------------------- mod_delay --------- */

static void test_mod_delay(void) {
    printf("\n=== mod_delay ===\n");
    mod_delay_t md;
    mod_delay_init(&md);
    mod_delay_set_amount(&md, 0.5f);

    /* Drive with a short burst of sine, then run a long tail. */
    float L[32768] = {0}, R[32768] = {0};
    for (int i = 0; i < 100; ++i) {
        float s = sinf(2.0f * 3.14159f * 440.0f * (float)i / 44100.0f);
        L[i] = s;
        R[i] = s;
    }
    mod_delay_process(&md, L, R, 32768);
    EXPECT(!has_nan_or_inf(L, 32768), "mod_delay: no NaN/inf");

    /* After ~350 ms (base_L_samples ≈ 0.350*44100 = 15435), we should see
     * the echo. */
    float late_peak = 0.0f;
    for (int i = 15000; i < 32000; ++i) {
        float a = fabsf(L[i]);
        if (a > late_peak) late_peak = a;
    }
    EXPECT(late_peak > 0.01f, "mod_delay: late echo present");
}

/* --------------------------------------------------- beauty_guard ------ */

static void test_beauty_guard(void) {
    printf("\n=== beauty_guard ===\n");
    beauty_guard_t bg;
    bg_init(&bg);

    /* Quiet input: gain should stay at 1.0. */
    float L[1024], R[1024];
    for (int i = 0; i < 1024; ++i) { L[i] = 0.1f; R[i] = 0.1f; }
    bg_process(&bg, L, R, 1024);
    EXPECT(fabsf(L[1023] - 0.1f) < 0.02f, "guard: quiet passes through");
    EXPECT(!bg_is_active(&bg), "guard: not active when quiet");

    /* Loud input: guard reduces gain. */
    for (int i = 0; i < 4096; ++i) { L[i % 1024] = 2.5f; R[i % 1024] = 2.5f; }
    for (int b = 0; b < 4; ++b) bg_process(&bg, L, R, 1024);
    EXPECT(peak_abs(L, 1024) < 1.0f, "guard: limits peak to <=1.0");
}

/* --------------------------------------------------- worlds ------------ */

static void test_worlds(void) {
    printf("\n=== worlds ===\n");
    EXPECT(worlds_count() >= 6, "worlds: at least 6 defined");
    for (int i = 0; i < worlds_count(); ++i) {
        const world_t *w = worlds_get(i);
        EXPECT(w != NULL, "world ptr non-null");
        EXPECT(w->name != NULL, "world has name");
        EXPECT(w->reverb_wet_hi > w->reverb_wet_lo, "wet range valid");
        EXPECT(w->color_ceiling >= w->color_floor, "color range valid");
    }
}

/* --------------------------------------------------- engine_v2 --------- */

static void test_engine_v2(void) {
    printf("\n=== engine_v2 ===\n");
    dsp_init();
    engine_v2_init(0xDEADBEEFu);

    int16_t buf[AUDIO_BUFFER_FRAMES * 2];

    /* First block — voices just opened up, will be quiet. */
    engine_v2_render(buf, AUDIO_BUFFER_FRAMES);
    EXPECT(peak_i16(buf, AUDIO_BUFFER_FRAMES * 2) <= 1.0f, "engine: no overflow first block");

    /* Render 5s — should reach steady amplitude. */
    float peak = 0.0f;
    float total_rms = 0.0f;
    int blocks = (int)(5.0f * 44100.0f / AUDIO_BUFFER_FRAMES);
    for (int b = 0; b < blocks; ++b) {
        engine_v2_render(buf, AUDIO_BUFFER_FRAMES);
        float p = peak_i16(buf, AUDIO_BUFFER_FRAMES * 2);
        float r = rms_i16(buf, AUDIO_BUFFER_FRAMES * 2);
        if (p > peak) peak = p;
        total_rms += r;
    }
    total_rms /= (float)blocks;
    printf("    after 5s: peak=%.3f rms=%.3f\n", peak, total_rms);
    EXPECT(peak > 0.05f, "engine: produces audible peak");
    EXPECT(peak < 1.0f,  "engine: stays below clipping");
    EXPECT(total_rms > 0.005f, "engine: has RMS energy");

    /* World switch should not break. */
    for (int w = 0; w < worlds_count(); ++w) {
        engine_v2_set_world(w);
        for (int b = 0; b < 8; ++b) engine_v2_render(buf, AUDIO_BUFFER_FRAMES);
        EXPECT(peak_i16(buf, AUDIO_BUFFER_FRAMES * 2) <= 1.0f, "world switch: bounded");
    }

    /* Macro changes don't crash. */
    engine_v2_set_density(0.0f);
    engine_v2_set_density(1.0f);
    engine_v2_set_motion(0.0f);
    engine_v2_set_motion(1.0f);
    engine_v2_set_blur(0.0f);
    engine_v2_set_blur(1.0f);
    engine_v2_set_texture(0.0f);
    engine_v2_set_texture(1.0f);
    engine_v2_set_glow(0.0f);
    engine_v2_set_glow(1.0f);
    for (int b = 0; b < 8; ++b) engine_v2_render(buf, AUDIO_BUFFER_FRAMES);
    EXPECT(peak_i16(buf, AUDIO_BUFFER_FRAMES * 2) <= 1.0f, "macro sweep: bounded");

    /* Freeze should hold harmony state. */
    const hf_voice_t *v = hf_voice(HF_VOICE_ROOT);
    int locked_midi = v->midi_note;
    engine_v2_set_freeze(true);
    for (int b = 0; b < 256; ++b) engine_v2_render(buf, AUDIO_BUFFER_FRAMES);
    EXPECT(v->midi_note == locked_midi, "freeze: harmony unchanged");
    engine_v2_set_freeze(false);

    /* New field should re-roll. */
    engine_v2_new_field(0xCAFEu);
    EXPECT(hf_active_count() >= 3, "new field: still active set");
}

/* ----------------------------------------------------- main ------------ */

int main(void) {
    printf("=========================\n");
    printf(" Engine V2 module tests \n");
    printf("=========================\n");

    dsp_init();
    reverb_init();

    test_motion();
    test_harmony_field();
    test_field_voice();
    test_texture();
    test_diffuser();
    test_mod_delay();
    test_beauty_guard();
    test_worlds();
    test_engine_v2();

    printf("\n-------------------------\n");
    printf(" Passed: %d   Failed: %d\n", pass, fail);
    printf("-------------------------\n");
    return fail == 0 ? 0 : 1;
}
